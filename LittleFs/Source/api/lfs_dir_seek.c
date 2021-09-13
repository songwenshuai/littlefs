/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_dir_rawseek(lfs_t *lfs, lfs_dir_t *dir, lfs_off_t off);

int lfs_dir_seek(lfs_t *lfs, lfs_dir_t *dir, lfs_off_t off) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_TRACE("lfs_dir_seek(%p, %p, %"PRIu32")",
            (void*)lfs, (void*)dir, off);

    err = lfs_dir_rawseek(lfs, dir, off);

    LFS_TRACE("lfs_dir_seek -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
