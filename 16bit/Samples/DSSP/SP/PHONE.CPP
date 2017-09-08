/*****************************************************************************/
//
// PHONE.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the phone functions for the provider.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
/*****************************************************************************/

#include "stdafx.h"
#include "dssp.h"

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
    const char* pszText;
    
} g_ButtonFunctions[] = {
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_NONE,      	"" },
    { PHONEBUTTONMODE_CALL,    PHONEBUTTONFUNCTION_NONE,      	"Call" },
    { PHONEBUTTONMODE_DISPLAY, PHONEBUTTONFUNCTION_NONE,      	"Display" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_HOLD,      	"Hold" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_RECALL,    	"Release" },
    { PHONEBUTTONMODE_LOCAL,   PHONEBUTTONFUNCTION_VOLUMEUP,  	"Vol>>" },
    { PHONEBUTTONMODE_LOCAL,   PHONEBUTTONFUNCTION_VOLUMEDOWN,	"<<Vol" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_TRANSFER,  	"Transfer" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_FORWARD,   	"Forward" },
    { PHONEBUTTONMODE_DUMMY,   PHONEBUTTONFUNCTION_NONE,      	"Msg Waiting" },
    { PHONEBUTTONMODE_KEYPAD,  PHONEBUTTONFUNCTION_NONE,      	"" },
    { PHONEBUTTONMODE_FEATURE, PHONEBUTTONFUNCTION_CONFERENCE, 	"Conference" }
};    

////////////////////////////////////////////////////////////////////////////
// CDSPhone::InitializePhoneDevice
//
// Initialize the phone device for this service provider.
// This is called from the CDSProvider::Perform16bitInit function in the
// context of our companion application.
//
void CDSPhone::InitializePhoneDevice (const char * pszSwitchInfo)
{             
    // Set the connection (phone) info
    SetConnInfo (pszSwitchInfo);
    
    // Grab the settings from the emulator.
    EMSETTINGS Settings;
    GetDeviceInfo()->DRV_GetSwitchSettings (&Settings);
    
    // Add the handset device to our hookswitch list.
    AddHookSwitchDevice (PHONEHOOKSWITCHDEV_HANDSET, 
                (PHONEHOOKSWITCHMODE_ONHOOK | PHONEHOOKSWITCHMODE_MIC | PHONEHOOKSWITCHMODE_SPEAKER |
                PHONEHOOKSWITCHMODE_MICSPEAKER | PHONEHOOKSWITCHMODE_UNKNOWN),
                g_hsStates[Settings.wHandsetHookswitch], 
                (DWORD)Settings.wVolHandset,
                (DWORD)Settings.wGainHandset);
                
    // Setup the display buffer for the phone.  It uses a standard line feed so don't
    // change the default line break character.
    SetupDisplay(DISPLAY_COLS, DISPLAY_ROWS);

    // Add all the buttons to our phone.
    for (int i = 0; i < TOTAL_STD_BUTTONS; i++)
    	AddButton (PHONEBUTTONFUNCTION_NONE, PHONEBUTTONMODE_KEYPAD,
                           PHONELAMPMODE_DUMMY, PHONELAMPMODE_DUMMY, CString(g_szKeypad[i],1));
	
	// Other buttons not in the normal digitpad range (0-9,#,*)    
    for (;i < BUTTON_COUNT; i++)
    {   
        DWORD dwAvailLampStates = 0;
        if (Settings.wButtonModes[i] == BUTTONFUNCTION_CALL)
            dwAvailLampStates = (PHONELAMPMODE_OFF | PHONELAMPMODE_STEADY |
                    PHONELAMPMODE_WINK | PHONELAMPMODE_FLASH);
        else if (Settings.wLampStates[i] == LAMPSTATE_NONE)
            dwAvailLampStates = PHONELAMPMODE_DUMMY;
        else
            dwAvailLampStates = (PHONELAMPMODE_OFF | PHONELAMPMODE_STEADY |
                    PHONELAMPMODE_WINK);
        AddButton (g_ButtonFunctions[Settings.wButtonModes[i]].dwFunction,
                             g_ButtonFunctions[Settings.wButtonModes[i]].dwMode,  
                             dwAvailLampStates,
                             g_LampStates[Settings.wLampStates[i]],
                             g_ButtonFunctions[Settings.wButtonModes[i]].pszText);                            
    }

    // Setup the initial state of the display.
    SetDisplay (Settings.szDisplay);

    // Setup the ringer modes
    GetPhoneCaps()->dwNumRingModes = 4;
    GetPhoneStatus()->dwRingMode = (DWORD) Settings.wRingMode;
        
}// CDSProvider::InitializePhoneDevice

////////////////////////////////////////////////////////////////////////////
// CDSPhone::ReceiveData
//
// This method is called when a string is returned from the Telebridge
// device.  It evaluates the string and determines how to complete the
// current outstanding request.
//
BOOL CDSPhone::ReceiveData(DWORD dwData, const LPVOID lpBuff, DWORD /*dwSize*/)
{
    WORD wResponse = (WORD) dwData;

	// Get the current request and process it.
    CTSPIRequest* pReq = GetCurrentRequest();
    if (pReq != NULL)
    {   
	    switch (pReq->GetCommand())
	    {
	        case REQUEST_SETHOOKSWITCHGAIN:
	            {
	                TSPIHOOKSWITCHPARAM* pHSParam = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
	                GetDeviceInfo()->DRV_SetGain(pHSParam->dwHookSwitchDevice, pHSParam->dwParam);
	            }
	            break;
	        case REQUEST_SETHOOKSWITCHVOL:
	            {
	                TSPIHOOKSWITCHPARAM* pHSParam = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
	                GetDeviceInfo()->DRV_SetVolume(pHSParam->dwHookSwitchDevice, pHSParam->dwParam);
	            }
	            break;
	        case REQUEST_SETHOOKSWITCH:
	            {   
	                TSPIHOOKSWITCHPARAM* pHSParam = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
	                GetDeviceInfo()->DRV_SetHookswitch(pHSParam->dwHookSwitchDevice, pHSParam->dwParam);
	            }
	            break;
	        case REQUEST_SETRING:    
	            {   
	                TSPIRINGPATTERN* pRingPattern = (TSPIRINGPATTERN*) pReq->GetDataPtr();
	                GetDeviceInfo()->DRV_SetRing(pRingPattern->dwRingMode);
	            }
	            break;
	        default:
	            CompleteRequest(pReq, PHONEERR_OPERATIONFAILED);
	            break;
	    }
	    CompleteRequest(pReq, 0);
    }            
    
    // Pass this onto an "asynchronous" response manage for the device.  
    // This is for things like offering calls, lamp state changes, etc.  
    ProcessAsynchDeviceResponse (wResponse, lpBuff);
    return TRUE;

}// CDSPhone::ProcessData

//////////////////////////////////////////////////////////////////////////
// CDSPhone::ProcessAsynchDeviceResponse
//
// Process an asynchronous response from the device.
//
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
                SetButtonState ((int)lpChange->wButtonLampID, g_ButtonStates[lpChange->wButtonState]);
            }                    
            break;
            
        // Ringer mode changed
        case EMRESULT_RINGCHANGE:
            SetRingMode ((DWORD)lpData);
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
            
        default:
            break;
    }

}// CDSPhone::ProcessAsynchDeviceResponse
