#include "littlefs_cfg.h"
#include "lfs.h"
#include "littlefs_ram_port.h"

extern int main_(int argc, char **argv);

/*********************************************************************
*
*       main()
*/
int main(int argc, char* argv[], char* envp[]) {
  main_(argc, argv);
  return (0);
}
