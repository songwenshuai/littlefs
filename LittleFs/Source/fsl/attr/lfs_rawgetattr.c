/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
uint16_t lfs_tag_id(lfs_tag_t tag);
lfs_size_t lfs_tag_size(lfs_tag_t tag);
int lfs_dir_fetch(lfs_t *lfs,
        lfs_mdir_t *dir, const lfs_block_t pair[2]);
lfs_stag_t lfs_dir_find(lfs_t *lfs, lfs_mdir_t *dir,
        const char **path, uint16_t *id);
lfs_stag_t lfs_dir_get(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_tag_t gmask, lfs_tag_t gtag, void *buffer);
uint32_t lfs_min(uint32_t a, uint32_t b);

// static 
lfs_ssize_t lfs_rawgetattr(lfs_t *lfs, const char *path,
        uint8_t type, void *buffer, lfs_size_t size) {
    lfs_mdir_t cwd;
    lfs_stag_t tag = lfs_dir_find(lfs, &cwd, &path, NULL);
    if (tag < 0) {
        return tag;
    }

    uint16_t id = lfs_tag_id(tag);
    if (id == 0x3ff) {
        // special case for root
        id = 0;
        int err = lfs_dir_fetch(lfs, &cwd, lfs->root);
        if (err) {
            return err;
        }
    }

    tag = lfs_dir_get(lfs, &cwd, LFS_MKTAG(0x7ff, 0x3ff, 0),
            LFS_MKTAG(LFS_TYPE_USERATTR + type,
                id, lfs_min(size, lfs->attr_max)),
            buffer);
    if (tag < 0) {
        if (tag == LFS_ERR_NOENT) {
            return LFS_ERR_NOATTR;
        }

        return tag;
    }

    return lfs_tag_size(tag);
}

