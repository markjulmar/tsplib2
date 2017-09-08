/******************************************************************************/
//                                                                        
// PHONE.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the phone device code for the service provider.
// 
// Copyright (C) 1997-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// Original Copyright © 1994-2004 JulMar Entertainment Technology, Inc. All rights reserved.
//
// "This program is free software; you can redistribute it and/or modify it under the terms of 
// the GNU General Public License as published by the Free Software Foundation; version 2 of the License.
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General 
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program; if not, write 
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
// Or, contact: JulMar Technology, Inc. at: info@julmar.com." 
//                                                           
/******************************************************************************/

#include "stdafx.h"
#include "dssp.h"
#pragma warning(disable:4201)
#include <mmsystem.h>
#pragma warning(default:4201)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/*----------------------------------------------------------------------------
	GLOBALS
-----------------------------------------------------------------------------*/

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

// Button states
const DWORD g_ButtonStates[] = {
    PHONEBUTTONSTATE_UP,
    PHONEBUTTONSTATE_DOWN
};    

// This array translates the button functions into TAPI mode/functions with
// a text face for the phone button.
const struct
{
    DWORD dwMode;
    DWORD dwFunction;
    LPCTSTR pszText;
    
} g_ButtonFunctions[] = {
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_NONE,		_T("") },
    { PHONEBUTTONMODE_CALL,    PHONEBUTTONFUNCTION_CALLAPP,     _T("Call") },
    { PHONEBUTTONMODE_DISPLAY, PHONEBUTTONFUNCTION_CALLID,		_T("Display") },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_HOLD,		_T("Hold") },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_DROP,		_T("Release") },
    { PHONEBUTTONMODE_LOCAL,   PHONEBUTTONFUNCTION_VOLUMEUP,	_T("Vol>>") },
    { PHONEBUTTONMODE_LOCAL,   PHONEBUTTONFUNCTION_VOLUMEDOWN,	_T("<<Vol") },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_TRANSFER,	_T("Transfer") },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_FORWARD,		_T("Forward") },
    { PHONEBUTTONMODE_DUMMY,   PHONEBUTTONFUNCTION_MSGINDICATOR,_T("Msg Waiting") },
    { PHONEBUTTONMODE_KEYPAD,  PHONEBUTTONFUNCTION_NONE,		_T("") },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_CONFERENCE,	_T("Conference") }
};    

/*****************************************************************************
** Procedure:  CDSPhone::CDSPhone
**
** Arguments:  void
**
** Returns:    void
**
** Description:  Constructor for the phone object
**
*****************************************************************************/
CDSPhone::CDSPhone()
{
}// CDSPhone::CDSPhone

/*****************************************************************************
** Procedure:  CDSPhone::~CDSPhone
**
** Arguments:  void
**
** Returns:    void
**
** Description:  Destructor for the phone object
**
*****************************************************************************/
CDSPhone::~CDSPhone()
{
}// CDSPhone::~CDSPhone

