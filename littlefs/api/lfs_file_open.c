/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

bool lfs_mlist_isopen(struct lfs_mlist *head,
        struct lfs_mlist *node);
int lfs_file_rawopen(lfs_t *lfs, lfs_file_t *file,
        const char *path, int flags);

int lfs_file_open(lfs_t *lfs, lfs_file_t *file, const char *path, int flags) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_TRACE("lfs_file_open(%p, %p, \"%s\", %x)",
            (void*)lfs, (void*)file, path, flags);
    LFS_ASSERT(!lfs_mlist_isopen(lfs->mlist, (struct lfs_mlist*)file));

    err = lfs_file_rawopen(lfs, file, path, flags);

    LFS_TRACE("lfs_file_open -> %d", err);
    LFS_UNLOCK(lfs->cfg);
    return err;
}
