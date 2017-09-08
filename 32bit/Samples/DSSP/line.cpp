/******************************************************************************/
//                                                                        
// LINE.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the line device code for the service provider.
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
LPCTSTR g_ComplMsgs[] = {
	{ _T("Completion Message #1") },
	{ _T("Completion Message #2") },
	{ _T("Completion Message #3") },
	{ _T("Completion Message #4") },
	NULL 
};  

/*****************************************************************************
** Procedure:  CDSLine::CDSLine
**
** Arguments:  void
**
** Returns:    void
**
** Description:  Constructor for the line object
**
*****************************************************************************/
CDSLine::CDSLine()
{
}// CDSLine::CDSLine

/*****************************************************************************
** Procedure:  CDSLine::~CDSLine
**
** Arguments:  void
**
** Returns:    void
**
** Description:  Destructor for the line object
**
*****************************************************************************/
CDSLine::~CDSLine()
{
}// CDSLine::~CDSLine

/*****************************************************************************
** Procedure:  CDSLine::Init
**
** Arguments:  'pDev'			-	Device object this line belongs to
**             'dwLineDeviceID'	-	Unique line identifier within the TSP
**             'dwPos'			-	Index position of line within device array
**             'dwItemData'		-   Used when line was dynamically created (P&P).
**
** Returns:    void
**
** Description:  This function is called by the device owner to initialize
**               the line object.
**
*****************************************************************************/
VOID CDSLine::Init (CTSPIDevice* pDev, DWORD dwLineDeviceID, DWORD dwPos, DWORD /*dwItemData*/)
{
	// Let the base class initialize first.
	CTSPILineConnection::Init (pDev, dwLineDeviceID, dwPos);

    // Set the name associated with this line.  This is optional, it gives
    // the user a displayable name which is associated with the line.  Most
	// applications use this name in their UI.
    SetName (_T("DSSP Line #1"));

    // Grab the version information from the emulator.
    EMVERSIONINFO VerInfo;
    GetDeviceInfo()->DRV_GetVersionInfo (&VerInfo);

    // Set the connection (line) info
    SetConnInfo (VerInfo.szSwitchInfo);

    // Add our terminal devices.  For our demo provider, we support a handset
    // only.  Mark that all elements are directed to this terminal.
    LINETERMCAPS tCaps;
    tCaps.dwTermDev		= LINETERMDEV_PHONE;
    tCaps.dwTermModes	= (LINETERMMODE_BUTTONS | LINETERMMODE_LAMPS | LINETERMMODE_DISPLAY |
						   LINETERMMODE_RINGER | LINETERMMODE_HOOKSWITCH | LINETERMMODE_MEDIABIDIRECT);
    tCaps.dwTermSharing = LINETERMSHARING_SHAREDEXCL;   
    AddTerminal (_T("Handset"), tCaps, tCaps.dwTermModes);

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
    lpCaps->dwGatherDigitsMinTimeout = 500;		// 250 is the minimum for the TSP++ library timer thread
    lpCaps->dwGatherDigitsMaxTimeout = 32000;   
    lpCaps->dwDevCapFlags |= (LINEDEVCAPFLAGS_CROSSADDRCONF | LINEDEVCAPFLAGS_CLOSEDROP);

    // Setup the USER->USER information sizes.  We don't actually do anything with
    // the user information, but we allow the function calls to proceed as if they
    // DID do something.
    lpCaps->dwUUIAcceptSize = lpCaps->dwUUIAnswerSize = lpCaps->dwUUIMakeCallSize =\
    lpCaps->dwUUIDropSize = lpCaps->dwUUISendUserUserInfoSize = lpCaps->dwUUICallInfoSize = 1024;

    // Grab the address settings from the emulator.
    EMSETTINGS Settings;
    if (!GetDeviceInfo()->DRV_GetSwitchSettings (&Settings))
		return;

    // Go through and add each address and setup the address capabilities
    for (WORD i = 0; i < Settings.wAddressCount; i++)
    {   
		// Add the address based on the information given by the emulator.
        EMADDRESSINFO AddressInfo;
        AddressInfo.wAddressID = i;
        if (!GetDeviceInfo()->DRV_GetAddressInfo (&AddressInfo))
			continue;

        CreateAddress (	AddressInfo.szAddress,		// Dialable address (phone#)
						Settings.szOwner,			// Address name (used for identification)
						TRUE,						// Allow incoming calls
						TRUE,						// Allow outgoing calls
						MEDIAMODE_ALL,				// Avalabile media modes
						LINEBEARERMODE_VOICE,		// Single bearer mode for this address
                        0,							// Minimum data rate on address
						0,							// Maximum data rate on address
						NULL,						// Dialing parameters (LINEDIALPARAMS)
						1,							// Max number of active calls on address
						1,							// Max number of OnHold calls on address
						1,							// Max number of OnHoldPending calls on address
						8,							// Max number of calls in a conference on address
						3);							// Max number of calls transferred into a conference on address

        // Add the completion messages valid for this address.
        CTSPIAddressInfo* pAddr = GetAddress(i);
		if (pAddr != NULL)
		{
			for (int x = 0; g_ComplMsgs[x] != NULL; x++)
				pAddr->AddCompletionMessage (g_ComplMsgs[x]);
        
			// Adjust the ADDRESSCAPS for this address
			LPLINEADDRESSCAPS lpAddrCaps = pAddr->GetAddressCaps();
			lpAddrCaps->dwCallCompletionModes = (LINECALLCOMPLMODE_CAMPON | 
							LINECALLCOMPLMODE_CALLBACK | LINECALLCOMPLMODE_INTRUDE | 
							LINECALLCOMPLMODE_MESSAGE);

			lpAddrCaps->dwMaxCallDataSize = 4096;
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
    }

	// Add the WAV devices which will be our handset I/O
	if (waveInGetNumDevs() > 0)
		AddDeviceClass(_T("wave/in"), (DWORD)0);
	if (waveOutGetNumDevs() > 0)
		AddDeviceClass(_T("wave/out"), (DWORD)0);

    // Cancel any forwarding in effect - we cannot DETECT where it is
    // forwarded to, so we will initialize it to something we know and therefore
    // be able to correctly report forwarding status information
    GetDeviceInfo()->DRV_Forward(0xffffffff, NULL);

}// CDSLine::Init

/*****************************************************************************
** Procedure:  CDSLine::GetDevConfig
**
** Arguments:  'strDeviceClass' - Device class which is being queried.
**             'lpDeviceConfig' - VARSTRING to return the requested data to
**
** Returns:    TAPI result code
**
** Description:  This function is used by applications to query device
**               configuration from our TSP.  It is overriden to supply 
**               "dummy" information about a DATAMODEM ability so that
**               applications like HyperTerminal will talk to us.
**
*****************************************************************************/
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

/*****************************************************************************
** Procedure:  CDSLine::SetDevConfig
**
** Arguments:  'strDeviceClass' - Device class which is being set.
**             'lpDeviceConfig' - VARSTRING to set the requested data to
**             'dwSize'         - Size of the VARSTRING data
**
** Returns:    void
**
** Description:  This function is called by applications to set the
**               device configuration.  It is overriden to handle the
**               "modem" case for HyperTerminal.  It does nothing.
**
*****************************************************************************/
LONG CDSLine::SetDevConfig(CString& strDeviceClass, LPVOID const /*lpDevConfig*/, DWORD /*dwSize*/)
{
	if (strDeviceClass == _T("comm/datamodem"))
		return FALSE;
	return LINEERR_INVALDEVICECLASS;

}// CDSLine::SetDevConfig

/*****************************************************************************
** Procedure:  CDSLine::ReceieveData
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
BOOL CDSLine::ReceiveData (DWORD dwData, const LPVOID lpBuff, DWORD dwSize)
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
                // 8. Set call params
				// 9. Set call data (handled by TSP++)
                case REQUEST_ACCEPT:
                case REQUEST_SETTERMINAL:
                case REQUEST_SECURECALL:
                case REQUEST_GENERATETONE:
                case REQUEST_SENDUSERINFO:
                case REQUEST_RELEASEUSERINFO:
                case REQUEST_UNCOMPLETECALL:
                case REQUEST_SETCALLPARAMS:
				case REQUEST_SETCALLDATA:
                    // We don't do anything - simply return 0 to people who
                    // call this function and pretend that it did whatever.
                    CompleteRequest(pReq, 0);
                    break;
  
				// Wait request - this is a specialized packet used to
				// wait for synchronous requests from our server.
				case REQUEST_WAITFORREQ:
					fRequestHandled = processWaitReq (pReq, wResponse, lpBuff, dwSize);
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
            }
        }            
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

}// CDSLine::ReceiveData

/*****************************************************************************
** Procedure:  CDSLine::ProcessAsynchDeviceResponse
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
void CDSLine::ProcessAsynchDeviceResponse(WORD wResult, const LPVOID lpData)
{                           
    static DWORD dwCompletionID = 0L;
    switch (wResult)
    {   
        // An address has changed states on the device - see if it is an
        // offering call.  If so, create a new call on the address in question.
        case EMRESULT_ADDRESSCHANGED:                      
        {   
            LPEMADDRESSCHANGE lpChange = (LPEMADDRESSCHANGE) lpData;
            if (lpChange->wNewState == ADDRESSSTATE_OFFERING || lpChange->wNewState == ADDRESSSTATE_OFFERINGT)
            {
                HandleNewCall ((DWORD)lpChange->wAddressID, lpChange->dwMediaModes, dwCompletionID,
					           (lpChange->wNewState == ADDRESSSTATE_OFFERINGT));
                dwCompletionID = 0L;
            }   
            else
                UpdateCallState ((DWORD)lpChange->wAddressID, (int)lpChange->wNewState, (int)lpChange->wStateInfo, lpChange->dwMediaModes);
        }
        break;

        // An offering call is ringing.
        case EMRESULT_RING:
            OnRingDetected(0);
            break;

        // A tone (specific frequency) was detected on the device.
        case EMRESULT_TONEDETECTED:
        {
			LPEMTONEBUFF lpTone = (LPEMTONEBUFF) lpData;
            CTSPIAddressInfo* pAddr = GetAddress(lpTone->wAddressID);
            ASSERT (pAddr != NULL);
                
            // Call must be connected or proceeding/dialing
            CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_CONNECTED);
            if (pCall == NULL)
                pCall = pAddr->FindCallByState(LINECALLSTATE_PROCEEDING);
            if (pCall == NULL)
                pCall = pAddr->FindCallByState(LINECALLSTATE_DIALING);
            
            // The emulator sends the frequency as three valid entries which can then be checked together.
            if (pCall != NULL)
                pCall->OnTone (lpTone->dwFreq[0], lpTone->dwFreq[1], lpTone->dwFreq[2]);
        }
        break;
			
        // A call completion request has completed
        case EMRESULT_COMPLRESULT:
        {     
			LPEMCOMPLETECALL emComplete = (LPEMCOMPLETECALL) lpData;

            // Locate the completion request if available.
            DWORD dwReqID = emComplete->wCompletionType;
            TSPICOMPLETECALL* pComplete = FindCallCompletionRequest(dwReqID, NULL);
            if (pComplete != NULL)
            {                   
                if (pComplete->dwCompletionMode == LINECALLCOMPLMODE_CAMPON)
                {
                    // The call for this address is about to go connected..
                    ASSERT (pComplete->pCall != NULL);
                    pComplete->pCall->GetCallInfo()->dwCompletionID = pComplete->dwCompletionID;
                    pComplete->pCall->SetCallReason (LINECALLREASON_CALLCOMPLETION);
                }
                else if (pComplete->dwCompletionMode == LINECALLCOMPLMODE_CALLBACK)
                    dwCompletionID = pComplete->dwCompletionID;
                RemoveCallCompletionRequest (pComplete->dwCompletionID);
            }                   
            else
                dwCompletionID = 0L;
        }
        break;
       
        // A digit was detected from the remote side - supply it to the
        // connected call appearance on the address specified for digit
        // monitoring/gathering support.
        case EMRESULT_DIGITDETECTED:
        {               
            static char cLastChar;
            LPEMDIGITBUFF lpDigit = (LPEMDIGITBUFF) lpData;
            CTSPIAddressInfo* pAddr = GetAddress(lpDigit->wAddressID);
            ASSERT (pAddr != NULL);
            CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_CONNECTED);
            if (pCall != NULL)
            {   
                DWORD dwType = LINEDIGITMODE_PULSE;
                if (lpDigit->fDTMF)
                {   
                    // The emulator switch sends a ZERO digit to indicate
                    // silence on the line after a DTMF tone detection.  We
                    // interpret this to mean that the digit is UP and fake
                    // a DTMF END tone.
                    if (lpDigit->cDigit != (char)0)
                    {
                        dwType = LINEDIGITMODE_DTMF;
                        cLastChar = lpDigit->cDigit;
                    }
                    else
                        dwType = LINEDIGITMODE_DTMFEND;                            
                }
                else
                    cLastChar = lpDigit->cDigit;
                pCall->OnDigit (dwType, cLastChar);
            }
        }
        break;
        
        // An offering call has callerID information.
        case EMRESULT_CALLERID:                      
        {
            LPEMCALLERID lpCallerInfo = (LPEMCALLERID) lpData;
            CTSPIAddressInfo* pAddr = GetAddress ((DWORD)lpCallerInfo->wAddressID);
            CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_OFFERING);
            if (pCall != NULL)
                pCall->SetCallerIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                    lpCallerInfo->szAddressInfo, lpCallerInfo->szName);                                    
        }
        break;
        
        // An offering call was redirected from another address
        case EMRESULT_REDIRECTID:
        {
            LPEMCALLERID lpCallerInfo = (LPEMCALLERID) lpData;
            CTSPIAddressInfo* pAddr = GetAddress ((DWORD)lpCallerInfo->wAddressID);
            CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_OFFERING);
            if (pCall != NULL)
            {
                // Set the call reason to REDIRECT
                pCall->SetCallReason(LINECALLREASON_REDIRECT);
                
                // Move the redirecting information into the CALLED information
                // and REDIRECTING information.  Move the original CALLED 
                // information into the REDIRECTED information.  Since the
                // emulator cannot redirect itself, we are guarenteed that this
                // redirection occurred from the redirecting ID.
                pCall->SetRedirectionIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                    pAddr->GetDialableAddress(), pAddr->GetName());
                pCall->SetRedirectingIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                    lpCallerInfo->szAddressInfo, lpCallerInfo->szName);
                pCall->SetCalledIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                    lpCallerInfo->szAddressInfo, lpCallerInfo->szName);
            }                                            
        }
        break;
    }

}// CDSLine::ProcessAsynchDeviceResponse

/*****************************************************************************
** Procedure:  CDSLine::UpdateCallState
**
** Arguments:  'dwAddressID' - Address the call has changed on.
**             'iNewState' - New state of the call reported by the emulator
**             'iStateInfo' - Secondary information of the call state
**             'dwMediaMode' - New media mode of the call
**
** Returns:    void
**
** Description: Update an existing call's call state for an address.  
**				Emulator has changed the state of a call.
**
*****************************************************************************/
void CDSLine::UpdateCallState (DWORD dwAddressID, int iNewState, int iStateInfo, DWORD /*dwMediaModes*/)
{
    CTSPIAddressInfo* pAddr = GetAddress(dwAddressID);
    CTSPICallAppearance* pCall = NULL;

    // Locate the call appearance this is referring to.  It will either be a 
    // direct call appearance, or a consultation call appearance.
    if (pAddr->GetCallCount() == 1)
        pCall = pAddr->GetCallInfo(0);
    else
    {   
        // Look for a call in a particular state - the following is the order
        // we search in.  The emulator switch always works with the active call, i.e.
        // the connected call will always become offline *before* the onHold call
        // moves back to connected.
        static DWORD dwStateOrder[] = {
			LINECALLSTATE_CONNECTED, 
			LINECALLSTATE_PROCEEDING,
			LINECALLSTATE_ONHOLD, 
			LINECALLSTATE_ONHOLDPENDTRANSFER,
			LINECALLSTATE_ONHOLDPENDCONF, 
			LINECALLSTATE_CONFERENCED,
			LINECALLSTATE_BUSY,
			LINECALLSTATE_DIALTONE,
            (DWORD)-1L
        };
        
        for (int i = 0; dwStateOrder[i] != (DWORD)-1L; i++)
        {
            pCall = pAddr->FindCallByState(dwStateOrder[i]);
            if (pCall)
                break;
        }
    }        

	// If we never found a call, create one for TAPI.  The user
	// is interacting with the phone device directly via the emulator.
	if (pCall == NULL && iNewState != ADDRESSSTATE_OFFLINE)
	{
		// Create a call appearance on the address in question.  This function will create
		// the necessary class library call structures and notify TAPI about a new call.
		CTSPIAddressInfo* pAddress = GetAddress (dwAddressID);
		ASSERT (pAddress != NULL);
		pCall = pAddress->CreateCallAppearance (
											NULL,						// Existing TAPI hCall to assicate with (NULL=create).
											0,							// LINECALLINFO Call Parameter flags
											LINECALLORIGIN_OUTBOUND,	// Call origin	
											LINECALLREASON_DIRECT);		// Call reason
		// Now transition this call to the proper state based on what
		// the emulator says it should be.
		pCall->SetCallState(g_CallStates[iNewState], 0, LINEMEDIAMODE_INTERACTIVEVOICE);
		pCall->SetCallID(GetTickCount());
	}

	// Otherwise, transition the call to the appropriate state.
    else if (pCall && pCall->GetCallState() != LINECALLSTATE_IDLE)
    {   
        if (iNewState == ADDRESSSTATE_OFFLINE)
        {
            if (pCall->GetCallState() == LINECALLSTATE_CONNECTED)
                pCall->SetCallState(LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_UNAVAIL);
            pCall->SetCallState (LINECALLSTATE_IDLE);
        }
        else if (iNewState == ADDRESSSTATE_ONHOLD)
        {
            if (pCall->GetCallState() != LINECALLSTATE_ONHOLD &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDTRANSFER &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDCONF)
            {
                switch (iStateInfo)
                {
                    case HOLDTYPE_NORMAL:
                        pCall->SetCallState(LINECALLSTATE_ONHOLD);
                        break;
                    case HOLDTYPE_TRANSFER:
                        pCall->SetCallState(LINECALLSTATE_ONHOLDPENDTRANSFER);
                        break;
                    case HOLDTYPE_CONFERENCE:
                        pCall->SetCallState(LINECALLSTATE_ONHOLDPENDCONF);
                        break;                                            
                    default:
                        ASSERT (FALSE);
                        break;
                }
            }
        }
        else if (iNewState == ADDRESSSTATE_INCONF)
        {
            ASSERT (pCall->GetCallState() == LINECALLSTATE_ONHOLDPENDCONF);
            pCall->SetCallState (LINECALLSTATE_CONNECTED);
        }                                                    
        else if (iNewState == ADDRESSSTATE_CONNECT)
        {   
            // If this call isn't transitioning from a HOLDing pattern, then
            // mark the connection ID (we assume that it is transitioning from
            // DIAL or PROCEED state).
            if (pCall->GetCallState() != LINECALLSTATE_ONHOLD &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDTRANSFER &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDCONF)
            {
                if (pCall->GetCallInfo()->dwReason != LINECALLREASON_DIRECT)                
                {
                    pCall->SetConnectedIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                pAddr->GetDialableAddress(), pAddr->GetName());
                }
            }                               
        
            // If a conference call just reverted to two-party call due to
            // either a remove conference or an unhold, then idle the conference
            // call itself and transition the final party into the two party call.
            if (pCall->GetCallState() == LINECALLSTATE_CONNECTED && 
                pCall->GetCallType() == CALLTYPE_CONFERENCE)
            {
                CTSPIConferenceCall* pConfCall = (CTSPIConferenceCall*) pCall;
                ASSERT (pConfCall->GetConferenceCount() == 1);
                pCall = pConfCall->GetConferenceCall(0);
                pConfCall->SetCallState(LINECALLSTATE_IDLE);
                pCall->SetCallState(LINECALLSTATE_CONNECTED);
            }    
            
            // If this was a PROCEEDING call which just connected, transition to
            // the RINGBACK state for a second to see "ring events".
            else if (pCall->GetCallState() == LINECALLSTATE_PROCEEDING)
            {
                pCall->SetCallState(LINECALLSTATE_RINGBACK);
                pCall->SetCallState(LINECALLSTATE_CONNECTED);
            }
            
            // Otherwise simply transition the call
            else if (pCall->GetCallState() != LINECALLSTATE_CONNECTED)
                pCall->SetCallState(LINECALLSTATE_CONNECTED);
        }
        
        else if (iNewState == ADDRESSSTATE_DIALTONE ||
                 iNewState == ADDRESSSTATE_ONLINE ||
                 iNewState == ADDRESSSTATE_BUSY)
        {
            if (pCall->GetCallState() != g_CallStates[iNewState])
                pCall->SetCallState(g_CallStates[iNewState]);
        }
    }        
    
}// CDSLine::UpdateCallState

