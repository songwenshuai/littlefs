/*
 * lfs utility functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef LFS_CFG_H
#define LFS_CFG_H

#define _LOOKAHEAD_MAX  128
#define _BLOCK_SIZE_    4096
#define _PROG_SIZE_     256
#define _READ_SIZE_     256
#define _FS_SIZE_       16777216    // 16 MB
#define _CACHE_SIZE     _PROG_SIZE_
#define _BLOCK_CYCLES   (-1)
#define _SRC_DIR        "./"
#define _DST_DIR        "./mklfs.img"

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

