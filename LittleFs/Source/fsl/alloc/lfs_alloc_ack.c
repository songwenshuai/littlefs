/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

// indicate allocated blocks have been committed into the filesystem, this
// is to prevent blocks from being garbage collected in the middle of a
// commit operation
static void lfs_alloc_ack(lfs_t *lfs) {
    lfs->free.ack = lfs->cfg->block_count;
}

