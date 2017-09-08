//*****************************************************************************
//
// STDAFX.H
//
// Standard header for the DSSP Emulator
//
// Written 04/19/96
//
//*****************************************************************************

#include <afxwin.h>
#include <ping.h>
#include <ctl3d.h>
#include <ctype.h>
#include "..\sp\emintf.h"

#define TAPI_CURRENT_VERSION 0x00014000
                                
// Define our timer events.
#define TIMER_RING        100     // This is used for RING events
#define TIMER_PAINT_LAMPS 101     // This is used to paint lamps
                                
// Button define info.
#define FIRST_FUNCTION_INDEX    12
#define LAST_FUNCTION_INDEX     27
#define BTN_VOLUP               28
#define BTN_VOLDOWN             29
#define BTN_HOLD                30
#define BTN_DROP                31
#define BTN_MSGWAITING          32
