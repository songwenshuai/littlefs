/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_dir_rawrewind(lfs_t *lfs, lfs_dir_t *dir);

int lfs_dir_rewind(lfs_t *lfs, lfs_dir_t *dir) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_TRACE("lfs_dir_rewind(%p, %p)", (void*)lfs, (void*)dir);

    err = lfs_dir_rawrewind(lfs, dir);

    LFS_TRACE("lfs_dir_rewind -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
