/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

#ifndef LFS_READONLY
// static 
lfs_stag_t lfs_fs_parent(lfs_t *lfs, const lfs_block_t pair[2],
        lfs_mdir_t *parent) {
    // use fetchmatch with callback to find pairs
    parent->tail[0] = 0;
    parent->tail[1] = 1;
    lfs_block_t cycle = 0;
    while (!lfs_pair_isnull(parent->tail)) {
        if (cycle >= lfs->cfg->block_count/2) {
            // loop detected
            return LFS_ERR_CORRUPT;
        }
        cycle += 1;

        lfs_stag_t tag = lfs_dir_fetchmatch(lfs, parent, parent->tail,
                LFS_MKTAG(0x7ff, 0, 0x3ff),
                LFS_MKTAG(LFS_TYPE_DIRSTRUCT, 0, 8),
                NULL,
                lfs_fs_parent_match, &(struct lfs_fs_parent_match){
                    lfs, {pair[0], pair[1]}});
        if (tag && tag != LFS_ERR_NOENT) {
            return tag;
        }
    }

    return LFS_ERR_NOENT;
}
#endif

