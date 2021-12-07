/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
uint16_t lfs_tag_id(lfs_tag_t tag);
lfs_stag_t lfs_dir_find(lfs_t *lfs, lfs_mdir_t *dir,
        const char **path, uint16_t *id);
int lfs_dir_getinfo(lfs_t *lfs, lfs_mdir_t *dir,
        uint16_t id, struct lfs_info *info);

/// General fs operations ///
// static 
int lfs_rawstat(lfs_t *lfs, const char *path, struct lfs_info *info) {
    lfs_mdir_t cwd;
    lfs_stag_t tag = lfs_dir_find(lfs, &cwd, &path, NULL);
    if (tag < 0) {
        return (int)tag;
    }

    return lfs_dir_getinfo(lfs, &cwd, lfs_tag_id(tag), info);
}

