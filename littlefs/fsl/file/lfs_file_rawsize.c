/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
uint32_t lfs_max(uint32_t a, uint32_t b);

// static 
lfs_soff_t lfs_file_rawsize(lfs_t *lfs, lfs_file_t *file) {
    (void)lfs;

#ifndef LFS_READONLY
    if (file->flags & LFS_F_WRITING) {
        return lfs_max(file->pos, file->ctz.size);
    }
#endif

    return file->ctz.size;
}


