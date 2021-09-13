/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

// static 
int lfs_dir_getgstate(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_gstate_t *gstate) {
    lfs_gstate_t temp;
    lfs_stag_t res = lfs_dir_get(lfs, dir, LFS_MKTAG(0x7ff, 0, 0),
            LFS_MKTAG(LFS_TYPE_MOVESTATE, 0, sizeof(temp)), &temp);
    if (res < 0 && res != LFS_ERR_NOENT) {
        return res;
    }

    if (res != LFS_ERR_NOENT) {
        // xor together to find resulting gstate
        lfs_gstate_fromle32(&temp);
        lfs_gstate_xor(gstate, &temp);
    }

    return 0;
}

