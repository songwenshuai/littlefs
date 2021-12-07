/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

/// Internal operations predeclared here ///
void lfs_pair_fromle32(lfs_block_t pair[2]);
uint16_t lfs_tag_id(lfs_tag_t tag);
uint16_t lfs_tag_type3(lfs_tag_t tag);
lfs_stag_t lfs_dir_fetchmatch(lfs_t *lfs,
        lfs_mdir_t *dir, const lfs_block_t pair[2],
        lfs_tag_t fmask, lfs_tag_t ftag, uint16_t *id,
        int (*cb)(void *data, lfs_tag_t tag, const void *buffer), void *data);
int lfs_dir_find_match(void *data,
        lfs_tag_t tag, const void *buffer);
lfs_stag_t lfs_dir_get(lfs_t *lfs, const lfs_mdir_t *dir,
        lfs_tag_t gmask, lfs_tag_t gtag, void *buffer);

// static 
lfs_stag_t lfs_dir_find(lfs_t *lfs, lfs_mdir_t *dir,
        const char **path, uint16_t *id) {
    // we reduce path to a single name if we can find it
    const char *name = *path;
    if (id) {
        *id = 0x3ff;
    }

    // default to root dir
    lfs_stag_t tag = LFS_MKTAG(LFS_TYPE_DIR, 0x3ff, 0);
    dir->tail[0] = lfs->root[0];
    dir->tail[1] = lfs->root[1];

    while (true) {
nextname:
        // skip slashes
        name += strspn(name, "/");
        lfs_size_t namelen = strcspn(name, "/");

        // skip '.' and root '..'
        if ((namelen == 1 && memcmp(name, ".", 1) == 0) ||
            (namelen == 2 && memcmp(name, "..", 2) == 0)) {
            name += namelen;
            goto nextname;
        }

        // skip if matched by '..' in name
        const char *suffix = name + namelen;
        lfs_size_t sufflen;
        int depth = 1;
        while (true) {
            suffix += strspn(suffix, "/");
            sufflen = strcspn(suffix, "/");
            if (sufflen == 0) {
                break;
            }

            if (sufflen == 2 && memcmp(suffix, "..", 2) == 0) {
                depth -= 1;
                if (depth == 0) {
                    name = suffix + sufflen;
                    goto nextname;
                }
            } else {
                depth += 1;
            }

            suffix += sufflen;
        }

        // found path
        if (name[0] == '\0') {
            return tag;
        }

        // update what we've found so far
        *path = name;

        // only continue if we hit a directory
        if (lfs_tag_type3(tag) != LFS_TYPE_DIR) {
            return LFS_ERR_NOTDIR;
        }

        // grab the entry data
        if (lfs_tag_id(tag) != 0x3ff) {
            lfs_stag_t res = lfs_dir_get(lfs, dir, LFS_MKTAG(0x700, 0x3ff, 0),
                    LFS_MKTAG(LFS_TYPE_STRUCT, lfs_tag_id(tag), 8), dir->tail);
            if (res < 0) {
                return res;
            }
            lfs_pair_fromle32(dir->tail);
        }

        // find entry matching name
        while (true) {
            tag = lfs_dir_fetchmatch(lfs, dir, dir->tail,
                    LFS_MKTAG(0x780, 0, 0),
                    LFS_MKTAG(LFS_TYPE_NAME, 0, namelen),
                     // are we last name?
                    (strchr(name, '/') == NULL) ? id : NULL,
                    lfs_dir_find_match, &(struct lfs_dir_find_match){
                        lfs, name, namelen});
            if (tag < 0) {
                return tag;
            }

            if (tag) {
                break;
            }

            if (!dir->split) {
                return LFS_ERR_NOENT;
            }
        }

        // to next name
        name += namelen;
    }
}
