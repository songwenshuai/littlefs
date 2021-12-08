/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_fs_rawtraverse(lfs_t *lfs,
        int (*cb)(void *data, lfs_block_t block), void *data,
        bool includeorphans);

int lfs_fs_traverse(lfs_t *lfs, int (*cb)(void *, lfs_block_t), void *data) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_fs_traverse(%p, %p, %p)",
            (void*)lfs, (void*)(uintptr_t)cb, data);

    err = lfs_fs_rawtraverse(lfs, cb, data, true);

    LFS_WARN("lfs_fs_traverse -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
