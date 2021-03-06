/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
int lfs_fs_demove(lfs_t *lfs);
int lfs_fs_deorphan(lfs_t *lfs);

#ifndef LFS_READONLY
// static 
int lfs_fs_forceconsistency(lfs_t *lfs) {
    int err = lfs_fs_demove(lfs);
    if (err) {
        return err;
    }

    err = lfs_fs_deorphan(lfs);
    if (err) {
        return err;
    }

    return 0;
}
#endif
