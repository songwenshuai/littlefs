/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

bool lfs_mlist_isopen(struct lfs_mlist *head,
        struct lfs_mlist *node);
lfs_soff_t lfs_file_rawseek(lfs_t *lfs, lfs_file_t *file,
        lfs_soff_t off, int whence);

lfs_soff_t lfs_file_seek(lfs_t *lfs, lfs_file_t *file,
        lfs_soff_t off, int whence) {
    int err = LFS_LOCK(lfs->cfg);
    if (err) {
        return err;
    }
    LFS_WARN("lfs_file_seek(%p, %p, %"PRId32", %d)",
            (void*)lfs, (void*)file, off, whence);
    LFS_ASSERT(lfs_mlist_isopen(lfs->mlist, (struct lfs_mlist*)file));

    lfs_soff_t res = lfs_file_rawseek(lfs, file, off, whence);

    LFS_WARN("lfs_file_seek -> %"PRId32, res);
    LFS_UNLOCK(lfs->cfg);
    return res;
}
