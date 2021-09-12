/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

static int lfs_file_rawrewind(lfs_t *lfs, lfs_file_t *file) {
    lfs_soff_t res = lfs_file_rawseek(lfs, file, 0, LFS_SEEK_SET);
    if (res < 0) {
        return (int)res;
    }

    return 0;
}

