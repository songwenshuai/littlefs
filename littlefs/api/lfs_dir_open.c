/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

bool lfs_mlist_isopen(struct lfs_mlist *head,
        struct lfs_mlist *node);
int lfs_dir_rawopen(lfs_t *lfs, lfs_dir_t *dir, const char *path);

int lfs_dir_open(lfs_t *lfs, lfs_dir_t *dir, const char *path) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_dir_open(%p, %p, \"%s\")", (void*)lfs, (void*)dir, path);
    LFS_ASSERT(!lfs_mlist_isopen(lfs->mlist, (struct lfs_mlist*)dir));

    err = lfs_dir_rawopen(lfs, dir, path);

    LFS_WARN("lfs_dir_open -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
