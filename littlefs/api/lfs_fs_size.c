/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

lfs_ssize_t lfs_fs_rawsize(lfs_t *lfs);

lfs_ssize_t lfs_fs_size(lfs_t *lfs) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_fs_size(%p)", (void*)lfs);

    lfs_ssize_t res = lfs_fs_rawsize(lfs);

    LFS_WARN("lfs_fs_size -> %"PRId32, res);
    LFS_UNLOCK(lfs->cfg);
    return res;
}
