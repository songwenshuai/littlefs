/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

// drop the lookahead buffer, this is done during mounting and failed
// traversals in order to avoid invalid lookahead state
// static 
void lfs_alloc_drop(lfs_t *lfs) {
    lfs->free.size = 0;
    lfs->free.i = 0;
    lfs_alloc_ack(lfs);
}

