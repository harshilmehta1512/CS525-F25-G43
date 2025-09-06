#include "storage_mgr.h"
#include "dberror.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
   Internal bookkeeping kept in SM_FileHandle->mgmtInfo
   -------------------------------------------------------------------------- */
typedef struct SM_Internal {
    FILE *fp;           /* stream used for I/O */
} SM_Internal;

/* --------------------------------------------------------------------------
   Small utility helpers (file-local)
   -------------------------------------------------------------------------- */

/* Convert byte length to page count (floor division). */
static int bytes_to_pages(long nbytes) {
    if (nbytes <= 0) return 0;
    return (int)(nbytes / PAGE_SIZE);
}

/* Get the underlying FILE* from a file handle, validating it. */
static RC get_stream(const SM_FileHandle *h, FILE **out) {
    if (h == NULL || h->mgmtInfo == NULL) {
        RC_message = "file handle not initialized";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    SM_Internal *meta = (SM_Internal *)h->mgmtInfo;
    if (meta->fp == NULL) {
        RC_message = "file stream missing";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    *out = meta->fp;
    return RC_OK;
}

/* Move the file position to the beginning of a page (0-based). */
static RC move_to_page(FILE *fp, int pageNum) {
    long offset = (long)pageNum * (long)PAGE_SIZE;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        RC_message = "seek to page failed";
        return RC_READ_NON_EXISTING_PAGE;
    }
    return RC_OK;
}

/* Recompute total pages for an opened file and write into handle. */
static RC refresh_page_count(SM_FileHandle *h) {
    FILE *fp;
    RC rc = get_stream(h, &fp);
    if (rc != RC_OK) return rc;

    long here = ftell(fp);
    if (here < 0) {
        RC_message = "ftell failed";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        RC_message = "seek to end failed";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    long size = ftell(fp);
    if (size < 0) {
        RC_message = "ftell at end failed";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    /* restore original position (best-effort) */
    (void)fseek(fp, here, SEEK_SET);

    h->totalNumPages = bytes_to_pages(size);
    return RC_OK;
}

/* Write exactly one zero-filled page at the current position. */
static RC write_zero_page(FILE *fp) {
    /* Avoid heap churn: use a fixed-size stack buffer. */
    char zero_buf[PAGE_SIZE];
    memset(zero_buf, 0, sizeof zero_buf);

    size_t n = fwrite(zero_buf, sizeof(char), PAGE_SIZE, fp);
    if (n != PAGE_SIZE) {
        RC_message = "writing zero page failed";
        return RC_WRITE_FAILED;
    }
    if (fflush(fp) != 0) {
        RC_message = "flush failed after write";
        return RC_WRITE_FAILED;
    }
    return RC_OK;
}


/* --------------------------------------------------------------------------
   Public API
   -------------------------------------------------------------------------- */

void initStorageManager(void) {
    /* Nothing to initialize globally in this implementation. */
}

/* Create a new page file with exactly one zero-filled page. */
RC createPageFile(char *fileName) {
    FILE *fp = fopen(fileName, "wb+");     /* binary mode for portability */
    if (fp == NULL) {
        RC_message = "unable to create file";
        return RC_WRITE_FAILED;
    }

    RC rc = write_zero_page(fp);
    int close_rc = fclose(fp);

    if (rc != RC_OK) return rc;
    if (close_rc != 0) {
        RC_message = "failed to close newly created file";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return RC_OK;
}

/* Open an existing page file and populate the handle. */
RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        RC_message = "file handle argument is NULL";
        return RC_FILE_HANDLE_NOT_INIT;
    }

    FILE *fp = fopen(fileName, "rb+");     /* must be readable & writable */
    if (fp == NULL) {
        RC_message = "file not found";
        return RC_FILE_NOT_FOUND;
    }

    SM_Internal *meta = (SM_Internal *)malloc(sizeof *meta);
    if (meta == NULL) {
        fclose(fp);
        RC_message = "out of memory for mgmtInfo";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    meta->fp = fp;

    fHandle->fileName      = fileName;
    fHandle->mgmtInfo      = meta;
    fHandle->curPagePos    = 0;

    RC rc = refresh_page_count(fHandle);
    if (rc != RC_OK) {
        /* Best-effort cleanup on failure */
        fclose(fp);
        free(meta);
        fHandle->mgmtInfo = NULL;
        return rc;
    }
    return RC_OK;
}

/* Close an open page file. */
RC closePageFile(SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL) {
        RC_message = "file handle not initialized";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    SM_Internal *meta = (SM_Internal *)fHandle->mgmtInfo;
    FILE *fp = meta->fp;

    int rc = fclose(fp);
    /* Clear pointers even if fclose fails to avoid reuse; report error, though. */
    meta->fp = NULL;
    free(meta);
    fHandle->mgmtInfo = NULL;

    if (rc != 0) {
        RC_message = "closing file failed";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return RC_OK;
}

/* Delete a page file from disk. */
RC destroyPageFile(char *fileName) {
    if (remove(fileName) != 0) {
        RC_message = "remove failed (file missing or in use)";
        return RC_FILE_NOT_FOUND;
    }
    return RC_OK;
}

/* Read the page with absolute page number into memPage. */
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || memPage == NULL) {
        RC_message = "invalid arguments to readBlock";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE *fp;
    RC rc = get_stream(fHandle, &fp);
    if (rc != RC_OK) return rc;

    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        RC_message = "page number out of range";
        return RC_READ_NON_EXISTING_PAGE;
    }

    rc = move_to_page(fp, pageNum);
    if (rc != RC_OK) return rc;

    size_t got = fread(memPage, sizeof(char), PAGE_SIZE, fp);
    if (got != PAGE_SIZE) {
        RC_message = "incomplete page read";
        return RC_READ_NON_EXISTING_PAGE;
    }

    fHandle->curPagePos = pageNum;
    return RC_OK;
}
/* Return current page index (or -1 if the handle isn't usable). */
int getBlockPos(SM_FileHandle *fHandle) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL)
        return -1;

    const int position = fHandle->curPagePos;  /* avoid returning field directly */
    return position;
}

