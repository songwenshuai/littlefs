/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_rawrename(lfs_t *lfs, const char *oldpath, const char *newpath);

#ifndef LFS_READONLY
int lfs_rename(lfs_t *lfs, const char *oldpath, const char *newpath) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_rename(%p, \"%s\", \"%s\")", (void*)lfs, oldpath, newpath);

    err = lfs_rawrename(lfs, oldpath, newpath);

    LFS_WARN("lfs_rename -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
#endif
