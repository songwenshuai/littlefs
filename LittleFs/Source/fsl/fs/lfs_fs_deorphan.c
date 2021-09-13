/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
uint8_t lfs_gstate_getorphans(const lfs_gstate_t *a);
bool lfs_gstate_hasorphans(const lfs_gstate_t *a);
void lfs_pair_fromle32(lfs_block_t pair[2]);
bool lfs_pair_isnull(const lfs_block_t pair[2]);
bool lfs_pair_sync(
        const lfs_block_t paira[2],
        const lfs_block_t pairb[2]);
#ifndef LFS_READONLY
int lfs_dir_commit(lfs_t *lfs, lfs_mdir_t *dir,
        const struct lfs_mattr *attrs, int attrcount);
#endif
int lfs_dir_drop(lfs_t *lfs, lfs_mdir_t *dir, lfs_mdir_t *tail);
int lfs_dir_fetch(lfs_t *lfs,
        lfs_mdir_t *dir, const lfs_block_t pair[2]);
lfs_stag_t lfs_dir_get(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_tag_t gmask, lfs_tag_t gtag, void *buffer);
#ifndef LFS_READONLY
lfs_stag_t lfs_fs_parent(lfs_t *lfs, const lfs_block_t pair[2],
        lfs_mdir_t *parent);
#endif
int lfs_fs_preporphans(lfs_t *lfs, int8_t orphans);
void lfs_pair_tole32(lfs_block_t pair[2]);

#ifndef LFS_READONLY
// static 
int lfs_fs_deorphan(lfs_t *lfs) {
    if (!lfs_gstate_hasorphans(&lfs->gstate)) {
        return 0;
    }

    // Fix any orphans
    lfs_mdir_t pdir = {.split = true, .tail = {0, 1}};
    lfs_mdir_t dir;

    // iterate over all directory directory entries
    while (!lfs_pair_isnull(pdir.tail)) {
        int err = lfs_dir_fetch(lfs, &dir, pdir.tail);
        if (err) {
            return err;
        }

        // check head blocks for orphans
        if (!pdir.split) {
            // check if we have a parent
            lfs_mdir_t parent;
            lfs_stag_t tag = lfs_fs_parent(lfs, pdir.tail, &parent);
            if (tag < 0 && tag != LFS_ERR_NOENT) {
                return tag;
            }

            if (tag == LFS_ERR_NOENT) {
                // we are an orphan
                LFS_DEBUG("Fixing orphan {0x%"PRIx32", 0x%"PRIx32"}",
                        pdir.tail[0], pdir.tail[1]);

                err = lfs_dir_drop(lfs, &pdir, &dir);
                if (err) {
                    return err;
                }

                // refetch tail
                continue;
            }

            lfs_block_t pair[2];
            lfs_stag_t res = lfs_dir_get(lfs, &parent,
                    LFS_MKTAG(0x7ff, 0x3ff, 0), tag, pair);
            if (res < 0) {
                return res;
            }
            lfs_pair_fromle32(pair);

            if (!lfs_pair_sync(pair, pdir.tail)) {
                // we have desynced
                LFS_DEBUG("Fixing half-orphan {0x%"PRIx32", 0x%"PRIx32"} "
                            "-> {0x%"PRIx32", 0x%"PRIx32"}",
                        pdir.tail[0], pdir.tail[1], pair[0], pair[1]);

                lfs_pair_tole32(pair);
                err = lfs_dir_commit(lfs, &pdir, LFS_MKATTRS(
                        {LFS_MKTAG(LFS_TYPE_SOFTTAIL, 0x3ff, 8), pair}));
                lfs_pair_fromle32(pair);
                if (err) {
                    return err;
                }

                // refetch tail
                continue;
            }
        }

        pdir = dir;
    }

    // mark orphans as fixed
    return lfs_fs_preporphans(lfs, -lfs_gstate_getorphans(&lfs->gstate));
}
#endif
