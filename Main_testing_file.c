// Main_tester.c — Distinct runner for Storage Manager

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"
#include <stdbool.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define main sm_professor_suite_entry
#include "test_assign1_1.c"
#undef main

/* test_helper macros require this global */
char *testName = NULL;

/* --------------------------------------------------------------------------
   Local utilities for page initialization and validation
   -------------------------------------------------------------------------- */

/* Create a repeating XOR-based pattern for easy detection */
static void make_pattern(SM_PageHandle buf, unsigned char base, int mod) {
    if (mod <= 0) mod = 1;
    for (int i = 0; i < PAGE_SIZE; i++) {
        buf[i] = (char)((base ^ (i % mod)) & 0xFF);
    }
}

/* Confirm buffer contents match the expected XOR-based pattern */
static void check_pattern(const SM_PageHandle buf, unsigned char base, int mod, const char *ctx) {
    if (mod <= 0) mod = 1;
    for (int i = 0; i < PAGE_SIZE; i++) {
        unsigned char want = (unsigned char)((base ^ (i % mod)) & 0xFF);
        if ((unsigned char)buf[i] != want) {
            char msg[200];
            snprintf(msg, sizeof(msg),
                     "%s: mismatch at byte %d (got 0x%02X, expected 0x%02X)",
                     ctx ? ctx : "pattern-check", i, (unsigned char)buf[i], want);
            ASSERT_TRUE(false, msg);
        }
    }
    ASSERT_TRUE(true, ctx ? ctx : "pattern verified");
}

/* Allocate a zeroed page or terminate test if allocation fails */
static SM_PageHandle get_buffer(const char *why) {
    SM_PageHandle p = (SM_PageHandle)calloc(PAGE_SIZE, 1);
    ASSERT_TRUE(p != NULL, why ? why : "allocation failed");
    return p;
}

/* --------------------------------------------------------------------------
   Extended tests unique to this runner
   -------------------------------------------------------------------------- */

/* XT1: Expand file capacity to a fixed size, write and verify last page */
static void xt_capacity_and_lastpage(void) {
    const char *fname = "xt_case1.bin";
    const int   pages_needed = 8;
    SM_FileHandle fh;

    testName = "XT1: capacity growth and last-page roundtrip";
    SM_PageHandle buf = get_buffer("XT1 alloc");

    TEST_CHECK(createPageFile((char*)fname));
    TEST_CHECK(openPageFile((char*)fname, &fh));

    TEST_CHECK(ensureCapacity(pages_needed, &fh));
    ASSERT_TRUE(fh.totalNumPages >= pages_needed, "XT1: capacity reached target");

    make_pattern(buf, 'X', 11);
    TEST_CHECK(writeBlock(fh.totalNumPages - 1, &fh, buf));

    memset(buf, 0, PAGE_SIZE);
    TEST_CHECK(readLastBlock(&fh, buf));
    check_pattern(buf, 'X', 11, "XT1: last-page pattern ok");

    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile((char*)fname));
    free(buf);

    TEST_DONE();
}

/* XT2: Append several blocks one-by-one, write to a mid block, verify position */
static void xt_append_and_midblock(void) {
    const char *fname = "xt_case2.bin";
    const int   appends = 6;
    SM_FileHandle fh;

    testName = "XT2: append sequence and mid-block check";
    SM_PageHandle buf = get_buffer("XT2 alloc");

    TEST_CHECK(createPageFile((char*)fname));
    TEST_CHECK(openPageFile((char*)fname, &fh));

    int start_pages = fh.totalNumPages;
    for (int i = 0; i < appends; i++) {
        TEST_CHECK(appendEmptyBlock(&fh));
    }
    ASSERT_TRUE(fh.totalNumPages == start_pages + appends, "XT2: page count after appends");

    int mid = (fh.totalNumPages > 2) ? fh.totalNumPages / 2 : 0;

    make_pattern(buf, 'Y', 5);
    TEST_CHECK(writeBlock(mid, &fh, buf));

    memset(buf, 0, PAGE_SIZE);
    TEST_CHECK(readBlock(mid, &fh, buf));
    check_pattern(buf, 'Y', 5, "XT2: mid-page pattern ok");

    ASSERT_TRUE(getBlockPos(&fh) == mid, "XT2: curPagePos reflects mid page");

    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile((char*)fname));
    free(buf);

    TEST_DONE();
}

/* --------------------------------------------------------------------------
   Entry point
   -------------------------------------------------------------------------- */
int main(void) {
    printf("== Main_tester runner ==\n");

    initStorageManager();

    printf("-- Running professor baseline tests --\n");
    int base_rc = sm_professor_suite_entry();

    printf("-- Running custom extended tests --\n");
    xt_capacity_and_lastpage();
    xt_append_and_midblock();

    printf("== All tests finished (baseline rc=%d) ==\n", base_rc);
    return base_rc;
}