/*****************************************************************************
** Procedure:  CDSLine::HandleNewCall
**
** Arguments:  'dwAddressID' - Address the call has changed on.
**             'dwMediaModes' - New media mode(s) of the call
**             'dwCompletionID' - Completion ID if this was parked call.
**
** Returns:    void
**
** Description: Process a newly detected call on the line.
**
*****************************************************************************/
void CDSLine::HandleNewCall (DWORD dwAddressID, DWORD dwMediaModes, DWORD dwCompletionID, BOOL fExternal)
{   
    // Default the media mode.
    if (dwMediaModes == 0)
        dwMediaModes = LINEMEDIAMODE_INTERACTIVEVOICE;

    // Now, if the line doesn't have anybody watching for this type of call,
    // then drop the call and don't tell TAPI about it.  This is inline with the TAPI
	// spec since TAPI will ignore any call which will not have any owner (since no app
	// is looking for the specified media mode).
    if ((GetDefaultMediaDetection() & dwMediaModes) == 0)
    {
        DTRACE (TRC_MIN, "NewCall dropped, media mode=0x%lx, required=0x%lx\r\n", dwMediaModes, GetDefaultMediaDetection());
        GetDeviceInfo()->DRV_DropCall (dwAddressID);
        return;
    }

    // Create a call appearance on the address in question.  This function will create
	// the necessary class library call structures and notify TAPI about a new call.
    CTSPIAddressInfo* pAddress = GetAddress (dwAddressID);
    ASSERT (pAddress != NULL);
	DWORD dwOrigin = (fExternal) ? 
		(LINECALLORIGIN_INBOUND | LINECALLORIGIN_EXTERNAL) : 
		(LINECALLORIGIN_INBOUND | LINECALLORIGIN_INTERNAL);

    CTSPICallAppearance* pCall = pAddress->CreateCallAppearance (
										NULL,						// Existing TAPI hCall to assicate with (NULL=create).
										0,							// LINECALLINFO Call Parameter flags
										dwOrigin,					// Call origin
										LINECALLREASON_DIRECT);		// Call reason
	// Set a call-id into the call - we don't use the callid in this provider
	// so it is just a dummy value.
	pCall->SetCallID(GetTickCount());

    // If this has a completion ID, then set the new call reason and completion ID.
    if (dwCompletionID != 0L)
    {
        pCall->GetCallInfo()->dwCompletionID = dwCompletionID;
        pCall->SetCallReason (LINECALLREASON_CALLCOMPLETION);   
    }

	// If it is an external call, then mark the trunk
	if (fExternal)
		pCall->SetTrunkID(dwAddressID+1);

    // Reset the ring counter if necessary.
    OnRingDetected (0, TRUE);

    // Report an offering call to TAPI.
    pCall->SetCallState(LINECALLSTATE_OFFERING, 0, dwMediaModes);

}// CDSLine::HandleNewCall