/*****************************************************************************
** Procedure:  CDSPhone::Init
**
** Arguments:  'pDev'				-	Device object this phone belongs to
**             'dwPhoneDeviceID'	-	Unique phone identifier within the TSP
**             'dwPos'				-	Index position of phone within device array
**             'dwItemData'		-   Used when line was dynamically created (P&P).
**
** Returns:    void
**
** Description:  This function is called by the device owner to initialize
**               the phone object.
**
*****************************************************************************/
#pragma optimize("g", off) // VC5 SP2 bug with optimizer (see KB Q167347)
VOID CDSPhone::Init (CTSPIDevice* pDev, DWORD dwPhoneDeviceID, DWORD dwPos, DWORD /*dwItemData*/)
{
	// Let the base class initialize first.
	CTSPIPhoneConnection::Init (pDev, dwPhoneDeviceID, dwPos);

    // Grab the version information from the emulator.
    EMVERSIONINFO VerInfo;
    if (!GetDeviceInfo()->DRV_GetVersionInfo (&VerInfo))
		return;

    // Set the connection (phone) info
    SetConnInfo (VerInfo.szSwitchInfo);
    
    // Grab the settings from the emulator.
    EMSETTINGS Settings;
    if (!GetDeviceInfo()->DRV_GetSwitchSettings (&Settings))
		return;
    
    // Add the handset device to our hookswitch list.
    AddHookSwitchDevice(PHONEHOOKSWITCHDEV_HANDSET,				// Hookswitch device
						(PHONEHOOKSWITCHMODE_ONHOOK |			// Modes available to hookswitch
						 PHONEHOOKSWITCHMODE_MIC | 
						 PHONEHOOKSWITCHMODE_SPEAKER |
						 PHONEHOOKSWITCHMODE_MICSPEAKER | 
						 PHONEHOOKSWITCHMODE_UNKNOWN),
						 g_hsStates[Settings.wHandsetHookswitch],	// States supported by hookswitch
						 Settings.wVolHandset,						// Current Volume level of hookswitch (0-0xffff)
						 Settings.wGainHandset);					// Current Gain level of hookswitch (0-0xffff)
                
    // Setup the display buffer for the phone.  It uses a standard line feed so don't
    // change the default line break character.
    SetupDisplay(DISPLAY_COLS, DISPLAY_ROWS);

    // Add all the buttons to our phone.
    for (int i = 0; i < BUTTON_COUNT; i++)
    {   
		// If the button is one of our STANDARD buttons (0-9,A-D,#,*), then it is a keypad
		// button.
		if (i < TOTAL_STD_BUTTONS)
        {
            AddButton (PHONEBUTTONFUNCTION_NONE,		// Button function
					   PHONEBUTTONMODE_KEYPAD,			// Button mode
                       PHONELAMPMODE_DUMMY,				// Available Lamp states (Dummy = None)
					   PHONELAMPMODE_DUMMY,				// Current Lamp state (Dummy = None)
					   CString(g_szKeypad[i],1));		// Text name of button
        }                               

		// Otherwise the button is a "soft" button, the mode and function are determined
		// by how the user sets up the emulator.  The emulator reports the button functions
		// through the EMSETTINGS structure.
        else
        {   
			// Determine the available lamp states based on the reported mode of the
			// button from the emulator.
            DWORD dwAvailLampStates = 0;
            if (Settings.wButtonModes[i] == BUTTONFUNCTION_CALL)
                dwAvailLampStates = (PHONELAMPMODE_OFF | PHONELAMPMODE_STEADY | PHONELAMPMODE_WINK | PHONELAMPMODE_FLASH);
            else if (Settings.wLampStates[i] == LAMPSTATE_NONE)
                dwAvailLampStates = PHONELAMPMODE_DUMMY;
            else
                dwAvailLampStates = (PHONELAMPMODE_OFF | PHONELAMPMODE_STEADY | PHONELAMPMODE_WINK);

			// Add the button
            AddButton (g_ButtonFunctions[Settings.wButtonModes[i]].dwFunction,
                       g_ButtonFunctions[Settings.wButtonModes[i]].dwMode,  
                       dwAvailLampStates,
                       g_LampStates[Settings.wLampStates[i]],
                       g_ButtonFunctions[Settings.wButtonModes[i]].pszText);                            
        }                               
    }

    // Setup the initial state of the display.
    SetDisplay (Settings.szDisplay);

    // Setup the ringer modes
    LPPHONECAPS pPhoneCaps = GetPhoneCaps();
    LPPHONESTATUS pPhoneStatus = GetPhoneStatus();
    pPhoneCaps->dwNumRingModes = 4;
    pPhoneStatus->dwRingMode = (DWORD) Settings.wRingMode;

	// Add the WAV devices which will be our handset I/O
	if (waveInGetNumDevs() > 0)
		AddDeviceClass(_T("wave/in"), (DWORD)0);
	if (waveOutGetNumDevs() > 0)
		AddDeviceClass(_T("wave/out"), (DWORD)0);

}// CDSPhone::Init
#pragma optimize("g", on)

