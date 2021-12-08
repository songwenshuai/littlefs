/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
int lfs_bd_erase(lfs_t *lfs, lfs_block_t block);
void lfs_cache_drop(lfs_t *lfs, lfs_cache_t *rcache);
bool lfs_gstate_iszero(const lfs_gstate_t *a);
void lfs_gstate_tole32(lfs_gstate_t *a);
void lfs_gstate_xor(lfs_gstate_t *a, const lfs_gstate_t *b);
int lfs_pair_cmp(
        const lfs_block_t paira[2],
        const lfs_block_t pairb[2]);
void lfs_pair_fromle32(lfs_block_t pair[2]);
bool lfs_pair_isnull(const lfs_block_t pair[2]);
void lfs_pair_swap(lfs_block_t pair[2]);
int lfs_alloc(lfs_t *lfs, lfs_block_t *block);
int lfs_dir_commit_commit(void *p, lfs_tag_t tag, const void *buffer);
int lfs_dir_commit_size(void *p, lfs_tag_t tag, const void *buffer);
int lfs_dir_commitattr(lfs_t *lfs, struct lfs_commit *commit,
        lfs_tag_t tag, const void *buffer);
int lfs_dir_commitcrc(lfs_t *lfs, struct lfs_commit *commit);
int lfs_dir_commitprog(lfs_t *lfs, struct lfs_commit *commit,
        const void *buffer, lfs_size_t size);
int lfs_dir_getgstate(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_gstate_t *gstate);
int lfs_dir_split(lfs_t *lfs,
        lfs_mdir_t *dir, const struct lfs_mattr *attrs, int attrcount,
        lfs_mdir_t *source, uint16_t split, uint16_t end);
int lfs_dir_traverse(lfs_t *lfs,
        const lfs_mdir_t *dir, lfs_off_t off, lfs_tag_t ptag,
        const struct lfs_mattr *attrs, int attrcount,
        lfs_tag_t tmask, lfs_tag_t ttag,
        uint16_t begin, uint16_t end, int16_t diff,
        int (*cb)(void *data, lfs_tag_t tag, const void *buffer), void *data);
lfs_ssize_t lfs_fs_rawsize(lfs_t *lfs);
#ifndef LFS_READONLY
int lfs_fs_relocate(lfs_t *lfs,
        const lfs_block_t oldpair[2], lfs_block_t newpair[2]);
#endif
uint32_t lfs_alignup(uint32_t a, uint32_t alignment);
uint32_t lfs_fromle32(uint32_t a);
uint32_t lfs_min(uint32_t a, uint32_t b);
void lfs_pair_tole32(lfs_block_t pair[2]);
uint32_t lfs_tole32(uint32_t a);

