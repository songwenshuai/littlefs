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
lfs_ssize_t lfs_file_rawwrite(lfs_t *lfs, lfs_file_t *file,
        const void *buffer, lfs_size_t size);
#endif

#ifndef LFS_READONLY
lfs_ssize_t lfs_file_write(lfs_t *lfs, lfs_file_t *file,
        const void *buffer, lfs_size_t size) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_file_write(%p, %p, %p, %"PRIu32")",
            (void*)lfs, (void*)file, buffer, size);
    LFS_ASSERT(lfs_mlist_isopen(lfs->mlist, (struct lfs_mlist*)file));

    lfs_ssize_t res = lfs_file_rawwrite(lfs, file, buffer, size);

    LFS_WARN("lfs_file_write -> %"PRId32, res);
    LFS_UNLOCK(lfs->cfg);
    return res;
}
#endif
