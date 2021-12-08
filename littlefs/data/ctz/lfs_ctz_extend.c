/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
int lfs_bd_erase(lfs_t *lfs, lfs_block_t block);
int lfs_bd_prog(lfs_t *lfs,
        lfs_cache_t *pcache, lfs_cache_t *rcache, bool validate,
        lfs_block_t block, lfs_off_t off,
        const void *buffer, lfs_size_t size);
int lfs_bd_read(lfs_t *lfs,
        const lfs_cache_t *pcache, lfs_cache_t *rcache, lfs_size_t hint,
        lfs_block_t block, lfs_off_t off,
        void *buffer, lfs_size_t size);
void lfs_cache_drop(lfs_t *lfs, lfs_cache_t *rcache);
int lfs_ctz_index(lfs_t *lfs, lfs_off_t *off);
uint32_t lfs_ctz(uint32_t a);
int lfs_alloc(lfs_t *lfs, lfs_block_t *block);
uint32_t lfs_fromle32(uint32_t a);
uint32_t lfs_tole32(uint32_t a);

#ifndef LFS_READONLY
// static 
int lfs_ctz_extend(lfs_t *lfs,
        lfs_cache_t *pcache, lfs_cache_t *rcache,
        lfs_block_t head, lfs_size_t size,
        lfs_block_t *block, lfs_off_t *off) {
    while (true) {
        // go ahead and grab a block
        lfs_block_t nblock;
        int err = lfs_alloc(lfs, &nblock);
        if (err) {
            return err;
        }

        {
            err = lfs_bd_erase(lfs, nblock);
            if (err) {
                if (err == LFS_ERR_CORRUPT) {
                    goto relocate;
                }
                return err;
            }

            if (size == 0) {
                *block = nblock;
                *off = 0;
                return 0;
            }

            lfs_size_t noff = size - 1;
            lfs_off_t index = lfs_ctz_index(lfs, &noff);
            noff = noff + 1;

            // just copy out the last block if it is incomplete
            if (noff != lfs->cfg->block_size) {
                for (lfs_off_t i = 0; i < noff; i++) {
                    uint8_t data;
                    err = lfs_bd_read(lfs,
                            NULL, rcache, noff-i,
                            head, i, &data, 1);
                    if (err) {
                        return err;
                    }

                    err = lfs_bd_prog(lfs,
                            pcache, rcache, true,
                            nblock, i, &data, 1);
                    if (err) {
                        if (err == LFS_ERR_CORRUPT) {
                            goto relocate;
                        }
                        return err;
                    }
                }

                *block = nblock;
                *off = noff;
                return 0;
            }

            // append block
            index += 1;
            lfs_size_t skips = lfs_ctz(index) + 1;
            lfs_block_t nhead = head;
            for (lfs_off_t i = 0; i < skips; i++) {
                nhead = lfs_tole32(nhead);
                err = lfs_bd_prog(lfs, pcache, rcache, true,
                        nblock, 4*i, &nhead, 4);
                nhead = lfs_fromle32(nhead);
                if (err) {
                    if (err == LFS_ERR_CORRUPT) {
                        goto relocate;
                    }
                    return err;
                }

                if (i != skips-1) {
                    err = lfs_bd_read(lfs,
                            NULL, rcache, sizeof(nhead),
                            nhead, 4*i, &nhead, sizeof(nhead));
                    nhead = lfs_fromle32(nhead);
                    if (err) {
                        return err;
                    }
                }
            }

            *block = nblock;
            *off = 4*skips;
            return 0;
        }

relocate:
        LFS_WARN("Bad block at 0x%"PRIx32, nblock);

        // just clear cache and try a new block
        lfs_cache_drop(lfs, pcache);
    }
}
#endif

