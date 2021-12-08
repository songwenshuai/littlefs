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
File    : SEGGER_SYS_Win32.c
Purpose : Implementation for API functions.
Revision: $Rev: 11360 $
*/

#ifdef _WIN32

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SEGGER_SYS.h"
#include <conio.h>
#include <windows.h>

/*********************************************************************
*
*       Imported data
*
**********************************************************************
*/

extern int SEGGER_SYS__StartedFromExplorer;

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_SYS_Init()
*/
void SEGGER_SYS_Init(void) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE                     hStdOutput;
  //
  hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  if (!GetConsoleScreenBufferInfo(hStdOutput, &csbi)) {
    return;
  }
  //
  // if cursor position is (0, 0) then started from Explorer not CMD.
  //
  SEGGER_SYS__StartedFromExplorer = csbi.dwCursorPosition.X == 0 && csbi.dwCursorPosition.Y == 0;
}

/*********************************************************************
*
*       SEGGER_SYS_Exit()
*/
void SEGGER_SYS_Exit(void) {
  /* Nothing */
}

#endif  // _WIN32

/****** End Of File *************************************************/
