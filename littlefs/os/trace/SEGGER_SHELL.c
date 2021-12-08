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
File        : SEGGER_SHELL.c
Purpose     : Simple shell API.
Revision    : $Rev: 16195 $
*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_SHELL.h"
#include "SEGGER.h"
#include <string.h>
#include <stdlib.h>

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

static int _ExecuteBye      (SEGGER_SHELL_CONTEXT *pSelf);
static int _ExecuteCommands (SEGGER_SHELL_CONTEXT *pSelf);

/*********************************************************************
*
*       Lint configuration
*
**********************************************************************
*/

//lint -efunc(818,SEGGER_SHELL_InputText) suppress "could be declared as pointing to const"
//lint -efunc(818,SEGGER_SHELL_Printf)    suppress "could be declared as pointing to const"

/*********************************************************************
*
*       Public const data
*
**********************************************************************
*/

const SEGGER_SHELL_COMMAND_API SEGGER_SHELL_COMMAND_Bye          = { "bye",      "Exit application.", 0, 0, _ExecuteBye         };
const SEGGER_SHELL_COMMAND_API SEGGER_SHELL_COMMAND_Exit         = { "exit",     "Exit application.", 0, 0, _ExecuteBye         };
const SEGGER_SHELL_COMMAND_API SEGGER_SHELL_COMMAND_Quit         = { "quit",     "Exit application.", 0, 0, _ExecuteBye         };
const SEGGER_SHELL_COMMAND_API SEGGER_SHELL_COMMAND_QuestionMark = { "?",        "Show help.", "[-xv]", "-x\tShow syntax\n-v\tShow syntax and options", SEGGER_SHELL_ExecuteHelp };
const SEGGER_SHELL_COMMAND_API SEGGER_SHELL_COMMAND_Help         = { "help",     "Show help.", "[-xv]", "-x\tShow syntax\n-v\tShow syntax and options", SEGGER_SHELL_ExecuteHelp };
const SEGGER_SHELL_COMMAND_API SEGGER_SHELL_COMMAND_Commands     = { "commands", "Show commands.",    0, 0, _ExecuteCommands    };

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _SEGGER_SHELL_PATH_Cat()
*
*  Function description
*    Catenate to end of path.
*
*  Parameters
*    sPath   - Pointer to path name.
*    PathLen - Capacity of path name.
*    sAdd    - Text to be added.
*    AddLen  - Octet length of the text to be added.
*
*  Return value
*    == 0 - Success.
*    <  0 - Error, path overflow.
*/
static int _SEGGER_SHELL_PATH_Cat(char *sPath, unsigned PathLen, const char *sAdd, unsigned AddLen) {
  unsigned L;
  int      Status;
  //
  Status = 0;
  if (SEGGER_strlen(sPath) + AddLen + 1 >= PathLen) {
    Status = SEGGER_SHELL_ERROR_STRING_TOO_LONG;
  }
  L = SEGGER_strlen(sPath);
  while (L+1 < PathLen && AddLen > 0) {
    sPath[L++] = *sAdd++;
    --AddLen;
  }
  sPath[L] = '\0';
  //
  return Status;
}

/*********************************************************************
*
*       _SEGGER_SHELL_PATH_AddSlash()
*
*  Function description
*    Append '/' if not already at end.
*
*  Parameters
*    sPath   - Pointer to path name where extension will be added.
*    PathLen - Capacity of path name.
*
*  Return value
*    == 0 - Success.
*    <  0 - Error, path overflow.
*/
static int _SEGGER_SHELL_PATH_AddSlash(char *sPath, unsigned PathLen) {
  unsigned L;
  int      Status;
  //
  Status = 0;
  L = SEGGER_strlen(sPath);
  if (L == 0 || sPath[L-1] != '/') {
    Status = _SEGGER_SHELL_PATH_Cat(sPath, PathLen, "/", 1);
  }
  return Status;
}

/*********************************************************************
*
*       _SEGGER_SHELL_PATH_Root()
*
*  Function description
*    Truncate to root folder.
*
*  Parameters
*    sPath   - Pointer to path name.
*    PathLen - Capacity of path name.
*
*  Return value
*    == 0 - Success.
*    <  0 - Error, path overflow.
*/
static int _SEGGER_SHELL_PATH_Root(char *sPath, unsigned PathLen) {
  unsigned L;
  //
  L = SEGGER_strlen(sPath);
  while (L > 0 && sPath[L-1] != ':') {
    --L;
  }
  sPath[L] = '\0';
  return _SEGGER_SHELL_PATH_AddSlash(sPath, PathLen);
}

/*********************************************************************
*
*       _SEGGER_SHELL_PATH_Normalize()
*
*  Function description
*    Normalize path name.  If the path is a root, place a slash
*    after the volume name.
*
*  Parameters
*    sPath   - Pointer to path name.
*    PathLen - Capacity of path name.
*
*  Return value
*    == 0 - Success.
*    <  0 - Error, path overflow.
*/
static int _SEGGER_SHELL_PATH_Normalize(char *sPath, unsigned PathLen) {
  unsigned L;
  int      Status;
  //
  Status = 0;
  L = SEGGER_strlen(sPath);
  if (L == 0 || sPath[L-1] == ':') {
    Status = _SEGGER_SHELL_PATH_Root(sPath, PathLen);
  }
  return Status;
}

/*********************************************************************
*
*       _SEGGER_SHELL_PATH_Up()
*
*  Function description
*    Go up one folder.
*
*  Parameters
*    sPath   - Pointer to path name.
*    PathLen - Capacity of path name.
*/
static void _SEGGER_SHELL_PATH_Up(char *sPath, unsigned PathLen) {
  unsigned L;
  //
  for (L = SEGGER_strlen(sPath); L+1 > 0; --L) {
    if (sPath[L] == ':') {
      sPath[L+1] = 0;
      break;
    } else if (sPath[L] == '/') {
      sPath[L] = 0;
      break;
    }
  }
  _SEGGER_SHELL_PATH_Normalize(sPath, PathLen);
}

