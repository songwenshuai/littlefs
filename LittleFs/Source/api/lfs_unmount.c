/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_unmount(lfs_t *lfs) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_TRACE("lfs_unmount(%p)", (void*)lfs);

    err = lfs_rawunmount(lfs);

    LFS_TRACE("lfs_unmount -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}