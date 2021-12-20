/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 1995 - 2021 SEGGER Microcontroller GmbH                  *
*                                                                    *
*       Internet: segger.com  Support: support_embos@segger.com      *
*                                                                    *
**********************************************************************
*                                                                    *
*       embOS * Real time operating system for microcontrollers      *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product or a real-time            *
*       operating system for in-house use.                           *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       OS version: V5.14.0.0                                        *
*                                                                    *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------
File    : OS_Main.c
Purpose : embOS sample program.
*/

#include "lfs.h"

#include "RTOS.h"
#include "SEGGER_SYS.h"
#include "SEGGER_MEM.h"
#include "SEGGER_SHELL.h"

#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */
#include <stdarg.h> /* needed for va_*         */

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

#define _PROG_SIZE_     256
#define _READ_SIZE_     256
#define _CACHE_SIZE     _PROG_SIZE_
#define _BLOCK_SIZE_    4096

#define _LOOKAHEAD_MAX  128
#define _FS_SIZE_       (16 * 1024 * 1024)    // 16 MB
#define _BLOCK_CYCLES   (-1)

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static OS_STACKPTR int          StackSHELL[128];  // Task stacks
static OS_TASK                  TCBSHELL;         // Task control blocks
static OS_MUTEX                 Mutex;
static SEGGER_MEM_CONTEXT       MemContext;
static SEGGER_SHELL_CONTEXT     ShellContext;
static SEGGER_SHELL_CONSOLE_API ShellConsole;
static struct lfs_config        LittleFsCfg;
static        lfs_t             LittleFsHandler;
static             char         LittleFsCwd[256];
static const       char        *prefixes[] = {"", "K", "M", "G"};
static             int          LittleFsMounted;

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       LittleFsHexDump
*
*  Function description
*    Print data as hex dump to debug output.
*
*  Parameters
*    pSelf    : Pointer to shell context.
*    Len      : Len of data.
*    pvData   : Pointer to data to be output.
*/
static void LittleFsHexDump(SEGGER_SHELL_CONTEXT *pSelf, U32 Len, const void *pvData) {
  char Buff[16*3+1];
  char *p;
  int  Cnt;
  unsigned Addr;
  const U8 *pData = (const U8 *)pvData;           //lint !e9079  D:100[e]
  static const char HexDigit[] = "0123456789ABCDEF";

  p = Buff;
  Cnt = 16;
  Addr = 0;
  while (Len > 0u) {
    *p++ = HexDigit[*pData >> 4];
    *p++ = HexDigit[*pData & 0xFu];
    pData++;
    Len--;
    *p++ = ' ';
    if (--Cnt == 0 || Len == 0u) {
      *p = '\0';
      SEGGER_SHELL_Printf(pSelf, "%03x0  %s", Addr, Buff);
      p = Buff;
      Cnt = 16;
      Addr++;
    }
  }
}

/*********************************************************************
*
*       LittleFsErr2Str()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static const char *LittleFsErr2Str(int error) {
    switch (error) {
    case LFS_ERR_IO:          return "Error during device operation";
    case LFS_ERR_CORRUPT:     return "Corrupted";
    case LFS_ERR_NOENT:       return "No directory entry";
    case LFS_ERR_EXIST:       return "Entry already exists";
    case LFS_ERR_NOTDIR:      return "Entry is not a dir";
    case LFS_ERR_ISDIR:       return "Entry is a dir";
    case LFS_ERR_NOTEMPTY:    return "Directory is not empty";
    case LFS_ERR_BADF:        return "Bad file number";
    case LFS_ERR_FBIG:        return "File too large";
    case LFS_ERR_INVAL:       return "Invalid argument";
    case LFS_ERR_NOSPC:       return "No space left on device";
    case LFS_ERR_NOMEM:       return "No more memory available";
    case LFS_ERR_NOATTR:      return "No data/attr available";
    case LFS_ERR_NAMETOOLONG: return "File name too long";
    }
    return "Unknown error";
}

/*********************************************************************
*
*       _SEGGER_SHELL_PrintHelpList()
*
*  Function description
*    Print help information in list format.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    Flags - Formatting flags.
*/
static int _PrintFileList(SEGGER_SHELL_CONTEXT *pSelf, const char *Path) {
  lfs_dir_t dir;
  struct lfs_info info;

  int err = lfs_dir_open(&LittleFsHandler, &dir, Path);
  if (err < 0) {
    SEGGER_SHELL_Printf(pSelf, "Error opening directory");
    return err;
  }

  while ((err = lfs_dir_read(&LittleFsHandler, &dir, &info)) != 0) {
    if (err < 0) {
      SEGGER_SHELL_Printf(pSelf, "Error reading directory");
      lfs_dir_close(&LittleFsHandler, &dir);
      return err;
    }

    if (err == 0) break;

    switch (info.type) {
    case LFS_TYPE_REG: SEGGER_SHELL_Printf(pSelf, "file "); break;
    case LFS_TYPE_DIR: SEGGER_SHELL_Printf(pSelf, " dir "); break;
    default:           SEGGER_SHELL_Printf(pSelf, "   ? "); break;
    }

    if (info.type == LFS_TYPE_REG) {
      for (int i = sizeof(prefixes) / sizeof(prefixes[0]) - 1; i >= 0; i--) {
        if (info.size >= (1 << 10 * i) - 1) {
          SEGGER_SHELL_Printf(pSelf, "%*lu%sB ", 4 - (i != 0), info.size >> 10 * i, prefixes[i]);
          break;
        }
      }
    }

    if (info.type == LFS_TYPE_DIR) {
      SEGGER_SHELL_Printf(pSelf, "      ");
    }
  
    SEGGER_SHELL_Printf(pSelf, "%s\n", info.name);
  }

  err = lfs_dir_close(&LittleFsHandler, &dir);
  if (err < 0) {
    SEGGER_SHELL_Printf(pSelf, "Error close directory");
    return err;
  }

  return 0;
}

