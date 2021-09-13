/*
 * lfs util functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#if defined(__GNUC__)
#define _GNU_SOURCE
#endif
#if defined(_MSC_VER)
#pragma warning(disable:5105)
#endif
#include "lfs.h"
#include "app.h"

#include <errno.h>

#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */
#include <stdarg.h> /* needed for va_*         */

#include <ctype.h>
#include <string.h>
#include <limits.h>

#if defined(__GNUC__)
#include <unistd.h>
#include </usr/include/dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef _MSC_VER
#include <stdio.h>  /* needed for vsnprintf    */
#include "dirent.h"
#endif

int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int lfs_erase(const struct lfs_config *c, lfs_block_t block);
int lfs_sync(const struct lfs_config *c);

/*
 * vscprintf:
 * MSVC implements this as _vscprintf, thus we just 'symlink' it here
 * GNU-C-compatible compilers do not implement this, thus we implement it here
 */
#ifdef _MSC_VER
#define vscprintf _vscprintf
#endif

/*
 * asprintf, vasprintf:
 * MSVC does not implement these, thus we implement them here
 * GNU-C-compatible compilers implement these with the same names, thus we
 * don't have to do anything
 */
#ifdef _MSC_VER
int vasprintf(char **strp, const char *format, va_list ap)
{
    int len = vscprintf(format, ap);
    if (len == -1)
        return -1;
    char *str = (char*)malloc((size_t) len + 1);
    if (!str)
        return -1;
    int retval = vsnprintf(str, len + 1, format, ap);
    if (retval == -1) {
        free(str);
        return -1;
    }
    *strp = str;
    return retval;
}

int asprintf(char **strp, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int retval = vasprintf(strp, format, ap);
    va_end(ap);
    return retval;
}
#endif

static int dump_file(lfs_t *lfs, const char *srcpath, const char  *dstpath) {
	int r;
	lfs_file_t lfs_f;
	FILE *F = NULL;
	uint8_t buffer[4096];


	r = lfs_file_open(lfs, &lfs_f, srcpath, LFS_O_RDONLY);
	if (r < 0) {
		fprintf(stderr, "lfs: fail to open %s\n", srcpath);
		return -1;
	}

	F = fopen(dstpath, "wb");
	if (!F) {
		fprintf(stderr, "host: fail to open: %s. errno=%d (%s)\r\n",
				dstpath, errno, strerror(errno));
		r = -1;
		goto dump_file_err;
	}

	do {
		r = lfs_file_read(lfs, &lfs_f, buffer, sizeof(buffer));
		if (r < 0) {
			fprintf(stderr, "lfs: read failure: %s\n", srcpath);
			break;
		} else if (r == 0) {
			break;
		}
		if (fwrite(buffer, r, 1, F) != 1) {
			fprintf(stderr, "host: write failure: %s\n", dstpath);
			r = -1;
			break;
		}
	} while (1);

dump_file_err:
	if (F) fclose(F);
	lfs_file_close(lfs, &lfs_f);
	return r;
}


static int dump_dir(lfs_t *lfs, const char *srcpath, const char  *dstpath) {
	lfs_dir_t srcdir;
	struct lfs_info dir_info;
	int r;
	int status = 0;

	r = lfs_dir_open(lfs, &srcdir, srcpath);
	if (r < 0) {
		fprintf(stderr, "lfs: Failed to open dir %s\n", srcpath);
		return -1;
	}

	do {
		r = lfs_dir_read(lfs, &srcdir, &dir_info);
		if (r <= 0) break;
		if (!strcmp(dir_info.name, ".") || !strcmp(dir_info.name, "..")) {
			continue;
		}

		char *new_srcpath = NULL;
		char *new_dstpath = NULL;
		char t;

		switch (dir_info.type) {
		case LFS_TYPE_DIR:
			t = 'D';
		    break;
		case LFS_TYPE_REG:
			t = 'F';
			break;
		default:
			fprintf(stderr, "lfs: %s/%s: unsupported LFS_TYPE 0x%02x\n", srcpath, dir_info.name, dir_info.type);
			continue;
		}

		if (asprintf(&new_srcpath, "%s/%s", srcpath, dir_info.name) < 0) goto alloc_err;
		if (asprintf(&new_dstpath, "%s/%s", dstpath, dir_info.name) < 0) goto alloc_err;
		printf("%c: %s > %s\n",t, new_srcpath, new_dstpath);

		switch (dir_info.type) {
		case LFS_TYPE_DIR:
			mkdir(new_dstpath, 0777 );
			status = dump_dir(lfs, new_srcpath, new_dstpath);
		    break;
		case LFS_TYPE_REG:
			status = dump_file(lfs, new_srcpath, new_dstpath);
			break;
		}

alloc_err:
		free(new_srcpath);
		free(new_dstpath);

	} while (!status);


	lfs_dir_close(lfs, &srcdir);
	return status;
}

int dumplfs(dumplfs_cfg_t *dumplfs_cfg) {
	static struct lfs_config cfg;
	static lfs_t lfs;
    int c;              // Current option
    int fs_size = 0;    // File system size
    int err;

    if ((dumplfs_cfg->src == NULL) || (dumplfs_cfg->dstdir == NULL) || (dumplfs_cfg->block_size <= 0) || (dumplfs_cfg->prog_size <= 0) ||
        (dumplfs_cfg->read_size <= 0)) {
    	fprintf(stderr, "parameter cannot be null\r\n");
        exit(1);
    }

    // Mount the file system
    cfg.read  = lfs_read;
    cfg.prog  = lfs_prog;
    cfg.erase = lfs_erase;
    cfg.sync  = lfs_sync;

    /* read the data from dumplfs_cfg->src */
    FILE *img = fopen(dumplfs_cfg->src, "rb");
    if (!img) {
    	fprintf(stderr, "can't open image file: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}
    /* find the size of the filesystem */
    fseek(img, 0, SEEK_END);
    fs_size = ftell(img);
    if (fs_size < 0) {
    	fprintf(stderr, "can't read the image file file: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
    }
    fseek(img, 0, SEEK_SET);

    if (fs_size % dumplfs_cfg->block_size) {
    	fprintf(stderr, "image size is not aligned to dumplfs_cfg->block_size\r\n");
    	return -1;
    }

    cfg.block_size  = dumplfs_cfg->block_size;
    cfg.read_size   = dumplfs_cfg->read_size;
    cfg.prog_size   = dumplfs_cfg->prog_size;
    cfg.cache_size  = dumplfs_cfg->cache_size;
    cfg.block_cycles = dumplfs_cfg->block_cycles;
    cfg.block_count = fs_size / cfg.block_size;
    if (32 * ((cfg.block_count + 31) / 32) > dumplfs_cfg->lookahead_size)
        cfg.lookahead_size = dumplfs_cfg->lookahead_size;

	cfg.context = calloc(1, fs_size);
	if (!cfg.context) {
		fprintf(stderr, "no memory for mount\r\n");
		return -1;
	}

	if (fread(cfg.context, fs_size, 1, img) != 1) {
		fprintf(stderr, "Fail to read the image file\r\n");
		return -1;
	}
	fclose(img);

	err = lfs_mount(&lfs, &cfg);
	if (err < 0) {
		fprintf(stderr, "mount error: error=%d\r\n", err);
		return -1;
	}

	err = dump_dir(&lfs, "", dumplfs_cfg->dstdir);
	return err;
}

