/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

// static 
void lfs_mlist_append(lfs_t *lfs, struct lfs_mlist *mlist) {
    mlist->next = lfs->mlist;
    lfs->mlist = mlist;
}