/*********************************************************************
*
*       LittleFsLock()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsLock(const struct lfs_config *c) {

  OS_MUTEX_Lock(&Mutex);
  return LFS_ERR_OK;
}

/*********************************************************************
*
*       LittleFsUnlock()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsUnlock(const struct lfs_config *c) {

  OS_MUTEX_Unlock(&Mutex);
  return LFS_ERR_OK;
}

/*********************************************************************
*
*       LittleFsCreateImg()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsCreateImg(SEGGER_SHELL_CONTEXT *pSelf) {
  char       * sArg;

  if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
    if (strcmp(sArg, "-d") == 0) {
      if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
        FILE *img = fopen(sArg, "wb+");
        if (!img) {
          SEGGER_SHELL_Printf(pSelf, "can't create image file: errno=%d (%s)\r\n", errno, strerror(errno));
          return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
        }
        fwrite(LittleFsCfg.context, 1, _FS_SIZE_, img);
        fclose(img);
      }
    }
  }

  return 0;
}

/*********************************************************************
*
*       LittleFsInit()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsInit(SEGGER_SHELL_CONTEXT *pSelf) {
  OS_MUTEX_Create(&Mutex);  // Creates mutex

  LittleFsCfg.read         = lfs_read;
  LittleFsCfg.prog         = lfs_prog;
  LittleFsCfg.erase        = lfs_erase;
  LittleFsCfg.sync         = lfs_sync;
  LittleFsCfg.lock         = LittleFsLock,
  LittleFsCfg.unlock       = LittleFsUnlock,
  LittleFsCfg.block_size   = _BLOCK_SIZE_;
  LittleFsCfg.read_size    = _READ_SIZE_;
  LittleFsCfg.prog_size    = _PROG_SIZE_;
  LittleFsCfg.cache_size   = _CACHE_SIZE;
  LittleFsCfg.block_cycles = _BLOCK_CYCLES;
  LittleFsCfg.block_count  = _FS_SIZE_ / LittleFsCfg.block_size;
  if (32 * ((LittleFsCfg.block_count + 31) / 32) > _LOOKAHEAD_MAX)
      LittleFsCfg.lookahead_size = _LOOKAHEAD_MAX;

  LittleFsCfg.context = SEGGER_MEM_ZeroAlloc(&MemContext, _FS_SIZE_);
  if (!LittleFsCfg.context) {
      SEGGER_SHELL_Printf(pSelf, "no memory for mount\r\n");
      return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }

  strncpy(LittleFsCwd, "/", sizeof(LittleFsCwd));
  return 0;
}

/*********************************************************************
*
*       LittleFsDeinit()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsDeinit(SEGGER_SHELL_CONTEXT *pSelf) {
  OS_MUTEX_Delete(&Mutex);
  SEGGER_MEM_Free(&MemContext, LittleFsCfg.context);
  return 0;
}

/*********************************************************************
*
*       LittleFsFormat()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsFormat(SEGGER_SHELL_CONTEXT *pSelf) {
  int err;

  if (LittleFsMounted) {
    SEGGER_SHELL_Printf(pSelf, "LFS is mounted, please unmount it first.");
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }
  if (SEGGER_SHELL_CountTotalArgs(pSelf) != 1) {
    SEGGER_SHELL_Printf(pSelf, "Are you sure? Please issue command \"format\" to proceed.\r\n");
  }
  
  err = lfs_format(&LittleFsHandler, &LittleFsCfg);
  if (err < 0) {
      SEGGER_SHELL_Printf(pSelf, "format error: error=%d", err);
      return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }

  return 0;
}

/*********************************************************************
*
*       LittleFsMount()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsMount(SEGGER_SHELL_CONTEXT *pSelf) {
  int err;

  if (LittleFsMounted) {
    SEGGER_SHELL_Printf(pSelf, "LFS already mounted\r\n");
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }

  err = lfs_mount(&LittleFsHandler, &LittleFsCfg);
  if (err < 0) {
    SEGGER_SHELL_Printf(pSelf, "mount error: error=%d\r\n", err);
    lfs_unmount(&LittleFsHandler);
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }

  LittleFsMounted = 1;
  return 0;
}

/*********************************************************************
*
*       LittleFsUnmount()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsUnmount(SEGGER_SHELL_CONTEXT *pSelf) {
  int err;

  if (!LittleFsMounted) {
    SEGGER_SHELL_Printf(pSelf, "LFS not mounted\r\n");
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }

  err = lfs_unmount(&LittleFsHandler);
  if (err < 0) {
    SEGGER_SHELL_Printf(pSelf, "unmount error: error=%d\r\n", err);
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }

  LittleFsMounted = 0;
  return 0;
}

/*********************************************************************
*
*       LittleFsMakeDir()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsMakeDir(SEGGER_SHELL_CONTEXT *pSelf) {
  char       * sArg;
  int          err;

  if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
    if (strcmp(sArg, "-d") == 0) {
      if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
        err = lfs_mkdir(&LittleFsHandler, sArg);
        if (err < 0) {
          SEGGER_SHELL_Printf(pSelf, "can't create directory %s: error=%d\r\n", sArg, err);
          return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
        }
      }
    }
  }

  return 0;
}

/*********************************************************************
*
*       LittleFsList()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsList(SEGGER_SHELL_CONTEXT *pSelf) {
  int          err;

  if (!LittleFsMounted) {
    SEGGER_SHELL_Printf(pSelf, "LFS not mounted\r\n");
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }

  err = _PrintFileList(pSelf, LittleFsCwd);
  if (err < 0) {
    SEGGER_SHELL_Printf(pSelf, "file list error=%d\r\n", err);
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  }
  return 0;
}

/*********************************************************************
*
*       LittleFsTouch()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsTouch(SEGGER_SHELL_CONTEXT *pSelf) {
  int                    err;
  char                 * sArg;
  lfs_file_t             file;
  struct lfs_file_config defaults = {0};
  char buf[_CACHE_SIZE] = {0};

  defaults.buffer = buf;

  if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
    if (strcmp(sArg, "-d") == 0) {
      if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
        err = lfs_file_opencfg(&LittleFsHandler, &file, sArg, LFS_O_RDWR | LFS_O_CREAT, &defaults);
        if(err < 0) {
          SEGGER_SHELL_Printf(pSelf, "Error while touching file: %d\n", err);
          return err;
        }
        lfs_file_close(&LittleFsHandler, &file);
      }
    }
  }

  return 0;
}

/*********************************************************************
*
*       LittleFsChangeDir()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsChangeDir(SEGGER_SHELL_CONTEXT *pSelf) {
  char       * sArg;

  if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
    if (strcmp(sArg, "-d") == 0) {
      if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
        /* TODO - check if a directory actually exists */
        if (sArg == NULL || strcmp(sArg, "/") == 0) {
          strncpy(LittleFsCwd, "/", sizeof(LittleFsCwd));
        } else if (strcmp(sArg, "..") == 0) {
          /* return one lvl back */
        } else if (strcmp(sArg, ".") != 0) {
          if (sArg[0] == '/') {
            /* absolute path */
            strncpy(LittleFsCwd, sArg, sizeof(LittleFsCwd));
          } else {
            /* relative path */
            strncat(LittleFsCwd, sArg, sizeof(LittleFsCwd) - strlen(LittleFsCwd) - 1);
          }
        }
      }
    }
  }

  return 0;
}

