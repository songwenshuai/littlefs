/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

extern void lfs_free(void *p);
void lfs_mlist_remove(lfs_t *lfs, struct lfs_mlist *mlist);
int lfs_file_rawsync(lfs_t *lfs, lfs_file_t *file);

// static 
int lfs_file_rawclose(lfs_t *lfs, lfs_file_t *file) {
#ifndef LFS_READONLY
    int err = lfs_file_rawsync(lfs, file);
#else
    int err = 0;
#endif

    // remove from list of mdirs
    lfs_mlist_remove(lfs, (struct lfs_mlist*)file);

    // clean up memory
    if (!file->cfg->buffer) {
        lfs_free(file->cache.buffer);
    }

    return err;
}


