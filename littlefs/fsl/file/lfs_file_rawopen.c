/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
int lfs_file_rawopencfg(lfs_t *lfs, lfs_file_t *file,
        const char *path, int flags,
        const struct lfs_file_config *cfg);

// static 
int lfs_file_rawopen(lfs_t *lfs, lfs_file_t *file,
        const char *path, int flags) {
    static const struct lfs_file_config defaults = {0};
    int err = lfs_file_rawopencfg(lfs, file, path, flags, &defaults);
    return err;
}

