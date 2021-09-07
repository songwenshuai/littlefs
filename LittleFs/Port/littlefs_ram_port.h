/*
 * lfs utility functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef LFS_PORT_H
#define LFS_PORT_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct mklfs_cfg {
    char *src;        // Source directory <pack-dir>
    char *dst;        // Destination image <image-file-path>
    int fs_size;      // File system size <filesystem-size>
    lfs_size_t block_size;   // Block size <block-size>
    lfs_size_t prog_size;    // Prog size <prog-size>
    lfs_size_t read_size;    // Read size <read-size>
    lfs_size_t lookahead_size;
    lfs_size_t cache_size;
    int32_t block_cycles;
} mklfs_cfg_t;

typedef struct dumplfs_cfg {
    char *dstdir;   // Destination directory <output-dir>
    char *src;      // Source image <image-file-path>
    lfs_size_t block_size; // Block size <block-size>
    lfs_size_t read_size;  // Read size <read-size>
    lfs_size_t prog_size;  // Prog size <prog-size>
    lfs_size_t lookahead_size;
    lfs_size_t cache_size;
    int32_t block_cycles;
} dumplfs_cfg_t;

int mklfs(mklfs_cfg_t *mklfs_cfg);
int dumplfs(dumplfs_cfg_t *dumplfs_cfg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

