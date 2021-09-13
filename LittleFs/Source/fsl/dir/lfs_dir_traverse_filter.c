/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

#ifndef LFS_READONLY
// static 
int lfs_dir_traverse_filter(void *p,
        lfs_tag_t tag, const void *buffer) {
    lfs_tag_t *filtertag = p;
    (void)buffer;

    // which mask depends on unique bit in tag structure
    uint32_t mask = (tag & LFS_MKTAG(0x100, 0, 0))
            ? LFS_MKTAG(0x7ff, 0x3ff, 0)
            : LFS_MKTAG(0x700, 0x3ff, 0);

    // check for redundancy
    if ((mask & tag) == (mask & *filtertag) ||
            lfs_tag_isdelete(*filtertag) ||
            (LFS_MKTAG(0x7ff, 0x3ff, 0) & tag) == (
                LFS_MKTAG(LFS_TYPE_DELETE, 0, 0) |
                    (LFS_MKTAG(0, 0x3ff, 0) & *filtertag))) {
        return true;
    }

    // check if we need to adjust for created/deleted tags
    if (lfs_tag_type1(tag) == LFS_TYPE_SPLICE &&
            lfs_tag_id(tag) <= lfs_tag_id(*filtertag)) {
        *filtertag += LFS_MKTAG(0, lfs_tag_splice(tag), 0);
    }

    return false;
}
#endif

