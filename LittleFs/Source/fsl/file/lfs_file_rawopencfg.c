/*
 * The little filesystem
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "lfs.h"

extern void *lfs_malloc(size_t size);
void lfs_cache_zero(lfs_t *lfs, lfs_cache_t *pcache);
void lfs_ctz_fromle32(struct lfs_ctz *ctz);

/// Top level file operations ///
// static 
int lfs_file_rawopencfg(lfs_t *lfs, lfs_file_t *file,
        const char *path, int flags,
        const struct lfs_file_config *cfg) {
#ifndef LFS_READONLY
    // deorphan if we haven't yet, needed at most once after poweron
    if ((flags & LFS_O_WRONLY) == LFS_O_WRONLY) {
        int err = lfs_fs_forceconsistency(lfs);
        if (err) {
            return err;
        }
    }
#else
    LFS_ASSERT((flags & LFS_O_RDONLY) == LFS_O_RDONLY);
#endif

    // setup simple file details
    int err;
    file->cfg = cfg;
    file->flags = flags;
    file->pos = 0;
    file->off = 0;
    file->cache.buffer = NULL;

    // allocate entry for file if it doesn't exist
    lfs_stag_t tag = lfs_dir_find(lfs, &file->m, &path, &file->id);
    if (tag < 0 && !(tag == LFS_ERR_NOENT && file->id != 0x3ff)) {
        err = tag;
        goto cleanup;
    }

    // get id, add to list of mdirs to catch update changes
    file->type = LFS_TYPE_REG;
    lfs_mlist_append(lfs, (struct lfs_mlist *)file);

#ifdef LFS_READONLY
    if (tag == LFS_ERR_NOENT) {
        err = LFS_ERR_NOENT;
        goto cleanup;
#else
    if (tag == LFS_ERR_NOENT) {
        if (!(flags & LFS_O_CREAT)) {
            err = LFS_ERR_NOENT;
            goto cleanup;
        }

        // check that name fits
        lfs_size_t nlen = strlen(path);
        if (nlen > lfs->name_max) {
            err = LFS_ERR_NAMETOOLONG;
            goto cleanup;
        }

        // get next slot and create entry to remember name
        err = lfs_dir_commit(lfs, &file->m, LFS_MKATTRS(
                {LFS_MKTAG(LFS_TYPE_CREATE, file->id, 0), NULL},
                {LFS_MKTAG(LFS_TYPE_REG, file->id, nlen), path},
                {LFS_MKTAG(LFS_TYPE_INLINESTRUCT, file->id, 0), NULL}));
        if (err) {
            err = LFS_ERR_NAMETOOLONG;
            goto cleanup;
        }

        tag = LFS_MKTAG(LFS_TYPE_INLINESTRUCT, 0, 0);
    } else if (flags & LFS_O_EXCL) {
        err = LFS_ERR_EXIST;
        goto cleanup;
#endif
    } else if (lfs_tag_type3(tag) != LFS_TYPE_REG) {
        err = LFS_ERR_ISDIR;
        goto cleanup;
#ifndef LFS_READONLY
    } else if (flags & LFS_O_TRUNC) {
        // truncate if requested
        tag = LFS_MKTAG(LFS_TYPE_INLINESTRUCT, file->id, 0);
        file->flags |= LFS_F_DIRTY;
#endif
    } else {
        // try to load what's on disk, if it's inlined we'll fix it later
        tag = lfs_dir_get(lfs, &file->m, LFS_MKTAG(0x700, 0x3ff, 0),
                LFS_MKTAG(LFS_TYPE_STRUCT, file->id, 8), &file->ctz);
        if (tag < 0) {
            err = tag;
            goto cleanup;
        }
        lfs_ctz_fromle32(&file->ctz);
    }

    // fetch attrs
    for (unsigned i = 0; i < file->cfg->attr_count; i++) {
        // if opened for read / read-write operations
        if ((file->flags & LFS_O_RDONLY) == LFS_O_RDONLY) {
            lfs_stag_t res = lfs_dir_get(lfs, &file->m,
                    LFS_MKTAG(0x7ff, 0x3ff, 0),
                    LFS_MKTAG(LFS_TYPE_USERATTR + file->cfg->attrs[i].type,
                        file->id, file->cfg->attrs[i].size),
                        file->cfg->attrs[i].buffer);
            if (res < 0 && res != LFS_ERR_NOENT) {
                err = res;
                goto cleanup;
            }
        }

#ifndef LFS_READONLY
        // if opened for write / read-write operations
        if ((file->flags & LFS_O_WRONLY) == LFS_O_WRONLY) {
            if (file->cfg->attrs[i].size > lfs->attr_max) {
                err = LFS_ERR_NOSPC;
                goto cleanup;
            }

            file->flags |= LFS_F_DIRTY;
        }
#endif
    }

    // allocate buffer if needed
    if (file->cfg->buffer) {
        file->cache.buffer = file->cfg->buffer;
    } else {
        file->cache.buffer = lfs_malloc(lfs->cfg->cache_size);
        if (!file->cache.buffer) {
            err = LFS_ERR_NOMEM;
            goto cleanup;
        }
    }

    // zero to avoid information leak
    lfs_cache_zero(lfs, &file->cache);

    if (lfs_tag_type3(tag) == LFS_TYPE_INLINESTRUCT) {
        // load inline files
        file->ctz.head = LFS_BLOCK_INLINE;
        file->ctz.size = lfs_tag_size(tag);
        file->flags |= LFS_F_INLINE;
        file->cache.block = file->ctz.head;
        file->cache.off = 0;
        file->cache.size = lfs->cfg->cache_size;

        // don't always read (may be new/trunc file)
        if (file->ctz.size > 0) {
            lfs_stag_t res = lfs_dir_get(lfs, &file->m,
                    LFS_MKTAG(0x700, 0x3ff, 0),
                    LFS_MKTAG(LFS_TYPE_STRUCT, file->id,
                        lfs_min(file->cache.size, 0x3fe)),
                    file->cache.buffer);
            if (res < 0) {
                err = res;
                goto cleanup;
            }
        }
    }

    return 0;

cleanup:
    // clean up lingering resources
#ifndef LFS_READONLY
    file->flags |= LFS_F_ERRED;
#endif
    lfs_file_rawclose(lfs, file);
    return err;
}