/*****************************************************************************
** Procedure:  CDSLine::SetDefaultMediaDetection
**
** Arguments:  'dwMediaModes' - New media mode(s) of the call
**
** Returns:    TAPI result code
**
** Description: Change the media detection which is being monitored for.
**				If we have existing calls with the media mode selected, then offer
**				those calls to TAPI now.
**
*****************************************************************************/
LONG CDSLine::SetDefaultMediaDetection (DWORD dwMediaModes)
{   
    static EMADDRESSINFO AddressInfo;

    // Mark the information on the line
    LONG lResult = CTSPILineConnection::SetDefaultMediaDetection(dwMediaModes);
    if (lResult != 0)
        return lResult;
    
    // Ask the emulator for existing calls on this line
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
				// If this address supports the media mode, then bubble it up to TAPI
				// by creating a new call appearance.
                if (AddressInfo.dwMediaMode & dwMediaModes)
                {
                    CTSPICallAppearance* pCall = pAddr->CreateCallAppearance(NULL, 0,
                                             LINECALLORIGIN_UNAVAIL, LINECALLREASON_UNAVAIL);              
                    ASSERT (pCall != NULL);
					pCall->SetCallID(GetTickCount());
                    pCall->SetCallState (g_CallStates[AddressInfo.wAddressState], 0, AddressInfo.dwMediaMode); 
                }               
            }                   
        }   
    }                   
    
    return 0L;
    
}// CDSLine::SetDefaultMediaModes

