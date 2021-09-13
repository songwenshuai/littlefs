/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

int lfs_fs_rawtraverse(lfs_t *lfs,
        int (*cb)(void *data, lfs_block_t block), void *data,
        bool includeorphans);
int lfs_fs_size_count(void *p, lfs_block_t block);

// static 
lfs_ssize_t lfs_fs_rawsize(lfs_t *lfs) {
    lfs_size_t size = 0;
    int err = lfs_fs_rawtraverse(lfs, lfs_fs_size_count, &size, false);
    if (err) {
        return err;
    }

    return size;
}
