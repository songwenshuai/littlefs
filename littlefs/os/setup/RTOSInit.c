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
Purpose : Initializes and handles the hardware for embOS
*/

#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include "RTOS.h"
#include "UDPCOM.h"
#if (OS_PROFILE != 0)
#include "SEGGER_SYSVIEW_Win32.h"
#endif

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
//
// If between two embOS system tick ISRs more than TIME_DIFF_MAX milliseconds
// have elapsed, then tDiff will be set to TIME_DIFF_MAX. This might occur when
// one debugs and single steps through the application or if the CPU load is
// very high. This  value can be changed as desired. Please consider that if
// TIME_DIFF_MAX cuts off system ticks, that the embOS time doesn't resemble
// the real time anymore. The limit can be disabled with a value of 0.
//
#ifndef   TIME_DIFF_MAX
  #define TIME_DIFF_MAX  (1)
#endif

/*********************************************************************
*
*       System tick settings
*/
#define OS_TIMER_FREQ  ((OS_U32)_TimerFrequency.QuadPart)
#define OS_TICK_FREQ   (1000u)
#define OS_INT_FREQ    (OS_TICK_FREQ)

/*********************************************************************
*
*       embOSView settings
*/
#ifndef   OS_VIEW_IFSELECT
  #define OS_VIEW_IFSELECT  OS_VIEW_IF_ETHERNET
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static LARGE_INTEGER _TimerFrequency = {0};
static LARGE_INTEGER _CycleStamp     = {0};

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _ISRTickThread()
*/
static void _ISRTickThread(void) {
  int tDiff;
  int t;
  int tLast;

#if (OS_PROFILE != 0)
  SEGGER_SYSVIEW_X_SetISRName("Tick ISR");
#endif
  //
  // Switch to higher timer resolution
  //
  timeBeginPeriod(1);
  tLast = timeGetTime();
  while (1) {
    t     = timeGetTime();
    tDiff = t - tLast;
    tLast = t;
#if (TIME_DIFF_MAX != 0)
    if (tDiff > TIME_DIFF_MAX) {
      tDiff = TIME_DIFF_MAX;
    }
#endif
    OS_INT_Enter();
    //
    // Call OS_TICK_Handle() for each passed millisecond.
    //
    while (tDiff-- > 0) {
      OS_TICK_Handle();
    }
    //
    // Save the current cycle counter value. This value is used in _OS_GetHWTimerCycles()
    // in order to return the elapsed cycles since the last recorded system tick.
    //
    QueryPerformanceCounter(&_CycleStamp);
    OS_INT_Leave();
    //
    // SleepEx()'s second parameter *MUST* be TRUE when used with QueueUserAPC. Otherwise 'Nonpaged Pool'
    // (cf. ProcessExplorer) is congested *COMPLETELY* since we are NOT in an alertable state and thus the
    // queue will NEVER be flushed.
    //
    SleepEx(INFINITE, TRUE);
  }
}

/*********************************************************************
*
*       _voidAPC()
*
*  Function description
*    Dummy APC function. Is required because we (ab)use the
*    WIN32 QueueUserAPC() API function to wake up a thread
*/
static void APIENTRY _voidAPC(ULONG_PTR Dummy) {
  OS_USEPARA(Dummy);
}

/*********************************************************************
*
*       _CbSignalTickProc()
*
*  Function description
*    Timer callback function which periodically queues an APC in order
*    to resume the ISR tick thread.
*/
static void CALLBACK _CbSignalTickProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
  static int IsThreadNameSet = 0;

  OS_USEPARA(uID);
  OS_USEPARA(uMsg);
  OS_USEPARA(dw1);
  OS_USEPARA(dw2);
  if (IsThreadNameSet == 0) {
    IsThreadNameSet = 1;
    OS_SIM_SetThreadName(-1, "Tick ISR (Helper Thread)");
  }
  QueueUserAPC(_voidAPC, (void*)dwUser, 0);
}

/*********************************************************************
*
*       _OS_GetHWTimerCycles()
*
*  Function description
*    Returns the current hardware timer count value.
*
*  Return value
*    Current timer count value.
*/
static unsigned int _OS_GetHWTimerCycles(void) {
  unsigned int  Cycles;
  LARGE_INTEGER Counter;

  QueryPerformanceCounter(&Counter);
  Cycles = (unsigned int)(Counter.QuadPart - _CycleStamp.QuadPart);  // Calculate the elapsed cycles since the last system tick.
  return Cycles;
}

/*********************************************************************
*
*       _OS_GetHWTimer_IntPending()
*
*  Function description
*    Returns if the hardware timer interrupt pending flag is set.
*
*  Return value
*    == 0: Interrupt pending flag not set.
*    != 0: Interrupt pending flag set.
*/
static unsigned int _OS_GetHWTimer_IntPending(void) {
  return 0;
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_InitHW()
*
*  Function description
*    Initialize the hardware required for embOS to run.
*/
void OS_InitHW(void) {
  HANDLE hISRThread;

  OS_INT_IncDI();
  //
  // Get time stamp counter (TSC) frequency. Usually it's the CPU frequency divided by 1024.
  //
  QueryPerformanceFrequency(&_TimerFrequency);
  //
  // Start tick ISR
  //
  QueryPerformanceCounter(&_CycleStamp);  // Initial value for system tick zero.
  hISRThread = (HANDLE)OS_SIM_CreateISRThreadEx(_ISRTickThread, "Tick ISR");
  timeSetEvent(1, 0, _CbSignalTickProc, (DWORD_PTR)hISRThread, (TIME_PERIODIC | TIME_CALLBACK_FUNCTION));
  //
  // Inform embOS about the timer settings
  //
  {
    OS_SYSTIMER_CONFIG SysTimerConfig = {OS_TIMER_FREQ, OS_INT_FREQ, OS_TIMER_UPCOUNTING, _OS_GetHWTimerCycles, _OS_GetHWTimer_IntPending};
    OS_TIME_ConfigSysTimer(&SysTimerConfig);
  }
  //
  // Initialize communication for embOSView
  //
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_ETHERNET)
  UDP_Process_Init();
#endif
  //
  // Configure and initialize SEGGER SystemView
  //
#if (OS_PROFILE != 0)
  SEGGER_SYSVIEW_Conf();
#endif
  OS_INT_DecRI();
}

/*********************************************************************
*
*       Optional communication with embOSView
*
**********************************************************************
*/

/*********************************************************************
*
*       OS_COM_Send1()
*
*  Function description
*    Sends one character.
*/
void OS_COM_Send1(OS_U8 c) {
#if (OS_VIEW_IFSELECT == OS_VIEW_IF_ETHERNET)
  UDP_Process_Send1(c);
#elif (OS_VIEW_IFSELECT == OS_VIEW_DISABLED)
  OS_USEPARA(c);           // Avoid compiler warning
  OS_COM_ClearTxActive();  // Let embOS know that Tx is not busy
#endif
}

/*************************** End of file ****************************/