/*****************************************************************************
** Procedure:  CDSLine::OnCallFeaturesChanged
**
** Arguments:  'pCall' - Call that changed
**             'dwFeatures' - new feature list
**
** Returns:    Modified feature list
**
** Description: This method is called whenever the call features have changed 
**              due to state changes.
**
*****************************************************************************/
DWORD CDSLine::OnCallFeaturesChanged (CTSPICallAppearance* pCall, DWORD dwFeatures)
{
	// We don't allow conferences to be held.
	if (pCall->GetCallType() == CALLTYPE_CONFERENCE)
		dwFeatures &= ~LINECALLFEATURE_HOLD;
	return dwFeatures;

}// CDSLine::OnCallFeaturesChanged

/*****************************************************************************
** Procedure:  CDSLine::OnAddressFeaturesChanged
**
** Arguments:  'pAddr' - Address that changed
**             'dwFeatures' - new feature list
**
** Returns:    Modified feature list
**
** Description: This method is called whenever the address features have changed due to
**              state changes.
**
*****************************************************************************/
DWORD CDSLine::OnAddressFeaturesChanged (CTSPIAddressInfo* pAddr, DWORD dwFeatures)
{
	// Don't allow pickup/makecall/unpark if any activity on the address - even
	// if the call is onHOLD.  We only allow consultation calls to be created via
	// transfer/conference events on an active address.  This is an emulator design
	// restriction.  The TSP++ library automatically adds these features when the active
	// call goes onHOLD.
	if (pAddr->GetAddressStatus()->dwNumOnHoldCalls + 
		pAddr->GetAddressStatus()->dwNumOnHoldPendCalls + 
		pAddr->GetAddressStatus()->dwNumActiveCalls +
		GetLineDevStatus()->dwNumActiveCalls)
		dwFeatures &= ~(LINEADDRFEATURE_MAKECALL | LINEADDRFEATURE_PICKUP | 
						LINEADDRFEATURE_UNPARK | LINEADDRFEATURE_FORWARD);
	return dwFeatures;

}// CDSLine::OnAddressFeaturesChanged

