/*
 * lfs utility functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef LFS_PORT_H
#define LFS_PORT_H

#include "lfs.h"
#include "fal.h"

#ifdef __cplusplus
extern "C"
{
#endif

void _lfs_load_config(struct lfs_config* lfs_cfg, const struct fal_partition *part);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif

