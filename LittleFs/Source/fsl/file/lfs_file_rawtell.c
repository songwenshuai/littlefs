/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

static lfs_soff_t lfs_file_rawtell(lfs_t *lfs, lfs_file_t *file) {
    (void)lfs;
    return file->pos;
}

