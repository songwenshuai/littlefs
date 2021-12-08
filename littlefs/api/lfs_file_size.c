/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

bool lfs_mlist_isopen(struct lfs_mlist *head,
        struct lfs_mlist *node);
lfs_soff_t lfs_file_rawsize(lfs_t *lfs, lfs_file_t *file);

lfs_soff_t lfs_file_size(lfs_t *lfs, lfs_file_t *file) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_file_size(%p, %p)", (void*)lfs, (void*)file);
    LFS_ASSERT(lfs_mlist_isopen(lfs->mlist, (struct lfs_mlist*)file));

    lfs_soff_t res = lfs_file_rawsize(lfs, file);

    LFS_WARN("lfs_file_size -> %"PRId32, res);
    LFS_UNLOCK(lfs->cfg);
    return res;
}
