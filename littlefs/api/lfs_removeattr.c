/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_rawremoveattr(lfs_t *lfs, const char *path, uint8_t type);

#ifndef LFS_READONLY
int lfs_removeattr(lfs_t *lfs, const char *path, uint8_t type) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_removeattr(%p, \"%s\", %"PRIu8")", (void*)lfs, path, type);

    err = lfs_rawremoveattr(lfs, path, type);

    LFS_WARN("lfs_removeattr -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
#endif
