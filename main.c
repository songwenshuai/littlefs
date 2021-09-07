#include "littlefs_cfg.h"
#include "lfs.h"
#include "littlefs_ram_port.h"

#define _BLOCK_SIZE_    4096
#define _PROG_SIZE_     256
#define _READ_SIZE_     256
#define _FS_SIZE_       16777216    // 16 MB
#define _CACHE_SIZE     _PROG_SIZE_
#define _BLOCK_CYCLES   (-1)

static mklfs_cfg_t mklfs_cfg;
static dumplfs_cfg_t dumplfs_cfg;

/*********************************************************************
*
*       main()
*/
int main(int argc, char* argv[], char* envp[]) {
  mklfs_cfg.src             = "./";
  mklfs_cfg.dst             = "./mklfs.img";
  mklfs_cfg.block_size      = _BLOCK_SIZE_;
  mklfs_cfg.prog_size       = _PROG_SIZE_;
  mklfs_cfg.read_size       = _READ_SIZE_;
  mklfs_cfg.cache_size      = _CACHE_SIZE;
  mklfs_cfg.block_cycles    = _BLOCK_CYCLES;
  mklfs_cfg.fs_size         = _FS_SIZE_;

  mklfs(&mklfs_cfg);
  return (0);
}
