/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_ctz_find(lfs_t *lfs,
        const lfs_cache_t *pcache, lfs_cache_t *rcache,
        lfs_block_t head, lfs_size_t size,
        lfs_size_t pos, lfs_block_t *block, lfs_off_t *off);
int lfs_file_flush(lfs_t *lfs, lfs_file_t *file);
lfs_soff_t lfs_file_rawseek(lfs_t *lfs, lfs_file_t *file,
        lfs_soff_t off, int whence);
lfs_soff_t lfs_file_rawsize(lfs_t *lfs, lfs_file_t *file);
lfs_ssize_t lfs_file_rawwrite(lfs_t *lfs, lfs_file_t *file,
        const void *buffer, lfs_size_t size);

#ifndef LFS_READONLY
// static 
int lfs_file_rawtruncate(lfs_t *lfs, lfs_file_t *file, lfs_off_t size) {
    LFS_ASSERT((file->flags & LFS_O_WRONLY) == LFS_O_WRONLY);

    if (size > LFS_FILE_MAX) {
        return LFS_ERR_INVAL;
    }

    lfs_off_t pos = file->pos;
    lfs_off_t oldsize = lfs_file_rawsize(lfs, file);
    if (size < oldsize) {
        // need to flush since directly changing metadata
        int err = lfs_file_flush(lfs, file);
        if (err) {
            return err;
        }

        // lookup new head in ctz skip list
        err = lfs_ctz_find(lfs, NULL, &file->cache,
                file->ctz.head, file->ctz.size,
                size, &file->block, &file->off);
        if (err) {
            return err;
        }

        // need to set pos/block/off consistently so seeking back to
        // the old position does not get confused
        file->pos = size;
        file->ctz.head = file->block;
        file->ctz.size = size;
        file->flags |= LFS_F_DIRTY | LFS_F_READING;
    } else if (size > oldsize) {
        // flush+seek if not already at end
        lfs_soff_t res = lfs_file_rawseek(lfs, file, 0, LFS_SEEK_END);
        if (res < 0) {
            return (int)res;
        }

        // fill with zeros
        while (file->pos < size) {
            res = lfs_file_rawwrite(lfs, file, &(uint8_t){0}, 1);
            if (res < 0) {
                return (int)res;
            }
        }
    }

    // restore pos
    lfs_soff_t res = lfs_file_rawseek(lfs, file, pos, LFS_SEEK_SET);
    if (res < 0) {
      return (int)res;
    }

    return 0;
}
#endif

