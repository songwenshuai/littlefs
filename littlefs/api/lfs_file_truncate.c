/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

bool lfs_mlist_isopen(struct lfs_mlist *head,
        struct lfs_mlist *node);
int lfs_file_rawtruncate(lfs_t *lfs, lfs_file_t *file, lfs_off_t size);

#ifndef LFS_READONLY
int lfs_file_truncate(lfs_t *lfs, lfs_file_t *file, lfs_off_t size) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_file_truncate(%p, %p, %"PRIu32")",
            (void*)lfs, (void*)file, size);
    LFS_ASSERT(lfs_mlist_isopen(lfs->mlist, (struct lfs_mlist*)file));

    err = lfs_file_rawtruncate(lfs, file, size);

    LFS_WARN("lfs_file_truncate -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
#endif
