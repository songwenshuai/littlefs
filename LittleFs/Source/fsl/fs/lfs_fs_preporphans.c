/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

#ifndef LFS_READONLY
static int lfs_fs_preporphans(lfs_t *lfs, int8_t orphans) {
    LFS_ASSERT(lfs_tag_size(lfs->gstate.tag) > 0 || orphans >= 0);
    lfs->gstate.tag += orphans;
    lfs->gstate.tag = ((lfs->gstate.tag & ~LFS_MKTAG(0x800, 0, 0)) |
            ((uint32_t)lfs_gstate_hasorphans(&lfs->gstate) << 31));

    return 0;
}
#endif
