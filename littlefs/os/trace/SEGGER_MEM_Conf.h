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
File        : SEGGER_MEM_Conf.h
Purpose     : Configuration file for SEGGER memory allocators.
*/

#ifndef SEGGER_MEM_CONF_H
#define SEGGER_MEM_CONF_H

//
// Define SEGGER_MEM_DEBUG: Debug level for SSL product
//                  0: No checks                      (Smallest and fastest code)
//                  1: Warnings & Panic checks
//                  2: Warnings, logs, & panic checks (Bigger code)
//
#ifndef   DEBUG
  #define DEBUG            0
#endif

#if DEBUG
  #ifndef   SEGGER_MEM_DEBUG
    #define SEGGER_MEM_DEBUG      2      // Default for debug builds
  #endif
  #ifndef   SEGGER_MEM_STATS
    #define SEGGER_MEM_STATS      1      // Default for debug builds, include statistics
  #endif
#else
  #ifndef   SEGGER_MEM_DEBUG
    #define SEGGER_MEM_DEBUG      0      // Default for release builds
  #endif
  #ifndef   SEGGER_MEM_STATS
    #define SEGGER_MEM_STATS      0      // Default for release builds, don't include statistics
  #endif
#endif

#endif

/****** End Of File *************************************************/
