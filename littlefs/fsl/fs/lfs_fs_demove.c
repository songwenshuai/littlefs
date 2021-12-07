/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
bool lfs_gstate_hasmove(const lfs_gstate_t *a);
uint16_t lfs_tag_id(lfs_tag_t tag);
#ifndef LFS_READONLY
int lfs_dir_commit(lfs_t *lfs, lfs_mdir_t *dir,
        const struct lfs_mattr *attrs, int attrcount);
#endif
int lfs_dir_fetch(lfs_t *lfs,
        lfs_mdir_t *dir, const lfs_block_t pair[2]);
#ifndef LFS_READONLY
void lfs_fs_prepmove(lfs_t *lfs,
        uint16_t id, const lfs_block_t pair[2]);
#endif

#ifndef LFS_READONLY
// static 
int lfs_fs_demove(lfs_t *lfs) {
    if (!lfs_gstate_hasmove(&lfs->gdisk)) {
        return 0;
    }

    // Fix bad moves
    LFS_DEBUG("Fixing move {0x%"PRIx32", 0x%"PRIx32"} 0x%"PRIx16,
            lfs->gdisk.pair[0],
            lfs->gdisk.pair[1],
            lfs_tag_id(lfs->gdisk.tag));

    // fetch and delete the moved entry
    lfs_mdir_t movedir;
    int err = lfs_dir_fetch(lfs, &movedir, lfs->gdisk.pair);
    if (err) {
        return err;
    }

    // prep gstate and delete move id
    uint16_t moveid = lfs_tag_id(lfs->gdisk.tag);
    lfs_fs_prepmove(lfs, 0x3ff, NULL);
    err = lfs_dir_commit(lfs, &movedir, LFS_MKATTRS(
            {LFS_MKTAG(LFS_TYPE_DELETE, moveid, 0), NULL}));
    if (err) {
        return err;
    }

    return 0;
}
#endif
