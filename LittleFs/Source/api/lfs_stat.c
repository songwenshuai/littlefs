/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_stat(lfs_t *lfs, const char *path, struct lfs_info *info) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_TRACE("lfs_stat(%p, \"%s\", %p)", (void*)lfs, path, (void*)info);

    err = lfs_rawstat(lfs, path, info);

    LFS_TRACE("lfs_stat -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
