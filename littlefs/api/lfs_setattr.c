/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_rawsetattr(lfs_t *lfs, const char *path,
        uint8_t type, const void *buffer, lfs_size_t size);

#ifndef LFS_READONLY
int lfs_setattr(lfs_t *lfs, const char *path,
        uint8_t type, const void *buffer, lfs_size_t size) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_setattr(%p, \"%s\", %"PRIu8", %p, %"PRIu32")",
            (void*)lfs, path, type, buffer, size);

    err = lfs_rawsetattr(lfs, path, type, buffer, size);

    LFS_WARN("lfs_setattr -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
#endif
