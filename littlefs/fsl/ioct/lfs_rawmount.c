/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
bool lfs_gstate_iszero(const lfs_gstate_t *a);
bool lfs_pair_isnull(const lfs_block_t pair[2]);
void lfs_superblock_fromle32(lfs_superblock_t *superblock);
bool lfs_tag_isdelete(lfs_tag_t tag);
bool lfs_tag_isvalid(lfs_tag_t tag);
void lfs_alloc_drop(lfs_t *lfs);
lfs_stag_t lfs_dir_fetchmatch(lfs_t *lfs,
        lfs_mdir_t *dir, const lfs_block_t pair[2],
        lfs_tag_t fmask, lfs_tag_t ftag, uint16_t *id,
        int (*cb)(void *data, lfs_tag_t tag, const void *buffer), void *data);
int lfs_dir_find_match(void *data,
        lfs_tag_t tag, const void *buffer);
lfs_stag_t lfs_dir_get(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_tag_t gmask, lfs_tag_t gtag, void *buffer);
int lfs_dir_getgstate(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_gstate_t *gstate);
int lfs_init(lfs_t *lfs, const struct lfs_config *cfg);
int lfs_rawunmount(lfs_t *lfs);

// static 
int lfs_rawmount(lfs_t *lfs, const struct lfs_config *cfg) {
    int err = lfs_init(lfs, cfg);
    if (err) {
        return err;
    }

    // scan directory blocks for superblock and any global updates
    lfs_mdir_t dir = {.tail = {0, 1}};
    lfs_block_t cycle = 0;
    while (!lfs_pair_isnull(dir.tail)) {
        if (cycle >= lfs->cfg->block_count/2) {
            // loop detected
            err = LFS_ERR_CORRUPT;
            goto cleanup;
        }
        cycle += 1;

        // fetch next block in tail list
        lfs_stag_t tag = lfs_dir_fetchmatch(lfs, &dir, dir.tail,
                LFS_MKTAG(0x7ff, 0x3ff, 0),
                LFS_MKTAG(LFS_TYPE_SUPERBLOCK, 0, 8),
                NULL,
                lfs_dir_find_match, &(struct lfs_dir_find_match){
                    lfs, "littlefs", 8});
        if (tag < 0) {
            err = tag;
            goto cleanup;
        }

        // has superblock?
        if (tag && !lfs_tag_isdelete(tag)) {
            // update root
            lfs->root[0] = dir.pair[0];
            lfs->root[1] = dir.pair[1];

            // grab superblock
            lfs_superblock_t superblock;
            tag = lfs_dir_get(lfs, &dir, LFS_MKTAG(0x7ff, 0x3ff, 0),
                    LFS_MKTAG(LFS_TYPE_INLINESTRUCT, 0, sizeof(superblock)),
                    &superblock);
            if (tag < 0) {
                err = tag;
                goto cleanup;
            }
            lfs_superblock_fromle32(&superblock);

            // check version
            uint16_t major_version = (0xffff & (superblock.version >> 16));
            uint16_t minor_version = (0xffff & (superblock.version >>  0));
            if ((major_version != LFS_DISK_VERSION_MAJOR ||
                 minor_version > LFS_DISK_VERSION_MINOR)) {
                LFS_ERROR("Invalid version v%"PRIu16".%"PRIu16,
                        major_version, minor_version);
                err = LFS_ERR_INVAL;
                goto cleanup;
            }

            // check superblock configuration
            if (superblock.name_max) {
                if (superblock.name_max > lfs->name_max) {
                    LFS_ERROR("Unsupported name_max (%"PRIu32" > %"PRIu32")",
                            superblock.name_max, lfs->name_max);
                    err = LFS_ERR_INVAL;
                    goto cleanup;
                }

                lfs->name_max = superblock.name_max;
            }

            if (superblock.file_max) {
                if (superblock.file_max > lfs->file_max) {
                    LFS_ERROR("Unsupported file_max (%"PRIu32" > %"PRIu32")",
                            superblock.file_max, lfs->file_max);
                    err = LFS_ERR_INVAL;
                    goto cleanup;
                }

                lfs->file_max = superblock.file_max;
            }

            if (superblock.attr_max) {
                if (superblock.attr_max > lfs->attr_max) {
                    LFS_ERROR("Unsupported attr_max (%"PRIu32" > %"PRIu32")",
                            superblock.attr_max, lfs->attr_max);
                    err = LFS_ERR_INVAL;
                    goto cleanup;
                }

                lfs->attr_max = superblock.attr_max;
            }
        }

        // has gstate?
        err = lfs_dir_getgstate(lfs, &dir, &lfs->gstate);
        if (err) {
            goto cleanup;
        }
    }

    // found superblock?
    if (lfs_pair_isnull(lfs->root)) {
        err = LFS_ERR_INVAL;
        goto cleanup;
    }

    // update littlefs with gstate
    if (!lfs_gstate_iszero(&lfs->gstate)) {
        LFS_WARN("Found pending gstate 0x%08"PRIx32"%08"PRIx32"%08"PRIx32,
                lfs->gstate.tag,
                lfs->gstate.pair[0],
                lfs->gstate.pair[1]);
    }
    lfs->gstate.tag += !lfs_tag_isvalid(lfs->gstate.tag);
    lfs->gdisk = lfs->gstate;

    // setup free lookahead, to distribute allocations uniformly across
    // boots, we start the allocator at a random location
    lfs->free.off = lfs->seed % lfs->cfg->block_count;
    lfs_alloc_drop(lfs);

    return 0;

cleanup:
    lfs_rawunmount(lfs);
    return err;
}
