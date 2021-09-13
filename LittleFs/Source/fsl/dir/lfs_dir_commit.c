/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
void lfs_cache_drop(lfs_t *lfs, lfs_cache_t *rcache);
bool lfs_gstate_iszero(const lfs_gstate_t *a);
void lfs_gstate_tole32(lfs_gstate_t *a);
void lfs_gstate_xor(lfs_gstate_t *a, const lfs_gstate_t *b);
int lfs_pair_cmp(
        const lfs_block_t paira[2],
        const lfs_block_t pairb[2]);
void lfs_pair_fromle32(lfs_block_t pair[2]);
uint8_t lfs_tag_chunk(lfs_tag_t tag);
uint16_t lfs_tag_id(lfs_tag_t tag);
uint16_t lfs_tag_type1(lfs_tag_t tag);
uint16_t lfs_tag_type3(lfs_tag_t tag);
int lfs_dir_commit_commit(void *p, lfs_tag_t tag, const void *buffer);
int lfs_dir_commitattr(lfs_t *lfs, struct lfs_commit *commit,
        lfs_tag_t tag, const void *buffer);
int lfs_dir_commitcrc(lfs_t *lfs, struct lfs_commit *commit);
#ifndef LFS_READONLY
int lfs_dir_compact(lfs_t *lfs,
        lfs_mdir_t *dir, const struct lfs_mattr *attrs, int attrcount,
        lfs_mdir_t *source, uint16_t begin, uint16_t end);
#endif
int lfs_dir_drop(lfs_t *lfs, lfs_mdir_t *dir, lfs_mdir_t *tail);
int lfs_dir_fetch(lfs_t *lfs,
        lfs_mdir_t *dir, const lfs_block_t pair[2]);
int lfs_dir_getgstate(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_gstate_t *gstate);
int lfs_dir_traverse(lfs_t *lfs,
        const lfs_mdir_t *dir, lfs_off_t off, lfs_tag_t ptag,
        const struct lfs_mattr *attrs, int attrcount,
        lfs_tag_t tmask, lfs_tag_t ttag,
        uint16_t begin, uint16_t end, int16_t diff,
        int (*cb)(void *data, lfs_tag_t tag, const void *buffer), void *data);
#ifndef LFS_READONLY
int lfs_file_flush(lfs_t *lfs, lfs_file_t *file);
#endif
#ifndef LFS_READONLY
int lfs_file_outline(lfs_t *lfs, lfs_file_t *file);
#endif
#ifndef LFS_READONLY
int lfs_fs_pred(lfs_t *lfs,
        const lfs_block_t pair[2], lfs_mdir_t *pdir);
#endif
void lfs_pair_tole32(lfs_block_t pair[2]);

