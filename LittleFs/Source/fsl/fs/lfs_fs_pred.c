/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_pair_cmp(
        const lfs_block_t paira[2],
        const lfs_block_t pairb[2]);
bool lfs_pair_isnull(const lfs_block_t pair[2]);
int lfs_dir_fetch(lfs_t *lfs,
        lfs_mdir_t *dir, const lfs_block_t pair[2]);

#ifndef LFS_READONLY
// static 
int lfs_fs_pred(lfs_t *lfs,
        const lfs_block_t pair[2], lfs_mdir_t *pdir) {
    // iterate over all directory directory entries
    pdir->tail[0] = 0;
    pdir->tail[1] = 1;
    lfs_block_t cycle = 0;
    while (!lfs_pair_isnull(pdir->tail)) {
        if (cycle >= lfs->cfg->block_count/2) {
            // loop detected
            return LFS_ERR_CORRUPT;
        }
        cycle += 1;

        if (lfs_pair_cmp(pdir->tail, pair) == 0) {
            return 0;
        }

        int err = lfs_dir_fetch(lfs, pdir, pdir->tail);
        if (err) {
            return err;
        }
    }

    return LFS_ERR_NOENT;
}
#endif
