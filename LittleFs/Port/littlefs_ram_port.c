/*
 * lfs util functions
 *
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "littlefs_cfg.h"
#include "lfs_util.h"
#include "lfs.h"
#include "littlefs_ram_port.h"
#ifdef _MSC_VER
#include "dirent.h"
#endif

#include <errno.h>

#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */
#include <stdarg.h> /* needed for va_*         */

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

static int lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    // Check if read is valid
    LFS_ASSERT(off  % c->read_size == 0);
    LFS_ASSERT(size % c->read_size == 0);
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_TRACE("lfs_read(%p, 0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)c, block, off, buffer, size);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
    memcpy(buffer, addr + (block * c->block_size) + off, size);
    return 0;
}

static int lfs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    // Check if write is valid
    LFS_ASSERT(off  % c->prog_size == 0);
    LFS_ASSERT(size % c->prog_size == 0);
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_TRACE("lfs_prog(%p, 0x%"PRIx32", %"PRIu32", %p, %"PRIu32")",
            (void*)c, block, off, buffer, size);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
	memcpy(addr + (block * c->block_size) + off, buffer, size);
    return 0;
}

static int lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    // Check if erase is valid
    LFS_ASSERT(block < c->block_count);
    LFS_ASSERT(c != NULL);
    LFS_ASSERT(c->context != NULL);

    LFS_TRACE("lfs_erase(%p, 0x%"PRIx32")", (void*)c, block);

    uint8_t *addr = NULL;
    addr = (uint8_t *)c->context;
    memset(addr + (block * c->block_size), 0, c->block_size);
    return 0;
}

static int lfs_sync(const struct lfs_config *c) {
	return 0;
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

int mklfs(mklfs_cfg_t *mklfs_cfg) {
	static struct lfs_config cfg;
	static lfs_t lfs;
    int err;

    if ((mklfs_cfg->src == NULL) || (mklfs_cfg->dst == NULL) || (mklfs_cfg->block_size <= 0) || (mklfs_cfg->prog_size <= 0) ||
        (mklfs_cfg->read_size <= 0) || (mklfs_cfg->fs_size <= 0)) {
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

	return 0;
}

// entry point
void boot_count_test(void) {
	// configuration of the filesystem is provided by this struct
	static struct lfs_config cfg;
	// variables used by the filesystem
	static lfs_t lfs;
	lfs_file_t file;

    // block device operations
    cfg.read  = lfs_read;
    cfg.prog  = lfs_prog;
    cfg.erase = lfs_erase;
    cfg.sync  = lfs_sync;

    // block device configuration
    cfg.read_size = 256;
    cfg.prog_size = 256;
    cfg.block_size = 4096;
    cfg.block_count = 128;
    cfg.cache_size = 256;
    cfg.lookahead_size = 128;
    cfg.block_cycles = -1;

    cfg.context = calloc(1, cfg.block_size * cfg.block_count);
	if (!cfg.context) {
		fprintf(stderr, "no memory for mount\r\n");
	}
    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    // print the boot count
    printf("boot_count: %d\n", boot_count);
}