/*****************************************************************************
** Procedure:  CDSLine::OnLineFeaturesChanged
**
** Arguments:  'dwFeatures' - new feature list
**
** Returns:    Modified feature list
**
** Description: This method is called whenever the line features have changed due to
**              state changes.
**
*****************************************************************************/
DWORD CDSLine::OnLineFeaturesChanged(DWORD dwFeatures)
{
	// Don't allow pickup/makecall/unpark if there are any active calls.
	if (GetLineDevStatus()->dwNumActiveCalls > 0)
		dwFeatures &= ~(LINEFEATURE_MAKECALL | LINEFEATURE_FORWARD);

	// Force the address to recalc its features in case we changed them.
	for (int i = 0; i < (int) GetAddressCount(); i++)
		GetAddress(i)->RecalcAddrFeatures();

	return dwFeatures;

}// CDSLine::OnLineFeaturesChanged

/*****************************************************************************
** Procedure:  CDSLine::HandleDialEvent
**
** Arguments:  'pReq' - Request this dial event was generated from.
**             'wResponse' - Current emulator response
**             'lpBuff' - Data structure (response based)
**             'parrAddress' - Addresses we are dialing to
**             'dwCountryCode' - Country code we are dialing to
**
** Returns:    TRUE/FALSE success indicator
**
** Description: Master function to process a dial request.  This is called from
**				both the lineMakeCall events and the lineDial events.
**
*****************************************************************************/
BOOL CDSLine::HandleDialEvent (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff,
                               CADObArray* parrAddress, DWORD dwCountryCode)
{                                   
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    DIALINFO* pDialInfo = (DIALINFO*) parrAddress->GetAt(0);
    BOOL fProcessed = FALSE, fPartialAddr = pDialInfo->fIsPartialAddress;
	BOOL fAlreadyDialing = (pCall->GetCallState() == LINECALLSTATE_DIALING || pCall->GetCallState() == LINECALLSTATE_PROCEEDING);
    
    switch (pReq->GetState())
    {   
        // State 1:
        // Send a dial string to the emulator device.  We only send the 
        // first address given in the object array.  Country code is
        // ignored.
        case STATE_INITIAL:           
        case STATE_DIALING:
            pReq->SetState (STATE_WAITFORONLINE);
            if (GetDeviceInfo()->DRV_Dial (pAddr->GetAddressID(), pDialInfo, dwCountryCode))
			{
				// If the call was already in the DIALING state, then we won't see a 
				// address online change.
				if (fAlreadyDialing)
				{
					pCall->SetCallState((fPartialAddr) ? LINECALLSTATE_DIALING : LINECALLSTATE_PROCEEDING);
                    CompleteRequest (pReq, 0);
				}
			}
            fProcessed = TRUE;
            break;

        // State 2:
        // Look for the address moving to a "ONLINE" state.  This indicates
        // that we are not connected to a call, and not in dialtone, but
        // transitioning somehow between them.
        case STATE_WAITFORONLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    fProcessed = TRUE;                        
                    if (lpAddrChange->wNewState == ADDRESSSTATE_ONLINE)
                    {
						// If the end of the string is a ';', then more digits
						// are to follow, leave the call in the DIALING state.
						if (fPartialAddr)
							pCall->SetCallState(LINECALLSTATE_DIALING);

                        // Move the the "proceeding" state on this call.
						else
							pCall->SetCallState(LINECALLSTATE_PROCEEDING);

                        // Go ahead and complete the request.  If the call changes
                        // to something other than "ONLINE", then the Update function
                        // will change the callstate.
                        CompleteRequest (pReq, 0);
                    }
                    else if (lpAddrChange->wNewState == ADDRESSSTATE_DIALTONE)
					{
						// Move to the DIALTONE state.
						pCall->SetCallState(LINECALLSTATE_DIALTONE, (lpAddrChange->wStateInfo == DIALTONETYPE_INTERNAL) ?
									LINEDIALTONEMODE_INTERNAL : LINEDIALTONEMODE_EXTERNAL);
						if (lpAddrChange->wStateInfo == DIALTONETYPE_EXTERNAL)
						{
							// Assign the TRUNK id based on the address + 1.
							pCall->SetTrunkID(lpAddrChange->wAddressID + 1);
						}
					}
                }
            }
            break;                
        
        default:
            ASSERT(FALSE);
            break;
    }

    // If we failed, then idle the call appearance.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSProvider::HandleDialEvent