/*********************************************************************
*
*       LittleFsRemove()
*
*  Function description
*    little fs parameter initialization.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int LittleFsRemove(SEGGER_SHELL_CONTEXT *pSelf) {
  int          err;
  char       * sArg;

  if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
    if (strcmp(sArg, "-d") == 0) {
      if (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
        if (strlen(sArg) > LFS_NAME_MAX) {
          SEGGER_SHELL_Printf(pSelf, "File cmd_name too long!");
          return -1;
        }
        /* first +1 for the path separator (/), second +1 for the null terminator */
        char *full_path = malloc(strlen(LittleFsCwd) + 1 + strlen(sArg) + 1);
        strcpy(full_path, LittleFsCwd);
        strcat(full_path, "/");
        strcat(full_path, sArg);
        struct lfs_info info;
        err = lfs_stat(&LittleFsHandler, full_path, &info);
        if (err < 0) {
          SEGGER_SHELL_Printf(pSelf, "lfs_stat failed with %ld", err);
          free(full_path);
          return -1;
        }
        if (info.type != LFS_TYPE_REG) {
          SEGGER_SHELL_Printf(pSelf, "This is not a file!");
          free(full_path);
          return -1;
        }
        err = lfs_remove(&LittleFsHandler, full_path);
        if (err < 0) {
          SEGGER_SHELL_Printf(pSelf, "File removal failed with %ld", err);
        }
        SEGGER_SHELL_Printf(pSelf, "File %s removed!", sArg);
        free(full_path);
      }
    }
  }

  return 0;
}