/*********************************************************************
*
*       _SEGGER_SHELL_PATH_AddPart()
*
*  Function description
*    Traverse relative one directory.
*
*  Parameters
*    sPath   - Pointer to path name where extension will be added.
*    PathLen - Capacity of path name.
*    sNew    - Pointer to string specifying new directory.
*    NewLen  - Octet length of the string specifying new directory.
*
*  Return value
*    == 0 - Success.
*    <  0 - Error, path overflow.
*/
static int _SEGGER_SHELL_PATH_AddPart(char *sPath, unsigned PathLen, const char *sNew, unsigned NewLen) {
  int Status;
  //
  Status = 0;
  if (NewLen == 2 && sNew[0] == '.' && sNew[1] == '.') {
    _SEGGER_SHELL_PATH_Up(sPath, PathLen);
  } else if (NewLen == 1 && sNew[0] == '.') {
    /* Relative to current directory, don't add '.' */
  } else {
    Status = _SEGGER_SHELL_PATH_AddSlash(sPath, PathLen);
    if (Status >= 0) {
      Status = _SEGGER_SHELL_PATH_Cat(sPath, PathLen, sNew, NewLen);
    }
  }
  //
  return Status;
}

/*********************************************************************
*
*       _SEGGER_SHELL_FindEnd()
*
*  Function description
*    Find end of current path element.
*
*  Parameters
*    sPath - Pointer to path name.
*
*  Return value
*    Pointer to end of element.  Will point to a '/' or zero character.
*/
static const char *_SEGGER_SHELL_FindEnd(const char *sPath) {
  while (*sPath != '\0' && *sPath != '/') {
    ++sPath;
  }
  return sPath;
}

/*********************************************************************
*
*       _ToLower()
*
*  Function description
*    Convert to lower case.
*
*  Parameters
*    C - Character to convert.
*
*  Return value
*    Converted character.
*/
static int _ToLower(char C) {
  if ('A' <= C && C <= 'Z') {
    C = C - 'A' + 'a';
  }
  return C;
}

/*********************************************************************
*
*       _IsSpace()
*
*  Function description
*    Predicate: Ignorable whitespace?
*
*  Parameters
*    C - Character to test.
*
*  Return value
*    != 0 - Character is ignorable whitespace
*    == 0 - Character is significant.
*/
static int _IsSpace(char C) {
  return C == ' ';
}

/*********************************************************************
*
*       _SEGGER_SHELL_SkipLeadingSpaces()
*
*  Function description
*    Skip leading spaces.
*
*  Parameters
*    sInput - Input string.
*
*  Return value
*    Pointer to first nonspace character found in input string.
*/
static char * _SEGGER_SHELL_SkipLeadingSpaces(char *sInput) {
  while (_IsSpace(*sInput)) {
    ++sInput;
  }
  return sInput;
}

/*********************************************************************
*
*       _SEGGER_SHELL_GetParsedArgument()
*
*  Function description
*    Retrieve an argument from the processed argument list.
*
*  Parameters
*    pText - List of zero-terminated strings.
*    N     - Index of string to return.
*
*  Return value
*    != 0 - Argument N exists and is retrieved.
*    == 0 - Argument N does not exist in the list.
*/
static char * _SEGGER_SHELL_GetParsedArgument(char *pText, unsigned N) {
  while (N > 0) {
    if (pText[0] == 0) {
      return 0;
    }
    pText = pText + strlen(pText) + 1;
    --N;
  }
  return pText;
}

/*********************************************************************
*
*       _ExecuteCommands()
*
*  Function description
*    Execute the 'commands' command and display help information in
*    column format.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code.
*/
static int _ExecuteCommands(SEGGER_SHELL_CONTEXT *pSelf) {
  SEGGER_SHELL_PrintCommands(pSelf);
  return 0;
}

/*********************************************************************
*
*       _ExecuteBye()
*
*  Function description
*    Execute the 'bye' command and set the context's completed flag.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code: always zero indicating success.
*/
static int _ExecuteBye(SEGGER_SHELL_CONTEXT *pSelf) {
  SEGGER_SHELL_Exit(pSelf);
  return 0;
}

/*********************************************************************
*
*       _SEGGER_SHELL_PrintOptions()
*
*  Function description
*    Print option list associated with a command.
*
*  Parameters
*    pSelf       - Pointer to shell context.
*    sOptionText - Complete option text (with tabs and newlines).
*    Indent      - Initial indentation, in columns.
*/
static void _SEGGER_SHELL_PrintOptions(SEGGER_SHELL_CONTEXT *pSelf, const char *sOptionText, int Indent) {
  const char * pWork;
  const char * pStart;
  int          OptionWidth;
  //
  pWork = sOptionText;
  OptionWidth = 0;
  while (*pWork != '\0') {
    //
    // Find first field.
    //
    pStart = pWork;
    while (*pWork && *pWork != '\t') {
      ++pWork;
    }
    if (pWork - pStart > OptionWidth) {
      OptionWidth = pWork - pStart;
    }
    if (*pWork == '\t') {
      ++pWork;
    }
    pStart = pWork;
    while (*pWork && *pWork != '\n') {
      ++pWork;
    }
    if (*pWork == '\n') {
      ++pWork;
    }
  }
  //
  pWork = sOptionText;
  while (*pWork) {
    //
    // Find first field.
    //
    pStart = pWork;
    while (*pWork && *pWork != '\t') {
      ++pWork;
    }
    SEGGER_SHELL_Printf(pSelf, "%*s%.*s", Indent, "", OptionWidth, pStart);
    SEGGER_SHELL_Printf(pSelf, "    ");
    if (*pWork == '\t') {
      ++pWork;
    }
    pStart = pWork;
    while (*pWork && *pWork != '\n') {
      ++pWork;
    }
    SEGGER_SHELL_Printf(pSelf, "%.*s\n", pWork - pStart, pStart);
    if (*pWork == '\n') {
      ++pWork;
    }
  }
}