/*****************************************************************************
** Procedure:  CDSPhone::ReceieveData
**
** Arguments:  'dwData' - Data request from device/TSP
**             'lpBuff' - Buffer from device
**             'dwSize' - Size of buffer (device-specific)
**
** Returns:    void
**
** Description:  This function is called when either data is received from
**               the emulator, a timer has gone off (above), or we have a new
**               command to process and were idle.
**
*****************************************************************************/
BOOL CDSPhone::ReceiveData (DWORD dwData, const LPVOID lpBuff, DWORD /*dwSize*/)
{
    WORD wResponse = (WORD) dwData;
    BOOL fRequestHandled = FALSE;

	// Get the current request and process it.
    CTSPIRequest* pReq = GetCurrentRequest();
    if (pReq != NULL && pReq->GetState() == STATE_INITIAL)
	{
		// Make sure we don't process this request twice.
		pReq->SetState(STATE_WAITFORCOMPLETE);

		// Process the command in the request.
		switch (pReq->GetCommand())
		{
			// TAPI request to adjust the gain of the mic.
			case REQUEST_SETHOOKSWITCHGAIN:
			{
				TSPIHOOKSWITCHPARAM* pHSParam = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
				GetDeviceInfo()->DRV_SetGain(pHSParam->dwHookSwitchDevice, pHSParam->dwParam);
				fRequestHandled = TRUE;
			}
			break;

			// TAPI request to adjust the volume
			case REQUEST_SETHOOKSWITCHVOL:
			{
				TSPIHOOKSWITCHPARAM* pHSParam = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
				GetDeviceInfo()->DRV_SetVolume(pHSParam->dwHookSwitchDevice, pHSParam->dwParam);
				fRequestHandled = TRUE;
			}
			break;

			// TAPI request to take the hookswitch on/off hook.
			case REQUEST_SETHOOKSWITCH:
			{   
				TSPIHOOKSWITCHPARAM* pHSParam = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
				GetDeviceInfo()->DRV_SetHookswitch(pHSParam->dwHookSwitchDevice, pHSParam->dwParam);
				fRequestHandled = TRUE;
			}
			break;

			// TAPI request to change the ringer style.
			case REQUEST_SETRING:    
			{   
				TSPIRINGPATTERN* pRingPattern = (TSPIRINGPATTERN*) pReq->GetDataPtr();
				GetDeviceInfo()->DRV_SetRing(pRingPattern->dwRingMode);
				fRequestHandled = TRUE;
			}
			break;
		}

		// All of our phone requests are single action and can be completed immediately.
		if (fRequestHandled)
			CompleteRequest(pReq, 0);
	}

	// If the request was not handled, then pass this onto an "asynchronous" 
	// response manage for the device.  This is for things like offering calls, 
	// lamp state changes, etc.  This will also handle the conditions of dropping
	// consultation calls and re-connecting the original call.
    if (!fRequestHandled)
		ProcessAsynchDeviceResponse (wResponse, lpBuff);

	// Return whether we had a TAPI request which fielded this response.
	// Returning TRUE will stop processing on this response and it will not be
	// seen by any other line/phone in the device list.
    return fRequestHandled;

}// CDSPhone::ReceiveData

/*****************************************************************************
** Procedure:  CDSPhone::ProcessAsynchDeviceResponse
**
** Arguments:  'wResponse' - Command code received from device
**             'lpBuff'    - Data buffer (structure) specific to code
**
** Returns:    void
**
** Description:  This function processes any responses from the device which
**               are not matched to a pending line request.
**
*****************************************************************************/
void CDSPhone::ProcessAsynchDeviceResponse(WORD wResult, const LPVOID lpData)
{                           
    switch (wResult)
    {
        // A lamp has changed states.
        case EMRESULT_LAMPCHANGED:        
        {
            LPEMLAMPCHANGE lpChange = (LPEMLAMPCHANGE) lpData;
            SetLampState ((int)lpChange->wButtonLampID, g_LampStates[lpChange->wLampState]);
        }                                        
        break;
        
        // A hookswitch device has changed states.
        case EMRESULT_HSCHANGED:
        {
            LPEMHOOKSWITCHCHANGE lpChange = (LPEMHOOKSWITCHCHANGE) lpData;
            ASSERT (lpChange->wHookswitchID == HSDEVICE_HANDSET);
            SetHookSwitch (PHONEHOOKSWITCHDEV_HANDSET, g_hsStates[lpChange->wHookswitchState]);
        }
        break;

        // A button has changed
        case EMRESULT_BUTTONCHANGED:
        {
            LPEMBUTTONCHANGE lpChange = (LPEMBUTTONCHANGE) lpData;
            SetButtonState (lpChange->wButtonLampID, g_ButtonStates[lpChange->wButtonState]);
        }                    
        break;
        
        // Ringer mode changed
        case EMRESULT_RINGCHANGE:
            SetRingMode (*((LPDWORD)lpData));
            break;
        
        // Volume/Gain of the handset changed
        case EMRESULT_LEVELCHANGED:
        {
            LPEMLEVELCHANGE lpChange = (LPEMLEVELCHANGE) lpData;
            if (lpChange->wLevelType == LEVELTYPE_MIC)
                SetGain (PHONEHOOKSWITCHDEV_HANDSET, lpChange->wLevel);
            else if (lpChange->wLevelType == LEVELTYPE_SPEAKER)
                SetVolume (PHONEHOOKSWITCHDEV_HANDSET, lpChange->wLevel);
        }
        break;                        
        
        // The display has changed.
        case EMRESULT_DISPLAYCHANGED:
        {
            LPEMDISPLAY lpChange = (LPEMDISPLAY) lpData;
            SetDisplay (lpChange->szDisplay);
        }        
        break;
    }

}// CDSPhone::ProcessAsynchDeviceResponse
