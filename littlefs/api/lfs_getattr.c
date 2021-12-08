/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

lfs_ssize_t lfs_rawgetattr(lfs_t *lfs, const char *path,
        uint8_t type, void *buffer, lfs_size_t size);

lfs_ssize_t lfs_getattr(lfs_t *lfs, const char *path,
        uint8_t type, void *buffer, lfs_size_t size) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_getattr(%p, \"%s\", %"PRIu8", %p, %"PRIu32")",
            (void*)lfs, path, type, buffer, size);

    lfs_ssize_t res = lfs_rawgetattr(lfs, path, type, buffer, size);

    LFS_WARN("lfs_getattr -> %"PRId32, res);
    LFS_UNLOCK(lfs->cfg);
    return res;
}