/*********************************************************************
*
*       _SEGGER_SHELL_PrintUsage()
*
*  Function description
*    Print usage for single command with syntax and options.
*
*  Parameters
*    pSelf  - Pointer to shell context.
*    pAPI   - Command to print syntax.
*    Flags  - Formatting flags.
*    Indent - Initial indentation, in columns.
*/
static void _SEGGER_SHELL_PrintUsage(      SEGGER_SHELL_CONTEXT     * pSelf,
                                     const SEGGER_SHELL_COMMAND_API * pAPI,
                                           unsigned                   Flags,
                                           int                        Indent) {
  int ConsoleWidth;
  int w;
  //
  //
  ConsoleWidth = SEGGER_SHELL_GetWidth(pSelf);
  if (Flags & SEGGER_SHELL_HELP_FLAG_SHOW_COMMAND) {
    SEGGER_SHELL_Printf(pSelf,
                        SEGGER_SHELL_MONO(SEGGER_SHELL_ANSI_BRIGHT SEGGER_SHELL_ANSI_YELLOW_TEXT)
                        "%*s   "
                        SEGGER_SHELL_MONO(SEGGER_SHELL_ANSI_NORMAL SEGGER_SHELL_ANSI_WHITE_TEXT)
                        "%s",
                        -Indent,
                        pAPI->sName,
                        pAPI->sDescription);
    w = Indent + 3 + strlen(pAPI->sDescription);
  } else {
    w = 0;
  }
  if (pAPI->sSyntax && (Flags & SEGGER_SHELL_HELP_FLAG_SHOW_USAGE)) {
    //
    // If Usage fits on one line with command name, print it inline.
    //
    if (w > 0) {
      w += 2 + 8 + strlen(pAPI->sName) + 1 + strlen(pAPI->sDescription);
      if (w >= ConsoleWidth) {
        SEGGER_SHELL_Printf(pSelf, "\n%*s     ", -Indent, "");
      } else {
        SEGGER_SHELL_Printf(pSelf, "  ");
      }
    }
    SEGGER_SHELL_Printf(pSelf, "Usage: %s %s\n", pAPI->sName, pAPI->sSyntax);
    if (pAPI->sOptions && (Flags & SEGGER_SHELL_HELP_FLAG_SHOW_OPTIONS)) {
      _SEGGER_SHELL_PrintOptions(pSelf, pAPI->sOptions, Indent + 5);
    }
  } else {
    SEGGER_SHELL_Printf(pSelf, "\n");
  }
}

