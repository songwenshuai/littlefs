/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

void lfs_alloc_ack(lfs_t *lfs);
int lfs_file_relocate(lfs_t *lfs, lfs_file_t *file);

#ifndef LFS_READONLY
// static 
int lfs_file_outline(lfs_t *lfs, lfs_file_t *file) {
    file->off = file->pos;
    lfs_alloc_ack(lfs);
    int err = lfs_file_relocate(lfs, file);
    if (err) {
        return err;
    }

    file->flags &= ~LFS_F_INLINE;
    return 0;
}
#endif

