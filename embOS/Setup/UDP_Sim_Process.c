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

----------------------------------------------------------------------
File    : UDP_Sim_Process.c
Purpose : UPD communication for embOSView and embOS Simulation
--------  END-OF-HEADER  ---------------------------------------------
*/

#include <Windows.h>
#include "RTOS.h"
#include "UDPCOM.h"
#include <stdio.h>
#if (OS_PROFILE != 0)
#include "SEGGER_SYSVIEW_Win32.h"
#endif

/*********************************************************************
*
*       Configuration
*
**********************************************************************
*/
#ifndef   EMBOSVIEW_UDP_PORT            // This is the default UDP port for embOSView communication.
  #define EMBOSVIEW_UDP_PORT   50021    // If you change it please modify it also in embOSView.
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define PROTOCOL_SIZE       5           // Size of embOSView protocol bytes (SD0, SD1, Size, Checksum, ED)
#define MAX_UDP_PORT_TRIES  10          // Sets how many udp ports are tried to use up from EMBOSVIEW_UDP_PORT

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static SOCKADDR_IN _TargetAddr;
static SOCKET      _sock;
static HANDLE      _hThread;
static char        _aRxData[1024];

/*********************************************************************
*
*       Local functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _OnRx()
*
*  Function description
*    UDP RX Thread
*/
static void _OnRx(void) {
  int i;
  int NumRxBytes;
  int Len;

  OS_SIM_SetThreadName(-1, "embOSView Communication");
#if (OS_PROFILE != 0)
  SEGGER_SYSVIEW_X_SetISRName("embOSView Communication");
#endif
  while (1) {
    memset(_aRxData, 0, sizeof(_aRxData));
    Len = sizeof(_TargetAddr);
    //
    // Receive packed, get IP address and UDP port from incoming packet
    //
    NumRxBytes = recvfrom(_sock, _aRxData, sizeof(_aRxData), 0, (SOCKADDR*)&_TargetAddr, &Len);
    //
    // Handle received embOSView protocol
    //
    for (i = 0u; i < NumRxBytes; i++) {
      OS_INT_Enter();
      OS_COM_OnRx(_aRxData[i]);
      OS_INT_Leave();
    }
  }
}

/*********************************************************************
*
*       Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*       UDP_Process_Send1()
*
*  Function description
*    Sends the UDP protocol
*/
void UDP_Process_Send1(char c) {
  static char OutPacket[0x500];
  OS_INT      SD1;
  OS_INT      NumBytes;
  OS_INT      i;
  OS_INT      Offset;
  OS_INT      NumTxBytes;

  Offset = 0;
  //
  // Send UDP packet only if we already got a target address
  //
  if (_TargetAddr.sin_addr.s_addr != 0u) {
    SD1      = OS_COM_GetNextChar();  // Get SD1
    NumBytes = OS_COM_GetNextChar();  // Get NumBytes
    if (NumBytes >= 0xF0) {
      NumBytes = ((NumBytes & 0x0F) << 8u) | (OS_COM_GetNextChar() & 0xFF);
      Offset   = 1;
    }
    //
    // Alloc packet, we need a packet for the amount of data bytes plus protocol bytes
    //
    memset(OutPacket, 0, sizeof(OutPacket));
    //
    // Copy protocol bytes and data in out UDP packet
    //
    OutPacket[0] = c;
    OutPacket[1] = SD1;
    if (NumBytes >= 0xF0) {
      OutPacket[2] = (NumBytes >> 8) | 0xF0;
      OutPacket[3] = NumBytes & 0xFF;
    } else {
      OutPacket[2] = NumBytes;
    }

    for (i = 0; i < NumBytes + 2 + Offset; i++) {
      OutPacket[i + 3 + Offset] = OS_COM_GetNextChar();
    }

    NumTxBytes = sendto(_sock, OutPacket, NumBytes + PROTOCOL_SIZE + Offset, 0, (SOCKADDR*)&_TargetAddr, sizeof(_TargetAddr));
    if (NumTxBytes == -1) {
      OutputDebugString("sendto error\n");
    }
    do {
      i = OS_COM_GetNextChar(); // Call the state machine until there are no more characters to send
    } while (i >= 0);
  } else {
    //
    // No target address so far, discard response
    //
    OS_COM_ClearTxActive();
  }
}

/*********************************************************************
*
*       UDP_Process_Init()
*
*  Function description
*    Initializes the UDP communication for embOSView
*/
void UDP_Process_Init(void) {
  WORD        wVersionRequested;
  WSADATA     wsaData;
  SOCKADDR_IN LAddr;
  int         Result;
  int         LastError;
  int         NumTries;
  char        sText[100];

  _TargetAddr.sin_addr.s_addr = 0;
  NumTries = 0;
  //
  // Initialize winsock. Required to use TCP/IP
  //
  wVersionRequested = MAKEWORD(2, 0);
  if (WSAStartup(wVersionRequested, &wsaData) != 0) {
    OutputDebugString("Could not init WinSock.\n");
  }
  //
  // Create datagram socket and enable sending of broadcasts over this socket
  //
  _sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (_sock == INVALID_SOCKET) {
    OutputDebugString ("Could not create socket.\n");
  }
  memset(&LAddr, 0, sizeof(LAddr));
  LAddr.sin_family      = AF_INET;
  LAddr.sin_port        = htons(EMBOSVIEW_UDP_PORT);
  LAddr.sin_addr.s_addr = INADDR_ANY;

  //
  // If the EMBOSVIEW_UDP_PORT port number is in use we also try the next MAX_UDP_PORT_TRIES udp port numbers
  //
  do {
    Result = bind(_sock, (struct sockaddr *)&LAddr, sizeof(LAddr));
    LastError = WSAGetLastError();  // We need this erro code in order to know if the error was caused by an already used port number
    NumTries++;
    LAddr.sin_port = htons(EMBOSVIEW_UDP_PORT + NumTries);  // Check if next port number is free
  } while ((Result == SOCKET_ERROR) && (LastError == WSAEADDRINUSE) && (NumTries < MAX_UDP_PORT_TRIES));

  if (Result != SOCKET_ERROR) {
    //
    // Create thread for incoming UDP packets
    //
    _hThread = (HANDLE)OS_SIM_CreateISRThreadEx(_OnRx, "embOSView Communication");

    sprintf(sText, "Please configure embOSView to your IP addres and port number %u.\n", EMBOSVIEW_UDP_PORT + NumTries - 1);
    OutputDebugString(sText);
  } else {
    OutputDebugString("Could not bind socket.\n");
  }
}

/****** End Of File *************************************************/