/*********************************************************************
*
*       _SEGGER_SHELL_PrintGroupTitle()
*
*  Function description
*    Print command group title.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    pAPI  - Pointer to group item API.
*/
static void _SEGGER_SHELL_PrintGroupTitle(      SEGGER_SHELL_CONTEXT     * pSelf,
                                          const SEGGER_SHELL_COMMAND_API * pAPI) {
  SEGGER_SHELL_Printf(pSelf, "%s\n", pAPI->sName);
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
static void _SEGGER_SHELL_PrintHelpList(SEGGER_SHELL_CONTEXT *pSelf, unsigned Flags) {
  const SEGGER_SHELL_COMMAND * pCommand;
  int                          MaxWidth;
  //
  MaxWidth = 0;
  for (pCommand = pSelf->pCommands; pCommand; pCommand = pCommand->pNext) {
    if (pCommand->pAPI->sName[0] != '-') {
      if ((int)strlen(pCommand->pAPI->sName) > MaxWidth) {
        MaxWidth = strlen(pCommand->pAPI->sName);
      }
    }
  }
  //
  if (pSelf->sName) {
    SEGGER_SHELL_Printf(pSelf, 
                        "\n"
                        SEGGER_SHELL_MONO(SEGGER_SHELL_ANSI_BRIGHT SEGGER_SHELL_ANSI_MAGENTA_TEXT)
                        "%s Commands:\n\n"
                        SEGGER_SHELL_MONO(SEGGER_SHELL_ANSI_NORMAL SEGGER_SHELL_ANSI_WHITE_TEXT),
                        pSelf->sName);
  } else {
    SEGGER_SHELL_Printf(pSelf,
                        "\n"
                        SEGGER_SHELL_MONO(SEGGER_SHELL_ANSI_BRIGHT SEGGER_SHELL_ANSI_MAGENTA_TEXT)
                        "Commands:\n\n"
                        SEGGER_SHELL_MONO(SEGGER_SHELL_ANSI_NORMAL SEGGER_SHELL_ANSI_WHITE_TEXT));
  }
  for (pCommand = pSelf->pCommands; pCommand; pCommand = pCommand->pNext) {
    if (pCommand->pAPI->sName[0] == '-') {
      _SEGGER_SHELL_PrintGroupTitle(pSelf, pCommand->pAPI);
    } else {
      _SEGGER_SHELL_PrintUsage(pSelf, pCommand->pAPI, Flags, MaxWidth);
    }
  }
  SEGGER_SHELL_Printf(pSelf, "\n");
}

/*********************************************************************
*
*       _SEGGER_SHELL_AddCommand()
*
*  Function description
*    Add command to shell command set.
*
*  Parameters
*    pSelf    - Pointer to shell context.
*    pCommand - Pointer to an array of commands terminated by
*               a zero command set structure.
*
*  Return value
*    >= 0 - Success.
*     < 0 - Error indication.
*/
static int _SEGGER_SHELL_AddCommand(SEGGER_SHELL_CONTEXT *pSelf, SEGGER_SHELL_COMMAND *pCommand) {
  // Prevent adding command to two contexts.
  if (pCommand->pNext != NULL) {
    return SEGGER_SHELL_ERROR_COMMAND_ALREADY_INSTALLED;
  }
  if (pSelf->pCommands == NULL) {
    pSelf->pCommands = pCommand;
  } else {
    SEGGER_SHELL_COMMAND *pTmp;
    //
    pTmp = pSelf->pCommands;
    while (pTmp->pNext != NULL) {
      pTmp = pTmp->pNext;
    }
    pTmp->pNext = pCommand;
  }
  //
  return 0;
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_SHELL_Init()
*
*  Function description
*    Initialize shell context.
*
*  Parameters
*    pSelf           - Pointer to shell context.
*    pConsoleAPI     - Pointer to console I/O API implementation.
*    pMem            - Pointer to memory allocator to use for dynamic storage.
*/
void SEGGER_SHELL_Init(      SEGGER_SHELL_CONTEXT     * pSelf,
                       const SEGGER_SHELL_CONSOLE_API * pConsoleAPI,
                             SEGGER_MEM_CONTEXT       * pMem) {
  memset(pSelf, 0, sizeof(*pSelf));
  pSelf->pConsoleAPI     = pConsoleAPI;
  pSelf->pMemory         = pMem;
}

/*********************************************************************
*
*       SEGGER_SHELL_InitEx()
*
*  Function description
*    Initialize the Shell context.
*
*  Parameters
*    pSelf           - Pointer to shell context.
*    pConsoleAPI     - Pointer to console I/O API implementation.
*    pConsoleContext - Pointer to user-provided context to pass to console
*                      API functions.
*    pMem            - Pointer to memory allocator to use for dynamic storage.
*/
void SEGGER_SHELL_InitEx(      SEGGER_SHELL_CONTEXT        * pSelf,
                         const SEGGER_SHELL_CONSOLE_EX_API * pConsoleAPI,
                               void                        * pConsoleContext,
                               SEGGER_MEM_CONTEXT          * pMem) {
  memset(pSelf, 0, sizeof(*pSelf));
  pSelf->pConsoleExAPI   = pConsoleAPI;
  pSelf->pConsoleContext = pConsoleContext;
  pSelf->pMemory         = pMem;
}

/*********************************************************************
*
*       SEGGER_SHELL_InheritExternal()
*
*  Function description
*    Inherit a shell context from an existing shell context.
*
*    This function sets the Shell context from an (argc, argv) pair
*    provided by the operating system to main().  The context is set
*    up such that the first argument extracted will come from argv[1].
*
*  Parameters
*    pSelf        - Pointer to shell context.
*    sProgramName - Zero-terminated program name.
*    argc         - Argument count supplied by the client or OS.
*    argv         - Argument vector supplied by the client or OS.
*/
void SEGGER_SHELL_InheritExternal(SEGGER_SHELL_CONTEXT * pSelf,
                                  char                 * sProgramName,
                                  int                    argc,
                                  char                 * argv[]) {
  //
  // Take a copy of the argument vector.
  //
  if (sProgramName) {
    pSelf->sProgramName = sProgramName;
  } else if (argc > 0) {
    pSelf->sProgramName = argv[0];
  } else {
    pSelf->sProgramName = "<none>";
  }
  pSelf->argc        = (unsigned)argc;
  pSelf->argv        = argv;
  pSelf->pWork       = NULL;
  pSelf->ArgumentIdx = 0;
}

/*********************************************************************
*
*       SEGGER_SHELL_Inherit()
*
*  Function description
*    Inherit a shell context.  All state is inherited from the parent
*    context apart excluding the list of commands.
*
*  Parameters
*    pSelf - Pointer to shell context to inherit into.
*    pFrom - Pointer to shell context to inherit from.
*    sName - Name of shell context (can be null).
*/
void SEGGER_SHELL_Inherit(      SEGGER_SHELL_CONTEXT * pSelf,
                                SEGGER_SHELL_CONTEXT * pFrom,
                          const char                 * sName) {
  //
  // Take a copy of the context, but not the commands.
  //
  *pSelf           = *pFrom;
  pSelf->sName     = sName;
  pSelf->pCommands = NULL;
}

/*********************************************************************
*
*       SEGGER_SHELL_Process()
*
*  Function description
*    Process the command line.  This function extracts the next argument
*    from the command line, looks it up the in list of commands, and
*    executes it if found.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    >= 0 - Command executed without error.
*     < 0 - Command not found or command execution error.
*/
int SEGGER_SHELL_Process(SEGGER_SHELL_CONTEXT *pSelf) {
  const SEGGER_SHELL_COMMAND_API * pCommand;
  char                           * sName;
  int                              Status;
  //
  Status = SEGGER_SHELL_ReadNextArg(pSelf, &sName);
  if (Status == SEGGER_SHELL_ERROR_MORE_ARGS_EXPECTED) {
    Status = 0;
  } else if (Status >= 0) {
    Status = SEGGER_SHELL_FindCommand(pSelf, sName, &pCommand);
    if (Status >= 0) {
      Status = pCommand->pfExecute(pSelf);
    }
  }
  return Status;
}

/*********************************************************************
*
*       SEGGER_SHELL_ReadNextArg()
*
*  Function description
*    Read the next argument.  This gets the next argument from the
*    command line, looks it up the in command vector, and executes it.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    pArg  - Pointer to the place to store the next command line
*            argument pointer.
*
*  Return value
*    >= 0 - Argument fetched without error.
*     < 0 - Read beyond end of arguments.
*/
int SEGGER_SHELL_ReadNextArg(SEGGER_SHELL_CONTEXT *pSelf, char **pArg) {
  char *p;
  //
  p = NULL;
  if (pSelf->ArgumentIdx < pSelf->argc) {
    if (pSelf->ArgumentIdx == 0) {
      p = pSelf->sProgramName;
    } else if (pSelf->argv) {
      p = pSelf->argv[pSelf->ArgumentIdx];
    } else {
      p = _SEGGER_SHELL_GetParsedArgument(pSelf->pWork, pSelf->ArgumentIdx);
    }
    pSelf->ArgumentIdx++;
    if (pArg) {
      *pArg = p;
    }
    return 0;
  } else {
    if (pArg) {
      *pArg = 0;
    }
    return SEGGER_SHELL_ERROR_MORE_ARGS_EXPECTED;
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_ReadNextU32()
*
*  Function description
*    Read the next argument.
*
*  Parameters
*    pSelf        - Pointer to shell context.
*    pArg         - Pointer to the place to store the next U32 parsed
*                   from the command line.
*    DefaultValue - Value to substitute if argument is not provided.
*
*  Return value
*    >= 0 - Argument fetched without error.
*     < 0 - Read beyond end of arguments.
*/
int SEGGER_SHELL_ReadNextU32(SEGGER_SHELL_CONTEXT *pSelf, U32 *pArg, U32 DefaultValue) {
  char * p;
  int    Status;
  //
  p = NULL;
  Status = SEGGER_SHELL_ReadNextArg(pSelf, &p);
  if (Status == SEGGER_SHELL_ERROR_MORE_ARGS_EXPECTED) {
    *pArg = DefaultValue;
  } else {
    *pArg = strtoul(p, 0, 0);
  }
  return 0;
}

/*********************************************************************
*
*       SEGGER_SHELL_PeekNextArg()
*
*  Function description
*    Look ahead to the next argument on the command line but do not
*    advance to it.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    pArg  - Pointer to the place to store the next command line
*            argument pointer.
*
*  Return value
*    >= 0 - Argument fetched without error.
*     < 0 - Read beyond end of arguments.
*/
int SEGGER_SHELL_PeekNextArg(SEGGER_SHELL_CONTEXT *pSelf, char **pArg) {
  char *p ;
  //
  p = NULL;
  if (pSelf->ArgumentIdx < pSelf->argc) {
    if (pSelf->ArgumentIdx == 0) {
      p = pSelf->sProgramName;
    } else if (pSelf->argv) {
      p = pSelf->argv[pSelf->ArgumentIdx];
    } else {
      p = _SEGGER_SHELL_GetParsedArgument(pSelf->pWork, pSelf->ArgumentIdx);
    }
    if (pArg) {
      *pArg = p;
    }
    return 0;
  } else {
    if (pArg) {
      *pArg = NULL;
    }
    return SEGGER_SHELL_ERROR_MORE_ARGS_EXPECTED;
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_CountTotalArgs()
*
*  Function description
*    Get the total number of arguments in the argument vector.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Total number of arguments in the argument vector.
*/
int SEGGER_SHELL_CountTotalArgs(const SEGGER_SHELL_CONTEXT *pSelf) {
  return pSelf->argc;
}

/*********************************************************************
*
*       SEGGER_SHELL_CanReadArg()
*
*  Function description
*    Get the total number of unread arguments in the argument vector.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Total number of unread arguments in the argument vector.
*/
int SEGGER_SHELL_CanReadArg(const SEGGER_SHELL_CONTEXT *pSelf) {
  return pSelf->argc - pSelf->ArgumentIdx;
}

/*********************************************************************
*
*       SEGGER_SHELL_ParseInput()
*
*  Function description
*    Parse a textual command line ready for argument extraction.
*
*  Parameters
*    pSelf  - Pointer to shell context.
*    sInput - Command line to parse.  This argument is modified
*             during parsing.
*
*  Return value
*    >= 0 - Parse OK.
*     < 0 - Parse error.
*/
int SEGGER_SHELL_ParseInput(SEGGER_SHELL_CONTEXT *pSelf, char *sInput) {
  char *sOutput;
  //
  // No arguments to begin with.
  //
  pSelf->argc        = 0;
  pSelf->argv        = 0;
  pSelf->ArgumentIdx = 0;
  pSelf->pWork       = sInput;
  //
  sOutput = pSelf->pWork;
  //
  for (;;) {
    //
    // Skip spaces to the next argument.
    //
    sInput = _SEGGER_SHELL_SkipLeadingSpaces(sInput);
    //
    // If we've come to the end of input, we're done.
    if (*sInput == 0) {
      break;
    }
    //
    // Register new argument.
    //
    ++pSelf->argc;
    //
    // Gather single token which may be quoted.  For instance,
    // foo"bar" -> foobar, foo" "bar -> foo bar.  Because Windows
    // uses backslash in file paths, no special processing is
    // undertaken when a backslash is seen.  We might revisit this
    // so that quotation marks can be added, e.g. foo\"bar -> foo"bar.
    //
    for (;;) {
      //
      if (*sInput == '\0') {
        //
        // Argument terminates at end of line.
        //
        break;
      } else if (_IsSpace(*sInput)) {
        //
        // Argument terminates at space.
        //
        ++sInput;
        break;
      } else if (*sInput == '"') {
        //
        // Quoted text will allow spaces in arguments.
        //
        ++sInput;
        while (*sInput && *sInput != '"') {
          *sOutput++ = *sInput++;
        }
        if (*sInput == '"') {
          ++sInput;
        }
      } else {
        //
        // Gather this character into token.
        //
        *sOutput++ = *sInput++;
      }
    }
    //
    // Terminate this argument.
    //
    *sOutput++ = '\0';
  }
  //
  // Terminate list of arguments.
  //
  *sOutput++ = '\0';
  //
  // Set up command name.
  //
  pSelf->sProgramName = pSelf->pWork;
  //
  return 0;
}

/*********************************************************************
*
*       SEGGER_SHELL_DecodeError()
*
*  Function description
*    Decode an error or status code.
*
*  Parameters
*    Status  - Error or status to decode.
*    sText   - Pointer to object that receives the decoded error string.
*    TextLen - Size of the object receiving the decoded error string.
*/
void SEGGER_SHELL_DecodeError(int Status, char *sText, unsigned TextLen) {
  //
  // Can't have a zero-length object, but guard against it.
  //
  if (TextLen == 0) {
    return;
  }
  //
  switch (Status) {
  case 0:
    sText[0] = 0;
    break;
  case SEGGER_SHELL_ERROR_MORE_ARGS_EXPECTED:
    strncpy(sText, "More arguments expected", TextLen);
    break;
  case SEGGER_SHELL_ERROR_COMMAND_NOT_RECOGNIZED:
    strncpy(sText, "Command not recognized", TextLen);
    break;
  case SEGGER_SHELL_ERROR_COMMAND_ALREADY_INSTALLED:
    strncpy(sText, "Command already installed", TextLen);
    break;
  default:
    strncpy(sText, "Unknown error", TextLen);
    break;
  }
  //
  // Ensure always terminated [strncpy].
  //
  sText[TextLen-1] = 0;
}

/*********************************************************************
*
*       SEGGER_SHELL_PrintError()
*
*  Function description
*    Print a decoded error.
*
*  Parameters
*    pSelf  - Pointer to shell context.
*    Status - Status to decode and print.
*/
void SEGGER_SHELL_PrintError(SEGGER_SHELL_CONTEXT *pSelf, int Status) {
  char sErrorText[128];
  //
  if (Status < 0) {
    SEGGER_SHELL_DecodeError(Status, sErrorText, sizeof(sErrorText));
    SEGGER_SHELL_Printf(pSelf, "Error: %s\n", sErrorText);
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_AddCommand()
*
*  Function description
*    Add a command to the shell command set.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    pAPI  - Pointer to shell API.
*
*  Return value
*    >= 0 - Success.
*     < 0 - Error indication.
*/
int SEGGER_SHELL_AddCommand(SEGGER_SHELL_CONTEXT *pSelf, const SEGGER_SHELL_COMMAND_API *pAPI) {
  SEGGER_SHELL_COMMAND *pCommand;
  //
  pCommand = SEGGER_PTR2PTR(SEGGER_SHELL_COMMAND,
                            SEGGER_MEM_Alloc(pSelf->pMemory, sizeof(SEGGER_SHELL_COMMAND)));
  if (pCommand == NULL) {
    return SEGGER_SHELL_ERROR_OUT_OF_MEMORY;
  } else {
    pCommand->pAPI  = pAPI;
    pCommand->pNext = 0;
    pCommand->Flags = 1;
    return _SEGGER_SHELL_AddCommand(pSelf, pCommand);
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_InputText()
*
*  Function description
*    Read text for the command interpreter.
*
*  Parameters
*    pSelf    - Pointer to shell context.
*    sText    - Pointer to buffer that will receives user input.
*               Zero terminated on exit.
*    TextLen  - Size of object that receives user input.
*
*  Return value
*    < 0 - Error reading text (e.g. eof)
*   >= 0 - Success.
*/
int SEGGER_SHELL_InputText(SEGGER_SHELL_CONTEXT *pSelf, char *sText, unsigned TextLen) {
  if (pSelf->pConsoleAPI != NULL) {
    return pSelf->pConsoleAPI->pfGetString(sText, TextLen);
  } else {
    return pSelf->pConsoleExAPI->pfGetString(pSelf->pConsoleContext, sText, TextLen);
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_FindCommand()
*
*  Function description
*    Find command in command vector.
*
*  Parameters
*    pSelf        - Pointer to shell context.
*    sName        - Command to find.
*    ppCommandAPI - Pointer to (pointer to) command API for the command.  If the
*                   command is not found, the returned pointer is set
*                   to NULL.
*
*  Return value
*     < 0 - Command not found.
*    >= 0 - Command found.
*/
int SEGGER_SHELL_FindCommand(const SEGGER_SHELL_CONTEXT *pSelf, const char *sName, const SEGGER_SHELL_COMMAND_API **ppCommandAPI) {
  const SEGGER_SHELL_COMMAND *pCommand;
  //
  for (pCommand = pSelf->pCommands; pCommand != NULL; pCommand = pCommand->pNext) {
    if (SEGGER_strcasecmp(pCommand->pAPI->sName, sName) == 0) {
      if (ppCommandAPI) {
        *ppCommandAPI = pCommand->pAPI;
      }
      return 0;
    }
  }
  //
  if (ppCommandAPI != NULL) {
    *ppCommandAPI = NULL;
  }
  return SEGGER_SHELL_ERROR_COMMAND_NOT_RECOGNIZED;
}

/*********************************************************************
*
*       SEGGER_SHELL_Exit()
*
*  Function description
*    Set the shell exit flag and clean up. The exit flags indicates
*    that the user (or application) has requested the shell to exit.
*
*  Parameters
*    pSelf - Pointer to shell context.
*/
void SEGGER_SHELL_Exit(SEGGER_SHELL_CONTEXT *pSelf) {
  SEGGER_SHELL_COMMAND *pCommand;
  SEGGER_SHELL_COMMAND *pNext;
  //
  pCommand = pSelf->pCommands;
  while (pCommand) {
    pNext = pCommand->pNext;
    if (pCommand->Flags) {
      SEGGER_MEM_Free(pSelf->pMemory, pCommand);
    } else {
      pCommand->pNext = 0;
    }
    pCommand = pNext;
  }
  pSelf->pCommands = 0;
  pSelf->Done      = 1;
}

/*********************************************************************
*
*       SEGGER_SHELL_IsExited()
*
*  Function description
*    Return the shell exit flag.  The exit flag indicates that the user
*    (or application) has requested the shell to quit.
*
*  Parameters
*    pSelf - Pointer to shell to test.
*
*  Return value
*    == 0    No exit requested
*    != 0    Exit requested
*/
int SEGGER_SHELL_IsExited(const SEGGER_SHELL_CONTEXT *pSelf) {
  return pSelf->Done;
}

/*********************************************************************
*
*       SEGGER_SHELL_Printf()
*
*  Function description
*    Print formatted output.
*
*  Parameters
*    pSelf   - Pointer to shell context.
*    sFormat - Format control string.
*/
void SEGGER_SHELL_Printf(SEGGER_SHELL_CONTEXT *pSelf, const char *sFormat, ...) {
  va_list Params;
  //
  va_start(Params, sFormat);
  if (pSelf->pConsoleAPI != NULL) {
    pSelf->pConsoleAPI->pfPrintString(sFormat, Params);
  } else {
    pSelf->pConsoleExAPI->pfPrintString(pSelf->pConsoleContext, sFormat, Params);
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_Printvf()
*
*  Function description
*    Print formatted output.
*
*  Parameters
*    pSelf   - Pointer to shell context.
*    sFormat - Format control string.
*    Params  - Variadic parameter list.
*/
void SEGGER_SHELL_Printvf(SEGGER_SHELL_CONTEXT *pSelf, const char *sFormat, va_list Params) {
  if (pSelf->pConsoleAPI != NULL) {
    pSelf->pConsoleAPI->pfPrintString(sFormat, Params);
  } else {
    pSelf->pConsoleExAPI->pfPrintString(pSelf->pConsoleContext, sFormat, Params);
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_Puts()
*
*  Function description
*    Print string, no newline.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    sText - Pointer to zero-terminated string.
*/
void SEGGER_SHELL_Puts(SEGGER_SHELL_CONTEXT *pSelf, const char *sText) {
  SEGGER_SHELL_Printf(pSelf, "%s", sText);
}

/*********************************************************************
*
*       SEGGER_SHELL_Enter()
*
*  Function description
*    Enter a simple interactive shell.  Commands are read and processed
*    until the shell exit flag is set.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    == 0    Shell ran to completion without error.
*    != 0    Error reading user input or executing inherited command.
*/
int SEGGER_SHELL_Enter(SEGGER_SHELL_CONTEXT *pSelf) {
  char acInput[128];
  int  Status;
  //
  // Interactive or non-interactive?
  //
  if (SEGGER_SHELL_CanReadArg(pSelf) <= 1) {
    //
    // No arguments, sign on and enter interactive mode.
    //
    while (!SEGGER_SHELL_IsExited(pSelf)) {
      //
      // Get input with prompt.
      //
      SEGGER_SHELL_Printf(pSelf, "> ");
      Status = SEGGER_SHELL_InputText(pSelf, acInput, sizeof(acInput));
      if (Status >= 0) {
        Status = SEGGER_SHELL_ParseInput(pSelf, acInput);
      }
      if (Status >= 0 && SEGGER_SHELL_CanReadArg(pSelf) > 0) {
        //
        // There is at least a command, so execute it.
        //
        Status = SEGGER_SHELL_Process(pSelf);
      }
      if (Status < 0) {
        SEGGER_SHELL_Printf(pSelf, "Error: %d\n", Status);
      }
    }
    //
    Status = 0;
    //
  } else {
    //
    // Non-interactive, just run the command from the OS.
    //
    Status = SEGGER_SHELL_ReadNextArg(pSelf, NULL);
    if (Status >= 0) {
      Status = SEGGER_SHELL_Process(pSelf);
    }
  }
  //
  return Status;
}

/*********************************************************************
*
*       SEGGER_SHELL_SetName()
*
*  Function description
*    Enter a simple interactive shell.  Commands are read and processed
*    until the shell exit flag is set.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    sName - Pointer to shell name.
*/
void SEGGER_SHELL_SetName(SEGGER_SHELL_CONTEXT *pSelf, const char *sName) {
  pSelf->sName = sName;
}

/*********************************************************************
*
*       SEGGER_SHELL_PrintUsage()
*
*  Function description
*    Print usage for a single command with syntax and options.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    pAPI  - Pointer to shell command API.
*    Flags - Formatting flags.
*/
void SEGGER_SHELL_PrintUsage(      SEGGER_SHELL_CONTEXT     * pSelf,
                             const SEGGER_SHELL_COMMAND_API * pAPI,
                                   unsigned                   Flags) {
  _SEGGER_SHELL_PrintUsage(pSelf, pAPI, Flags | SEGGER_SHELL_HELP_FLAG_SHOW_USAGE, 4);
}

/*********************************************************************
*
*       SEGGER_SHELL_PrintHelp()
*
*  Function description
*    Print help information for a shell context.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    Flags - Formatting flags.
*/
void SEGGER_SHELL_PrintHelp(SEGGER_SHELL_CONTEXT *pSelf, unsigned Flags) {
  _SEGGER_SHELL_PrintHelpList(pSelf, Flags);
}

/*********************************************************************
*
*       SEGGER_SHELL_PrintCommands()
*
*  Function description
*    Print all commands registered to a shell context using a compact
*    columnar format.
*
*  Parameters
*    pSelf - Pointer to shell context.
*/
void SEGGER_SHELL_PrintCommands(SEGGER_SHELL_CONTEXT *pSelf) {
  const SEGGER_SHELL_COMMAND * pCommand;
  int                          MaxWidth;
  int                          ConsoleWidth;
  int                          NumColumns;
  int                          Column;
  //
  ConsoleWidth = SEGGER_SHELL_GetWidth(pSelf);
  MaxWidth = 1;
  for (pCommand = pSelf->pCommands; pCommand; pCommand = pCommand->pNext) {
    if (pCommand->pAPI->sName[0] != '-') {
      if ((int)strlen(pCommand->pAPI->sName) > MaxWidth) {
        MaxWidth = strlen(pCommand->pAPI->sName);
      }
    }
  }
  //
  NumColumns = (ConsoleWidth - 4) / (MaxWidth + 4);
  Column = 0;
  //
  for (pCommand = pSelf->pCommands; pCommand; pCommand = pCommand->pNext) {
    if (pCommand->pAPI->sName[0] == '-') {
      if (Column) {
        SEGGER_SHELL_Printf(pSelf, "\n");
      }
      SEGGER_SHELL_Printf(pSelf, "\n%s\n", pCommand->pAPI->sName);
      Column = 0;
    } else {
      if (Column == 0) {
        SEGGER_SHELL_Printf(pSelf, "\n");
      } else {
        SEGGER_SHELL_Printf(pSelf, "    ");
      }
      SEGGER_SHELL_Printf(pSelf, "%*s", -MaxWidth, pCommand->pAPI->sName);
      Column = (Column + 1) % NumColumns;
    }
  }
  if (Column) {
    SEGGER_SHELL_Printf(pSelf, "\n");
  }
  SEGGER_SHELL_Printf(pSelf, "\n");
}

/*********************************************************************
*
*       SEGGER_SHELL_ExecuteHelp()
*
*  Function description
*    Execute the 'help' command and display help information.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Standard status code.
*/
int SEGGER_SHELL_ExecuteHelp(SEGGER_SHELL_CONTEXT *pSelf) {
  char       * sArg;
  const char * sCommand;
  int          Flags;
  int          Status;
  //
  Flags    = SEGGER_SHELL_HELP_FLAG_SHOW_COMMAND;
  Status   = 0;
  sCommand = 0;
  //
  while (SEGGER_SHELL_ReadNextArg(pSelf, &sArg) >= 0) {
    if (strcmp(sArg, "-x") == 0) {
      Flags |= SEGGER_SHELL_HELP_FLAG_SHOW_USAGE;
    } else if (strcmp(sArg, "-v") == 0) {
      Flags |= SEGGER_SHELL_HELP_FLAG_SHOW_USAGE | SEGGER_SHELL_HELP_FLAG_SHOW_OPTIONS;
    } else if (sArg[0] != '-') {
      sCommand = sArg;
    }
  }
  //
  if (sCommand) {
    const SEGGER_SHELL_COMMAND_API *pAPI;
    //
    Status = SEGGER_SHELL_FindCommand(pSelf, sCommand, &pAPI);
    if (Status == 0) {
      SEGGER_SHELL_PrintUsage(pSelf, pAPI, SEGGER_SHELL_HELP_FLAG_SHOW_USAGE | SEGGER_SHELL_HELP_FLAG_SHOW_OPTIONS);
    }
  } else {
    SEGGER_SHELL_PrintHelp(pSelf, Flags);
  }
  //
  return Status;
}

/*********************************************************************
*
*       SEGGER_SHELL_ResetArgIterator()
*
*  Function description
*    Reposition the argument iterator to re-read arguments.
*
*  Parameters
*    pSelf - Pointer to shell context.
*    Index - Iterator index.
*/
void SEGGER_SHELL_ResetArgIterator(SEGGER_SHELL_CONTEXT *pSelf, unsigned Index) {
  if (Index < pSelf->argc) {
    pSelf->ArgumentIdx = Index;
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_GetWidth()
*
*  Function description
*    Get width of console.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Console width.
*/
int SEGGER_SHELL_GetWidth(SEGGER_SHELL_CONTEXT * pSelf) {
  if (pSelf->pConsoleAPI != NULL && pSelf->pConsoleAPI->pfGetWidth != NULL) {
    return pSelf->pConsoleAPI->pfGetWidth();
  } if (pSelf->pConsoleExAPI != NULL && pSelf->pConsoleExAPI->pfGetWidth != NULL) {
    return pSelf->pConsoleExAPI->pfGetWidth(pSelf->pConsoleContext);
  } else {
    return 80;
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_GetHeight()
*
*  Function description
*    Get height of console.
*
*  Parameters
*    pSelf - Pointer to shell context.
*
*  Return value
*    Console height.
*/
int SEGGER_SHELL_GetHeight(SEGGER_SHELL_CONTEXT * pSelf) {
  if (pSelf->pConsoleAPI != NULL && pSelf->pConsoleAPI->pfGetHeight != NULL) {
    return pSelf->pConsoleAPI->pfGetHeight();
  } if (pSelf->pConsoleExAPI != NULL && pSelf->pConsoleExAPI->pfGetHeight != NULL) {
    return pSelf->pConsoleExAPI->pfGetHeight(pSelf->pConsoleContext);
  } else {
    return 80;
  }
}

/*********************************************************************
*
*       SEGGER_SHELL_AddCommands()
*
*  Function description
*    Add core shell commands.
*
*  Parameters
*    pSelf - Pointer to shell context.
*/
void SEGGER_SHELL_AddCommands(SEGGER_SHELL_CONTEXT *pSelf) {
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_SHELL_COMMAND_Help);
  SEGGER_SHELL_AddCommand(pSelf, &SEGGER_SHELL_COMMAND_Bye);
}

/*********************************************************************
*
*       SEGGER_SHELL_PATH_Add()
*
*  Function description
*    Change to new folder.
*
*  Parameters
*    sPath   - Pointer to path name.
*    PathLen - Capacity of path name.
*    sNew    - Pointer to zero-terminated string specifying new folder.
*
*  Return value
*    == 0 - Success.
*    <  0 - Error, path overflow.
*/
int SEGGER_SHELL_PATH_Add(char *sPath, unsigned PathLen, const char *sNew) {
  int          Status;
  const char * pEnd;
  //
  Status = 0;
  //
  if (sNew[0] == '/') {
    Status = _SEGGER_SHELL_PATH_Root(sPath, PathLen);
    while (*sNew == '/') {
      ++sNew;
    }
  }
  while (Status >= 0) {
    pEnd = _SEGGER_SHELL_FindEnd(sNew);
    Status = _SEGGER_SHELL_PATH_AddPart(sPath, PathLen, sNew, pEnd-sNew);
    if (Status < 0 || *pEnd == '\0') {
      break;
    }
    sNew = pEnd + 1;
  }
  //
  return Status;
}

/*********************************************************************
*
*       SEGGER_SHELL_PATH_AddExtension()
*
*  Function description
*    Append file name extension.
*
*  Parameters
*    sPath   - Pointer to path name where extension will be added.
*    PathLen - Capacity of path name.
*    sExt    - Extension to add, e.g. ".bas".
*
*  Return value
*    == 0 - Success.
*    <  0 - Error, path overflow.
*/
int SEGGER_SHELL_PATH_AddExtension(char *sPath, unsigned PathLen, const char *sExt) {
  unsigned L;
  int      Status;
  //
  Status = 0;
  L = SEGGER_strlen(sPath);
  while (L > 0) {
    --L;
    if (sPath[L] == '/' || sPath[L] == ':' || L == 0) {
      Status = _SEGGER_SHELL_PATH_Cat(sPath, PathLen, sExt, strlen(sExt));
      break;
    }
    if (sPath[L] == '.') {
      break;
    }
  }
  //
  return Status;
}

/*********************************************************************
*
*       SEGGER_SHELL_PATH_Split()
*
*  Function description
*    Split path at final '/'.
*
*  Parameters
*    sPath   - Pointer to path name.
*    PathLen - Capacity of path name.
*
*  Return value
*    Pointer to final part.
*/
char * SEGGER_SHELL_PATH_Split(char *sPath, unsigned PathLen) {
  unsigned L;
  //
  _SEGGER_SHELL_PATH_Normalize(sPath, PathLen);
  L = SEGGER_strlen(sPath) - 1u;
  while (sPath[L] != '/') {
    --L;
  }
  sPath[L] = 0;
  return &sPath[L+1];
}

/*********************************************************************
*
*       SEGGER_SHELL_PATH_Match()
*
*  Function description
*    Match string against pattern.
*
*  Parameters
*    sPat - Zero-terminated pattern string.
*    sStr - Zero-terminated string to match.
*
*  Return value
*    0 -- No match.
*    1 -- Match.
*/
int SEGGER_SHELL_PATH_Match(const char *sPat, const char *sStr) {
  const char   * s;
  const char   * p;
  int            Star;
  //
  Star = 0;
  //
Again:
  for (s = sStr, p = sPat; *s; ++s, ++p) {
    switch (*p) {
    case '?':
      break;
      //
    case '*':
      Star = 1;
      sStr = s;
      sPat = p;
      if (*++sPat == 0) {
        return 1;
      }
      goto Again;
      //
    default:
      if (*s != *p) {
        if (!Star) {
          return 0;
        }
        ++sStr;
        goto Again;
      }
      break;
    }
  }
  if (*p == '*') {
    ++p;
  }
  return *p == 0;
}

/*********************************************************************
*
*       SEGGER_SHELL_PATH_CaseMatch()
*
*  Function description
*    Match string against pattern, case-insensitive.
*
*  Parameters
*    sPat - Zero-terminated pattern string.
*    sStr - Zero-terminated string to match.
*
*  Return value
*    0 -- No match.
*    1 -- Match.
*/
int SEGGER_SHELL_PATH_CaseMatch(const char *sPat, const char *sStr) {
  const char   * s;
  const char   * p;
  int            Star;
  //
  Star = 0;
  //
Again:
  for (s = sStr, p = sPat; *s; ++s, ++p) {
    switch (*p) {
    case '?':
      break;
      //
    case '*':
      Star = 1;
      sStr = s;
      sPat = p;
      if (*++sPat == 0) {
        return 1;
      }
      goto Again;
      //
    default:
      if (_ToLower(*s) != _ToLower(*p)) {
        if (!Star) {
          return 0;
        }
        ++sStr;
        goto Again;
      }
      break;
    }
  }
  if (*p == '*') {
    ++p;
  }
  return *p == 0;
}

/*************************** End of file ****************************/
