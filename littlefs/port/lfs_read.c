/*
 * lfs util functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

#include <errno.h>

#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */
#include <stdarg.h> /* needed for va_*         */

// static 
int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    // Check if read is valid
    LFS_ASSERT(off  % c->read_size == 0);
    LFS_ASSERT(size % c->read_size == 0);
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_WARN("lfs_read(%p, 0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)c, block, off, buffer, size);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
    memcpy(buffer, addr + (block * c->block_size) + off, size);
    return 0;
}
