/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_dir_rawclose(lfs_t *lfs, lfs_dir_t *dir);

int lfs_dir_close(lfs_t *lfs, lfs_dir_t *dir) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_dir_close(%p, %p)", (void*)lfs, (void*)dir);

    err = lfs_dir_rawclose(lfs, dir);

    LFS_WARN("lfs_dir_close -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