#ifndef LFS_READONLY
// static 
int lfs_dir_compact(lfs_t *lfs,
        lfs_mdir_t *dir, const struct lfs_mattr *attrs, int attrcount,
        lfs_mdir_t *source, uint16_t begin, uint16_t end) {
    // save some state in case block is bad
    const lfs_block_t oldpair[2] = {dir->pair[0], dir->pair[1]};
    bool relocated = false;
    bool tired = false;

    // should we split?
    while (end - begin > 1) {
        // find size
        lfs_size_t size = 0;
        int err = lfs_dir_traverse(lfs,
                source, 0, 0xffffffff, attrs, attrcount,
                LFS_MKTAG(0x400, 0x3ff, 0),
                LFS_MKTAG(LFS_TYPE_NAME, 0, 0),
                begin, end, -begin,
                lfs_dir_commit_size, &size);
        if (err) {
            return err;
        }

        // space is complicated, we need room for tail, crc, gstate,
        // cleanup delete, and we cap at half a block to give room
        // for metadata updates.
        if (end - begin < 0xff &&
                size <= lfs_min(lfs->cfg->block_size - 36,
                    lfs_alignup((lfs->cfg->metadata_max ?
                            lfs->cfg->metadata_max : lfs->cfg->block_size)/2,
                        lfs->cfg->prog_size))) {
            break;
        }

        // can't fit, need to split, we should really be finding the
        // largest size that fits with a small binary search, but right now
        // it's not worth the code size
        uint16_t split = (end - begin) / 2;
        err = lfs_dir_split(lfs, dir, attrs, attrcount,
                source, begin+split, end);
        if (err) {
            // if we fail to split, we may be able to overcompact, unless
            // we're too big for even the full block, in which case our
            // only option is to error
            if (err == LFS_ERR_NOSPC && size <= lfs->cfg->block_size - 36) {
                break;
            }
            return err;
        }

        end = begin + split;
    }

    // increment revision count
    dir->rev += 1;
    // If our revision count == n * block_cycles, we should force a relocation,
    // this is how littlefs wear-levels at the metadata-pair level. Note that we
    // actually use (block_cycles+1)|1, this is to avoid two corner cases:
    // 1. block_cycles = 1, which would prevent relocations from terminating
    // 2. block_cycles = 2n, which, due to aliasing, would only ever relocate
    //    one metadata block in the pair, effectively making this useless
    if (lfs->cfg->block_cycles > 0 &&
            (dir->rev % ((lfs->cfg->block_cycles+1)|1) == 0)) {
        if (lfs_pair_cmp(dir->pair, (const lfs_block_t[2]){0, 1}) == 0) {
            // oh no! we're writing too much to the superblock,
            // should we expand?
            lfs_ssize_t res = lfs_fs_rawsize(lfs);
            if (res < 0) {
                return res;
            }

            // do we have extra space? littlefs can't reclaim this space
            // by itself, so expand cautiously
            if ((lfs_size_t)res < lfs->cfg->block_count/2) {
                LFS_WARN("Expanding superblock at rev %"PRIu32, dir->rev);
                int err = lfs_dir_split(lfs, dir, attrs, attrcount,
                        source, begin, end);
                if (err && err != LFS_ERR_NOSPC) {
                    return err;
                }

                // welp, we tried, if we ran out of space there's not much
                // we can do, we'll error later if we've become frozen
                if (!err) {
                    end = begin;
                }
            }
        } else {
            // we're writing too much, time to relocate
            tired = true;
            goto relocate;
        }
    }

    // begin loop to commit compaction to blocks until a compact sticks
    while (true) {
        {
            // setup commit state
            struct lfs_commit commit = {
                .block = dir->pair[1],
                .off = 0,
                .ptag = 0xffffffff,
                .crc = 0xffffffff,

                .begin = 0,
                .end = (lfs->cfg->metadata_max ?
                    lfs->cfg->metadata_max : lfs->cfg->block_size) - 8,
            };

            // erase block to write to
            int err = lfs_bd_erase(lfs, dir->pair[1]);
            if (err) {
                if (err == LFS_ERR_CORRUPT) {
                    goto relocate;
                }
                return err;
            }

            // write out header
            dir->rev = lfs_tole32(dir->rev);
            err = lfs_dir_commitprog(lfs, &commit,
                    &dir->rev, sizeof(dir->rev));
            dir->rev = lfs_fromle32(dir->rev);
            if (err) {
                if (err == LFS_ERR_CORRUPT) {
                    goto relocate;
                }
                return err;
            }

            // traverse the directory, this time writing out all unique tags
            err = lfs_dir_traverse(lfs,
                    source, 0, 0xffffffff, attrs, attrcount,
                    LFS_MKTAG(0x400, 0x3ff, 0),
                    LFS_MKTAG(LFS_TYPE_NAME, 0, 0),
                    begin, end, -begin,
                    lfs_dir_commit_commit, &(struct lfs_dir_commit_commit){
                        lfs, &commit});
            if (err) {
                if (err == LFS_ERR_CORRUPT) {
                    goto relocate;
                }
                return err;
            }

            // commit tail, which may be new after last size check
            if (!lfs_pair_isnull(dir->tail)) {
                lfs_pair_tole32(dir->tail);
                err = lfs_dir_commitattr(lfs, &commit,
                        LFS_MKTAG(LFS_TYPE_TAIL + dir->split, 0x3ff, 8),
                        dir->tail);
                lfs_pair_fromle32(dir->tail);
                if (err) {
                    if (err == LFS_ERR_CORRUPT) {
                        goto relocate;
                    }
                    return err;
                }
            }

            // bring over gstate?
            lfs_gstate_t delta = {0};
            if (!relocated) {
                lfs_gstate_xor(&delta, &lfs->gdisk);
                lfs_gstate_xor(&delta, &lfs->gstate);
            }
            lfs_gstate_xor(&delta, &lfs->gdelta);
            delta.tag &= ~LFS_MKTAG(0, 0, 0x3ff);

            err = lfs_dir_getgstate(lfs, dir, &delta);
            if (err) {
                return err;
            }

            if (!lfs_gstate_iszero(&delta)) {
                lfs_gstate_tole32(&delta);
                err = lfs_dir_commitattr(lfs, &commit,
                        LFS_MKTAG(LFS_TYPE_MOVESTATE, 0x3ff,
                            sizeof(delta)), &delta);
                if (err) {
                    if (err == LFS_ERR_CORRUPT) {
                        goto relocate;
                    }
                    return err;
                }
            }

            // complete commit with crc
            err = lfs_dir_commitcrc(lfs, &commit);
            if (err) {
                if (err == LFS_ERR_CORRUPT) {
                    goto relocate;
                }
                return err;
            }

            // successful compaction, swap dir pair to indicate most recent
            LFS_ASSERT(commit.off % lfs->cfg->prog_size == 0);
            lfs_pair_swap(dir->pair);
            dir->count = end - begin;
            dir->off = commit.off;
            dir->etag = commit.ptag;
            // update gstate
            lfs->gdelta = (lfs_gstate_t){0};
            if (!relocated) {
                lfs->gdisk = lfs->gstate;
            }
        }
        break;

relocate:
        // commit was corrupted, drop caches and prepare to relocate block
        relocated = true;
        lfs_cache_drop(lfs, &lfs->pcache);
        if (!tired) {
            LFS_WARN("Bad block at 0x%"PRIx32, dir->pair[1]);
        }

        // can't relocate superblock, filesystem is now frozen
        if (lfs_pair_cmp(dir->pair, (const lfs_block_t[2]){0, 1}) == 0) {
            LFS_ERROR("Superblock 0x%"PRIx32" has become unwritable",
                    dir->pair[1]);
            return LFS_ERR_NOSPC;
        }

        // relocate half of pair
        int err = lfs_alloc(lfs, &dir->pair[1]);
        if (err && (err != LFS_ERR_NOSPC || !tired)) {
            return err;
        }

        tired = false;
        continue;
    }

    if (relocated) {
        // update references if we relocated
        LFS_WARN("Relocating {0x%"PRIx32", 0x%"PRIx32"} "
                    "-> {0x%"PRIx32", 0x%"PRIx32"}",
                oldpair[0], oldpair[1], dir->pair[0], dir->pair[1]);
        int err = lfs_fs_relocate(lfs, oldpair, dir->pair);
        if (err) {
            return err;
        }
    }

    return 0;
}
#endif

