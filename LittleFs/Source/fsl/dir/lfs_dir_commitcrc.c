/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_bd_prog(lfs_t *lfs,
        lfs_cache_t *pcache, lfs_cache_t *rcache, bool validate,
        lfs_block_t block, lfs_off_t off,
        const void *buffer, lfs_size_t size);
int lfs_bd_read(lfs_t *lfs,
        const lfs_cache_t *pcache, lfs_cache_t *rcache, lfs_size_t hint,
        lfs_block_t block, lfs_off_t off,
        void *buffer, lfs_size_t size);
int lfs_bd_sync(lfs_t *lfs,
        lfs_cache_t *pcache, lfs_cache_t *rcache, bool validate);

#ifndef LFS_READONLY
// static 
int lfs_dir_commitcrc(lfs_t *lfs, struct lfs_commit *commit) {
    // align to program units
    const lfs_off_t end = lfs_alignup(commit->off + 2*sizeof(uint32_t),
            lfs->cfg->prog_size);

    lfs_off_t off1 = 0;
    uint32_t crc1 = 0;

    // create crc tags to fill up remainder of commit, note that
    // padding is not crced, which lets fetches skip padding but
    // makes committing a bit more complicated
    while (commit->off < end) {
        lfs_off_t off = commit->off + sizeof(lfs_tag_t);
        lfs_off_t noff = lfs_min(end - off, 0x3fe) + off;
        if (noff < end) {
            noff = lfs_min(noff, end - 2*sizeof(uint32_t));
        }

        // read erased state from next program unit
        lfs_tag_t tag = 0xffffffff;
        int err = lfs_bd_read(lfs,
                NULL, &lfs->rcache, sizeof(tag),
                commit->block, noff, &tag, sizeof(tag));
        if (err && err != LFS_ERR_CORRUPT) {
            return err;
        }

        // build crc tag
        bool reset = ~lfs_frombe32(tag) >> 31;
        tag = LFS_MKTAG(LFS_TYPE_CRC + reset, 0x3ff, noff - off);

        // write out crc
        uint32_t footer[2];
        footer[0] = lfs_tobe32(tag ^ commit->ptag);
        commit->crc = lfs_crc(commit->crc, &footer[0], sizeof(footer[0]));
        footer[1] = lfs_tole32(commit->crc);
        err = lfs_bd_prog(lfs,
                &lfs->pcache, &lfs->rcache, false,
                commit->block, commit->off, &footer, sizeof(footer));
        if (err) {
            return err;
        }

        // keep track of non-padding checksum to verify
        if (off1 == 0) {
            off1 = commit->off + sizeof(uint32_t);
            crc1 = commit->crc;
        }

        commit->off += sizeof(tag)+lfs_tag_size(tag);
        commit->ptag = tag ^ ((lfs_tag_t)reset << 31);
        commit->crc = 0xffffffff; // reset crc for next "commit"
    }

    // flush buffers
    int err = lfs_bd_sync(lfs, &lfs->pcache, &lfs->rcache, false);
    if (err) {
        return err;
    }

    // successful commit, check checksums to make sure
    lfs_off_t off = commit->begin;
    lfs_off_t noff = off1;
    while (off < end) {
        uint32_t crc = 0xffffffff;
        for (lfs_off_t i = off; i < noff+sizeof(uint32_t); i++) {
            // check against written crc, may catch blocks that
            // become readonly and match our commit size exactly
            if (i == off1 && crc != crc1) {
                return LFS_ERR_CORRUPT;
            }

            // leave it up to caching to make this efficient
            uint8_t dat;
            err = lfs_bd_read(lfs,
                    NULL, &lfs->rcache, noff+sizeof(uint32_t)-i,
                    commit->block, i, &dat, 1);
            if (err) {
                return err;
            }

            crc = lfs_crc(crc, &dat, 1);
        }

        // detected write error?
        if (crc != 0) {
            return LFS_ERR_CORRUPT;
        }

        // skip padding
        off = lfs_min(end - noff, 0x3fe) + noff;
        if (off < end) {
            off = lfs_min(off, end - 2*sizeof(uint32_t));
        }
        noff = off + sizeof(uint32_t);
    }

    return 0;
}
#endif

