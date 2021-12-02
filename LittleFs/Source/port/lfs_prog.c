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
