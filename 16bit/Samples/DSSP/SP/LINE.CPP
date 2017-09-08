/*****************************************************************************/
//
// LINE.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the line functions for the provider.
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
	GLOBAL DATA
-----------------------------------------------------------------------------*/

// Array to translate Call states
DWORD g_CallStates[] = {
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

// Available completion messages
const char *g_ComplMsgs[] = {
    { "Completion Message #1" },
    { "Completion Message #2" },
    { "Completion Message #3" },
    { "Completion Message #4" },
    NULL 
};  

////////////////////////////////////////////////////////////////////////////
// CDSLine::InitializeLineDevice
//
// Initialize the line device for this service provider.  This is called
// from CDSProvider::Perform16bitInit in the context of our 16-bit companion
// application.
//
void CDSLine::InitializeLineDevice (const char *pszSwitchInfo)
{                                  
    // Set the connection (line) info
    // Add a line name so Windows 95 distinguishes us from other TAPI providers.
    SetConnInfo (pszSwitchInfo);
    SetName("DSSP Line #1");

    // Add our terminal devices.  For our demo provider, we support a handset
    // only.  Mark that all elements are directed to this terminal.
    LINETERMCAPS tCaps;
    tCaps.dwTermDev = LINETERMDEV_PHONE;
    tCaps.dwTermModes = (LINETERMMODE_BUTTONS | LINETERMMODE_LAMPS | LINETERMMODE_DISPLAY |
                         LINETERMMODE_RINGER | LINETERMMODE_HOOKSWITCH | LINETERMMODE_MEDIABIDIRECT);
    tCaps.dwTermSharing = LINETERMSHARING_SHAREDEXCL;   
    AddTerminal ("Handset", tCaps, tCaps.dwTermModes);
    
    // Now adjust the line device capabilities.  We don't support any of the
    // line device capability flags, and don't need dialing parameters since the
    // switch doesn't allow them to be adjusted.
    LPLINEDEVCAPS lpCaps = GetLineDevCaps();
    lpCaps->dwAnswerMode = LINEANSWERMODE_DROP;    
    lpCaps->dwMonitorDigitModes = (LINEDIGITMODE_PULSE | LINEDIGITMODE_DTMF | LINEDIGITMODE_DTMFEND);
    lpCaps->dwGenerateDigitModes = LINEDIGITMODE_DTMF;
    lpCaps->dwGenerateToneModes = LINETONEMODE_CUSTOM | LINETONEMODE_RINGBACK | LINETONEMODE_BUSY | LINETONEMODE_BEEP | LINETONEMODE_BILLING;
    lpCaps->dwGenerateToneMaxNumFreq = 3;                                                                                                    
    lpCaps->dwMonitorToneMaxNumFreq = 3;
    lpCaps->dwMonitorToneMaxNumEntries = 5;
    lpCaps->dwGatherDigitsMinTimeout = 2500;
    lpCaps->dwGatherDigitsMaxTimeout = 32000;   
    lpCaps->dwDevCapFlags |= (LINEDEVCAPFLAGS_CROSSADDRCONF | LINEDEVCAPFLAGS_CLOSEDROP);

    // Setup the USER->USER information sizes.  We don't actually do anything with
    // the user information, but we allow the function calls to proceed as if they
    // DID do something.
    lpCaps->dwUUIAcceptSize = 256;
    lpCaps->dwUUIAnswerSize = 256;
    lpCaps->dwUUIMakeCallSize = 256;
    lpCaps->dwUUIDropSize = 256;
    lpCaps->dwUUISendUserUserInfoSize = 1024;
    lpCaps->dwUUICallInfoSize = 256;

    // Grab the settings from the emulator.
    EMSETTINGS Settings;
    GetDeviceInfo()->DRV_GetSwitchSettings (&Settings);

    // Go through and add each address and setup the address capabilities
    for (WORD i = 0; i < Settings.wAddressCount; i++)
    {   
    	// Get the address information from the switch                       
        EMADDRESSINFO AddressInfo;
        AddressInfo.wAddressID = i;
        GetDeviceInfo()->DRV_GetAddressInfo (&AddressInfo);
        
        // Create an address to map this.
        CreateAddress (AddressInfo.szAddress, 	// Dialable address
        			   Settings.szOwner,        // English name
                       TRUE, 					// Allows outgoing calls
                       TRUE,                    // Allows incoming calls
                       MEDIAMODE_ALL,           // Media modes supported
                       LINEBEARERMODE_VOICE,    // Bearer mode
                       0, 						// Minimum data rate
                       115000,                  // Maximum data rate
                       NULL,                    // DIALPARAMS structure
                       1,                       // Max active calls
                       1,                       // Max onHold calls
                       1,                       // Max onHold pending calls
                       8,                       // Max conferenced calls
                       3);                      // Max transfer to conference calls
                              
        // Add the completion messages valid for this address.
        CTSPIAddressInfo* pAddr = GetAddress((int)i);
        for (int x = 0; g_ComplMsgs[x] != NULL; x++)
            pAddr->AddCompletionMessage (g_ComplMsgs[x]);

		// Adjust the address capabilities to reflect our emulator.        
        LPLINEADDRESSCAPS lpAddrCaps = pAddr->GetAddressCaps();
        lpAddrCaps->dwCallCompletionModes = (LINECALLCOMPLMODE_CAMPON | 
                        LINECALLCOMPLMODE_CALLBACK | LINECALLCOMPLMODE_INTRUDE | 
                        LINECALLCOMPLMODE_MESSAGE);
        lpAddrCaps->dwTransferModes = LINETRANSFERMODE_TRANSFER | LINETRANSFERMODE_CONFERENCE;
        lpAddrCaps->dwForwardModes = LINEFORWARDMODE_UNCOND;       
        lpAddrCaps->dwParkModes = LINEPARKMODE_DIRECTED | LINEPARKMODE_NONDIRECTED;
        lpAddrCaps->dwRemoveFromConfCaps = LINEREMOVEFROMCONF_LAST;
        lpAddrCaps->dwRemoveFromConfState = LINECALLSTATE_IDLE;
        lpAddrCaps->dwAddrCapFlags |= (LINEADDRCAPFLAGS_PARTIALDIAL | 
                LINEADDRCAPFLAGS_CONFDROP | LINEADDRCAPFLAGS_CONFERENCEMAKE |
                LINEADDRCAPFLAGS_FWDSTATUSVALID | LINEADDRCAPFLAGS_TRANSFERMAKE | 
                LINEADDRCAPFLAGS_TRANSFERHELD | LINEADDRCAPFLAGS_CONFERENCEHELD);
    }

}// CDSProvider::InitializeLineDevice

////////////////////////////////////////////////////////////////////////////
// CDSLine::GetDevConfig
//
// This function is used by applications to query device
// configuration from our TSP.  It is overriden to supply 
// "dummy" information about a DATAMODEM ability so that
// applications like HyperTerminal will talk to us.
//
LONG CDSLine::GetDevConfig(CString& strDeviceClass, LPVARSTRING lpDeviceConfig)
{
	if (strDeviceClass == _T("comm/datamodem"))
	{
		// We fill it in with junk.. some MODEM applications expect this
		// structure and won't talk to us unless we provide this function.
		lpDeviceConfig->dwUsedSize = lpDeviceConfig->dwNeededSize = sizeof(VARSTRING)+1;
		lpDeviceConfig->dwStringFormat = STRINGFORMAT_BINARY;
		lpDeviceConfig->dwStringOffset = sizeof(VARSTRING);
		lpDeviceConfig->dwStringSize = 1;
		return FALSE;
	}
	return LINEERR_INVALDEVICECLASS;

}// CDSLine::GetDevConfig

////////////////////////////////////////////////////////////////////////////
// CDSLine::SetDevConfig
//
// This function is called by applications to set the
// device configuration.  It is overriden to handle the
// "modem" case for HyperTerminal.  It does nothing.
//
LONG CDSLine::SetDevConfig(CString& strDeviceClass, LPVOID const /*lpDevConfig*/, DWORD /*dwSize*/)
{
	if (strDeviceClass == _T("comm/datamodem"))
		return FALSE;
	return LINEERR_INVALDEVICECLASS;

}// CDSLine::SetDevConfig

////////////////////////////////////////////////////////////////////////////
// CDSLine::SetDefaultMediaDetection
//
// Change the media detection which is being monitored for.
// If we have existing calls with the media mode selected, then offer
// those calls to TAPI now.
//
LONG CDSLine::SetDefaultMediaDetection(DWORD dwMediaModes)
{   
    static EMADDRESSINFO AddressInfo;

    // Mark the information on the line
    LONG lResult = CTSPILineConnection::SetDefaultMediaDetection(dwMediaModes);
    if (lResult != 0)
        return lResult;
    
    // Ask the emulator for existing calls on this line - we only have a single
    // line so there is no need to qualify the line to address relationship.
    for (int i = 0; i < (int) GetAddressCount(); i++)
    {
        memset (&AddressInfo, 0, sizeof(EMADDRESSINFO));
        AddressInfo.wAddressID = (WORD) i;
        GetDeviceInfo()->DRV_GetAddressInfo (&AddressInfo);

        // If the state of the call is an active state then tell TAPI about
        // the call.  Since we don't know (and cannot find out) the origin/reason,
        // report unavailable.
        if (AddressInfo.wAddressState != 0 &&
            AddressInfo.wAddressState != ADDRESSSTATE_OFFLINE &&
            AddressInfo.wAddressState != ADDRESSSTATE_DISCONNECT &&
            AddressInfo.wAddressState != ADDRESSSTATE_UNKNOWN)
        {   
            CTSPIAddressInfo* pAddr = GetAddress(i);
            if (pAddr->GetAddressStatus()->dwNumActiveCalls == 0 &&
                pAddr->GetAddressStatus()->dwNumOnHoldCalls == 0 &&
                pAddr->GetAddressStatus()->dwNumOnHoldPendCalls == 0)
            {               
                if (AddressInfo.dwMediaMode & dwMediaModes)
                {
                    CTSPICallAppearance* pCall = pAddr->CreateCallAppearance(NULL, 0,
                                             LINECALLORIGIN_UNAVAIL, LINECALLREASON_UNAVAIL);              
                    ASSERT (pCall != NULL);
                    pCall->SetCallState (g_CallStates[AddressInfo.wAddressState], 0, AddressInfo.dwMediaMode); 
                }               
            }                   
        }   
    }                   
    
    return 0L;
    
}// CDSLine::SetDefaultMediaModes

////////////////////////////////////////////////////////////////////////////
// CDSLine::OnCallFeaturesChanged
//
// This method is called whenever the call features have changed due to
// state changes.  The address/line have also been adjusted by the time
// this function is called.
//
DWORD CDSLine::OnCallFeaturesChanged(CTSPICallAppearance* pCall, DWORD dwCallFeatures)
{                                 
    CTSPIAddressInfo* pAddr = pCall->GetAddressInfo();
    
    // If the line has calls on it, then turn off forwarding ability.  Our
    // emulated switch doesn't allow forwarding with active calls.
    if (GetLineDevStatus()->dwNumActiveCalls > 0 || GetLineDevStatus()->dwNumOnHoldCalls > 0 ||
        	GetLineDevStatus()->dwNumOnHoldPendCalls > 0)
		SetLineFeatures (GetLineDevStatus()->dwLineFeatures &= ~LINEFEATURE_FORWARD);
    else
		pAddr->SetAddressFeatures(pAddr->GetAddressStatus()->dwAddressFeatures | LINEADDRFEATURE_FORWARD);
    
    // We don't adjust the call features any - simply the address/line.
    return dwCallFeatures;      

}// CDSLine::CheckCallFeatures

////////////////////////////////////////////////////////////////////////////
// CDSLine::ReceiveData
//
// This method is called when a string is returned from the Telebridge
// device.  It evaluates the string and determines how to complete the
// current outstanding request.
//
BOOL CDSLine::ReceiveData(DWORD dwData, const LPVOID lpBuff, DWORD /*dwSize*/)
{
    WORD wResponse = (WORD) dwData;

    // Cycle through all the requests seeing if anyone likes this response.  This
    // provider really doesn't need this approach - everything is guarenteed to 
    // complete and respond correctly by the emulated switch, but this loop is a 
    // good idea if there is outside interferance on the phone unit and states expected
    // may be superceded by an outside source.
    BOOL fRequestHandled = FALSE;
    for (int i = 0; i < GetRequestCount() && !fRequestHandled; i++)
    {
        CTSPIRequest* pReq = GetRequest(i);
        if (pReq != NULL)
        {   
            switch(pReq->GetCommand())
            {
                // 1. Accept an offering call.
                // 2. Alter the path of I/O from or to a terminal.
                // 3. Secure a call appearance.
                // 4. Generate a tone.    
                // 5. Send USER INFO to connected caller
                // 6. Release USER INFO record information
                // 7. Uncomplete Call request (simply remove request)
                // 8. Set call params (ignore)
                case REQUEST_ACCEPT:
                case REQUEST_SETTERMINAL:
                case REQUEST_SECURECALL:
                case REQUEST_GENERATETONE:
                case REQUEST_SENDUSERINFO:
                case REQUEST_RELEASEUSERINFO:
                case REQUEST_UNCOMPLETECALL:
                case REQUEST_SETCALLPARAMS:
                case REQUEST_SETDISPLAY:
                case REQUEST_SETLAMP:
                case REQUEST_SETBUTTONINFO:
                    // We don't do anything - simply return 0 to people who
                    // call this function and pretend that it did whatever.
                    pReq->GetConnectionInfo()->CompleteRequest(pReq, 0);
                    break;
            
                // Answer an offering call
                case REQUEST_ANSWER:
                    fRequestHandled = processAnswer (pReq, wResponse, lpBuff);
                    break;
                
                // Take the steps to go offhook and prepare a call for
                // dialing.
                case REQUEST_MAKECALL:
                    fRequestHandled = processMakeCall (pReq, wResponse, lpBuff);
                    break;
            
                // Take the steps to dial a call.    
                case REQUEST_DIAL:
                    fRequestHandled = processDial (pReq, wResponse, lpBuff);
                    break;
                
                // Park a call on an address
                case REQUEST_PARK:
                    fRequestHandled = processPark (pReq, wResponse, lpBuff);
                    break;
                    
                // Pickup a call on an address
                case REQUEST_PICKUP:
                    fRequestHandled = processPickup (pReq, wResponse, lpBuff);
                    break;
                    
                // Unpark a call parked on an address
                case REQUEST_UNPARK:
                    fRequestHandled = processUnpark (pReq, wResponse, lpBuff);
                    break;
                
                // Hold a call
                case REQUEST_HOLD:
                    fRequestHandled = processHold (pReq, wResponse, lpBuff);
                    break;
            
                // Unhold a call    
                case REQUEST_UNHOLD:
                    fRequestHandled = processUnhold (pReq, wResponse, lpBuff);
                    break;
            
                // SwapHold a call
                case REQUEST_SWAPHOLD:
                    fRequestHandled = processSwapHold (pReq, wResponse, lpBuff);
                    break;
                
                // Blind Transfer
                case REQUEST_BLINDXFER:
                    fRequestHandled = processBlindXfer (pReq, wResponse, lpBuff);
                    break;
                
                // Setup consultation transfer
                case REQUEST_SETUPXFER:
                    fRequestHandled = processSetupXfer (pReq, wResponse, lpBuff);
                    break;
            
                // Complete a transfer event
                case REQUEST_COMPLETEXFER:
                    fRequestHandled = processCompleteXfer (pReq, wResponse, lpBuff);
                    break;
            
                // Drop a call - move to idle.    
                case REQUEST_DROPCALL:
                case REQUEST_DROPCALLONCLOSE:
                case REQUEST_DROPNOOWNER:
                    fRequestHandled = processDropCall (pReq, wResponse, lpBuff);
                    break;
            
                // Generate a series of DTMF digits
                case REQUEST_GENERATEDIGITS:
                    fRequestHandled = processGenDigits (pReq, wResponse, lpBuff);
                    break;
                
                // Forward the address
                case REQUEST_FORWARD:
                    fRequestHandled = processForward (pReq, wResponse, lpBuff);
                    break;
                
                // Complete a non-connected call
                case REQUEST_COMPLETECALL:
                    fRequestHandled = processCompleteCall (pReq, wResponse, lpBuff);
                    break;
                
                // Redirect an offering call
                case REQUEST_REDIRECT:
                    fRequestHandled = processRedirect (pReq, wResponse, lpBuff);
                    break;
                
                // Setup a new conference call
                case REQUEST_SETUPCONF:                                       
                case REQUEST_PREPAREADDCONF:
                    fRequestHandled = processSetupConf(pReq, wResponse, lpBuff);
                    break;

                // Add a new call to an existing conference
                case REQUEST_ADDCONF:
                    fRequestHandled = processAddConf(pReq, wResponse, lpBuff);
                    break;
                
                // Remove the last call from the conference - revert back
                // to two-party call.
                case REQUEST_REMOVEFROMCONF:
                    fRequestHandled = processRemoveConf(pReq, wResponse, lpBuff);
                    break;
                
                default:
	            	CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
                    break;
            }
        }            
    }
    
    // If the request was not handled, then pass this onto an "asynchronous" 
    // response manage for the device.  This is for things like offering calls, 
    // lamp state changes, etc.  This will also handle the conditions of dropping
    // consultation calls and re-connecting the original call.
    if (!fRequestHandled)
        ProcessAsynchDeviceResponse (wResponse, lpBuff);
    return fRequestHandled;

}// CDSLine::ProcessData

