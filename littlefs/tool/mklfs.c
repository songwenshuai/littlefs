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

/*********************************************************************
*
*       define
*/

#define _PROG_SIZE_     256
#define _READ_SIZE_     256
#define _CACHE_SIZE     _PROG_SIZE_
#define _BLOCK_SIZE_    4096

#define _LOOKAHEAD_MAX  128
#define _FS_SIZE_       16777216    // 16 MB
#define _BLOCK_CYCLES   (-1)

typedef struct lfs_cfg {
    char *src;               // Source directory <pack-dir>
    char *dst;               // Destination image <image-file-path>
    int fs_size;             // File system size <filesystem-size>
    lfs_size_t block_size;   // Block size <block-size>
    lfs_size_t prog_size;    // Prog size <prog-size>
    lfs_size_t read_size;    // Read size <read-size>
    lfs_size_t lookahead_size;
    lfs_size_t cache_size;
    int32_t block_cycles;
} lfs_cfg_t;

int mklfs(lfs_cfg_t *mklfs_cfg);

/*********************************************************************
*
*       main_mklfs()
*/
int main_mklfs(int argc, char* argv[], char* envp[]) {
  static lfs_cfg_t mklfs_cfg;
  mklfs_cfg.src             = "./";
  mklfs_cfg.dst             = "./mklfs.img";
  mklfs_cfg.block_size      = _BLOCK_SIZE_;
  mklfs_cfg.prog_size       = _PROG_SIZE_;
  mklfs_cfg.read_size       = _READ_SIZE_;
  mklfs_cfg.cache_size      = _CACHE_SIZE;
  mklfs_cfg.block_cycles    = _BLOCK_CYCLES;
  mklfs_cfg.lookahead_size  = _LOOKAHEAD_MAX;
  mklfs_cfg.fs_size         = _FS_SIZE_;

  mklfs(&mklfs_cfg);

  return (0);
}

static void create_dir(lfs_t *lfs, char *src) {
    char *path;
    int ret;

    path = strchr(src, '/');
    if (path) {
        fprintf(stdout, "%s\r\n", path);

		if ((ret = lfs_mkdir(lfs, path)) < 0) {
			fprintf(stderr,"can't create directory %s: error=%d\r\n", path, ret);
			exit(1);
		}
	}
}


static void create_file(lfs_t *lfs, char *src) {
    char *path;
    int ret;

    path = strchr(src, '/');
    if (path) {
        fprintf(stdout, "%s\r\n", path);

        // Open source file
        FILE *srcf = fopen(src,"rb");
        if (!srcf) {
            fprintf(stderr,"can't open source file %s: errno=%d (%s)\r\n", src, errno, strerror(errno));
            exit(1);
        }

        // Open destination file
        lfs_file_t dstf;
        if ((ret = lfs_file_open(lfs, &dstf, path, LFS_O_WRONLY | LFS_O_CREAT)) < 0) {
            fprintf(stderr,"can't open destination file %s: error=%d\r\n", path, ret);
            exit(1);
        }

        if ((ret = lfs_file_rewind(lfs, &dstf)) <0) {
            fprintf(stderr,"can't rewind destination file %s: error=%d\r\n", path, ret);
            exit(1);
        }

		char c = fgetc(srcf);
		while (!feof(srcf)) {
			ret = lfs_file_write(lfs, &dstf, &c, 1);
			if (ret < 0) {
				fprintf(stderr,"can't write to destination file %s: error=%d\r\n", path, ret);
				exit(1);
			}
			c = fgetc(srcf);
		}

        // Close destination file
		ret = lfs_file_close(lfs, &dstf);
		if (ret < 0) {
			fprintf(stderr,"can't close destination file %s: error=%d\r\n", path, ret);
			exit(1);
		}

        // Close source file
        fclose(srcf);
    }
}


static void compact(lfs_t *lfs, char *src) {
    DIR *dir;
    struct dirent *ent;
    char curr_path[PATH_MAX];

    dir = opendir(src);
    if (dir) {
        while ((ent = readdir(dir))) {
            // Skip . and .. directories
            if ((strcmp(ent->d_name,".") != 0) && (strcmp(ent->d_name,"..") != 0)) {
                // Update the current path
                strcpy(curr_path, src);
                strcat(curr_path, "/");
                strcat(curr_path, ent->d_name);

                if (ent->d_type == DT_DIR) {
                    create_dir(lfs, curr_path);
                    compact(lfs, curr_path);
                } else if (ent->d_type == DT_REG) {
                    create_file(lfs, curr_path);
                }
            }
        }

        closedir(dir);
    }
}

int mklfs(lfs_cfg_t *mklfs_cfg) {
	static struct lfs_config cfg;
	static lfs_t lfs;
    int err;

    if ((mklfs_cfg->src == NULL)     || 
        (mklfs_cfg->dst == NULL)     || 
        (mklfs_cfg->block_size <= 0) || 
        (mklfs_cfg->prog_size  <= 0) ||
        (mklfs_cfg->read_size  <= 0) || 
        (mklfs_cfg->fs_size    <= 0)) {
        fprintf(stderr, "parameter cannot be null\r\n");
        exit(1);
    }

    // Mount the file system
    cfg.read  = lfs_read;
    cfg.prog  = lfs_prog;
    cfg.erase = lfs_erase;
    cfg.sync  = lfs_sync;

    cfg.block_size  = mklfs_cfg->block_size;
    cfg.read_size   = mklfs_cfg->read_size;
    cfg.prog_size   = mklfs_cfg->prog_size;
    cfg.cache_size  = mklfs_cfg->cache_size;
    cfg.block_cycles = mklfs_cfg->block_cycles;
    cfg.block_count = mklfs_cfg->fs_size / cfg.block_size;
    if (32 * ((cfg.block_count + 31) / 32) > mklfs_cfg->lookahead_size)
        cfg.lookahead_size = mklfs_cfg->lookahead_size;

	cfg.context = calloc(1, mklfs_cfg->fs_size);
	if (!cfg.context) {
		fprintf(stderr, "no memory for mount\r\n");
		return -1;
	}

	err = lfs_format(&lfs, &cfg);
	if (err < 0) {
		fprintf(stderr, "format error: error=%d\r\n", err);
		return -1;
	}

	err = lfs_mount(&lfs, &cfg);
	if (err < 0) {
		fprintf(stderr, "mount error: error=%d\r\n", err);
		return -1;
	}

	compact(&lfs, mklfs_cfg->src);

	FILE *img = fopen(mklfs_cfg->dst, "wb+");

	if (!img) {
		fprintf(stderr, "can't create image file: errno=%d (%s)\r\n", errno, strerror(errno));
		return -1;
	}

	fwrite(cfg.context, 1, mklfs_cfg->fs_size, img);

	fclose(img);

    // release any resources we were using
    err = lfs_unmount(&lfs);
	if (err < 0) {
		fprintf(stderr, "unmount error: error=%d\r\n", err);
		return -1;
	}

	return 0;
}
