/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

// static 
void lfs_mlist_remove(lfs_t *lfs, struct lfs_mlist *mlist) {
    for (struct lfs_mlist **p = &lfs->mlist; *p; p = &(*p)->next) {
        if (*p == mlist) {
            *p = (*p)->next;
            break;
        }
    }
}

