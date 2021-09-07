#include "littlefs_cfg.h"
#include "lfs.h"
#include "littlefs_ram_port.h"

static mklfs_cfg_t mklfs_cfg;

/*********************************************************************
*
*       main()
*/
int main(int argc, char* argv[], char* envp[]) {
  mklfs_cfg.src             = _SRC_DIR;
  mklfs_cfg.dst             = _DST_DIR;
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
