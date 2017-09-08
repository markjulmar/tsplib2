/******************************************************************************/
//                                                                        
// ATSPINT.H - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our interface definitions shared between the
// companion application and the service provider.
// 
// Copyright (C) 1996 Mark C. Smith
// All rights reserved                                                    
//                                                                        
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
// Change History
// -------------------------------------------------------------------
// 04/10/96 Mark Smith (MCS)
//      Initial revision.
// 
/******************************************************************************/

#pragma pack(1)

// These are modem response commands (results)
#define MODEM_OK                 0     // Modem command worked OK
#define MODEM_ERR                1     // Modem command failed
#define MODEM_BUSY               2     // Modem returned BUSY
#define MODEM_RING               3     // Modem line is ringing
#define MODEM_AT                 4     // Modem in command state
#define MODEM_INTERVAL_TIMER     5     // Interval timer from APP
#define MODEM_IGNORE             6     // Ignore response
#define MODEM_CONNECT            7     // CONNECT xxx
#define MODEM_CARRIER            8     // CARRIER xxx
#define MODEM_UNKNOWN            255   // Unknown command!

// Internal timer states
#define MODEM_TIMEOUT         	10     // Timeout value in seconds.

// Internal INIT states
#define INITMODEM_MSEC         0		// Timeout values (Sreg) in MSEC
#define INITMODEM_SEC          1		// Timeout values (Sreg) in SEC
#define INITMODEM_GENERIC      2		// No timeouts used

// New requests passed to the companion application
#define COMMAND_CLEARDTR         (COMMAND_END+1)
#define COMMAND_SETDTR           (COMMAND_END+2)
#define COMMAND_GETCOMMHANDLE    (COMMAND_END+3)
#define COMMAND_OPENSERIAL       (COMMAND_END+4)             

// This is the structure passed to the companion to open our ports.
typedef struct
{
	int iPort;
	DWORD dwBaud;
	DWORD dwConnId;
	
} SERIALINFO, FAR* LPSERIALINFO;

#pragma pack()
