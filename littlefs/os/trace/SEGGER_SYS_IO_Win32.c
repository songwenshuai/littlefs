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
File    : SEGGER_SYS_IO_Win32.c
Purpose : Implementation for API functions, debug output to
          the Windows debug console.
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
#include <stdio.h>
#include <string.h>
#include <conio.h>

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SEGGER_SYS_IO_Init()
*
*  Function description
*    Initialize I/O subsystem.
*/
void SEGGER_SYS_IO_Init(void) {
}

/*********************************************************************
*
*       SEGGER_SYS_IO_Print()
*
*  Function description
*    Output a string to standard output.
*
*  Parameters
*    sText - Text to output, zero terminated.
*/
void SEGGER_SYS_IO_Print(const char *sText) {
  printf("%s", sText);
}

/*********************************************************************
*
*       SEGGER_SYS_IO_Error()
*
*  Function description
*    Output a string to error output.
*
*  Parameters
*    sText - Text to output, zero terminated.
*/
void SEGGER_SYS_IO_Error(const char *sText) {
  fprintf(stderr, "%s", sText);
}

/*********************************************************************
*
*       SEGGER_SYS_IO_Debug()
*
*  Function description
*    Output a string to debug output.
*
*  Parameters
*    sText - Text to output, zero terminated.
*/
void SEGGER_SYS_IO_Debug(const char *sText) {
  fprintf(stderr, "%s", sText);
}

/*********************************************************************
*
*       SEGGER_SYS_IO_Gets()
*
*  Function description
*    Read a string from standard input.
*
*  Parameters
*    acText[TextByteCnt] - Buffer that contains text.
*
*  Return value
*     < 0 - Error reading string (end of file on standard input).
*    >= 0 - Number of characters read.
*/
int SEGGER_SYS_IO_Gets(char acText[], unsigned TextByteCnt) {
  if (fgets(acText, TextByteCnt, stdin) != 0) {
    unsigned Len = strlen(acText);
    if (Len > 0 && acText[Len-1] == '\n') {
      --Len;
      acText[Len] = '\0';
    }
    return (int)Len;
  } else {
    return -1;
  }
}

/*********************************************************************
*
*       SEGGER_SYS_IO_Getc()
*
*  Function description
*    Read a single character from standard input.
*
*  Return value
*     < 0 - Error indication.
*    >= 0 - Character.
*/
int SEGGER_SYS_IO_Getc(void) {
  return _getch();
}

#endif  // _WIN32

/****** End Of File *************************************************/