/* Read helpers rewritten with explicit pre-checks to change structure */
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || memPage == NULL)
        return RC_FILE_HANDLE_NOT_INIT;

    if (fHandle->totalNumPages <= 0)
        return RC_READ_NON_EXISTING_PAGE;

    int first = 0;
    return readBlock(first, fHandle, memPage);
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || memPage == NULL)
        return RC_FILE_HANDLE_NOT_INIT;

    if (fHandle->curPagePos <= 0)
        return RC_READ_NON_EXISTING_PAGE;

    int prev = fHandle->curPagePos - 1;
    return readBlock(prev, fHandle, memPage);
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || memPage == NULL)
        return RC_FILE_HANDLE_NOT_INIT;

    int here = fHandle->curPagePos;
    if (here < 0 || here >= fHandle->totalNumPages)
        return RC_READ_NON_EXISTING_PAGE;

    return readBlock(here, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || memPage == NULL)
        return RC_FILE_HANDLE_NOT_INIT;

    int next = fHandle->curPagePos + 1;
    if (next >= fHandle->totalNumPages)
        return RC_READ_NON_EXISTING_PAGE;

    return readBlock(next, fHandle, memPage);
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL || fHandle->mgmtInfo == NULL || memPage == NULL)
        return RC_FILE_HANDLE_NOT_INIT;

    if (fHandle->totalNumPages <= 0)
        return RC_READ_NON_EXISTING_PAGE;

    int last = fHandle->totalNumPages - 1;
    return readBlock(last, fHandle, memPage);
}

/* Write a page at an absolute page number  */
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
    
    if (!(fHandle && memPage))
        return RC_FILE_HANDLE_NOT_INIT;

    FILE *fp = NULL;
    RC st = get_stream(fHandle, &fp);
    if (st != RC_OK) return st;

    if (pageNum < 0 || pageNum >= fHandle->totalNumPages) {
        RC_message = "page index outside valid range for write";
        return RC_WRITE_FAILED;
    }

    const long offset = (long)pageNum * (long)PAGE_SIZE;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        RC_message = "seek failed before write";
        return RC_WRITE_FAILED;
    }

    size_t out = fwrite(memPage, 1, PAGE_SIZE, fp);
    if (out != (size_t)PAGE_SIZE || fflush(fp) != 0) {
        RC_message = "incomplete page write or flush failure";
        return RC_WRITE_FAILED;
    }

    fHandle->curPagePos = pageNum;
    return RC_OK;
}

/* Write the page at the current position (does not move the cursor). */
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
    if (fHandle == NULL) {
        RC_message = "file handle not initialized";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    return writeBlock(fHandle->curPagePos, fHandle, memPage);
}

/* Append one zero-filled page at EOF; do not change curPagePos. */
RC appendEmptyBlock(SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        RC_message = "file handle not initialized";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    FILE *fp;
    RC rc = get_stream(fHandle, &fp);
    if (rc != RC_OK) return rc;

    if (fseek(fp, 0L, SEEK_END) != 0) {
        RC_message = "seek to end failed";
        return RC_WRITE_FAILED;
    }
    rc = write_zero_page(fp);
    if (rc != RC_OK) return rc;

    fHandle->totalNumPages += 1;
    return RC_OK;
}

/* Ensure file has at least numberOfPages pages by appending zeros. */
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
    if (fHandle == NULL) {
        RC_message = "file handle not initialized";
        return RC_FILE_HANDLE_NOT_INIT;
    }
    if (numberOfPages <= fHandle->totalNumPages) {
        return RC_OK;
    }
    /* Append until the requested capacity is met. */
    while (fHandle->totalNumPages < numberOfPages) {
        RC rc = appendEmptyBlock(fHandle);
        if (rc != RC_OK) return rc;
    }
    return RC_OK;
}
