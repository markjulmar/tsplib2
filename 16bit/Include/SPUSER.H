/******************************************************************************/
//                                                                        
// SPUSER.H - TAPI Service Provider C++ Library header                     
//        
// This header defines the communication blocks passed between
// the thread executable and service provider under 16-bit TAPI.
//                                                                
// Copyright (C) 1996 Mark C. Smith
// All rights reserved                                                    
//                                                                        
// The SPLIB classes provide a basis for developing MS-TAPI complient     
// Service Providers.  They provide basic handling for all of the TSPI    
// APIs and a C-based handler which routes all requests through a set of C++     
// classes.                                                                 
//                                                                        
/******************************************************************************/

#ifndef _SPUSER_INC_
#define _SPUSER_INC_

#pragma pack(1)

/******************************************************************************/
//
// COPYDATA
//
// This message is used by Windows 95, Windows NT, and Win32s to perform
// automatic "thunking" between 16 and 32-bit window procedures for a message.
// It is used by the library whenever a pointer to a buffer needs to be passed
// to the companion application.  This allows the companion application to be
// a 32-bit application.
//
// The definitions and structures were taken from WINUSER.H (Win32 SDK).
//
/******************************************************************************/

#ifndef WM_COPYDATA
#define WM_COPYDATA  0x004A
typedef struct tagCOPYDATASTRUCT {
    DWORD dwData;
    DWORD cbData;
    LPVOID lpData;
} COPYDATASTRUCT, FAR *LPCOPYDATASTRUCT;
#endif

/******************************************************************************/
// User Messages
// 
// Define the user messages sent to our worker app which communicates
// with the actual phone device.
//
/******************************************************************************/
#define UM_TSPI_COMMAND (WM_USER + 600)

// Define all the sub-commands which can be sent via the above message
// These values are passed in the 'wParam' value of the above message.

#define COMMAND_OPENCONN    0x01              // Open a connection
#define COMMAND_CLOSECONN   0x02              // Close a connection
#define COMMAND_SENDDATA    0x03              // Send a buffer to a device
#define COMMAND_WAITINGREQ  0x04              // Request thread context
#define COMMAND_END         0x50              // Use any value after this.

// lParam for COMMAND_SENDDATA (passed in COPYDATA buffer)
typedef struct
{
    DWORD   dwConnId;         // Connection to send the string to
    LPVOID  lpBuff;           // Buffer (points to end of structure)
    DWORD   dwSize;           // Size of above buffer
} TSPSENDDATA, FAR* LPTSPSENDDATA;

/******************************************************************************/
// Define all the results which are passed back to us via the callback.
/******************************************************************************/

#define RESULT_INIT             0x01            // Init request (dwResult=hWnd).
#define RESULT_RCVDATA          0x02            // Received data from device
#define RESULT_INTERVALTIMER    0x03            // Periodic timer
#define RESULT_CONTEXTSWITCH    0x04            // Requested thread context
#define RESULT_END              0x50            // Use any value after this.

/******************************************************************************/
// Declare the callback function for the worker application
/******************************************************************************/
extern "C" VOID WINAPI _export DeviceNotify(WORD wCommand, DWORD dwConnId, DWORD dwResult, LPVOID lpvData, DWORD dwSize);

#pragma pack()
#endif // _SPUSER_INC_
