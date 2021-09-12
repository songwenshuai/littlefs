/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

#ifndef LFS_READONLY
static int lfs_rawremoveattr(lfs_t *lfs, const char *path, uint8_t type) {
    return lfs_commitattr(lfs, path, type, NULL, 0x3ff);
}
#endif
