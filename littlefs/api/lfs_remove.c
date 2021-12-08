/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_rawremove(lfs_t *lfs, const char *path);

#ifndef LFS_READONLY
int lfs_remove(lfs_t *lfs, const char *path) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_remove(%p, \"%s\")", (void*)lfs, path);

    err = lfs_rawremove(lfs, path);

    LFS_WARN("lfs_remove -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
#endif
