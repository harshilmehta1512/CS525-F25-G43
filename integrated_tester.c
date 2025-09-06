// all_tests.c
// Extended runner for Storage Manager — unique structure & helpers

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* --------------------------------------------------------------------------
   Bring in the original assignment tests, but treat their main as a function.
   This lets us call the baseline suite first, then our custom tests.
   -------------------------------------------------------------------------- */
#define main baseline_suite_entrypoint
#include "test_assign1_1.c"
#undef main

/* --------------------------------------------------------------------------
   Global test name required by test_helper.h macros
   -------------------------------------------------------------------------- */
char *testName = NULL;

/* --------------------------------------------------------------------------
   Local utilities: pattern stamping & verification (parameterized & distinct)
   -------------------------------------------------------------------------- */

/* Fill a page with a cyclic pattern: buf[i] = (seed + (i % cycle)) */
static void stamp_pattern(SM_PageHandle buf, unsigned char seed, int cycle) {
    for (int i = 0; i < PAGE_SIZE; ++i) {
        buf[i] = (unsigned char)(seed + (i % (cycle > 0 ? cycle : 1)));
    }
}

/* Verify page matches the pattern produced by stamp_pattern(seed, cycle) */
static void assert_pattern(const SM_PageHandle buf, unsigned char seed, int cycle, const char *context) {
    for (int i = 0; i < PAGE_SIZE; ++i) {
        unsigned char expect = (unsigned char)(seed + (i % (cycle > 0 ? cycle : 1)));
        if ((unsigned char)buf[i] != expect) {
            char msg[160];
            snprintf(msg, sizeof(msg),
                     "%s: mismatch at offset %d (expected 0x%02X, got 0x%02X)",
                     context ? context : "pattern-check", i, expect, (unsigned char)buf[i]);
            ASSERT_TRUE(false, msg);
        }
    }
    ASSERT_TRUE(true, context ? context : "pattern verified");
}

/* Allocate a zeroed page buffer or fail the test immediately */
static SM_PageHandle alloc_page_or_die(const char *why) {
    SM_PageHandle p = (SM_PageHandle)calloc(PAGE_SIZE, 1);
    ASSERT_TRUE(p != NULL, (why && *why) ? why : "allocation failed");
    return p;
}

/* --------------------------------------------------------------------------
   Extended tests 
   -------------------------------------------------------------------------- */

/* Test A: Ensure capacity jump, write the final page, read it back via readLastBlock */
static void test_capacity_jump_and_tail_io(void) {
    const char *fname = "sm_ext_A.bin";
    const int   want_pages = 9;   
    SM_FileHandle fh;

    testName = "A: ensureCapacity + write/read last page";
    SM_PageHandle page = alloc_page_or_die("A: buffer alloc");

    TEST_CHECK(createPageFile((char*)fname));
    TEST_CHECK(openPageFile((char*)fname, &fh));

    /* Grow to desired capacity in one go */
    TEST_CHECK(ensureCapacity(want_pages, &fh));
    ASSERT_TRUE(fh.totalNumPages >= want_pages, "A: capacity reached");

    /* Write to the final page (index = totalNumPages - 1) */
    stamp_pattern(page, (unsigned char)'Q', 13);
    TEST_CHECK(writeBlock(fh.totalNumPages - 1, &fh, page));

    /* Read back using readLastBlock and validate */
    memset(page, 0, PAGE_SIZE);
    TEST_CHECK(readLastBlock(&fh, page));
    assert_pattern(page, (unsigned char)'Q', 13, "A: last-page pattern ok");

    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile((char*)fname));
    free(page);

    TEST_DONE();
}

/* Test B: Append several pages stepwise; verify an interior page & cursor behavior */
static void test_append_growth_and_random_access(void) {
    const char *fname = "sm_ext_B.bin";
    const int   to_append = 7;  
    SM_FileHandle fh;

    testName = "B: appendEmptyBlock growth + interior page verification";
    SM_PageHandle page = alloc_page_or_die("B: buffer alloc");

    TEST_CHECK(createPageFile((char*)fname));
    TEST_CHECK(openPageFile((char*)fname, &fh));

    /* Append pages one by one */
    int start_pages = fh.totalNumPages;
    for (int k = 0; k < to_append; ++k) {
        TEST_CHECK(appendEmptyBlock(&fh));
    }
    ASSERT_TRUE(fh.totalNumPages == start_pages + to_append, "B: total pages updated after appends");

    /* Choose a deterministic interior page: roughly middle */
    int mid = (fh.totalNumPages > 2) ? (fh.totalNumPages / 2) : 0;

    /* Write & read that mid page explicitly */
    stamp_pattern(page, (unsigned char)'M', 10);
    TEST_CHECK(writeBlock(mid, &fh, page));

    memset(page, 0, PAGE_SIZE);
    TEST_CHECK(readBlock(mid, &fh, page));
    assert_pattern(page, (unsigned char)'M', 10, "B: mid-page pattern ok");

    /* Cursor check: getBlockPos should reflect last read position */
    int pos = getBlockPos(&fh);
    ASSERT_TRUE(pos == mid, "B: curPagePos points at last-read page");

    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile((char*)fname));
    free(page);

    TEST_DONE();
}

/* --------------------------------------------------------------------------
   Simple runner
   -------------------------------------------------------------------------- */

static int run_baseline_suite(void) {
    /* Run the original suite from test_assign1_1.c */
    return baseline_suite_entrypoint();
}

static int run_extended_suites(void) {
    /* Execute our distinct tests */
    test_capacity_jump_and_tail_io();
    test_append_growth_and_random_access();
    return 0;
}

int main(void) {
    /* line-buffer stdout so ASSERT/TEST output appears promptly */
    setvbuf(stdout, NULL, _IOLBF, 0);

    int rc = 0;

    /* 1) Baseline tests (provided by the assignment) */
    rc |= run_baseline_suite();

    /* 2) Our additional tests  */
    rc |= run_extended_suites();

    return rc;
}
