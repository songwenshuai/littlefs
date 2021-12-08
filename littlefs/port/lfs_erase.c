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
int lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    // Check if erase is valid
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_WARN("lfs_erase(%p, 0x%"PRIx32")", (void*)c, block);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
    memset(addr + (block * c->block_size), 0, c->block_size);
    return 0;
}
