/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_rawmkdir(lfs_t *lfs, const char *path);

#ifndef LFS_READONLY
int lfs_mkdir(lfs_t *lfs, const char *path) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_TRACE("lfs_mkdir(%p, \"%s\")", (void*)lfs, path);

    err = lfs_rawmkdir(lfs, path);

    LFS_TRACE("lfs_mkdir -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
#endif
