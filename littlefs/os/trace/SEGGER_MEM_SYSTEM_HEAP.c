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
File    : SEGGER_MEM_SYSTEM_HEAP.c
Purpose : SEGGER C library memory module implementation
Revision: $Rev: 15029 $
*/

#include "SEGGER_MEM.h"
#include <stdlib.h>

/*********************************************************************
*
*       Function prototypes
*
**********************************************************************
*/
static void*    _HAlloc     (void* pContext, unsigned NumBytes);
static void     _HFree      (void* pContext, void* pMem);
static void*    _HRealloc   (void* pContext, void* pMem, unsigned NumBytes);

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static const SEGGER_MEM_API _HEAPAPI = {
  _HAlloc,
  _HFree,
  _HRealloc,
  NULL
};

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

static void* _HAlloc(void* pContext, unsigned NumBytes) {
  SEGGER_MEM_USE_PARA(pContext);
  return malloc(NumBytes);
}

static void _HFree(void* pContext, void* pMem) {
  SEGGER_MEM_USE_PARA(pContext);
  free(pMem);
}

static void* _HRealloc(void* pContext, void* pMem, unsigned NumBytes) {
  SEGGER_MEM_USE_PARA(pContext);
  return realloc(pMem, NumBytes);
}

void SEGGER_MEM_SYSTEM_HEAP_Init(SEGGER_MEM_CONTEXT* pMem) {
  pMem->pAPI         = &_HEAPAPI;
  pMem->pContext     = NULL;
  pMem->pLockAPI     = NULL;
  pMem->pLockContext = NULL;
}

/****** End Of File* ************************************************/
