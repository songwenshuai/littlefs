/*
 * lfs util functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"
#include "app.h"

#include <errno.h>

#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */
#include <stdarg.h> /* needed for va_*         */

// Builtin functions, these may be replaced by more efficient
// toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more
// expensive basic C implementation for debugging purposes

// Allocate memory, only used if buffers are not provided to littlefs
// Note, memory must be 64-bit aligned
// static inline 
void *lfs_malloc(size_t size) {
#ifndef LFS_NO_MALLOC
    return malloc(size);
#else
    (void)size;
    return NULL;
#endif
}

// Builtin functions, these may be replaced by more efficient
// toolchain-specific implementations. LFS_NO_INTRINSICS falls back to a more
// expensive basic C implementation for debugging purposes

// Deallocate memory, only used if buffers are not provided to littlefs
// static inline 
void lfs_free(void *p) {
#ifndef LFS_NO_MALLOC
    free(p);
#else
    (void)p;
#endif
}

// static 
int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    // Check if read is valid
    LFS_ASSERT(off  % c->read_size == 0);
    LFS_ASSERT(size % c->read_size == 0);
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_TRACE("lfs_read(%p, 0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)c, block, off, buffer, size);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
    memcpy(buffer, addr + (block * c->block_size) + off, size);
    return 0;
}

// static 
int lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    // Check if write is valid
    LFS_ASSERT(off  % c->prog_size == 0);
    LFS_ASSERT(size % c->prog_size == 0);
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_TRACE("lfs_prog(%p, 0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)c, block, off, buffer, size);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
	memcpy(addr + (block * c->block_size) + off, buffer, size);
    return 0;
}

// static 
int lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    // Check if erase is valid
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_TRACE("lfs_erase(%p, 0x%"PRIx32")", (void*)c, block);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
    memset(addr + (block * c->block_size), 0, c->block_size);
    return 0;
}

//static 
int lfs_sync(const struct lfs_config *c) {
	return 0;
}

