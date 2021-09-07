/*
 * lfs utility functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef LFS_PORT_H
#define LFS_PORT_H

#include "littlefs_cfg.h"
#include "lfs.h"
#include "lfs_util.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct mklfs_cfg {
    char *src;        // Source directory <pack-dir>
    char *dst;        // Destination image <image-file-path>
    int block_size;   // Block size <block-size>
    int prog_size;    // Prog size <prog-size>
    int read_size;    // Read size <read-size>
    int fs_size;      // File system size <filesystem-size>
} mklfs_cfg_t;

typedef struct dumplfs_cfg {
    char *dstdir;   // Destination directory <output-dir>
    char *src;      // Source image <image-file-path>
    int block_size; // Block size <block-size>
    int read_size;  // Read size <read-size>
    int prog_size;  // Prog size <prog-size>
} dumplfs_cfg_t;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