#ifndef LFS_READONLY
// static 
int lfs_dir_commit(lfs_t *lfs, lfs_mdir_t *dir,
        const struct lfs_mattr *attrs, int attrcount) {
    // check for any inline files that aren't RAM backed and
    // forcefully evict them, needed for filesystem consistency
    for (lfs_file_t *f = (lfs_file_t*)lfs->mlist; f; f = f->next) {
        if (dir != &f->m && lfs_pair_cmp(f->m.pair, dir->pair) == 0 &&
                f->type == LFS_TYPE_REG && (f->flags & LFS_F_INLINE) &&
                f->ctz.size > lfs->cfg->cache_size) {
            int err = lfs_file_outline(lfs, f);
            if (err) {
                return err;
            }

            err = lfs_file_flush(lfs, f);
            if (err) {
                return err;
            }
        }
    }

    // calculate changes to the directory
    lfs_mdir_t olddir = *dir;
    bool hasdelete = false;
    for (int i = 0; i < attrcount; i++) {
        if (lfs_tag_type3(attrs[i].tag) == LFS_TYPE_CREATE) {
            dir->count += 1;
        } else if (lfs_tag_type3(attrs[i].tag) == LFS_TYPE_DELETE) {
            LFS_ASSERT(dir->count > 0);
            dir->count -= 1;
            hasdelete = true;
        } else if (lfs_tag_type1(attrs[i].tag) == LFS_TYPE_TAIL) {
            dir->tail[0] = ((lfs_block_t*)attrs[i].buffer)[0];
            dir->tail[1] = ((lfs_block_t*)attrs[i].buffer)[1];
            dir->split = (lfs_tag_chunk(attrs[i].tag) & 1);
            lfs_pair_fromle32(dir->tail);
        }
    }

    // should we actually drop the directory block?
    if (hasdelete && dir->count == 0) {
        lfs_mdir_t pdir;
        int err = lfs_fs_pred(lfs, dir->pair, &pdir);
        if (err && err != LFS_ERR_NOENT) {
            *dir = olddir;
            return err;
        }

        if (err != LFS_ERR_NOENT && pdir.split) {
            err = lfs_dir_drop(lfs, &pdir, dir);
            if (err) {
                *dir = olddir;
                return err;
            }
        }
    }

    if (dir->erased || dir->count >= 0xff) {
        // try to commit
        struct lfs_commit commit = {
            .block = dir->pair[0],
            .off = dir->off,
            .ptag = dir->etag,
            .crc = 0xffffffff,

            .begin = dir->off,
            .end = (lfs->cfg->metadata_max ?
                lfs->cfg->metadata_max : lfs->cfg->block_size) - 8,
        };

        // traverse attrs that need to be written out
        lfs_pair_tole32(dir->tail);
        int err = lfs_dir_traverse(lfs,
                dir, dir->off, dir->etag, attrs, attrcount,
                0, 0, 0, 0, 0,
                lfs_dir_commit_commit, &(struct lfs_dir_commit_commit){
                    lfs, &commit});
        lfs_pair_fromle32(dir->tail);
        if (err) {
            if (err == LFS_ERR_NOSPC || err == LFS_ERR_CORRUPT) {
                goto compact;
            }
            *dir = olddir;
            return err;
        }

        // commit any global diffs if we have any
        lfs_gstate_t delta = {0};
        lfs_gstate_xor(&delta, &lfs->gstate);
        lfs_gstate_xor(&delta, &lfs->gdisk);
        lfs_gstate_xor(&delta, &lfs->gdelta);
        delta.tag &= ~LFS_MKTAG(0, 0, 0x3ff);
        if (!lfs_gstate_iszero(&delta)) {
            err = lfs_dir_getgstate(lfs, dir, &delta);
            if (err) {
                *dir = olddir;
                return err;
            }

            lfs_gstate_tole32(&delta);
            err = lfs_dir_commitattr(lfs, &commit,
                    LFS_MKTAG(LFS_TYPE_MOVESTATE, 0x3ff,
                        sizeof(delta)), &delta);
            if (err) {
                if (err == LFS_ERR_NOSPC || err == LFS_ERR_CORRUPT) {
                    goto compact;
                }
                *dir = olddir;
                return err;
            }
        }

        // finalize commit with the crc
        err = lfs_dir_commitcrc(lfs, &commit);
        if (err) {
            if (err == LFS_ERR_NOSPC || err == LFS_ERR_CORRUPT) {
                goto compact;
            }
            *dir = olddir;
            return err;
        }

        // successful commit, update dir
        LFS_ASSERT(commit.off % lfs->cfg->prog_size == 0);
        dir->off = commit.off;
        dir->etag = commit.ptag;
        // and update gstate
        lfs->gdisk = lfs->gstate;
        lfs->gdelta = (lfs_gstate_t){0};
    } else {
compact:
        // fall back to compaction
        lfs_cache_drop(lfs, &lfs->pcache);

        int err = lfs_dir_compact(lfs, dir, attrs, attrcount,
                dir, 0, dir->count);
        if (err) {
            *dir = olddir;
            return err;
        }
    }

    // this complicated bit of logic is for fixing up any active
    // metadata-pairs that we may have affected
    //
    // note we have to make two passes since the mdir passed to
    // lfs_dir_commit could also be in this list, and even then
    // we need to copy the pair so they don't get clobbered if we refetch
    // our mdir.
    for (struct lfs_mlist *d = lfs->mlist; d; d = d->next) {
        if (&d->m != dir && lfs_pair_cmp(d->m.pair, olddir.pair) == 0) {
            d->m = *dir;
            for (int i = 0; i < attrcount; i++) {
                if (lfs_tag_type3(attrs[i].tag) == LFS_TYPE_DELETE &&
                        d->id == lfs_tag_id(attrs[i].tag)) {
                    d->m.pair[0] = LFS_BLOCK_NULL;
                    d->m.pair[1] = LFS_BLOCK_NULL;
                } else if (lfs_tag_type3(attrs[i].tag) == LFS_TYPE_DELETE &&
                        d->id > lfs_tag_id(attrs[i].tag)) {
                    d->id -= 1;
                    if (d->type == LFS_TYPE_DIR) {
                        ((lfs_dir_t*)d)->pos -= 1;
                    }
                } else if (lfs_tag_type3(attrs[i].tag) == LFS_TYPE_CREATE &&
                        d->id >= lfs_tag_id(attrs[i].tag)) {
                    d->id += 1;
                    if (d->type == LFS_TYPE_DIR) {
                        ((lfs_dir_t*)d)->pos += 1;
                    }
                }
            }
        }
    }

    for (struct lfs_mlist *d = lfs->mlist; d; d = d->next) {
        if (lfs_pair_cmp(d->m.pair, olddir.pair) == 0) {
            while (d->id >= d->m.count && d->m.split) {
                // we split and id is on tail now
                d->id -= d->m.count;
                int err = lfs_dir_fetch(lfs, &d->m, d->m.tail);
                if (err) {
                    return err;
                }
            }
        }
    }

    return 0;
}
#endif


