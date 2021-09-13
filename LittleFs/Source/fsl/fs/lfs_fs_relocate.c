/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

#ifndef LFS_READONLY
// static 
int lfs_fs_relocate(lfs_t *lfs,
        const lfs_block_t oldpair[2], lfs_block_t newpair[2]) {
    // update internal root
    if (lfs_pair_cmp(oldpair, lfs->root) == 0) {
        lfs->root[0] = newpair[0];
        lfs->root[1] = newpair[1];
    }

    // update internally tracked dirs
    for (struct lfs_mlist *d = lfs->mlist; d; d = d->next) {
        if (lfs_pair_cmp(oldpair, d->m.pair) == 0) {
            d->m.pair[0] = newpair[0];
            d->m.pair[1] = newpair[1];
        }

        if (d->type == LFS_TYPE_DIR &&
                lfs_pair_cmp(oldpair, ((lfs_dir_t*)d)->head) == 0) {
            ((lfs_dir_t*)d)->head[0] = newpair[0];
            ((lfs_dir_t*)d)->head[1] = newpair[1];
        }
    }

    // find parent
    lfs_mdir_t parent;
    lfs_stag_t tag = lfs_fs_parent(lfs, oldpair, &parent);
    if (tag < 0 && tag != LFS_ERR_NOENT) {
        return tag;
    }

    if (tag != LFS_ERR_NOENT) {
        // update disk, this creates a desync
        int err = lfs_fs_preporphans(lfs, +1);
        if (err) {
            return err;
        }

        // fix pending move in this pair? this looks like an optimization but
        // is in fact _required_ since relocating may outdate the move.
        uint16_t moveid = 0x3ff;
        if (lfs_gstate_hasmovehere(&lfs->gstate, parent.pair)) {
            moveid = lfs_tag_id(lfs->gstate.tag);
            LFS_DEBUG("Fixing move while relocating "
                    "{0x%"PRIx32", 0x%"PRIx32"} 0x%"PRIx16"\n",
                    parent.pair[0], parent.pair[1], moveid);
            lfs_fs_prepmove(lfs, 0x3ff, NULL);
            if (moveid < lfs_tag_id(tag)) {
                tag -= LFS_MKTAG(0, 1, 0);
            }
        }

        lfs_pair_tole32(newpair);
        err = lfs_dir_commit(lfs, &parent, LFS_MKATTRS(
                {LFS_MKTAG_IF(moveid != 0x3ff,
                    LFS_TYPE_DELETE, moveid, 0), NULL},
                {tag, newpair}));
        lfs_pair_fromle32(newpair);
        if (err) {
            return err;
        }

        // next step, clean up orphans
        err = lfs_fs_preporphans(lfs, -1);
        if (err) {
            return err;
        }
    }

    // find pred
    int err = lfs_fs_pred(lfs, oldpair, &parent);
    if (err && err != LFS_ERR_NOENT) {
        return err;
    }

    // if we can't find dir, it must be new
    if (err != LFS_ERR_NOENT) {
        // fix pending move in this pair? this looks like an optimization but
        // is in fact _required_ since relocating may outdate the move.
        uint16_t moveid = 0x3ff;
        if (lfs_gstate_hasmovehere(&lfs->gstate, parent.pair)) {
            moveid = lfs_tag_id(lfs->gstate.tag);
            LFS_DEBUG("Fixing move while relocating "
                    "{0x%"PRIx32", 0x%"PRIx32"} 0x%"PRIx16"\n",
                    parent.pair[0], parent.pair[1], moveid);
            lfs_fs_prepmove(lfs, 0x3ff, NULL);
        }

        // replace bad pair, either we clean up desync, or no desync occured
        lfs_pair_tole32(newpair);
        err = lfs_dir_commit(lfs, &parent, LFS_MKATTRS(
                {LFS_MKTAG_IF(moveid != 0x3ff,
                    LFS_TYPE_DELETE, moveid, 0), NULL},
                {LFS_MKTAG(LFS_TYPE_TAIL + parent.split, 0x3ff, 8), newpair}));
        lfs_pair_fromle32(newpair);
        if (err) {
            return err;
        }
    }

    return 0;
}
#endif
