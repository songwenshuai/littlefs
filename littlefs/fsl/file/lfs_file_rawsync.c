/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
void lfs_ctz_tole32(struct lfs_ctz *ctz);
bool lfs_pair_isnull(const lfs_block_t pair[2]);
#ifndef LFS_READONLY
int lfs_dir_commit(lfs_t *lfs, lfs_mdir_t *dir,
        const struct lfs_mattr *attrs, int attrcount);
#endif
#ifndef LFS_READONLY
int lfs_file_flush(lfs_t *lfs, lfs_file_t *file);
#endif

#ifndef LFS_READONLY
// static 
int lfs_file_rawsync(lfs_t *lfs, lfs_file_t *file) {
    if (file->flags & LFS_F_ERRED) {
        // it's not safe to do anything if our file errored
        return 0;
    }

    int err = lfs_file_flush(lfs, file);
    if (err) {
        file->flags |= LFS_F_ERRED;
        return err;
    }


    if ((file->flags & LFS_F_DIRTY) &&
            !lfs_pair_isnull(file->m.pair)) {
        // update dir entry
        uint16_t type;
        const void *buffer;
        lfs_size_t size;
        struct lfs_ctz ctz;
        if (file->flags & LFS_F_INLINE) {
            // inline the whole file
            type = LFS_TYPE_INLINESTRUCT;
            buffer = file->cache.buffer;
            size = file->ctz.size;
        } else {
            // update the ctz reference
            type = LFS_TYPE_CTZSTRUCT;
            // copy ctz so alloc will work during a relocate
            ctz = file->ctz;
            lfs_ctz_tole32(&ctz);
            buffer = &ctz;
            size = sizeof(ctz);
        }

        // commit file data and attributes
        err = lfs_dir_commit(lfs, &file->m, LFS_MKATTRS(
                {LFS_MKTAG(type, file->id, size), buffer},
                {LFS_MKTAG(LFS_FROM_USERATTRS, file->id,
                    file->cfg->attr_count), file->cfg->attrs}));
        if (err) {
            file->flags |= LFS_F_ERRED;
            return err;
        }

        file->flags &= ~LFS_F_DIRTY;
    }

    return 0;
}
#endif

