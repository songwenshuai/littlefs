/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
void lfs_mlist_remove(lfs_t *lfs, struct lfs_mlist *mlist);

// static 
int lfs_dir_rawclose(lfs_t *lfs, lfs_dir_t *dir) {
    // remove from list of mdirs
    lfs_mlist_remove(lfs, (struct lfs_mlist *)dir);

    return 0;
}

