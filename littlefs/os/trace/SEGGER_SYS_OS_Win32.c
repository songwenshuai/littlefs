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
File    : SEGGER_SYS_OS_Win32.c
Purpose : API functions delivered by Windows.
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
#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include <conio.h>
#include <intrin.h>
#include <stdio.h>

/*********************************************************************
*
*       Package-scope data
*
**********************************************************************
*/

int SEGGER_SYS__StartedFromExplorer;

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

U32 SEGGER_SYS_GetProcessorSpeed(void) {
  return 0;
}

void SYS_OS_PrintPrefix(void (*pfOutput)(const char *)) {
  time_t Time;
  struct tm LocalTime;
  char acBuf[64];
  //
  time(&Time);
  localtime_s(&LocalTime, &Time);
  strftime(acBuf, sizeof(acBuf), "%H:%M:%S", &LocalTime);
  pfOutput(acBuf);
  pfOutput("  ");
}

/*********************************************************************
*
*       SEGGER_SYS_OS_Init()
*
*  Function description
*    Initialize system OS module.
*/
void SEGGER_SYS_OS_Init(void) {
  /* Empty */
}

/*********************************************************************
*
*       SEGGER_SYS_OS_PauseBeforeHalt()
*
*  Function description
*    Optionally pause in a windows environment to show an error
*    or success, whatever is appropriate.  In a command line
*    environment, never pause.
*/
void SEGGER_SYS_OS_PauseBeforeHalt(void) {
  if (SEGGER_SYS__StartedFromExplorer) {
    printf("\nPress Return to exit... ");
    (void)_getch();
  }
}

/*********************************************************************
*
*       SEGGER_SYS_OS_Halt()
*
*  Function description
*    Halt system and exit.
*/
void SEGGER_SYS_OS_Halt(int ExitCode) {
  if (ExitCode != 0) {
    SEGGER_SYS_OS_PauseBeforeHalt();
  }
  exit(ExitCode);
}

/*********************************************************************
*
*       SEGGER_SYS_OS_GetTimer()
*
*  Function description
*    Query high precision timer.
*/
U64 SEGGER_SYS_OS_GetTimer(void) {
  LARGE_INTEGER HPTimeStamp;
  //
  QueryPerformanceCounter(&HPTimeStamp);
  return HPTimeStamp.QuadPart;
}

/*********************************************************************
*
*       SEGGER_SYS_OS_GetFrequency()
*
*  Function description
*    Query frequency at which high precision timer increments.
*/
U64 SEGGER_SYS_OS_GetFrequency(void) {
  //
  static LARGE_INTEGER _Frequency;
  //
  if (_Frequency.QuadPart == 0) {
    QueryPerformanceFrequency(&_Frequency);
  }
  return _Frequency.QuadPart;
}

/*********************************************************************
*
*       SEGGER_SYS_OS_ConvertTicksToMicros()
*
*  Function description
*    Convert high precision timer ticks to microseconds.
*/
U64 SEGGER_SYS_OS_ConvertTicksToMicros(U64 Ticks) {
  return Ticks * 1000000ULL / SEGGER_SYS_OS_GetFrequency();
}

/*********************************************************************
*
*       SEGGER_SYS_OS_ConvertTicksToMicros()
*
*  Function description
*    Convert microseconds to high precision timer ticks.
*/
U64 SEGGER_SYS_OS_ConvertMicrosToTicks(U64 Microseconds) {
  return SEGGER_SYS_OS_GetFrequency() * Microseconds / 1000000UL;
}

/*********************************************************************
*
*       SEGGER_SYS_OS_GetCycleCounter()
*
*  Function description
*    Query the processor's cycle counter.
*
*  Additional information
*    64-bit cycle counter is returned in EDX:EAX.  We discard the
*    most significant 32 bits.
*/
U32 SEGGER_SYS_OS_GetCycleCounter(void) {
#if 0
  U32 CycleCounter = 0;  // Keeps PC-lint happy
  //
  __asm {
    rdtsc
    mov  [CycleCounter], eax
  }
  return CycleCounter;
#else
  return (U32)__rdtsc();
#endif
}

/*********************************************************************
*
*       SEGGER_SYS_OS_HasCycleCounter()
*
*  Function description
*    Query whether the processor cycle counter is supported and
*    can be read.
*/
int SEGGER_SYS_OS_HasCycleCounter(void) {
  return 1;
}

/*********************************************************************
*
*       SEGGER_SYS_OS_Delay()
*
*  Function description
*    Delay for at least a certain number of milliseconds.
*/
void SEGGER_SYS_OS_Delay(unsigned Milliseconds) {
  Sleep(Milliseconds);
}

/*********************************************************************
*
*       SYS_OS_GetTime()
*
*  Function description
*    Read millisecond timer.
*/
U32 SEGGER_SYS_OS_GetTime(void) {
  return GetTickCount();
}

#endif  // _WIN32

/****** End Of File *************************************************/