const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_CreateImg    = { "img",     "Create Img."         , "[-d]", "-d\tThe location of the image file\n", LittleFsCreateImg };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_Init         = { "init",    "init."               , 0, 0, LittleFsInit      };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_Deinit       = { "deinit",  "deinit."             , 0, 0, LittleFsDeinit    };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_Format       = { "format",  "Format."             , 0, 0, LittleFsFormat    };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_Mount        = { "mount",   "Mount."              , 0, 0, LittleFsMount     };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_Unmount      = { "unmount", "Unmount."            , 0, 0, LittleFsUnmount   };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_MakeDir      = { "mkdir",   "make dir."           , "[-d]", "-d\tcreate a dir\n"                  , LittleFsMakeDir   };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_List         = { "ls",      "file list."          , 0, 0, LittleFsList      };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_Touch        = { "touch",   "file list."          , "[-d]", "-d\tcreate a file\n"                 , LittleFsTouch     };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_ChangeDir    = { "cd",      "change dir."         , "[-d]", "-d\tchange dir\n"                    , LittleFsChangeDir };
const SEGGER_SHELL_COMMAND_API SEGGER_LITTLEFS_COMMAND_Remove       = { "rm",      "file or dir remove." , "[-d]", "-d\tfile or dir remove\n"            , LittleFsRemove    };

/*********************************************************************
*
*       SEGGER_LITTLEFS_AddCommands()
*
*  Function description
*    Add core littlefs commands.
*
*  Parameters
*    pSelf - Pointer to shell context.
*/
void SEGGER_LITTLEFS_AddCommands(SEGGER_SHELL_CONTEXT *pSelf) {
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_CreateImg);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_Format);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_Mount);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_List);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_Remove);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_ChangeDir);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_MakeDir);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_LITTLEFS_COMMAND_Unmount);
}

/*********************************************************************
*
*       SHELLTask()
*/
static void SHELLTask(void) {
  SEGGER_SYS_Init();
  SEGGER_MEM_SYSTEM_HEAP_Init(&MemContext);
  ShellConsole.pfGetString    = SEGGER_SYS_IO_Gets;
  ShellConsole.pfPrintString  = SEGGER_SYS_IO_Printvf;
  SEGGER_SHELL_Init(&ShellContext, &ShellConsole, &MemContext);
  SEGGER_SHELL_AddCommands(&ShellContext);
  SEGGER_LITTLEFS_AddCommands(&ShellContext);
  while (1) {
    SEGGER_SHELL_Enter(&ShellContext);
    OS_TASK_Delay(50);
  }
}

/*********************************************************************
*
*       main()
*/
int main(void) {
  OS_Init();                // Initialize embOS
  OS_InitHW();              // Initialize required hardware
  OS_TASK_CREATE(&TCBSHELL, "SHELL Task",  50, SHELLTask, StackSHELL);
  OS_Start();               // Start embOS
  return 0;
}

/*************************** End of file ****************************/
