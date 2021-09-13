/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
int lfs_deinit(lfs_t *lfs);

// static 
int lfs_rawunmount(lfs_t *lfs) {
    return lfs_deinit(lfs);
}
