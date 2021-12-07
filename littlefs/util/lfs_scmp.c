/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"


// Find the sequence comparison of a and b, this is the distance
// between a and b ignoring overflow
// static inline 
int lfs_scmp(uint32_t a, uint32_t b) {
    return (int)(unsigned)(a - b);
}
