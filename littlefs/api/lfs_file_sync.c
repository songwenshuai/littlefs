/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

bool lfs_mlist_isopen(struct lfs_mlist *head,
        struct lfs_mlist *node);
#ifndef LFS_READONLY
int lfs_file_rawsync(lfs_t *lfs, lfs_file_t *file);
#endif

#ifndef LFS_READONLY
int lfs_file_sync(lfs_t *lfs, lfs_file_t *file) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_file_sync(%p, %p)", (void*)lfs, (void*)file);
    LFS_ASSERT(lfs_mlist_isopen(lfs->mlist, (struct lfs_mlist*)file));

    err = lfs_file_rawsync(lfs, file);

    LFS_WARN("lfs_file_sync -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
#endif
