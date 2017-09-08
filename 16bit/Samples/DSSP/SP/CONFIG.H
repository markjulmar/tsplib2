/*****************************************************************************/
//
// CONFIG.H - Digital Switch Service Provider Sample
//                                                                        
// This file contains the arrays used to translate the emulator
// information into TAPI specific information.
// 
// Copyright (C) 1994, 1995, 1996 Mark C. Smith
// All rights reserved                                                    
//                                                                        
// This service provider communicates with an emulator program which
// simulates a digital environment for this provider to work in.
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
// Change History
// ----------------------------------------------------------------
// 10/02/94     Mark Smith (MCS)
//      Initial revision.
// 04/18/96     Mark Smith (MCS)
//      Converted to v1.21 and integrated to emulator2 code.
//
/*****************************************************************************/

// This array translates the button functions into TAPI mode/functions with
// a text face for the phone button.
typedef struct
{
    DWORD dwMode;
    DWORD dwFunction;
    const char *pszText;
    
} xlBUTTONFUNCTIONS;

const xlBUTTONFUNCTIONS g_ButtonFunctions[] = {
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_NONE,       "" },
    { PHONEBUTTONMODE_CALL,    PHONEBUTTONFUNCTION_NONE,       "Call" },
    { PHONEBUTTONMODE_DISPLAY, PHONEBUTTONFUNCTION_NONE,       "Display" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_HOLD,       "Hold" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_RECALL,     "Release" },
    { PHONEBUTTONMODE_LOCAL,   PHONEBUTTONFUNCTION_VOLUMEUP,   "Vol>>" },
    { PHONEBUTTONMODE_LOCAL,   PHONEBUTTONFUNCTION_VOLUMEDOWN, "<<Vol" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_TRANSFER,   "Transfer" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_FORWARD,    "Forward" },
    { PHONEBUTTONMODE_DUMMY,   PHONEBUTTONFUNCTION_NONE,       "Msg Waiting" },
    { PHONEBUTTONMODE_KEYPAD,  PHONEBUTTONFUNCTION_NONE,       "" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_CONFERENCE, "Conference" }
};    

// Translate hookswitch states from our switch to TAPI
const DWORD g_hsStates[] = { 
    PHONEHOOKSWITCHMODE_ONHOOK,
    PHONEHOOKSWITCHMODE_MICSPEAKER,
    PHONEHOOKSWITCHMODE_MIC,
    PHONEHOOKSWITCHMODE_SPEAKER
};    

// Keypad digits
const char g_szKeypad[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '*', '#' };

// Switch lamp states to TAPI translation
const DWORD g_LampStates[] = {
    PHONELAMPMODE_DUMMY,
    PHONELAMPMODE_OFF,
    PHONELAMPMODE_STEADY,
    PHONELAMPMODE_WINK,
    PHONELAMPMODE_FLASH
};    

// Dialtone states
const DWORD g_DialToneStates[] = {
    LINEDIALTONEMODE_INTERNAL,
    LINEDIALTONEMODE_EXTERNAL
};  

// Button states
const DWORD g_ButtonStates[] = {
    PHONEBUTTONSTATE_UP,
    PHONEBUTTONSTATE_DOWN
};    
  
// Busy states
const DWORD g_BusyStates[] = {
    LINEBUSYMODE_STATION,
    LINEBUSYMODE_TRUNK
};  

// Call states
const DWORD g_CallStates[] = {
    LINECALLSTATE_UNKNOWN,
    LINECALLSTATE_CONNECTED,
    LINECALLSTATE_BUSY,
    LINECALLSTATE_DISCONNECTED,
    LINECALLSTATE_DIALTONE,
    LINECALLSTATE_PROCEEDING,
    LINECALLSTATE_IDLE,
    LINECALLSTATE_OFFERING,
    LINECALLSTATE_ONHOLD,
    LINECALLSTATE_CONFERENCED
};    
