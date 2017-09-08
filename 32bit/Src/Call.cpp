/******************************************************************************/
//                                                                        
// CALL.CPP - Source code for the CTSPICallAppearance object.
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code for managing the call appearance       
// objects which are controlled by the CTSPIAddressInfo object.
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

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////
// Run-Time class information 

IMPLEMENT_DYNCREATE( CTSPICallAppearance, CObject )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::CTSPICallAppearance
//
// Constructor
//
CTSPICallAppearance::CTSPICallAppearance() : 
	m_pAddr(NULL), m_htCall(0), m_iCallType(CALLTYPE_NORMAL), m_lpGather(NULL),
	m_lpMediaControl(NULL), m_pConsult(NULL), m_pConf(NULL),
	m_lpvCallData(NULL), m_dwCallDataSize(0), m_lRefCount(1),
	m_dwReceivingFlowSpecSize(0), m_dwSendingFlowSpecSize(0),
	m_lpvSendingFlowSpec(NULL), m_lpvReceivingFlowSpec(NULL), m_dwFlags(0)
{
    FillBuffer (&m_CallInfo, 0, sizeof(LINECALLINFO));
    FillBuffer (&m_CallStatus, 0, sizeof(LINECALLSTATUS));

}// CTSPICallAppearance::CTSPICallAppearance

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::~CTSPICallAppearance
//
// Destructor
//
CTSPICallAppearance::~CTSPICallAppearance()
{
	DeleteToneMonitorList();
	
    // Delete any outstanding gathering or monitoring buffers.
    delete m_lpGather;

	// Decrement our media control usage.
	if (m_lpMediaControl != NULL)
		m_lpMediaControl->DecUsage();

	// Delete any calldata
	if (m_lpvCallData != NULL)
		FreeMem ((LPTSTR)m_lpvCallData);

	// Delete any FLOWSPEC for Quality of Service information.
	if (m_dwSendingFlowSpecSize > 0)
		FreeMem ((LPTSTR)m_lpvSendingFlowSpec);
	if (m_dwReceivingFlowSpecSize > 0)
		FreeMem ((LPTSTR)m_lpvReceivingFlowSpec);

	// Delete any pending events
   	for (int i = 0; i < m_arrEvents.GetSize(); i++)
    {
    	TIMEREVENT* lpEvent = (TIMEREVENT*) m_arrEvents[i];
		delete lpEvent;    	
    }
    m_arrEvents.RemoveAll();

}// CTSPICallAppearance::~CTSPICallAppearance

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Init
//
// Initialize the call appearance from our address owner.
//
VOID CTSPICallAppearance::Init (CTSPIAddressInfo* pAddr, HTAPICALL hCall, 
                                DWORD dwBearerMode,
                                DWORD dwRate, DWORD dwCallParamFlags, DWORD dwOrigin, 
                                DWORD dwReason, DWORD dwTrunk, DWORD dwCompletionID, 
                                BOOL fNewCall)
{
    m_pAddr = pAddr;
    m_htCall = hCall;

	// Mark whether we created this call or not.
    if (fNewCall)
		m_dwFlags |= IsNewCall;
    
    m_CallInfo.dwNeededSize = sizeof(LINECALLINFO);
    m_CallInfo.dwLineDeviceID = GetLineOwner()->GetDeviceID();
    m_CallInfo.dwAddressID = GetAddressOwner()->GetAddressID();
    m_CallInfo.dwBearerMode = dwBearerMode;
    m_CallInfo.dwRate = dwRate;
    m_CallInfo.dwMediaMode = 0;     // Will be set by 1st SetCallState
    m_CallInfo.dwCallID = 0;        // This may be used by the derived service provider.
    m_CallInfo.dwRelatedCallID = 0; // This may be used by the derived service provider.
    
    // Available Call states
    m_CallInfo.dwCallStates = (LINECALLSTATE_IDLE | LINECALLSTATE_CONNECTED | LINECALLSTATE_UNKNOWN |
                                  LINECALLSTATE_PROCEEDING | LINECALLSTATE_DISCONNECTED | 
                                  LINECALLSTATE_BUSY);
    if (GetAddressOwner()->CanMakeCalls() && CanHandleRequest(TSPI_LINEMAKECALL))
        m_CallInfo.dwCallStates |= (LINECALLSTATE_DIALING | LINECALLSTATE_DIALTONE | LINECALLSTATE_RINGBACK);
    if (GetAddressOwner()->CanAnswerCalls() && CanHandleRequest(TSPI_LINEANSWER))
        m_CallInfo.dwCallStates |= (LINECALLSTATE_OFFERING | LINECALLSTATE_ACCEPTED);
    if (CanHandleRequest(TSPI_LINEADDTOCONFERENCE))
        m_CallInfo.dwCallStates |= (LINECALLSTATE_CONFERENCED | LINECALLSTATE_ONHOLDPENDCONF);
    if (CanHandleRequest(TSPI_LINECOMPLETETRANSFER))
        m_CallInfo.dwCallStates |= LINECALLSTATE_ONHOLDPENDTRANSFER;

    // Origin/reason codes
    m_CallInfo.dwOrigin = dwOrigin;
    m_CallInfo.dwReason = dwReason;
    m_CallInfo.dwTrunk  = dwTrunk;
    m_CallInfo.dwCompletionID = dwCompletionID;     
    m_CallInfo.dwCallParamFlags = dwCallParamFlags;

    // Caller id information (unknown right now).
    m_CallInfo.dwCallerIDFlags = 
    m_CallInfo.dwCalledIDFlags = 
    m_CallInfo.dwConnectedIDFlags = 
    m_CallInfo.dwRedirectingIDFlags =
    m_CallInfo.dwRedirectionIDFlags = LINECALLPARTYID_UNKNOWN;

    // Initialize the call status to unknown
    m_CallStatus.dwNeededSize = sizeof(LINECALLSTATUS);
    m_CallStatus.dwCallState = LINECALLSTATE_UNKNOWN;

	// Grab the terminal information from our address parent.
	for (int i = 0; i < GetLineOwner()->GetTerminalCount(); i++)
		m_arrTerminals.Add (m_pAddr->GetTerminalInformation(i));

}// CTSPICallAppearance::Init

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnRequestComplete
//
// This method is called by the request object when an outstanding
// request completes.
//
VOID CTSPICallAppearance::OnRequestComplete (CTSPIRequest* pReq, LONG lResult)
{   
    WORD wCommand = pReq->GetCommand();
    
    // If this is a request to set the caller parameters, then set
    // the various fields of the CALL STATUS structure and notify
    // TAPI.
    if (wCommand == REQUEST_SETCALLPARAMS)
    {
        if (lResult == 0)
        {
            TSPICALLPARAMS* pCallParam = (TSPICALLPARAMS*) pReq->GetDataPtr();
            ASSERT (pCallParam != NULL);
            SetBearerMode (pCallParam->dwBearerMode);
            SetDialParameters (pCallParam->DialParams);
        }        
    } 
    
	// If this was a request to set the call treatment for unanswered calls,
	// and it completed ok, adjust our LINECALLINFO record
	else if (wCommand == REQUEST_SETCALLTREATMENT)
	{
		if (lResult == 0)
		{
			DWORD dwCallTreatment = pReq->GetDataSize();
			SetCallTreatment (dwCallTreatment);
		}
	}

	// If this is a lineSetMediaControl event, then store the new MediaControl
	// information in the address (and all of it's calls)
	else if (wCommand == REQUEST_MEDIACONTROL)
	{
		if (lResult == 0)
			SetMediaControl((TSPIMEDIACONTROL*)pReq->GetDataPtr());
	}

    // If this is a request to change the terminal information for the
    // call, and it completed ok, then reset our internal terminal array.
    else if (wCommand == REQUEST_SETTERMINAL)
    {
        if (lResult == 0)
        {
            TSPILINESETTERMINAL* pTermStruct = (TSPILINESETTERMINAL*) pReq->GetDataPtr();
            ASSERT (pTermStruct != NULL);
            if (pTermStruct->pCall != NULL)
            	SetTerminalModes ((int)pTermStruct->dwTerminalID, pTermStruct->dwTerminalModes, pTermStruct->bEnable);
        }
    }

    else if (wCommand == REQUEST_DROPCALL)
	{
		if (lResult == 0)
		{
			// Remove all pending requests for the call.
			// This can be called to close a call handle which has not been 
			// completely setup (ie: the original asynch request which created the
			// call hasn't completely finished).  In this case, we need to return
			// LINEERR_OPERATIONFAILED for each pending request.
			GetLineOwner()->RemovePendingRequests (this, REQUEST_ALL, LINEERR_OPERATIONFAILED, false, pReq);
		}
		else
		{
			// Unmark the drop flags
			m_dwFlags &= ~IsDropped;
		}
	}

	// If this is a request to ACCEPT a call, and the TSP completed it successfully,
	// then change the call state to accepted.
	else if (wCommand == REQUEST_ACCEPT)
	{
		if (lResult == 0)
			SetCallState(LINECALLSTATE_ACCEPTED);
	}

    // If this is a request to secure the call, and it completed ok, then
    // set the status bits.
    else if (wCommand == REQUEST_SECURECALL)
    {
        if (lResult == 0)
            SetCallParameterFlags (m_CallInfo.dwCallParamFlags | LINECALLPARAMFLAGS_SECURE);
    }   

	// If this is a request to swap hold with another call appearance, and the
	// other call appearance (or this one) is a consultant call created for conferencing,
	// then swap the call types.
	else if (wCommand == REQUEST_SWAPHOLD)
	{
		if (lResult == 0)
		{
			// Don't swap conference and consultation calls (V2.21b)
			CTSPICallAppearance* pCall = (CTSPICallAppearance*) pReq->GetDataPtr();
			if (pCall && GetAttachedCall() == pCall &&
				GetCallType() != CALLTYPE_CONFERENCE && pCall->GetCallType() != CALLTYPE_CONFERENCE)
			{
				int iType = GetCallType();
				SetCallType(pCall->GetCallType());
				pCall->SetCallType(iType);
			}
		}
	}
    
    // If this is a GenerateDigit or GenerateTone request, then complete it
    // with TAPI.
    else if (wCommand == REQUEST_GENERATEDIGITS || wCommand == REQUEST_GENERATETONE)
    {   
        CTSPILineConnection* pLine = GetLineOwner();
        ASSERT (pLine != NULL);
        ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));

        TSPIGENERATE* pInfo = (TSPIGENERATE*) pReq->GetDataPtr();
        if (pLine != NULL)
            pLine->Send_TAPI_Event(this, LINE_GENERATE, (lResult == 0) ?
                           LINEGENERATETERM_DONE : LINEGENERATETERM_CANCEL,
                           pInfo->dwEndToEndID, GetTickCount());
    }
	
	// If this is a SetCallData request which has completed o.k.,
	// then really set the call data into the CALLINFO record.
	else if (wCommand == REQUEST_SETCALLDATA)
	{
		if (lResult == 0)
		{
			TSPICALLDATA* pCallData = (TSPICALLDATA*) pReq->GetDataPtr();
			ASSERT (pCallData != NULL);
			SetCallData (pCallData->lpvCallData, pCallData->dwSize);
		}
	}

    // Or if this is a request to set the quality of service which
	// was successful, then update the internal call record
	// FLOWSPEC information.
	else if (wCommand == REQUEST_SETQOS)
	{
		if (lResult == 0)
		{
			TSPIQOS* pQOS = (TSPIQOS*) pReq->GetDataPtr();
			ASSERT (pQOS != NULL);
			SetQualityOfService (pQOS->lpvSendingFlowSpec, pQOS->dwSendingSize,
								 pQOS->lpvReceivingFlowSpec, pQOS->dwReceivingSize);
		}
	}

}// CTSPICallAppearance::OnRequestComplete

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnCallInfoChange
//
// This method is called when any of the information in our call 
// information record has changed.  It informs TAPI through the
// asynch. callback function in our line.
//
VOID CTSPICallAppearance::OnCallInfoChange (DWORD dwCallInfo)
{
	// If we have no call handle (i.e. we have been deleted) then
	// ignore this event.  This happens when information within the
	// call record is changed by some request but TAPI itself has released 
	// the call handle.
	if (GetCallHandle() == NULL)
		return;

    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));

    if (pLine != NULL)
        pLine->Send_TAPI_Event(this, LINE_CALLINFO, dwCallInfo);

}// CTSPICallAppearance::OnCallInfoChange

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnCallStatusChange
//
// This method is called when the call state information record 
// changes.  It informs TAPI through the asynch. callback function in
// our line owner.
//
VOID CTSPICallAppearance::OnCallStatusChange (DWORD dwCallState, DWORD /*dwCallInfo*/, DWORD /*dwMediaModes*/)
{
	CEnterCode sLock (this);

    // If we are doing media monitoring, then check our list.  Do this before
    // the control lists get deleted due to an IDLE condition.
    if (m_lpMediaControl != NULL && dwCallState != m_CallStatus.dwCallState)
    {
        for (int i = 0; i < m_lpMediaControl->arrCallStates.GetSize(); i++)
        {
            LPLINEMEDIACONTROLCALLSTATE lpCallState = (LPLINEMEDIACONTROLCALLSTATE) m_lpMediaControl->arrCallStates[i];
            if ((lpCallState->dwCallStates & dwCallState) == dwCallState)
                OnMediaControl (lpCallState->dwMediaControl);
        }
    }
  
    // Mark the global call features we support right now.
	DWORD dwCallFeatures = 0L;
	CTSPIAddressInfo* pAddr = GetAddressOwner();
	LPLINEADDRESSCAPS lpAddrCaps = pAddr->GetAddressCaps();
    
    // Get the call features which are available at most any time.    
    dwCallFeatures |= (LINECALLFEATURE_SECURECALL |
    					 LINECALLFEATURE_SETCALLPARAMS |
    					 LINECALLFEATURE_SETMEDIACONTROL |
    					 LINECALLFEATURE_MONITORDIGITS |
    					 LINECALLFEATURE_MONITORMEDIA |
						 LINECALLFEATURE_MONITORTONES |
						 LINECALLFEATURE_SETTREATMENT |
						 LINECALLFEATURE_SETQOS |
						 LINECALLFEATURE_SETCALLDATA |
						 LINECALLFEATURE_DROP);

	// Add RELEASE user information if we have some,
	if (m_arrUserUserInfo.GetSize() > 0)
		dwCallFeatures |= LINECALLFEATURE_RELEASEUSERUSERINFO;

	// Add terminal support
  	if (m_arrTerminals.GetSize() > 0)
        dwCallFeatures |= LINECALLFEATURE_SETTERMINAL;
    
    // Add features available when call is active.
	if (IsConnectedCallState(dwCallState))
	{
    	// Add dialing capabilities
    	if (pAddr->CanMakeCalls() &&
        	(lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_DIALED))
         	dwCallFeatures |= LINECALLFEATURE_DIAL;
         
		dwCallFeatures |= (LINECALLFEATURE_GATHERDIGITS |
        					 LINECALLFEATURE_GENERATEDIGITS |
        					 LINECALLFEATURE_GENERATETONE);
	}

	// Now look specifically at the call state and determine what
	// we can do with the call.  This logic is taken directly from the 
	// TAPI and TSPI specification.
    switch(dwCallState)
    {
        case LINECALLSTATE_IDLE:
            // Turn off all monitoring and gathering in effect.
            m_CallInfo.dwMonitorDigitModes = 0L;
            m_CallInfo.dwMonitorMediaModes = 0L;
            DeleteToneMonitorList();
            delete m_lpGather;
            m_lpGather = NULL;
			if (m_lpMediaControl != NULL)
			{
				m_lpMediaControl->DecUsage();
				m_lpMediaControl = NULL;
			}

            // Remove any pending requests which have not started.
			GetLineOwner()->RemovePendingRequests (this, REQUEST_ALL, LINEERR_INVALCALLSTATE, TRUE);

            // Fall through intentional.
            
        case LINECALLSTATE_UNKNOWN:
            dwCallFeatures = 0L;
            break;

        case LINECALLSTATE_DISCONNECTED:
            dwCallFeatures = LINECALLFEATURE_DROP;
            break;

		case LINECALLSTATE_ACCEPTED:
        	dwCallFeatures |= (LINECALLFEATURE_REDIRECT | LINECALLFEATURE_SENDUSERUSER);
			if (pAddr->CanAnswerCalls())
				dwCallFeatures |= LINECALLFEATURE_ANSWER;
			break;

        case LINECALLSTATE_OFFERING:
        	dwCallFeatures |= (LINECALLFEATURE_REDIRECT | LINECALLFEATURE_ACCEPT | LINECALLFEATURE_SENDUSERUSER);
            if (pAddr->CanAnswerCalls())
                dwCallFeatures |= LINECALLFEATURE_ANSWER;
            break;

		case LINECALLSTATE_CONFERENCED:
		{
			CTSPIConferenceCall* pConfCall = GetConferenceOwner();
			if (pConfCall &&
				pConfCall->CanRemoveFromConference(this))
				dwCallFeatures |= LINECALLFEATURE_REMOVEFROMCONF;
		}
		break;

        case LINECALLSTATE_DIALTONE:
        case LINECALLSTATE_DIALING:
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_ONHOLD | 
				LINECALLSTATE_ONHOLDPENDTRANSFER | 
				LINECALLSTATE_ONHOLDPENDCONF))
				dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
			break;

        case LINECALLSTATE_ONHOLD:
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_CONNECTED | 
				LINECALLSTATE_DIALTONE | 
				LINECALLSTATE_PROCEEDING | 
				LINECALLSTATE_DIALING | 
				LINECALLSTATE_RINGBACK |
				LINECALLSTATE_BUSY))
				dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
			if ((lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_CONFERENCEHELD) &&
				 GetLineOwner()->IsConferenceAvailable(this))
				dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
			if (GetLineOwner()->IsTransferConsultAvailable(this) &&
				(lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERHELD))
				dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
			dwCallFeatures |= LINECALLFEATURE_UNHOLD;
			break;

        case LINECALLSTATE_ONHOLDPENDCONF:
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_CONNECTED | 
				LINECALLSTATE_DIALTONE | 
				LINECALLSTATE_PROCEEDING | 
				LINECALLSTATE_DIALING | 
				LINECALLSTATE_RINGBACK |
				LINECALLSTATE_BUSY))
				dwCallFeatures |= (LINECALLFEATURE_SWAPHOLD | LINECALLFEATURE_ADDTOCONF);
			dwCallFeatures |= LINECALLFEATURE_UNHOLD;
			break;

        case LINECALLSTATE_ONHOLDPENDTRANSFER:
			if (GetLineOwner()->IsTransferConsultAvailable(this))
				dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_CONNECTED | 
				LINECALLSTATE_DIALTONE | 
				LINECALLSTATE_PROCEEDING | 
				LINECALLSTATE_DIALING | 
				LINECALLSTATE_RINGBACK |
				LINECALLSTATE_BUSY))
				dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
			dwCallFeatures |= LINECALLFEATURE_UNHOLD;
			break;

        case LINECALLSTATE_RINGBACK:
			if (GetLineOwner()->IsConferenceAvailable(this))
				dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;                
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_ONHOLD | 
				LINECALLSTATE_ONHOLDPENDTRANSFER | 
				LINECALLSTATE_ONHOLDPENDCONF))
				dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
			if (GetLineOwner()->FindCallByState(LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLD) &&
					(lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERHELD))
				dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
			dwCallFeatures |= (LINECALLFEATURE_COMPLETECALL | LINECALLFEATURE_SENDUSERUSER);
			break;

        case LINECALLSTATE_BUSY:
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_ONHOLD | 
				LINECALLSTATE_ONHOLDPENDTRANSFER | 
				LINECALLSTATE_ONHOLDPENDCONF))
				dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
			dwCallFeatures |= LINECALLFEATURE_COMPLETECALL;
			break;

        case LINECALLSTATE_PROCEEDING:
			if (GetLineOwner()->IsConferenceAvailable(this))
				dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_ONHOLD | 
				LINECALLSTATE_ONHOLDPENDTRANSFER | 
				LINECALLSTATE_ONHOLDPENDCONF))
				dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
			dwCallFeatures |= LINECALLFEATURE_HOLD;
			break;

        case LINECALLSTATE_CONNECTED:
			if (GetCallType() != CALLTYPE_CONFERENCE)
			{
				if (GetLineOwner()->IsConferenceAvailable(this) &&
					GetConferenceOwner() == NULL)
					dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
				else
					dwCallFeatures |= LINECALLFEATURE_SETUPCONF;
			}
			else
				dwCallFeatures |= LINECALLFEATURE_PREPAREADDCONF;
			if ((pAddr->GetAddressStatus()->dwNumOnHoldCalls <
				pAddr->GetAddressCaps()->dwMaxNumOnHoldCalls) &&
				(pAddr->GetAddressStatus()->dwNumOnHoldPendCalls <
				pAddr->GetAddressCaps()->dwMaxNumOnHoldPendingCalls))
				dwCallFeatures |= LINECALLFEATURE_SETUPTRANSFER;
			dwCallFeatures |= LINECALLFEATURE_PARK;
			if (GetLineOwner()->FindCallByState(
				LINECALLSTATE_ONHOLD | 
				LINECALLSTATE_ONHOLDPENDTRANSFER | 
				LINECALLSTATE_ONHOLDPENDCONF))
				dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
			if (GetLineOwner()->FindCallByState(LINECALLSTATE_ONHOLDPENDTRANSFER) ||
				(GetLineOwner()->FindCallByState(LINECALLSTATE_ONHOLD) && 
					(lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERHELD)))
				dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
			dwCallFeatures |= (
				LINECALLFEATURE_BLINDTRANSFER |
				LINECALLFEATURE_HOLD |
				LINECALLFEATURE_SENDUSERUSER);
			break;

        default:
            break;
    }

	// Pull out all the feature bits which are not supported
	// by the address owner.
	dwCallFeatures &= lpAddrCaps->dwCallFeatures;

	// If this is a consultation call created for a transfer/conference
	// event then exclude the creation of another consultation based on this one.
	if (GetCallType() == CALLTYPE_CONSULTANT)
		dwCallFeatures &= ~(LINECALLFEATURE_SETUPTRANSFER | LINECALLFEATURE_SETUPCONF);

	// Allow the derived service provider to adjust this list.
	SetCallFeatures (pAddr->OnCallFeaturesChanged(this, dwCallFeatures));

}// CTSPICallAppearance::OnCallStatusChange

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::NotifyCallStatusChanged
//
// Function to tell TAPI that something in our callstatus structure
// has changed.
//  
void CTSPICallAppearance::NotifyCallStatusChanged()
{
	// No call handle?  TAPI doesn't care..
	if (GetCallHandle() == NULL)
		return;

	CTSPILineConnection* pLine = GetLineOwner();
	DWORD dwVersion = pLine->GetNegotiatedVersion();
	if (dwVersion == 0)
		dwVersion = GetSP()->GetSupportedVersion();

	// Determine what the parameters are for this callstate.
	DWORD dwP2 = 0;
	switch (m_CallStatus.dwCallState)
	{   
		case LINECALLSTATE_CONNECTED:
			dwP2 = m_CallStatus.dwCallStateMode;
			if (dwVersion < TAPIVER_14)
				dwP2 = 0;
			else if (dwVersion < TAPIVER_20 &&
				(dwP2 & (LINECONNECTEDMODE_ACTIVEHELD |
					LINECONNECTEDMODE_INACTIVEHELD |
					LINECONNECTEDMODE_CONFIRMED)))
				dwP2 = LINECONNECTEDMODE_ACTIVE;
			break;

		case LINECALLSTATE_DISCONNECTED:
			dwP2 = m_CallStatus.dwCallStateMode;
			if ((dwVersion < TAPIVER_14) &&
				dwP2 == LINEDISCONNECTMODE_NODIALTONE)
				dwP2 = LINEDISCONNECTMODE_UNKNOWN;
			else if (dwVersion < TAPIVER_20 &&
				(dwP2 & (LINEDISCONNECTMODE_NUMBERCHANGED |
					LINEDISCONNECTMODE_OUTOFORDER |
					LINEDISCONNECTMODE_TEMPFAILURE |
					LINEDISCONNECTMODE_QOSUNAVAIL |
					LINEDISCONNECTMODE_BLOCKED |
					LINEDISCONNECTMODE_DONOTDISTURB |
					LINEDISCONNECTMODE_CANCELLED)))
				dwP2 = LINEDISCONNECTMODE_UNKNOWN;
			break;

		case LINECALLSTATE_BUSY:
		case LINECALLSTATE_DIALTONE:
		case LINECALLSTATE_SPECIALINFO:
		case LINECALLSTATE_OFFERING:
			dwP2 = m_CallStatus.dwCallStateMode;
			break;

		// TAPI 1.4 extension
		case LINECALLSTATE_CONFERENCED:               
			// If the call was CREATED by us (i.e. a new call) which
			// TAPI may not know the conference owner of, then send the
			// conference owner in dwP2.
			{
				CTSPIConferenceCall* pConf = GetConferenceOwner();
				ASSERT(pConf != NULL && pConf->GetCallType() == CALLTYPE_CONFERENCE);
				if (pConf != NULL)
					dwP2 = reinterpret_cast<DWORD>(pConf->GetCallHandle());
				break;
			}
        
		default:
			break;
	}

	pLine->Send_TAPI_Event(this, LINE_CALLSTATE, m_CallStatus.dwCallState, dwP2, m_CallInfo.dwMediaMode);

}// CTSPICallAppearance::NotifyCallStatusChanged

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallFeatures
//
// Set the current features of the call w/o callng OnCallFeaturesChanged.
//
VOID CTSPICallAppearance::SetCallFeatures (DWORD dwFeatures, BOOL fNotifyTAPI)
{
	// Make sure the features are in the ADDRESSCAPS.
    CTSPIAddressInfo* pAddr = GetAddressOwner();
    if ((pAddr->GetAddressCaps()->dwCallFeatures & dwFeatures) != dwFeatures)
	{
		DTRACE(TRC_MIN, _T("LINEADDRESSCAPS.dwCallFeatures doesn't have 0x%lx in it.\r\n"), dwFeatures);
		ASSERT (FALSE);
		dwFeatures &= pAddr->GetAddressCaps()->dwCallFeatures;
	}

	if (dwFeatures != m_CallStatus.dwCallFeatures)
	{
		// Check to see if our "changing" state bit is set, if so, we
		// don't need to send a "state changed" message here.
		BOOL fTellTapi = ((m_dwFlags & IsChgState) == 0);

		CEnterCode sLock(this);
		m_CallStatus.dwCallFeatures = dwFeatures;
		sLock.Unlock();

		// Now adjust our CALLFEATURE2 field based on what our current features are.
		// We can't SET any of the bits here (not enouugh info) but we can take some away.
		SetCallFeatures2(m_CallStatus.dwCallFeatures2, fNotifyTAPI);

		// Tell TAPI that they changed.
		if (fTellTapi && fNotifyTAPI)
			NotifyCallStatusChanged();
	}
	
}// CTSPICallAppearance::SetCallFeatures

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallFeatures2
//
// Set the current secondary features of the call
//
void CTSPICallAppearance::SetCallFeatures2(DWORD dwCallFeatures2, BOOL fNotify /*=TRUE*/)
{
	if (GetAddressOwner()->GetAddressCaps()->dwCallFeatures2)
	{
		CEnterCode sLock(this);
		DWORD dwFeatures = m_CallStatus.dwCallFeatures;
		DWORD dwCurrFeatures = m_CallStatus.dwCallFeatures2;

		CTSPIAddressInfo* pAddr = GetAddressOwner();
		if (dwCallFeatures2 != 0 && (pAddr->GetAddressCaps()->dwCallFeatures2 & dwCallFeatures2) != dwCallFeatures2)
		{
			TRACE(_T("LINEADDRESSCAPS.dwCallFeatures2 doesn't have 0x%lx in it.\n"), dwCallFeatures2);
			dwCallFeatures2 &= pAddr->GetAddressCaps()->dwCallFeatures;
		}

		// Remove any non-applicable bits based on our current call features.
		if ((dwFeatures & LINECALLFEATURE_SETUPCONF) == 0)
			m_CallStatus.dwCallFeatures2 &= ~LINECALLFEATURE2_NOHOLDCONFERENCE;
		if ((dwFeatures & LINECALLFEATURE_SETUPTRANSFER ) == 0)
			m_CallStatus.dwCallFeatures2 &= ~LINECALLFEATURE2_ONESTEPTRANSFER;
		if ((dwFeatures & LINECALLFEATURE_COMPLETECALL) == 0)
			m_CallStatus.dwCallFeatures2 &= ~(LINECALLFEATURE2_COMPLCAMPON |
									LINECALLFEATURE2_COMPLCALLBACK |
									LINECALLFEATURE2_COMPLINTRUDE |
									LINECALLFEATURE2_COMPLMESSAGE);
		if ((dwFeatures & LINECALLFEATURE_COMPLETETRANSF) == 0)
			m_CallStatus.dwCallFeatures2 &= ~(LINECALLFEATURE2_TRANSFERNORM |
									LINECALLFEATURE2_TRANSFERCONF);
		if ((dwFeatures & LINECALLFEATURE_PARK) == 0)
			m_CallStatus.dwCallFeatures2 &= ~(LINECALLFEATURE2_PARKDIRECT |
									LINECALLFEATURE2_PARKNONDIRECT);

		// Notify TAPI if the features changed...
		if (dwCurrFeatures != m_CallStatus.dwCallFeatures2)
		{
			BOOL fTellTapi = ((m_dwFlags & IsChgState) == 0);
			if (fTellTapi && fNotify)
			{
				sLock.Unlock();
				NotifyCallStatusChanged();
			}
		}
	}

}// CTSPICallAppearance::SetCallFeatures2

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DeleteToneMonitorList
//
// Delete the list of tones we are monitoring for.     
//
VOID CTSPICallAppearance::DeleteToneMonitorList()
{
	CEnterCode sLock(this);  // Synch access to object
    for (int i = 0; i < m_arrMonitorTones.GetSize(); i++)
    {
        TSPITONEMONITOR* lpTone = (TSPITONEMONITOR*) m_arrMonitorTones[i];
        delete lpTone;
    }   
    m_arrMonitorTones.RemoveAll();

	// Delete any events for tones.
	for (i = 0; i < m_arrEvents.GetSize(); i++)
	{                        
    	TIMEREVENT* lpEvent = (TIMEREVENT*) m_arrEvents[i];
    	if (lpEvent->iEventType == TIMEREVENT::ToneDetect)
    	{
    		delete lpEvent;
     		m_arrEvents.RemoveAt(i);
    		i--;
    	}
    }                 
                        
}// CTSPICallAppearance::DeleteToneMonitorList
            
////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Close
//
// This method is called to close the call appearance.  Once this is
// completed, the call appearance will be invalid (and deleted).
// This is called during lineCloseCall.
//
LONG CTSPICallAppearance::Close()
{   
	// Wait for any drop request to complete.
	GetLineOwner()->WaitForAllRequests (this, REQUEST_DROPCALL);

	// Reset the call handle
	m_htCall = 0;

	// Deallocate the call appearance from the address if it is IDLE.
	// Otherwise, we will wait until the call DOES go idle.
	if (GetCallState() == LINECALLSTATE_IDLE)
		GetAddressOwner()->RemoveCallAppearance(this);

    // 'this' pointer might now be invalid!
    return FALSE;

}// CTSPICallAppearance::Close

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Drop
//
// Drop the call appearance and transition the call to IDLE.  The
// USERUSER info is local to our SP.  This is called by lineDrop and
// the Close() method.
//
LONG CTSPICallAppearance::Drop(DRV_REQUESTID dwRequestId, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
	if (dwRequestId > 0 && (GetCallStatus()->dwCallFeatures & LINECALLFEATURE_DROP) == 0)
		return LINEERR_OPERATIONUNAVAIL;

	// If the DROP flag is already set, the call is probably in the process of
	// being dropped - otherwise it would be in a state reflective of a dropped
	// call (Idle).
	if (m_dwFlags & IsDropped)
		return 0; // Outlook2K now returns an error if we give an error back here!

	// Mark the call as dropped so we never allow a second drop.
	m_dwFlags |= IsDropped;

    // Make sure the call state allows this call to be dropped.
    // According to the TAPI spec, any state except IDLE can
    // be dropped.
    if (GetCallState() != LINECALLSTATE_IDLE)
    {
        // Submit the drop request
        if (!AddAsynchRequest(REQUEST_DROPCALL, dwRequestId, lpsUserUserInfo, dwSize))
            return LINEERR_OPERATIONFAILED;
        return dwRequestId;
    }
    return LINEERR_INVALCALLSTATE;
    
}// CTSPICallAppearance::Drop

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Accept
//
// Accepts ownership of the specified offered call. It may 
// optionally send the specified user-to-user information to the 
// calling party.  In some environments (such as ISDN), the station 
// will not begin to ring until the call is accepted.
//
LONG CTSPICallAppearance::Accept(DRV_REQUESTID dwRequestID, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_ACCEPT) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_OFFERING)
        return LINEERR_INVALCALLSTATE;
#endif

    // Everything seems ok, submit the accept request.
    if (AddAsynchRequest(REQUEST_ACCEPT, dwRequestID, lpsUserUserInfo, dwSize) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Accept

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SendUserUserInfo
//
// This function can be used to send user-to-user information at any time 
// during a connected call. If the size of the specified information to be 
// sent is larger than what may fit into a single network message (as in ISDN),
// the service provider is responsible for dividing the information into a 
// sequence of chained network messages (using "more data").
//
// The recipient of the UserUser information will receive a LINECALLINFO
// message with the 'dwUserUserInfoxxx' fields filled out on the 
// lineGetCallInfo function.
//
LONG CTSPICallAppearance::SendUserUserInfo (DRV_REQUESTID dwRequestID, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SENDUSERUSER) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Check the call state of this call and make sure it is connected
    // to a call.  Added new callstates (TAPI bakeoff fix).
    if ((GetCallState() & 
			(LINECALLSTATE_CONNECTED |
			 LINECALLSTATE_OFFERING |
			 LINECALLSTATE_ACCEPTED |
			 LINECALLSTATE_RINGBACK)) == 0)
        return LINEERR_INVALCALLSTATE;
#endif

    // Submit the request
    if (AddAsynchRequest(REQUEST_SENDUSERINFO, dwRequestID, lpsUserUserInfo, dwSize) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::SendUserUserInfo

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Secure
//
// Secures the call from any interruptions or interference that may 
// affect the call's media stream.
//
LONG CTSPICallAppearance::Secure (DRV_REQUESTID dwReqId)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SECURECALL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If this call is ALREADY secure, then ignore the request.
    DWORD dwCallFlags = m_CallInfo.dwCallParamFlags;
    if (dwCallFlags & LINECALLPARAMFLAGS_SECURE)
        return 0L;

#if STRICT_CALLSTATES
    // Check the call state of this call and make sure it is not idle.
    if (GetCallState() == LINECALLSTATE_IDLE)
        return LINEERR_INVALCALLSTATE;
#endif

    // Submit the request
    if (AddAsynchRequest(REQUEST_SECURECALL, dwReqId) != NULL)
        return (LONG) dwReqId;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Secure

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Answer
//
// Answer the specified OFFERING call
//
LONG CTSPICallAppearance::Answer(DRV_REQUESTID dwReq, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_ANSWER) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If this call appearance cannot be answered (outgoing only) then report unavailable.
    if (!GetAddressOwner()->CanAnswerCalls())
        return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_OFFERING && GetCallState() != LINECALLSTATE_ACCEPTED)
        return LINEERR_INVALCALLSTATE;
#endif

    // Everything seems ok, submit the answer request.
    if (AddAsynchRequest(REQUEST_ANSWER, dwReq, lpsUserUserInfo, dwSize) != NULL)
        return (LONG) dwReq;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Answer

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::BlindTransfer
//
// Transfer the call to a destination without any consultation call
// being created.  The destination address is local to our SP and was
// allocated through GlobalAlloc.
//
LONG CTSPICallAppearance::BlindTransfer(DRV_REQUESTID dwRequestId, 
                        CADObArray* parrDestAddr, DWORD dwCountryCode)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_BLINDTRANSFER) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Ok, make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_CONNECTED)
        return LINEERR_INVALCALLSTATE;
#endif
    
    // If the destination address has more than one entry, then return
    // an error.
    if (parrDestAddr->GetSize() != 1)
        return LINEERR_INVALADDRESS;
                     
    // Everything seems ok, submit the xfer request.
    if (AddAsynchRequest(REQUEST_BLINDXFER, dwRequestId, parrDestAddr, dwCountryCode) != NULL)
        return (LONG) dwRequestId;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::BlindTransfer

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::CompleteCall
//
// This method is used to specify how a call that could not be
// connected normally should be completed instead.  The network or
// switch may not be able to complete a call because the network
// resources are busy, or the remote station is busy or doesn't answer.
// 
LONG CTSPICallAppearance::CompleteCall (DRV_REQUESTID dwRequestId, LPDWORD lpdwCompletionID,
                                        TSPICOMPLETECALL* lpCompCall)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_COMPLETECALL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Ok, make sure the call state allows this.
    if ((GetCallState() & 
				(LINECALLSTATE_BUSY |
    			 LINECALLSTATE_RINGBACK |
    			 LINECALLSTATE_PROCEEDING)) == 0)
        return LINEERR_INVALCALLSTATE;
#endif

    // Validate the message id and completion mode
    if ((lpCompCall->dwCompletionMode & GetAddressOwner()->GetAddressCaps()->dwCallCompletionModes) == 0)
        return LINEERR_INVALCALLCOMPLMODE;
    if (lpCompCall->dwCompletionMode == LINECALLCOMPLMODE_MESSAGE &&
		lpCompCall->dwMessageId > GetAddressOwner()->GetAddressCaps()->dwNumCompletionMessages)
        return LINEERR_INVALMESSAGEID;

    // If the completion mode is not Message, then zero out the message id.
	if (lpCompCall->dwCompletionMode != LINECALLCOMPLMODE_MESSAGE)
		lpCompCall->dwMessageId = 0;

    // Store off the completion ID
    *lpdwCompletionID = (DWORD)lpCompCall;
    
    // Submit the request to the derived class
    if (AddAsynchRequest(REQUEST_COMPLETECALL, dwRequestId, lpCompCall))
        return (LONG) dwRequestId;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::CompleteCall

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Dial
//
// Dial an outgoing request onto this call appearance
//
LONG CTSPICallAppearance::Dial (DRV_REQUESTID dwRequestID, CADObArray* parrAddresses, DWORD dwCountryCode)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_DIAL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Make sure we are in the appropriate connection state.
    if (GetCallState() == LINECALLSTATE_IDLE ||
        GetCallState() == LINECALLSTATE_DISCONNECTED)
        return LINEERR_INVALCALLSTATE;
#endif
    
    // Set the CALLER id information (us)
	if (m_CallerID.strPartyId.IsEmpty() || m_CallerID.strPartyName.IsEmpty())
		SetCallerIDInformation (LINECALLPARTYID_ADDRESS | LINECALLPARTYID_NAME, 
			GetAddressOwner()->GetDialableAddress(), GetAddressOwner()->GetName());

	// Only set called id information if the call is still dialing.  Otherwise,
	// the digits dialed shouldn't be part of the telephone number and therefore
	// shouldn't be reported.  (v2.04 09/16/97 MCS)
	if ((GetCallState() & (LINECALLSTATE_DIALTONE | LINECALLSTATE_DIALING)) != 0)
	{
		if (parrAddresses->GetSize() > 0)
		{
			DIALINFO* pDialInfo = (DIALINFO*) parrAddresses->GetAt(0);
			DWORD dwAvail = 0;
			if (!pDialInfo->strNumber.IsEmpty())
				dwAvail = LINECALLPARTYID_ADDRESS;
			if (!pDialInfo->strName.IsEmpty())
				dwAvail |= LINECALLPARTYID_NAME;

			// If we have something to store..
			if (dwAvail > 0)
			{
				// If the call state is dialing, then APPEND the caller id
				// information. (06/16/97 MCS)  otherwise, replace it.
				CString strNumber = pDialInfo->strNumber;
				if (GetCallState() == LINECALLSTATE_DIALING)
					strNumber = m_CalledID.strPartyId + strNumber;
				SetCalledIDInformation (dwAvail, strNumber, pDialInfo->strName);
			}
		}            
	}

    // Submit a dial request
    if (AddAsynchRequest(REQUEST_DIAL, dwRequestID, parrAddresses, dwCountryCode) != NULL)
        return (LONG) dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Dial

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Hold
//
// Place the call appearance on hold.
//
LONG CTSPICallAppearance::Hold (DRV_REQUESTID dwRequestID)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_HOLD) == 0)
		return LINEERR_OPERATIONUNAVAIL;

   // Send the request
   if (AddAsynchRequest(REQUEST_HOLD, dwRequestID) != NULL)
      return (LONG) dwRequestID;

   return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Hold

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Park
//
// Park the call at a specified destination address
//
LONG CTSPICallAppearance::Park (DRV_REQUESTID dwRequestID, TSPILINEPARK* lpPark)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_PARK) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Make sure we are in the appropriate connection state.
    if (GetCallState() != LINECALLSTATE_CONNECTED)
        return LINEERR_INVALCALLSTATE;
#endif
    
    // Check whether or not our address supports the park mode requested.
    if ((lpPark->dwParkMode & GetAddressOwner()->GetAddressCaps()->dwParkModes) == 0)
        return LINEERR_INVALPARKMODE;
    
    // Submit the request
    if (AddAsynchRequest(REQUEST_PARK, dwRequestID, lpPark) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Park

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Unpark
//
// Unpark the specified address onto this call
//
LONG CTSPICallAppearance::Unpark (DRV_REQUESTID dwRequestID, CADObArray* parrAddresses)
{
    ASSERT (m_CallInfo.dwReason == LINECALLREASON_UNPARK);
    
    // Verify we have an address to unpark from.
    if (parrAddresses->GetSize() == 0)
        return LINEERR_INVALADDRESS;

    // Submit the request
    if (AddAsynchRequest(REQUEST_UNPARK, dwRequestID, parrAddresses) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONUNAVAIL;

}// CTSPICallAppearance::Unpark

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Pickup
//
// This function picks up a call alerting at the specified destination
// address and returns a call handle for the picked up call.  If invoked
// with a NULL for the 'lpszDestAddr' parameter, a group pickup is performed.
// If required by the device capabilities, 'lpszGroupID' specifies the
// group ID to which the alerting station belongs.
//
LONG CTSPICallAppearance::Pickup (DRV_REQUESTID dwRequestID, TSPILINEPICKUP* lpPickup)
{
    ASSERT (m_CallInfo.dwReason == LINECALLREASON_PICKUP);

    // Setup our callerid information       
    if (lpPickup->arrAddresses.GetSize() > 0)
    {
        DIALINFO* pDialInfo = (DIALINFO*) lpPickup->arrAddresses.GetAt(0);
        DWORD dwAvail = 0;
        if (!pDialInfo->strNumber.IsEmpty())
            dwAvail = LINECALLPARTYID_ADDRESS;
        if (!pDialInfo->strName.IsEmpty())
            dwAvail |= LINECALLPARTYID_NAME;
        if (dwAvail > 0)
            SetCallerIDInformation (dwAvail, pDialInfo->strNumber, pDialInfo->strName);
    }            

    // Submit the request
    if (AddAsynchRequest(REQUEST_PICKUP, dwRequestID, lpPickup) != NULL)
        return (LONG) dwRequestID;
   
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Pickup

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::MakeCall
//
// Make an outgoing call on this call appearance.
//
LONG CTSPICallAppearance::MakeCall (DRV_REQUESTID dwRequestID, TSPIMAKECALL* lpMakeCall)
{
#if STRICT_CALLSTATES
    // If the call is in the wrong call state or not available, return an error.
    if (GetCallState() != LINECALLSTATE_UNKNOWN)
        return LINEERR_INVALCALLSTATE;                
#endif
         
    // If we cannot dial out, give an error
    if (!GetAddressOwner()->CanMakeCalls())
        return LINEERR_OPERATIONUNAVAIL;                           

    // Store off some known call information pieces.
    m_CallInfo.dwOrigin = LINECALLORIGIN_OUTBOUND;
    m_CallInfo.dwReason = LINECALLREASON_DIRECT;
    m_CallInfo.dwCountryCode = lpMakeCall->dwCountryCode;
    m_CallInfo.dwCompletionID = 0L;

    // Set the bearer and media modes based on the call parameters or
    // default it to the voice settings.
    if (lpMakeCall->lpCallParams)
    {
        m_CallInfo.dwMediaMode = lpMakeCall->lpCallParams->dwMediaMode;
        m_CallInfo.dwBearerMode = lpMakeCall->lpCallParams->dwBearerMode;
        ASSERT (m_CallInfo.dwBearerMode == GetAddressOwner()->GetBearerMode() ||
                m_CallInfo.dwBearerMode == LINEBEARERMODE_PASSTHROUGH);
        m_CallInfo.dwCallParamFlags = lpMakeCall->lpCallParams->dwCallParamFlags;
        CopyBuffer (&m_CallInfo.DialParams, &lpMakeCall->lpCallParams->DialParams, sizeof(LINEDIALPARAMS));

		// If the Call params has calling ID information in it, then use it.
		LPCTSTR lpszBuff = NULL;
		if (lpMakeCall->lpCallParams->dwCallingPartyIDSize > 0 &&
			lpMakeCall->lpCallParams->dwCallingPartyIDOffset > 0)
			lpszBuff = (LPCTSTR)lpMakeCall->lpCallParams + lpMakeCall->lpCallParams->dwCallingPartyIDOffset;
		else
			lpszBuff = GetAddressOwner()->GetName();

		SetCallerIDInformation (LINECALLPARTYID_ADDRESS | LINECALLPARTYID_NAME, GetAddressOwner()->GetDialableAddress(), lpszBuff);
    }   
    else
    {   
        // Otherwise default to the highest priority for a call which is voice.
        m_CallInfo.dwMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
        m_CallInfo.dwBearerMode = GetAddressOwner()->GetBearerMode();
	    // Set the CALLER id information (us)
		SetCallerIDInformation (LINECALLPARTYID_ADDRESS | LINECALLPARTYID_NAME, GetAddressOwner()->GetDialableAddress(), GetAddressOwner()->GetName());
    }

    // Mark our (the originator) phone as automatically being taken offhook.
    m_CallInfo.dwCallParamFlags |= LINECALLPARAMFLAGS_ORIGOFFHOOK;

    // Setup our initial called id field to the first address within the
    // address list.  The worker code can override this later if necessary.
    if (lpMakeCall->arrAddresses.GetSize() > 0)
    {
        DIALINFO* pDialInfo = (DIALINFO*) lpMakeCall->arrAddresses.GetAt(0);
        DWORD dwAvail = 0;
        if (!pDialInfo->strNumber.IsEmpty())
            dwAvail = LINECALLPARTYID_ADDRESS;
        if (!pDialInfo->strName.IsEmpty())
            dwAvail |= LINECALLPARTYID_NAME;
        if (dwAvail > 0)
            SetCalledIDInformation (dwAvail, pDialInfo->strNumber, pDialInfo->strName);
    }            

    // Submit a call request
    if (AddAsynchRequest(REQUEST_MAKECALL, dwRequestID, lpMakeCall))
        return (LONG) dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::MakeCall

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Redirect
//
// This function redirects the specified offering call to the specified
// destination address.
//
LONG CTSPICallAppearance::Redirect (
DRV_REQUESTID dwRequestID,             // Asynch. request id
CADObArray* parrAddresses,             // Destination to direct to
DWORD dwCountryCode)                   // Country of destination
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_REDIRECT) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Make sure we are in the appropriate connection state.
    if (GetCallState() != LINECALLSTATE_OFFERING)
        return LINEERR_INVALCALLSTATE;
#endif

    // Store off the redirecting ID (us)
    SetRedirectingIDInformation (LINECALLPARTYID_ADDRESS | LINECALLPARTYID_NAME, GetAddressOwner()->GetDialableAddress(), GetAddressOwner()->GetName());

    // Store off the redirection ID (them)
    if (parrAddresses->GetSize() > 0)
    {
        DIALINFO* pDialInfo = (DIALINFO*) parrAddresses->GetAt(0);
        DWORD dwAvail = 0;
        if (!pDialInfo->strNumber.IsEmpty())
            dwAvail = LINECALLPARTYID_ADDRESS;
        if (!pDialInfo->strName.IsEmpty())
            dwAvail |= LINECALLPARTYID_NAME;
        if (dwAvail > 0)
            SetRedirectionIDInformation (dwAvail, pDialInfo->strNumber, pDialInfo->strName);
    }            
    else
        return LINEERR_INVALADDRESS;

    // Submit the request.
    if (AddAsynchRequest(REQUEST_REDIRECT, dwRequestID, parrAddresses, dwCountryCode) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Redirect

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetTerminalModes
//
// This is the function which should be called when a lineSetTerminal is
// completed by the derived service provider class.  It is also invoked by
// the owning address or line if a lineSetTerminal was issued for them.
// This stores or removes the specified terminal from the terminal modes 
// given.  TAPI will be notified.
//
VOID CTSPICallAppearance::SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal)
{
    // Adjust the value in our terminal map.
	CEnterCode sLock(this);  // Synch access to object
    if (iTerminalID < m_arrTerminals.GetSize())
    {
        DWORD dwCurrMode = m_arrTerminals[iTerminalID];
        if (fRouteToTerminal)
            dwCurrMode |= dwTerminalModes;
        else
            dwCurrMode &= ~dwTerminalModes;
        m_arrTerminals.SetAt(iTerminalID, dwCurrMode);
    }

    // Notify TAPI about the change.
    OnCallInfoChange (LINECALLINFOSTATE_TERMINAL);

}// CTSPICallAppearance::SetTerminalModes

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnTerminalCountChanged
//
// The terminal count has changed, either add or remove a terminal
// entry from our array
//
VOID CTSPICallAppearance::OnTerminalCountChanged (BOOL fAdded, int iPos, DWORD dwMode)
{
	CEnterCode sLock(this);  // Synch access to object
    if (fAdded)
        VERIFY (m_arrTerminals.Add (dwMode) == iPos);
    else
        m_arrTerminals.RemoveAt(iPos);

    // Notify TAPI about the change.
    OnCallInfoChange (LINECALLINFOSTATE_TERMINAL);

}// CTSPICallAppearance::OnTerminalCountChanged

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallParams
//
// Set the calling parameters for this call appearance
//
LONG CTSPICallAppearance::SetCallParams (DRV_REQUESTID dwRequestID, TSPICALLPARAMS* lpCallParams)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETCALLPARAMS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Make sure the call state allows changes.
    if (GetCallState() == LINECALLSTATE_IDLE ||
        GetCallState() == LINECALLSTATE_DISCONNECTED)
        return LINEERR_INVALCALLSTATE;
#endif

    if (AddAsynchRequest(REQUEST_SETCALLPARAMS, dwRequestID, lpCallParams) != NULL)
            return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::SetCallParams

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnDetectedNewMediaModes
//
// This method is invoked when a new media mode is detected for a call.
// This happens when the call is set to a new CALLSTATE with a new media
// mode and the LINECALLINFO record changes.  The mode can be a single mode
// (except media mode UNKNOWN), or it can be a combination which
// must include UNKNOWN.
//
// In general, the service provider code should always OR the new type
// into the existing media modes detected, as the application should 
// really be given the opprotunity to change the media mode via lineSetMediaMode.
//
VOID CTSPICallAppearance::OnDetectedNewMediaModes (DWORD dwModes)
{                                             
	CEnterCode sLock(this);  // Synch access to object
    CTSPILineConnection* pLine = GetLineOwner();

	// Remove the UNKNOWN media mode.
	dwModes &= ~LINEMEDIAMODE_UNKNOWN;

	// Do some validations.
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));
    ASSERT ((pLine->GetLineDevCaps()->dwMediaModes & dwModes) == dwModes);
    
    // If media monitoring is enabled, and we hit a mode TAPI
    // is interested in, tell it.
    if (m_CallInfo.dwMonitorMediaModes > 0 && 
        (m_CallInfo.dwMonitorMediaModes & dwModes) == dwModes)
        pLine->Send_TAPI_Event(this, LINE_MONITORMEDIA, dwModes, 0L, GetTickCount());

    // If we are doing media monitoring, then check our list.
    if (m_lpMediaControl != NULL)
    {
        // See if any older timer events which have not yet expired are no longer
        // valid due to a media mode change.
        for (int i = 0; i < m_arrEvents.GetSize(); i++)
        {
            TIMEREVENT* lpEvent = (TIMEREVENT*) m_arrEvents[i];
            if (lpEvent->iEventType == TIMEREVENT::MediaControlMedia)
            {
                if ((lpEvent->dwData2 & dwModes) == 0)
                {
                    m_arrEvents.RemoveAt(i);
                    i--;
                    delete lpEvent;
                }
            }
        }
        
        // Now go through our media events and see if any match up here.
        for (i = 0; i < m_lpMediaControl->arrMedia.GetSize(); i++)
        {
            LPLINEMEDIACONTROLMEDIA lpMedia = (LPLINEMEDIACONTROLMEDIA) m_lpMediaControl->arrMedia[i];
            if (lpMedia->dwMediaModes & dwModes)
            {
                if (lpMedia->dwDuration == 0)
                    OnMediaControl (lpMedia->dwMediaControl);
                else
                {
                    TIMEREVENT* lpTimer = new TIMEREVENT;
                    m_arrEvents.Add (lpTimer);
                    lpTimer->iEventType = TIMEREVENT::MediaControlMedia;
                    lpTimer->dwEndTime = GetTickCount() + lpMedia->dwDuration;
                    lpTimer->dwData1 = lpMedia->dwMediaControl;
                    lpTimer->dwData2 = lpMedia->dwMediaModes;
					GetSP()->AddTimedCall(this);
                }
            }
        }
    }

}// CTSPICallAppearance::OnDetectedNewMediaModes

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetMediaMode
//
// This function is called as a direct result of the 'lineSetMediaMode'
// API.  This will set the new media mode(s) in the CALLINFO structure
// and tell TAPI it has changed.
//
LONG CTSPICallAppearance::SetMediaMode (DWORD dwMediaMode)
{
    // Validate the media mode.  It should be one of the media modes set in our
    // CALLINFO structure.
    if (!GetAddressOwner()->CanSupportMediaModes (dwMediaMode))
        return LINEERR_INVALMEDIAMODE;

    // Adjust the media mode in the call record.  This function is designed to be
    // simply "advisory".  The media mode is not FORCED to be this new mode.
    m_CallInfo.dwMediaMode = dwMediaMode;
    OnCallInfoChange (LINECALLINFOSTATE_MEDIAMODE);
    return FALSE;

}// CTSPICallAppearance::SetMediaMode

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallerIDInformation
//
// Determines the validity and content of the caller party ID 
// information. The caller is the originator of the call.
//
VOID CTSPICallAppearance::SetCallerIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID, LPCTSTR lpszName, DWORD dwCountryCode)
{
	CEnterCode sLock(this);  // Synch access to object

	// Only send if changed.
	if ((dwFlags != m_CallInfo.dwCallerIDFlags) ||
		((dwFlags & LINECALLPARTYID_NAME) && _tcscmp(m_CallerID.strPartyName, lpszName)) ||
		((dwFlags & LINECALLPARTYID_ADDRESS) && _tcscmp(m_CallerID.strPartyId, lpszPartyID)))
	{
		m_CallInfo.dwCallerIDFlags = dwFlags;
		m_CallerID.strPartyName = lpszName;
		m_CallerID.strPartyId = GetSP()->ConvertDialableToCanonical(lpszPartyID, dwCountryCode);
    
		if (m_CallerID.strPartyName.IsEmpty())
			m_CallInfo.dwCallerIDFlags &= ~LINECALLPARTYID_NAME;
		if (m_CallerID.strPartyId.IsEmpty())        
			m_CallInfo.dwCallerIDFlags &= ~LINECALLPARTYID_ADDRESS;
    
		OnCallInfoChange(LINECALLINFOSTATE_CALLERID);
	}

}// CTSPICallAppearance::SetCallerIDInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCalledIDInformation
//
// Determines the validity and content of the called-party ID 
// information. The called party corresponds to the orignally addressed party.
//
VOID CTSPICallAppearance::SetCalledIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID, LPCTSTR lpszName, DWORD dwCountryCode)
{
	CEnterCode sLock(this);  // Synch access to object

	// Only send if changed.
	if ((dwFlags != m_CallInfo.dwCalledIDFlags) ||
		((dwFlags & LINECALLPARTYID_NAME) && _tcscmp(m_CalledID.strPartyName, lpszName)) ||
		((dwFlags & LINECALLPARTYID_ADDRESS) && _tcscmp(m_CalledID.strPartyId, lpszPartyID)))
	{
		m_CallInfo.dwCalledIDFlags = dwFlags;
		m_CalledID.strPartyName = lpszName;
		m_CalledID.strPartyId = GetSP()->ConvertDialableToCanonical(lpszPartyID, dwCountryCode);

		if (m_CalledID.strPartyName.IsEmpty())
			m_CallInfo.dwCalledIDFlags &= ~LINECALLPARTYID_NAME;
		if (m_CalledID.strPartyId.IsEmpty())        
			m_CallInfo.dwCalledIDFlags &= ~LINECALLPARTYID_ADDRESS;

		OnCallInfoChange(LINECALLINFOSTATE_CALLEDID);
	}

}// CTSPICallAppearance::SetCalledIDInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetConnectedIDInformation
//
// Determines the validity and content of the connected party ID 
// information. The connected party is the party that was actually 
// connected to. This may be different from the called-party ID if the 
// call was diverted.
//
VOID CTSPICallAppearance::SetConnectedIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID, LPCTSTR lpszName, DWORD dwCountryCode)
{
	CEnterCode sLock(this);  // Synch access to object

	// Only send if changed.
	if ((dwFlags != m_CallInfo.dwConnectedIDFlags) ||
		((dwFlags & LINECALLPARTYID_NAME) && _tcscmp(m_ConnectedID.strPartyName, lpszName)) ||
		((dwFlags & LINECALLPARTYID_ADDRESS) && _tcscmp(m_ConnectedID.strPartyId, lpszPartyID)))
	{
		m_CallInfo.dwConnectedIDFlags = dwFlags;
		m_ConnectedID.strPartyName = lpszName;
		m_ConnectedID.strPartyId = GetSP()->ConvertDialableToCanonical(lpszPartyID, dwCountryCode);

		if (m_ConnectedID.strPartyName.IsEmpty())
			m_CallInfo.dwConnectedIDFlags &= ~LINECALLPARTYID_NAME;
		if (m_ConnectedID.strPartyId.IsEmpty())        
			m_CallInfo.dwConnectedIDFlags &= ~LINECALLPARTYID_ADDRESS;

		OnCallInfoChange(LINECALLINFOSTATE_CONNECTEDID);
	}

}// CTSPICallAppearance::SetConnectedIDInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetRedirectionIDInformation
//
// Determines the validity and content of the redirection party ID 
// information. The redirection party identifies to the calling user 
// the number towards which diversion was invoked.
//
VOID CTSPICallAppearance::SetRedirectionIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID, LPCTSTR lpszName, DWORD dwCountryCode)
{
	CEnterCode sLock(this);  // Synch access to object

	// Only send if changed.
	if ((dwFlags != m_CallInfo.dwRedirectionIDFlags) ||
		((dwFlags & LINECALLPARTYID_NAME) && _tcscmp(m_RedirectionID.strPartyName, lpszName)) ||
		((dwFlags & LINECALLPARTYID_ADDRESS) && _tcscmp(m_RedirectionID.strPartyId, lpszPartyID)))
	{
		m_CallInfo.dwRedirectionIDFlags = dwFlags;
		m_RedirectionID.strPartyName = lpszName;
		m_RedirectionID.strPartyId = GetSP()->ConvertDialableToCanonical(lpszPartyID, dwCountryCode);

		if (m_RedirectionID.strPartyName.IsEmpty())
			m_CallInfo.dwRedirectionIDFlags &= ~LINECALLPARTYID_NAME;
		if (m_RedirectionID.strPartyId.IsEmpty())        
			m_CallInfo.dwRedirectionIDFlags &= ~LINECALLPARTYID_ADDRESS;

		OnCallInfoChange(LINECALLINFOSTATE_REDIRECTIONID);
	}

}// CTSPICallAppearance::SetRedirectionIDInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetRedirectingIDInformation
//
// Determines the validity and content of the redirecting party ID 
// information. The redirecting party identifies to the diverted-to 
// user the party from which diversion was invoked.
//
VOID CTSPICallAppearance::SetRedirectingIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID, LPCTSTR lpszName, DWORD dwCountryCode)
{
	CEnterCode sLock(this);  // Synch access to object

	// Only send if changed.
	if ((dwFlags != m_CallInfo.dwRedirectingIDFlags) ||
		((dwFlags & LINECALLPARTYID_NAME) && _tcscmp(m_RedirectingID.strPartyName, lpszName)) ||
		((dwFlags & LINECALLPARTYID_ADDRESS) && _tcscmp(m_RedirectingID.strPartyId, lpszPartyID)))
	{
		m_CallInfo.dwRedirectingIDFlags = dwFlags;
		m_RedirectingID.strPartyName = lpszName;
		m_RedirectingID.strPartyId = GetSP()->ConvertDialableToCanonical(lpszPartyID, dwCountryCode);

		if (m_RedirectingID.strPartyName.IsEmpty())
			m_CallInfo.dwRedirectingIDFlags &= ~LINECALLPARTYID_NAME;
		if (m_RedirectingID.strPartyId.IsEmpty())        
			m_CallInfo.dwRedirectingIDFlags &= ~LINECALLPARTYID_ADDRESS;

		OnCallInfoChange(LINECALLINFOSTATE_REDIRECTINGID);
	}

}// CTSPICallAppearance::SetRedirectingIDInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallState
//
// This function sets the current connection state for the device
// and notifies TAPI if asked to.
//
VOID CTSPICallAppearance::SetCallState(DWORD dwState, DWORD dwMode, DWORD dwMediaMode, BOOL fTellTapi)
{
	CEnterCode sLock(this);  // Synch access to object
    DWORD dwCurrState = m_CallStatus.dwCallState;
    DWORD dwCurrMode = m_CallStatus.dwCallStateMode;

	// If the current call state is IDLE, and we are moving out of IDLE, report an error.
	// According to information obtained at the TAPI bakeoff, a call appearances cannot
	// transition out of the idle state.
	if (dwCurrState == LINECALLSTATE_IDLE && dwState != LINECALLSTATE_IDLE)
	{
#ifdef _DEBUG		
		TRACE(_T("Attempt to set call 0x%lx (CallID 0x%lx) to %s from IDLE ignored\n"),
				this, m_CallInfo.dwCallID, GetCallStateName(dwState));
#endif
		return;
	}

	// Mark us as in the process of changing state.
	m_dwFlags |= IsChgState;

	// If this is the first time we have set the call appearace, mark it as now being
	// "known" to TAPI.
	if ((m_dwFlags & InitNotify) == 0)
	{
		// Mark this as being KNOWN to tapi.
		m_dwFlags |= InitNotify;

		// If the call has a pending MAKECALL request waiting, then make sure TAPI has
		// been notified before switching the call state.  TAPI will never send call state
		// change messages through to the application until the MAKECALL request returned an
		// OK response.
		CTSPILineConnection* pLine = GetLineOwner();
		CTSPIRequest* pRequest = pLine->FindRequest(this, REQUEST_MAKECALL);
		if (pRequest != NULL && !pRequest->HaveSentResponse())
		{
			// Complete the request with a zero success response, but do NOT delete
			// the request from the queue.
			pLine->CompleteRequest(pRequest, 0, TRUE, FALSE);
		}
	}

	// If the media mode wasn't supplied, use the media mode which is in our
	// call information record
	if (dwMediaMode == 0L)
	{
		if (m_CallInfo.dwMediaMode == 0)
		{
			// If we only have one bit besides UNKNOWN set, then unmark unknown.  
			// Otherwise leave it there since this call could be one of many media modes.
			m_CallInfo.dwMediaMode = GetAddressOwner()->GetAddressCaps()->dwAvailableMediaModes;
			DWORD dwTest = (m_CallInfo.dwMediaMode & ~LINEMEDIAMODE_UNKNOWN);
			if (((dwTest) & (dwTest - 1)) == 0)
				m_CallInfo.dwMediaMode = dwTest;
		}
		dwMediaMode = m_CallInfo.dwMediaMode;
	}
        
    // Otherwise, set the media mode.  This should be the INITIAL media mode
    // being detected by the provider for a call, OR a new media type which is
    // detected on an existing call (such as a fax tone).
    else  
    {   
        // If we have never set the media mode (initial state)
        if (m_CallInfo.dwMediaMode == 0)
            m_CallInfo.dwMediaMode = dwMediaMode;
        OnDetectedNewMediaModes (dwMediaMode);
    }

    // If the state has changed, then tell everyone who needs to know.
    if ((dwCurrState != dwState) || (dwMode > 0 && dwCurrMode != dwMode))
    {   
#ifdef _DEBUG
		DTRACE(TRC_MIN, _T("Call=0x%lx, CallStateChange Notify=%d, FROM <%s> to <%s>\r\n"), (DWORD)this, fTellTapi, GetCallStateName(), GetCallStateName(dwState));
#endif
		// Save off the new state/mode.
		m_CallStatus.dwCallState = dwState;
		if (dwMode > 0 || (dwCurrState != dwState))
			m_CallStatus.dwCallStateMode = dwMode;

		// Mark the time we entered this call state.
		GetSystemTime(&m_CallStatus.tStateEntryTime);

    	// Tell the address, and then the call appearance that the call
    	// status record is about to be changed. It can at this point call SetCallState
    	// here in order to affect the current state as TAPI has not yet been told.
		// This will change the call count fields stored in the LINEADDRESSSTATUS
		// structure. The notification will not be sent until after we have officially
		// changed the call state.
		m_pAddr->OnPreCallStateChange(this, dwState, dwCurrState);

		// If our call state was changed by the line or call notification
		// done above, then don't bother to go any further into this function
		// since we already came through here recursively.  (I.e. the line
		// called SetCallState() during the OnPreCallStateChange message.
		if (m_CallStatus.dwCallState != dwState)
		{
#ifdef _DEBUG
			TRACE(_T("Aborted PreCallStateChange 0x%lx, CallID 0x%lx Notify=%d, \"%s\" %lx to \"%s\" %lx\n"), 
				this, m_CallInfo.dwCallID, fTellTapi, GetCallStateName(dwCurrState), dwCurrMode, GetCallStateName(dwState), dwMode);
#endif			
			m_dwFlags &= ~IsChgState;
			return;
		}

#ifdef _DEBUG		
		TRACE(_T("CallStateChange 0x%lx, CallID 0x%lx Notify=%d, \"%s\" %lx to \"%s\" %lx\n"), 
				this, m_CallInfo.dwCallID, fTellTapi, GetCallStateName(dwCurrState), dwCurrMode, GetCallStateName(dwState), dwMode);
#endif

		// Update our own internal state and feature set.
        OnCallStatusChange (dwState, dwMode, dwMediaMode);

        // If we are related to another call, then tell it about our state change.
        // This is used by the conferencing calls to associate our call with a conference
        // and automatically remove it when we drop off.
        if (GetConferenceOwner() != NULL && GetCallType() != CALLTYPE_CONFERENCE)
        {
            CTSPICallAppearance* pConfCall = GetConferenceOwner();
            pConfCall->OnRelatedCallStateChange (this, dwState, dwCurrState);
        }
        
        // If we have an attached call (consultation or otherwise) tell them about
        // our state change.
        if (GetAttachedCall() != NULL)
            GetAttachedCall()->OnRelatedCallStateChange (this, dwState, dwCurrState);

		// Now run through all the other calls in the system and tell them to update their 
		// capabilities.  Ignore conference calls since they handle adjustment of children 
		// calls internally.
		if (GetCallType() != CALLTYPE_CONFERENCE)
		{
			// Spin through all the calls on this line excluding conference calls.
			CTSPILineConnection* pLine = GetLineOwner();
			CEnterCode sLine(pLine, FALSE);
			if (sLine.Lock(0))
			{
				for (int iAddr = 0; iAddr < (int) pLine->GetAddressCount(); iAddr++)
				{
					// Spin through all the other calls on this address excluding conference calls.
					CTSPIAddressInfo* pAddr = pLine->GetAddress(iAddr);
					CEnterCode sAddrLock (pAddr, FALSE);
					if (sAddrLock.Lock(0))
					{
						for (int iCall = 0; iCall < pAddr->GetCallCount(); iCall++)
						{
							CTSPICallAppearance* pCall = pAddr->GetCallInfo(iCall);
							if (pCall != this && pCall->GetCallType() != CALLTYPE_CONFERENCE)
								pCall->OnCallStatusChange(pCall->GetCallState(), 0, 0);
						}
						sAddrLock.Unlock();
					}
				}
			}
		}
        
        // Pass the notification to TAPI if necessary.  This is always the last
        // step performed so each object gets a chance to look at the new state before
        // we tell TAPI.
        if (fTellTapi)
			NotifyCallStatusChanged();
    }

	// Unlock the call object in case we delete it below.
	sLock.Unlock();

	// TAPI has been notified now.  Let the address/line send out
	// state events regarding call counts and recalc their feature set.
	m_pAddr->OnCallStateChange(this, dwState, dwCurrState);

	// We are done changing state
	m_dwFlags &= ~IsChgState;

	// If we have no TAPI handle anymore, then TAPI already discarded
	// this call - go ahead and remove it from our list when it goes IDLE.
	if (dwState == LINECALLSTATE_IDLE && dwCurrState != LINECALLSTATE_IDLE &&
		GetCallHandle() == NULL)
		GetAddressOwner()->RemoveCallAppearance(this);

}// CTSPICallAppearance::SetCallState

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnRelatedCallStateChange
//
// A related call has changed state
//
VOID CTSPICallAppearance::OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD /*dwOldState*/)
{
    // We should only get related call information from consultation calls.
    if (dwState == LINECALLSTATE_IDLE)
    {   
		// Call the notification handler if this is the NORMAL call.
		if (GetCallType() != CALLTYPE_CONSULTANT)
			OnConsultantCallIdle(GetConsultationCall());

		// Detach the call
		SetConsultationCall(NULL);

		// Now run through and see if any other call was attached to this call
		// appearance if WE are not idle.  This will allow for a "chain" of
		// attached call relationships.
		if (GetCallState() != LINECALLSTATE_IDLE)
		{
			pCall = GetAddressOwner()->FindAttachedCall(this);
			if (pCall != NULL)
				AttachCall(pCall);
		}

		// If we have no call attachment and we are a consultant call created for
		// some other call (lineSetupTransfer/Conference) then mark us a NORMAL
		// call now since the original purpose of this call has changed.  Also,
		// change our call features to allow this new call to be a target of a 
		// transfer/conference.
		if (GetAttachedCall() == NULL && GetCallType() == CALLTYPE_CONSULTANT)
		{
			SetCallType(CALLTYPE_NORMAL);
			OnCallStatusChange(dwState, m_CallStatus.dwCallStateMode, m_CallInfo.dwMediaMode);
		}
    }

}// CTSPICallAppearance::OnRelatedCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnConsultantCallIdle
//
// This member is called when the consultant call attached to this
// call goes IDLE.
//
// It should be overridden if the switch performs some action
// when a consultation call goes IDLE - such as restoring the original
// call.
//
void CTSPICallAppearance::OnConsultantCallIdle(CTSPICallAppearance* /*pConsultCall*/)
{
    /* Do nothing */
    
}// CTSPICallAppearance::OnConsultantCallIdle

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GatherCallInformation
//
// Gather the LINECALLINFO information for this call appearance
//
LONG CTSPICallAppearance::GatherCallInformation (LPLINECALLINFO lpCallInfo)    
{   
	// Determine what the negotiated version was for the line.
	DWORD dwTSPIVersion = GetLineOwner()->GetNegotiatedVersion();

	// Synch access to object
	CEnterCode sLock(this);  

    // Save off the information that TAPI provides.
    m_CallInfo.dwTotalSize = lpCallInfo->dwTotalSize;
    m_CallInfo.hLine = lpCallInfo->hLine;
    // NOTE: Original TSPI document (dated 1993) stated that TAPI.DLL filled in
    // these fields, but that doesn't appear to be the case anymore.  (5/2/97)
	// m_CallInfo.dwMonitorDigitModes = lpCallInfo->dwMonitorDigitModes;
	// m_CallInfo.dwMonitorMediaModes = lpCallInfo->dwMonitorMediaModes;
    m_CallInfo.dwNumOwners = lpCallInfo->dwNumOwners;
    m_CallInfo.dwNumMonitors = lpCallInfo->dwNumMonitors;
    m_CallInfo.dwAppNameSize = lpCallInfo->dwAppNameSize;
    m_CallInfo.dwAppNameOffset = lpCallInfo->dwAppNameOffset;
    m_CallInfo.dwDisplayableAddressSize = lpCallInfo->dwDisplayableAddressSize;
    m_CallInfo.dwDisplayableAddressOffset = lpCallInfo->dwDisplayableAddressOffset;
    m_CallInfo.dwCalledPartySize = lpCallInfo->dwCalledPartySize;
    m_CallInfo.dwCalledPartyOffset = lpCallInfo->dwCalledPartyOffset;
    m_CallInfo.dwCommentSize = lpCallInfo->dwCommentSize;
    m_CallInfo.dwCommentOffset = lpCallInfo->dwCommentOffset;
    
    // Now verify that we have enough space for our basic structure
	m_CallInfo.dwNeededSize = sizeof(LINECALLINFO);
	if (dwTSPIVersion < TAPIVER_20)
		m_CallInfo.dwNeededSize -= sizeof(DWORD)*7;

#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
    if (lpCallInfo->dwTotalSize < m_CallInfo.dwNeededSize)
	{
		lpCallInfo->dwNeededSize = m_CallInfo.dwNeededSize;
        return LINEERR_STRUCTURETOOSMALL;
	}
#endif

    // Copy our basic structure over
    CopyBuffer (lpCallInfo, &m_CallInfo, m_CallInfo.dwNeededSize);
    lpCallInfo->dwUsedSize = m_CallInfo.dwNeededSize;

	// Now handle some version manipulation
	if (dwTSPIVersion < TAPIVER_14)
	{
		if (lpCallInfo->dwOrigin == LINECALLORIGIN_INBOUND)
			lpCallInfo->dwOrigin = LINECALLORIGIN_UNAVAIL;
		if (lpCallInfo->dwBearerMode == LINEBEARERMODE_PASSTHROUGH)
			lpCallInfo->dwBearerMode = 0;
		if (lpCallInfo->dwReason & (LINECALLREASON_INTRUDE | LINECALLREASON_PARKED))
			lpCallInfo->dwReason = LINECALLREASON_UNAVAIL;
		if (lpCallInfo->dwMediaMode == LINEMEDIAMODE_VOICEVIEW)
			lpCallInfo->dwMediaMode = LINEMEDIAMODE_UNKNOWN;
	}
	
	if (dwTSPIVersion < TAPIVER_20)
	{
		if (lpCallInfo->dwBearerMode == LINEBEARERMODE_RESTRICTEDDATA)
			lpCallInfo->dwBearerMode = 0;
		if (lpCallInfo->dwReason & (LINECALLREASON_CAMPEDON | LINECALLREASON_ROUTEREQUEST))
			lpCallInfo->dwReason = LINECALLREASON_UNAVAIL;
	}

    // Fill in the caller id information.
    if (!m_CallerID.strPartyId.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwCallerIDOffset,
				lpCallInfo->dwCallerIDSize, m_CallerID.strPartyId);

    if (!m_CallerID.strPartyName.IsEmpty())
		AddDataBlock (lpCallInfo, lpCallInfo->dwCallerIDNameOffset, 
				lpCallInfo->dwCallerIDNameSize, m_CallerID.strPartyName);
    
    if (!m_CalledID.strPartyId.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwCalledIDOffset,
				lpCallInfo->dwCalledIDSize, m_CalledID.strPartyId);

    if (!m_CalledID.strPartyName.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwCalledIDNameOffset, 
				lpCallInfo->dwCalledIDNameSize, m_CalledID.strPartyName);

    if (!m_ConnectedID.strPartyId.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwConnectedIDOffset,
				lpCallInfo->dwConnectedIDSize, m_ConnectedID.strPartyId);

    if (!m_ConnectedID.strPartyName.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwConnectedIDNameOffset, 
				lpCallInfo->dwConnectedIDNameSize, m_ConnectedID.strPartyName);

    if (!m_RedirectionID.strPartyId.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwRedirectionIDOffset,
				lpCallInfo->dwRedirectionIDSize, m_RedirectionID.strPartyId);

    if (!m_RedirectionID.strPartyName.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwRedirectionIDNameOffset, 
				lpCallInfo->dwRedirectionIDNameSize, m_RedirectionID.strPartyName);

    if (!m_RedirectingID.strPartyId.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwRedirectingIDOffset,
				lpCallInfo->dwRedirectingIDSize, m_RedirectingID.strPartyId);

    if (!m_RedirectingID.strPartyName.IsEmpty())    
		AddDataBlock (lpCallInfo, lpCallInfo->dwRedirectingIDNameOffset, 
				lpCallInfo->dwRedirectingIDNameSize, m_RedirectingID.strPartyName);
    
    // If we have room for terminal entries, then include them.
    if (m_arrTerminals.GetSize() > 0)
    {
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize + 
			(m_arrTerminals.GetSize() * sizeof(DWORD)))
        {
			for (int i = 0; i < m_arrTerminals.GetSize(); i++)
			{
				DWORD dwValue = m_arrTerminals[i];
				AddDataBlock (lpCallInfo, lpCallInfo->dwTerminalModesOffset,
					lpCallInfo->dwTerminalModesSize, &dwValue, sizeof(DWORD));
			}
        }
    }

    // If we have room for the UserUser information, then copy it over.
    if (m_arrUserUserInfo.GetSize() > 0)
	{
		USERUSERINFO* pInfo = (USERUSERINFO*) m_arrUserUserInfo[0];
		AddDataBlock (lpCallInfo, lpCallInfo->dwUserUserInfoOffset,
				lpCallInfo->dwUserUserInfoSize, pInfo->lpvBuff, pInfo->dwSize);
	}

	// Add in the TAPI 2.0 extensions
	if (dwTSPIVersion >= TAPIVER_20)
	{
	    // If we have room for the CALLDATA information, then add it into
		// the callinfo record.
		if (m_dwCallDataSize > 0)
			AddDataBlock (lpCallInfo, lpCallInfo->dwCallDataOffset, 
					lpCallInfo->dwCallDataSize, m_lpvCallData, m_dwCallDataSize);

		// If we have negotiated a QOS, and have room for it, then add
		// the FLOWSPEC data into the CALLINFO record.
		if (m_dwSendingFlowSpecSize > 0)
			AddDataBlock (lpCallInfo, lpCallInfo->dwSendingFlowspecOffset,
					lpCallInfo->dwSendingFlowspecSize, m_lpvSendingFlowSpec, m_dwSendingFlowSpecSize);
		if (m_dwReceivingFlowSpecSize > 0)
			AddDataBlock (lpCallInfo, lpCallInfo->dwReceivingFlowspecOffset, 
					lpCallInfo->dwReceivingFlowspecSize, m_lpvReceivingFlowSpec, m_dwReceivingFlowSpecSize);
	}

    return FALSE;         
   
}// CTSPICallAppearance::GatherCallInformation

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GatherStatusInformation
//
// Gather all the current status information for this call
//
LONG CTSPICallAppearance::GatherStatusInformation(LPLINECALLSTATUS lpCallStatus)
{   
	// Determine what the negotiated version was for the line.
	DWORD dwTSPIVersion = GetLineOwner()->GetNegotiatedVersion();

	// Synch access to object
	CEnterCode sLock(this);  

    // Save off what TAPI provides
    m_CallStatus.dwTotalSize = lpCallStatus->dwTotalSize;
    m_CallStatus.dwCallPrivilege = lpCallStatus->dwCallPrivilege;
    
    // Now fill in the other fields.
    m_CallStatus.dwNeededSize = sizeof(LINECALLSTATUS);
	if (dwTSPIVersion < TAPIVER_20)
		m_CallStatus.dwNeededSize -= (sizeof(DWORD) + sizeof(SYSTEMTIME));
    
#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
    if (lpCallStatus->dwTotalSize < m_CallStatus.dwNeededSize)
	{
		lpCallStatus->dwNeededSize = m_CallStatus.dwNeededSize;
        return LINEERR_STRUCTURETOOSMALL;
	}
#endif

    // Copy our static structure into this one.
    CopyBuffer (lpCallStatus, &m_CallStatus, m_CallStatus.dwNeededSize);
    lpCallStatus->dwUsedSize = m_CallStatus.dwNeededSize;
    
    return 0L;

}// CTSPICallAppearance::GatherStatusInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Unhold
//
// Take the call appearance off hold
//
LONG CTSPICallAppearance::Unhold (DRV_REQUESTID dwRequestID)
{
#if STRICT_CALLSTATES
    // Make sure the call is in the HOLD state.
    if ((GetCallState() & 
				(LINECALLSTATE_ONHOLD |
				 LINECALLSTATE_ONHOLDPENDTRANSFER |
				 LINECALLSTATE_ONHOLDPENDCONF)) == 0)
        return LINEERR_INVALCALLSTATE;
#endif

    // Submit the request
    if (AddAsynchRequest(REQUEST_UNHOLD, dwRequestID) != NULL)
        return (LONG) dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Unhold

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::CompleteDigitGather
//
// This function completes a digit gathering session and deletes and 
// resets our digit gather.  It is called when a termination
// digit is found, the buffer is full, or a gather is cancelled.
//
VOID CTSPICallAppearance::CompleteDigitGather (DWORD dwReason)
{
	CEnterCode sLock(this);  // Synch access to object
    if (m_lpGather != NULL)
    {
        ASSERT (m_lpGather->lpBuffer != NULL);
        CTSPILineConnection* pLine = GetLineOwner();
        ASSERT (pLine != NULL);
        ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));
        pLine->Send_TAPI_Event(this, LINE_GATHERDIGITS, dwReason, m_lpGather->dwEndToEndID, GetTickCount());

        delete m_lpGather;
        m_lpGather = NULL;
    }

}// CTSPICallAppearance::CompleteDigitGather

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnInternalTimer
//
// This method is called by the address when our interval timer is
// set.  It allows us to check our digit gathering process.
//
BOOL CTSPICallAppearance::OnInternalTimer()
{                           
	CEnterCode Key (this, FALSE);
	if (Key.Lock (0))
	{
		// Get the current TICK count - it is used to timeout media events
		// on this call appearance.
		DWORD dwCurr = GetTickCount();

		// If we are collecting digits then see if our inter-digit or first-digit
		// timeout has expired.  If so, tell TAPI we have cancled the digit gathering
		// event because we haven't seen the digits quickly enough.
		if (m_lpGather != NULL)
		{
			// No digits collected at all?
			if (m_lpGather->dwCount == 0L)
			{
				// If we have a timeout and it is PAST our current tick then complete
				// the request with an error.
				if (m_lpGather->dwFirstDigitTimeout > 0L &&
					m_lpGather->dwFirstDigitTimeout + m_lpGather->dwLastTime < dwCurr)
					CompleteDigitGather (LINEGATHERTERM_FIRSTTIMEOUT);
			}                
			else // Have at least one character
			{
				// If we have a timeout and it is PAST our current tick then complete
				// the request with an error.
				if (m_lpGather->dwInterDigitTimeout > 0L &&
					m_lpGather->dwInterDigitTimeout + m_lpGather->dwLastTime < dwCurr)
					CompleteDigitGather (LINEGATHERTERM_INTERTIMEOUT);
			}                 
		}
    
		// See if we have any pending TIMER events we need to check on.  These are
		// inserted as a result of a detected TONE on the call which matched a
		// monitor event.  They are marked with the last time the tone on the call
		// changed (via OnTone) and if we see the tone long enough we send a completion.
		// 
		// If the tone changes (in OnTone) on the call and we didn't see the tone
		// long enough for the monitor event it is removed in the OnTone function.
		for (int i = 0; i < m_arrEvents.GetSize(); i++)
		{
			TIMEREVENT* lpEvent = (TIMEREVENT*) m_arrEvents[i];
			if (lpEvent->dwEndTime <= GetTickCount())
			{
				switch (lpEvent->iEventType)
				{
					case TIMEREVENT::MediaControlMedia:
					case TIMEREVENT::MediaControlTone:
						OnMediaControl (lpEvent->dwData1);
						break;

					case TIMEREVENT::ToneDetect:
						OnToneMonitorDetect(lpEvent->dwData1, lpEvent->dwData2);
						break;
                    
					default:
						break;
				}
				m_arrEvents.RemoveAt(i);
				i--;
				delete lpEvent;
			}
		}

		// Now check to see if we need any further interval timer activity.
		// If not we remove this call from the activity list which keeps our
		// timer running smoothly by not processing calls which don't need it.
		if (m_arrEvents.GetSize() == 0 && m_lpGather == NULL)
			return TRUE;
	}

	// Continue sending interval timers.
	return FALSE;

}// CTSPICallAppearance::OnInternalTimer

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnTone
//
// This method is called when a tone is detected for either the address
// this call appears on, or if possible, directly by the worker code for
// this call appearance.  This should only be invoked when a CHANGE is
// detected (ie: if tone transitions to silence, then we should see the
// tone, followed by a single silence indicator, followed by the next
// tone whenever that happens).
//
// The three frequency fields are the Hz value components which comprise this
// tone.  If fewer than three tone frequencies are required, then the unused
// entries should be zero.  If all three values are zero, this indicates
// silence on the media.
//
VOID CTSPICallAppearance::OnTone (DWORD dwFreq1, DWORD dwFreq2, DWORD dwFreq3)
{                              
	// See if any older timer events which have not yet expired are no longer
    // valid due to this tone change.  If the proper time DID elapse, and we
    // just didn't spin around quick enough to catch it, then send the notification
    // to TAPI.
	CEnterCode sLock(this);  // Synch access to object
    for (int i = 0; i < m_arrEvents.GetSize(); i++)
    {
    	TIMEREVENT* lpEvent = (TIMEREVENT*) m_arrEvents[i];
        if (lpEvent->iEventType == TIMEREVENT::MediaControlTone)
        {
        	if (lpEvent->dwEndTime <= GetTickCount())
                OnMediaControl (lpEvent->dwData1);
            m_arrEvents.RemoveAt(i);
            i--;       
            delete lpEvent;
        }
        else if (lpEvent->iEventType == TIMEREVENT::ToneDetect)
        {
        	if (lpEvent->dwEndTime <= GetTickCount())
            	OnToneMonitorDetect (lpEvent->dwData1, lpEvent->dwData2);
            m_arrEvents.RemoveAt(i);
            i--;          
            delete lpEvent;
        }
    }
    
	if (m_lpMediaControl != NULL)
	{        
        // Now go through our media events and see if any match up here.
        for (i = 0; i < m_lpMediaControl->arrTones.GetSize(); i++)
        {
            LPLINEMEDIACONTROLTONE lpTone = (LPLINEMEDIACONTROLTONE) m_lpMediaControl->arrTones[i];
            if (GetSP()->MatchTones (lpTone->dwFrequency1, lpTone->dwFrequency2, lpTone->dwFrequency3,
                                     dwFreq1, dwFreq2, dwFreq3))
            {
                if (lpTone->dwDuration == 0)
                    OnMediaControl (lpTone->dwMediaControl);
                else
                {
                    TIMEREVENT* lpTimer = new TIMEREVENT;
                    m_arrEvents.Add (lpTimer);
                    lpTimer->iEventType = TIMEREVENT::MediaControlTone;
                    lpTimer->dwEndTime = GetTickCount() + lpTone->dwDuration;
                    lpTimer->dwData1 = lpTone->dwMediaControl;
					GetSP()->AddTimedCall(this);
                }
            }
        }
    }
    
    // If we have any tone lists we are looking for, search them.
    for (i = 0; i < m_arrMonitorTones.GetSize(); i++)
    {
        TSPITONEMONITOR* lpToneList = (TSPITONEMONITOR*) m_arrMonitorTones[i];
        if (lpToneList)
        {
            for (int j = 0; j < lpToneList->arrTones.GetSize(); j++)
            {
                LPLINEMONITORTONE lpTone = (LPLINEMONITORTONE)lpToneList->arrTones[j];
                ASSERT (lpTone);
                if (GetSP()->MatchTones (lpTone->dwFrequency1, lpTone->dwFrequency2, lpTone->dwFrequency3,
                                        dwFreq1, dwFreq2, dwFreq3))
                {
                    if (lpTone->dwDuration == 0)
                        OnToneMonitorDetect (lpToneList->dwToneListID, lpTone->dwAppSpecific);
                    else
                    {
                        TIMEREVENT* lpTimer = new TIMEREVENT;
                        m_arrEvents.Add (lpTimer);
                        lpTimer->iEventType = TIMEREVENT::ToneDetect;
                        lpTimer->dwEndTime = GetTickCount() + lpTone->dwDuration;
                        lpTimer->dwData1 = lpToneList->dwToneListID;
                        lpTimer->dwData2 = lpTone->dwAppSpecific;
						GetSP()->AddTimedCall(this);
                    }
                }
            }
        }
    }

}// CTSPICallAppearance::OnTone

#ifdef _UNICODE
///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnDigit
//
// Non-UNICODE version of function.
//
VOID CTSPICallAppearance::OnDigit (DWORD dwType, char cDigit)
{                               
	TCHAR ch;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &cDigit, 1, &ch, sizeof(TCHAR));
	OnDigit (dwType, ch);

}// CTSPICallAppearance::OnDigit
#endif // _UNICODE

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnDigit
//
// This method should be called by the address when a digit is detected
// on the address this call appearance is attached to.  It may be called
// directly by the derived class if digit detection can be isolated to 
// a particular call (not always possible).
//
VOID CTSPICallAppearance::OnDigit (DWORD dwType, TCHAR cDigit)
{
    // If we are monitoring for this type of digit, send a 
    // digit monitor message to TAPI.
	CEnterCode sLock(this);  // Synch access to object
    if (m_CallInfo.dwMonitorDigitModes & dwType)
    {
        CTSPILineConnection* pLine = GetLineOwner();
        ASSERT (pLine != NULL);
        ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));
        pLine->Send_TAPI_Event(this, LINE_MONITORDIGITS, (DWORD)cDigit, dwType, GetTickCount());
    }

    // Add the character to our buffer. 
    if (m_lpGather != NULL)
    {   
        if (m_lpGather->dwDigitModes & dwType)
        {
#ifndef _UNICODE
			wchar_t cChar;
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &cDigit, 1, &cChar, sizeof(wchar_t));
			LPWSTR lpwStr = (LPWSTR) m_lpGather->lpBuffer;
			*(lpwStr + m_lpGather->dwCount) = cChar;
#else
            *(m_lpGather->lpBuffer+m_lpGather->dwCount) = cDigit;
#endif
            m_lpGather->dwCount++;
            m_lpGather->dwLastTime = GetTickCount();
        
            // Check for termination conditions.
            if (m_lpGather->dwCount == m_lpGather->dwSize)
                CompleteDigitGather (LINEGATHERTERM_BUFFERFULL);
            else if (m_lpGather->strTerminationDigits.Find(cDigit) >= 0)
                CompleteDigitGather (LINEGATHERTERM_TERMDIGIT);
        }                
    }

    // If we are doing media monitoring, then check our list.
    if (m_lpMediaControl != NULL)
    {
        for (int i = 0; i < m_lpMediaControl->arrDigits.GetSize(); i++)
        {
            LPLINEMEDIACONTROLDIGIT lpDigit = (LPLINEMEDIACONTROLDIGIT) m_lpMediaControl->arrDigits[i];
			if (lpDigit->dwDigit == (DWORD) cDigit)
                OnMediaControl (lpDigit->dwMediaControl);
        }
    }

}// CTSPICallAppearance::OnDigit

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GatherDigits
//
// Initiates the buffered gathering of digits on the specified call. 
// The application specifies a buffer in which to place the digits and the 
// maximum number of digits to be collected.
//
LONG CTSPICallAppearance::GatherDigits (TSPIDIGITGATHER* lpGather)
{   
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_GATHERDIGITS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If this is a request to CANCEL digit gathering, then do so.
    if (lpGather == NULL || lpGather->lpBuffer == NULL)
    {
        delete lpGather;
        CompleteDigitGather(LINEGATHERTERM_CANCEL);
        return FALSE;
    }

    // Verify that the parameters within the gather structure are valid.
    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    
    // Digit timeout values.
    if ((lpGather->dwFirstDigitTimeout > 0 &&
        (lpGather->dwFirstDigitTimeout < pLine->GetLineDevCaps()->dwGatherDigitsMinTimeout ||
         lpGather->dwFirstDigitTimeout > pLine->GetLineDevCaps()->dwGatherDigitsMaxTimeout)) ||
        (lpGather->dwInterDigitTimeout > 0 &&
         (lpGather->dwInterDigitTimeout < pLine->GetLineDevCaps()->dwGatherDigitsMinTimeout ||
         lpGather->dwInterDigitTimeout > pLine->GetLineDevCaps()->dwGatherDigitsMaxTimeout)))
        return LINEERR_INVALTIMEOUT;
    
    // If we cannot detect the type of digits requested, return an error.
    if ((lpGather->dwDigitModes & pLine->GetLineDevCaps()->dwMonitorDigitModes) != lpGather->dwDigitModes)
        return LINEERR_INVALDIGITMODE;
                                     
    // Validate the termination digits.
    if (lpGather->dwDigitModes & LINEDIGITMODE_PULSE)
    {
        if (!lpGather->strTerminationDigits.SpanExcluding(_T("0123456789")).IsEmpty())
            return LINEERR_INVALDIGITS;
    }
    
    if (lpGather->dwDigitModes & LINEDIGITMODE_DTMF)
    {
        if (!lpGather->strTerminationDigits.SpanExcluding(_T("0123456789ABCD*#")).IsEmpty())
            return LINEERR_INVALDIGITS;
    }
                                     
    // Everything looks ok, setup the new digit gathering.
	CEnterCode sLock(this);  // Synch access to object
    if (m_lpGather != NULL)
        delete m_lpGather;
    
    m_lpGather = lpGather;
    m_lpGather->dwCount = 0L;
    m_lpGather->dwLastTime = GetTickCount();

	// Make sure we see timer events.
	GetSP()->AddTimedCall(this);
	
    return FALSE;

}// CTSPICallAppearance::GatherDigits

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GenerateDigits
//
// Initiates the generation of the specified digits on the specified 
// call as inband tones using the specified signaling mode. Invoking this 
// function with a NULL value for lpszDigits aborts any digit generation 
// currently in progress.  Invoking lineGenerateDigits or lineGenerateTone 
// while digit generation is in progress aborts the current digit generation 
// or tone generation and initiates the generation of the most recently 
// specified digits or tone. 
//
LONG CTSPICallAppearance::GenerateDigits (TSPIGENERATE* lpGenerate)
{                                      
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_GENERATEDIGITS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

    // If we cannot detect the type of digits requested, return an error.
    if ((lpGenerate->dwMode & pLine->GetLineDevCaps()->dwGenerateDigitModes) != lpGenerate->dwMode)
        return LINEERR_INVALDIGITMODE;

    // Adjust the duration to the nearest available.
    if (lpGenerate->dwDuration < pLine->GetLineDevCaps()->MinDialParams.dwDigitDuration)
        lpGenerate->dwDuration = pLine->GetLineDevCaps()->MinDialParams.dwDigitDuration;
    else if (lpGenerate->dwDuration > pLine->GetLineDevCaps()->MaxDialParams.dwDigitDuration)
        lpGenerate->dwDuration = pLine->GetLineDevCaps()->MaxDialParams.dwDigitDuration;
    
    // Verify the digits in the list.
    if (lpGenerate->dwMode == LINEDIGITMODE_PULSE)
    {
        if (!lpGenerate->strDigits.SpanExcluding(_T("!0123456789")).IsEmpty())
            return LINEERR_INVALDIGITS;
    }
    else if (lpGenerate->dwMode == LINEDIGITMODE_DTMF)
    {
        if (!lpGenerate->strDigits.SpanExcluding(_T("!0123456789ABCD*#")).IsEmpty())
            return LINEERR_INVALDIGITS;
    }
    
    // Remove any existing generate digit requests.
    pLine->RemovePendingRequests(this, REQUEST_GENERATEDIGITS);
    
    // Submit the request to generate the digits.
    if (AddAsynchRequest(REQUEST_GENERATEDIGITS, 0, lpGenerate) != NULL)
        return FALSE;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::GenerateDigits

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GenerateTone
//
// Generates the specified inband tone over the specified call. Invoking 
// this function with a zero for dwToneMode aborts the tone generation 
// currently in progress on the specified call. Invoking lineGenerateTone or
// lineGenerateDigits while tone generation is in progress aborts the current 
// tone generation or digit generation and initiates the generation of 
// the newly specified tone or digits.
//
LONG CTSPICallAppearance::GenerateTone (TSPIGENERATE* lpGenerate)
{   
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_GENERATETONE) == 0)
		return LINEERR_OPERATIONUNAVAIL;
	
    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

    // If we cannot detect the type of digits requested, return an error.
    if ((lpGenerate->dwMode & pLine->GetLineDevCaps()->dwGenerateToneModes) != lpGenerate->dwMode)
        return LINEERR_INVALTONEMODE;
    
    // If a custom tone is specified, verify that it is within the parameters according
    // to our line.
    if (lpGenerate->arrTones.GetSize() > 0)
    {
        if (lpGenerate->arrTones.GetSize() > (int) pLine->GetLineDevCaps()->dwGenerateToneMaxNumFreq)
            return LINEERR_INVALTONE;
    }

    // Remove any existing generate tone requests.
    pLine->RemovePendingRequests(this, REQUEST_GENERATETONE);

    // Submit the request to the worker thread.
    if (AddAsynchRequest(REQUEST_GENERATETONE, 0, lpGenerate) != NULL)
        return FALSE;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::GenerateTone

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::MonitorDigits
//
// Enables and disables the unbuffered detection of digits received on the 
// call. Each time a digit of the specified digit mode(s) is detected, a 
// message is sent to the application indicating which digit has been detected.
//
LONG CTSPICallAppearance::MonitorDigits (DWORD dwDigitModes)
{   
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_MONITORDIGITS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Validate the call state.                             
    if (GetCallState() == LINECALLSTATE_IDLE || GetCallState() == LINECALLSTATE_DISCONNECTED)
        return LINEERR_INVALCALLSTATE;
#endif

    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

    // If we cannot detect the type of digits requested, return an error.
    if ((dwDigitModes & pLine->GetLineDevCaps()->dwMonitorDigitModes) != dwDigitModes)
        return LINEERR_INVALDIGITMODE;
    
    // Assign the digit modes detected.
    SetDigitMonitor (dwDigitModes);
    return FALSE;

}// CTSPICallAppearance::MonitorDigits

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::MonitorMedia
//
// Enables and disables the detection of media modes on the specified call. 
// When a media mode is detected, a message is sent to the application.
//
LONG CTSPICallAppearance::MonitorMedia (DWORD dwMediaModes)
{   
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_MONITORMEDIA) == 0)
		return LINEERR_OPERATIONUNAVAIL;
	
#if STRICT_CALLSTATES
    // Validate the call state.                             
    if (GetCallState() == LINECALLSTATE_IDLE)
        return LINEERR_INVALCALLSTATE;
#endif

    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

    // If we cannot detect the type of media requested, return an error.
    if ((dwMediaModes & pLine->GetLineDevCaps()->dwMediaModes) != dwMediaModes)
        return LINEERR_INVALMEDIAMODE;
    
    // Assign the digit modes detected.
    SetMediaMonitor(dwMediaModes);
    return FALSE;

}// CTSPICallAppearance::MonitorMedia

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::MonitorTones
//
// Enables and disables the detection of inband tones on the call. Each 
// time a specified tone is detected, a message is sent to the application. 
//
LONG CTSPICallAppearance::MonitorTones (TSPITONEMONITOR* lpMon)
{   
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_MONITORTONES) == 0)
		return LINEERR_OPERATIONUNAVAIL;
    
#if STRICT_CALLSTATES
    // Validate the call state.                             
    if (GetCallState() == LINECALLSTATE_IDLE)
        return LINEERR_INVALCALLSTATE;
#endif
    
    // Validate the tone monitor list.
    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
                                        
    // If it is disabled in the line device capabilities.                                        
    if (pLine->GetLineDevCaps()->dwMonitorToneMaxNumFreq == 0)
        return LINEERR_OPERATIONUNAVAIL;

	CEnterCode sLock(this);  // Synch access to object

    // If this entry already exists, locate it.
    int iPos = -1;
    for (int i = 0; i < m_arrMonitorTones.GetSize(); i++)
    {
        TSPITONEMONITOR* lpMyMon = (TSPITONEMONITOR*) m_arrMonitorTones[i];
        if (lpMyMon->dwToneListID == lpMon->dwToneListID)
        {
            iPos = i;
            break;
        }  
    }          
    
    // If this is a request to turn off tone monitoring for this tone list ID,
    // then remove the tone from our list.
    if (lpMon->arrTones.GetSize() == 0)
    {   
    	if (iPos >= 0)
    	{
        	TSPITONEMONITOR* lpMyMon = (TSPITONEMONITOR*) m_arrMonitorTones[iPos];
       		m_arrMonitorTones.RemoveAt(iPos);
        	delete lpMyMon;
        	delete lpMon;
        	return FALSE;
		}
		return LINEERR_INVALTONELIST;        	
    }

    // Otherwise, validate the tone list.    
    if (lpMon->arrTones.GetSize() > (int) pLine->GetLineDevCaps()->dwMonitorToneMaxNumEntries)
        return LINEERR_INVALTONE;
        
    // Verify the count of tones in each of the tone lists.
    for (i = 0; i < lpMon->arrTones.GetSize(); i++)
    {
        LPLINEMONITORTONE lpTone = (LPLINEMONITORTONE) lpMon->arrTones[i];
        int iFreqCount = 0;
        if (lpTone->dwFrequency1 > 0)
            iFreqCount++;
        if (lpTone->dwFrequency2 > 0)
            iFreqCount++;
        if (lpTone->dwFrequency3 > 0)
            iFreqCount++;
        if (iFreqCount > (int) pLine->GetLineDevCaps()->dwMonitorToneMaxNumFreq)
            return LINEERR_INVALTONE;
    }       
    
    // Looks ok, insert it into our list of detectable tones.
    // If it already existed, remove it first - this will replace the entry.
    if (iPos >= 0)
    {
        TSPITONEMONITOR* lpMyMon = (TSPITONEMONITOR*) m_arrMonitorTones[iPos];
        m_arrMonitorTones.RemoveAt(iPos);
        delete lpMyMon;
    }
    m_arrMonitorTones.Add (lpMon);
    return FALSE;
        
}// CTSPICallAppearance::MonitorTones

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SwapHold
//
// Swap this active call with another call on some type of
// hold
//
LONG CTSPICallAppearance::SwapHold(DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SWAPHOLD) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Make sure the held call is really onHold.
    if ((pCall->GetCallState() & 
				(LINECALLSTATE_ONHOLDPENDTRANSFER |
				 LINECALLSTATE_ONHOLDPENDCONF |
				 LINECALLSTATE_ONHOLD)) == 0)
        return LINEERR_INVALCALLSTATE;
#endif

    // Everything seems ok, submit the accept request.
    if (AddAsynchRequest(REQUEST_SWAPHOLD, dwRequestID, (LPCTSTR)pCall) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;
    
}// CTSPICallAppearance::SwapHold

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::ReleaseUserUserInfo
//
// Release a block of USER-USER information in our CALLINFO record.
//
LONG CTSPICallAppearance::ReleaseUserUserInfo(DRV_REQUESTID /*dwRequestID*/)
{
	// Validate the request.
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_RELEASEUSERUSERINFO) == 0)
		return LINEERR_OPERATIONUNAVAIL;

	// Delete the first entry in our UUI array.
	CEnterCode sLock(this);

	if (m_arrUserUserInfo.GetSize() > 0)
	{
		// Delete the first entry.
		m_arrUserUserInfo.RemoveAt(0);

		// Send out a notification about the change in user information.
		if (m_arrUserUserInfo.GetSize() > 0)
			OnCallInfoChange (LINECALLINFOSTATE_USERUSERINFO);

		// Or remove the UUI flag if we have no more.
		else
			m_CallStatus.dwCallFeatures	&= ~LINECALLFEATURE_RELEASEUSERUSERINFO;
	}
	return FALSE;
	
}// CTSPICallAppearance::ReleaseUserUserInfo

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnReceivedUserUserInfo
//
// Received a block of user-user information from the network.
// Store it inside our call appearance.
//    
VOID CTSPICallAppearance::OnReceivedUserUserInfo (LPVOID lpBuff, DWORD dwSize)
{                                              
	if (lpBuff != NULL && dwSize > 0)
	{
		USERUSERINFO* pInfo = new USERUSERINFO (lpBuff, dwSize);

		// Add it to our array.
		CEnterCode sLock(this);  // Synch access to object
		m_arrUserUserInfo.Add(pInfo);
    	if (GetAddressOwner()->GetAddressCaps()->dwCallFeatures & LINECALLFEATURE_RELEASEUSERUSERINFO)
			m_CallStatus.dwCallFeatures	|= LINECALLFEATURE_RELEASEUSERUSERINFO;

		// Send out a notification about the change in user information.
		OnCallInfoChange (LINECALLINFOSTATE_USERUSERINFO);
	}                      

}// CTSPICallAppearance::OnReceivedUserUserInfo

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallData
//
// Sets calldata into our CALLINFO record.  TAPI is notified of the
// change, and depending on the implementation, the calldata should
// be propagated to all systems which have a copy of this data.
//
LONG CTSPICallAppearance::SetCallData (DRV_REQUESTID dwRequestID, 
							   TSPICALLDATA* pCallData)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETCALLDATA) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
	// Don't allow on IDLE calls.
	if (GetCallState() == LINECALLSTATE_IDLE)
		return LINEERR_INVALCALLSTATE;
#endif

	// If the call data is too big, error it out.
	if (pCallData->dwSize > GetAddressOwner()->GetAddressCaps()->dwMaxCallDataSize)
		return LINEERR_INVALPARAM;

	// Submit the request.  The AsynchReply will copy the data
	// if successfull.
    if (AddAsynchRequest(REQUEST_SETCALLDATA, dwRequestID, (LPCTSTR)pCallData) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}//	CTSPICallAppearance::SetCallData

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallData
//
// Sets calldata into our CALLINFO record.  TAPI is notified of the
// change, and depending on the implementation, the calldata should
// be propagated to all systems which have a copy of this data.
//
VOID CTSPICallAppearance::SetCallData (LPVOID lpvCallData, DWORD dwSize)
{
	CEnterCode sLock(this);  // Synch access to object

	// If we have existing call data, delete it.
	if (m_lpvCallData != NULL)
		FreeMem ((LPTSTR)m_lpvCallData);

	// If the new call data is non-existant, then zero our fields.
	if (lpvCallData == NULL || dwSize == 0)
	{
		m_lpvCallData = NULL;
		m_dwCallDataSize = 0L;
	}
	else
	{
		m_lpvCallData = (LPVOID) AllocMem (dwSize);
		if (m_lpvCallData == NULL)
			return;
		CopyBuffer (m_lpvCallData, lpvCallData, dwSize);
		m_dwCallDataSize = dwSize;
	}

	// Notify TAPI of the change.
	OnCallInfoChange (LINECALLINFOSTATE_CALLDATA);

}// CTSPICallAppearance::SetCallData

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetTerminal
//
// Redirect the flow of specifiec terminal events to a destination
// terminal for this specific call.
//
LONG CTSPICallAppearance::SetTerminal (DRV_REQUESTID dwRequestID, 
									   TSPILINESETTERMINAL* lpLine)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETTERMINAL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // WARNING: DO NOT SET THE TERMINAL IDENTIFIER INTO THE CALL APPEARANCE ARRAY
    // HERE - THE DERIVED CLASS MUST DO THIS WHEN THE HARDWARE COMPLETES THE
    // EVENT OR WHEN IT IS CAPABLE OF PERFORMING THE ACTION!

    // Submit the request.
    if (AddAsynchRequest(REQUEST_SETTERMINAL, dwRequestID, lpLine) != NULL)
		return dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::SetTerminal

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetBearerMode
//
// This function sets the current bearer mode for this call.
//
VOID CTSPICallAppearance::SetBearerMode(DWORD dwBearerMode)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwBearerMode != dwBearerMode)
	{
		m_CallInfo.dwBearerMode = dwBearerMode;
		OnCallInfoChange (LINECALLINFOSTATE_BEARERMODE);
	}

}// CTSPICallAppearance::SetBearerMode

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetDataRate
//
// This field determines the rate in bits per second for the call
// appearance.  It is determined by the media type and physical line.
//
VOID CTSPICallAppearance::SetDataRate(DWORD dwRateBps)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwRate != dwRateBps)
	{
		m_CallInfo.dwRate = dwRateBps;
		OnCallInfoChange (LINECALLINFOSTATE_RATE);
	}

}// CTSPICallAppearance::SetDataRate

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetAppSpecificData
//
// Set the application specific data for the call appearance.  This
// will be visible ACROSS applications.
//
VOID CTSPICallAppearance::SetAppSpecificData(DWORD dwAppSpecific)
{
	CEnterCode sLock(this);
    m_CallInfo.dwAppSpecific = dwAppSpecific; 
    OnCallInfoChange(LINECALLINFOSTATE_APPSPECIFIC);

}// CTSPICallAppearance::SetAppSpecificData

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallID
//
// In some telephony environments, the switch or service provider may 
// assign a unique identifier to each call. This allows the call to be 
// tracked across transfers, forwards, or other events.  The field is
// not used in the base class and is available for derived service
// providers to use.
//
VOID CTSPICallAppearance::SetCallID (DWORD dwCallID)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwCallID != dwCallID)
	{
		m_CallInfo.dwCallID = dwCallID;
		OnCallInfoChange(LINECALLINFOSTATE_CALLID);
	}

}// CTSPICallAppearance::SetCallID

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetRelatedCallID
//
// Telephony environments that use the call ID often may find it 
// necessary to relate one call to another. The dwRelatedCallID field 
// may be used by the service provider for this purpose.  The field
// is not used in the base class and is available for derived service 
// providers to use.
//
VOID CTSPICallAppearance::SetRelatedCallID (DWORD dwCallID)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwRelatedCallID != dwCallID)
	{
		m_CallInfo.dwRelatedCallID = dwCallID;
		OnCallInfoChange(LINECALLINFOSTATE_RELATEDCALLID);
	}

}// CTSPICallAppearance::SetRelatedCallID

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetMediaControl
//
// This function enables and disables control actions on the media stream 
// associated with the specified line, address, or call. Media control 
// actions can be triggered by the detection of specified digits, media modes, 
// custom tones, and call states.  The new specified media controls replace 
// all the ones that were in effect for this line, address, or call prior 
// to this request.
//
void CTSPICallAppearance::SetMediaControl (TSPIMEDIACONTROL* lpMediaControl)
{   
	CEnterCode sLock(this);  // Synch access to object
	if (m_lpMediaControl)
		m_lpMediaControl->DecUsage();  // Will auto-delete if last one using this data.
    m_lpMediaControl = lpMediaControl;    
	if (m_lpMediaControl)
		m_lpMediaControl->IncUsage();

}// CTSPICallAppearance::SetMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnToneMonitorDetect
//
// A monitored tone has been detected, inform TAPI.
//
VOID CTSPICallAppearance::OnToneMonitorDetect (DWORD dwToneListID, DWORD dwAppSpecific)
{                                           
    CTSPILineConnection* pLine = GetLineOwner();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));

    if (pLine != NULL)
        pLine->Send_TAPI_Event(this, LINE_MONITORTONE, dwAppSpecific, dwToneListID, GetTickCount());

}// CTSPICallAppearance::OnToneMonitorDetect

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnMediaControl
//
// This method is called when a media control event was activated
// due to a media monitoring event being caught.
//
VOID CTSPICallAppearance::OnMediaControl (DWORD dwMediaControl)
{                                      
	// Default behavior is to send it up to the line owner object
	// and let it be processed there.  This function may be overriden
	// to process at the call level.
	GetLineOwner()->OnMediaControl(this, dwMediaControl);

}// CTSPICallAppearance::OnMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetID
//
// Manage device-level requests for information based on a device id.
//
LONG CTSPICallAppearance::GetID (CString& strDevClass, 
								LPVARSTRING lpDeviceID, HANDLE hTargetProcess)
{
	DEVICECLASSINFO* pDeviceClass = GetDeviceClass(strDevClass);
	if (pDeviceClass == NULL)
		return GetAddressOwner()->GetID(strDevClass, lpDeviceID, hTargetProcess);
	return GetSP()->CopyDeviceClass (pDeviceClass, lpDeviceID, hTargetProcess);
    
}// CTSPICallAppearance::GetID

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallTreatment
//
// The specified call treatment value is stored off in the
// LINECALLINFO record and TAPI is notified of the change.  If
// the call is currently in a state where the call treatment is
// relevent, then it goes into effect immediately.  Otherwise,
// the treatment will take effect the next time the call enters a
// relevent state.
//
VOID CTSPICallAppearance::SetCallTreatment(DWORD dwCallTreatment)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwCallTreatment != dwCallTreatment)
	{
		m_CallInfo.dwCallTreatment = dwCallTreatment;
		OnCallInfoChange (LINECALLINFOSTATE_TREATMENT);
	}

}// CTSPICallAppearance::SetCallTreatment

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallParameterFlags
//
// Specifies a collection of call-related parameters when the call is 
// outbound. These are same call parameters specified in lineMakeCall, of 
// type LINECALLPARAMFLAGS_.  Note that whenever you call this function
// to adjust a flag setting, retrieve the setting first and add your
// flags since they are REPLACED with this call.
//
VOID CTSPICallAppearance::SetCallParameterFlags (DWORD dwFlags)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwCallParamFlags != dwFlags)
	{
		m_CallInfo.dwCallParamFlags = dwFlags;
		OnCallInfoChange(LINECALLINFOSTATE_OTHER);
	}

}// CTSPICallAppearance::SetCallParameterFlags

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetDigitMonitor
//
// Specifies the various digit modes for which monitoring is 
// currently enabled, of type LINEDIGITMODE_.
//
VOID CTSPICallAppearance::SetDigitMonitor(DWORD dwDigitModes)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwMonitorDigitModes != dwDigitModes)
	{
		m_CallInfo.dwMonitorDigitModes = dwDigitModes;
		OnCallInfoChange(LINECALLINFOSTATE_MONITORMODES);
	}

}// CTSPICallAppearance::SetDigitMonitor

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetMediaMonitor
//
// Specifies the various media modes for which monitoring is currently 
// enabled, of type LINEMEDIAMODE_.
//
VOID CTSPICallAppearance::SetMediaMonitor(DWORD dwModes)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwMonitorMediaModes != dwModes)
	{
		m_CallInfo.dwMonitorMediaModes = dwModes;
		OnCallInfoChange(LINECALLINFOSTATE_MONITORMODES);
	}

}// CTSPICallAppearance::SetMediaMonitor

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetDialParameters
//
// Specifies the dialing parameters currently in effect on the call, of 
// type LINEDIALPARAMS. Unless these parameters are set by either 
// lineMakeCall or lineSetCallParams, their values will be the same as the 
// defaults used in the LINEDEVCAPS.
//
VOID CTSPICallAppearance::SetDialParameters (LINEDIALPARAMS& dp)
{
	CEnterCode sLock(this);  // Synch access to object
	if (memcmp(&m_CallInfo.DialParams, &dp, sizeof(LINEDIALPARAMS)) != 0)
	{
		CopyBuffer (&m_CallInfo.DialParams, &dp, sizeof(LINEDIALPARAMS));
		OnCallInfoChange(LINECALLINFOSTATE_DIALPARAMS);
	}

}// CTSPICallAppearance::SetDialParameters

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallOrigin
//
// This function sets the call origin for the call appearance and
// sends a TAPI event indicating that the origin has changed.
//
VOID CTSPICallAppearance::SetCallOrigin(DWORD dwOrigin)
{
	CEnterCode sLock(this);  // Synch access to object
	if (m_CallInfo.dwOrigin != dwOrigin)
	{
		m_CallInfo.dwOrigin = dwOrigin; 
		OnCallInfoChange(LINECALLINFOSTATE_ORIGIN);
	}

}// CTSPICallAppearance::SetCallOrigin

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallReason
//
// Specifies the reason why the call occurred. This field uses the 
// LINECALLREASON_ constants.
//
VOID CTSPICallAppearance::SetCallReason(DWORD dwReason)
{
	CEnterCode sLock(this);  // Synch access to object
	if (m_CallInfo.dwReason != dwReason)
	{
		m_CallInfo.dwReason = dwReason; 
		OnCallInfoChange(LINECALLINFOSTATE_REASON);
	}

}// CTSPICallAppearance::SetCallReason

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetDestinationCountry
//
// The country code of the destination party. Zero if unknown.
//
VOID CTSPICallAppearance::SetDestinationCountry (DWORD dwCountryCode)
{
	CEnterCode sLock(this);  // Synch access to object
	if (m_CallInfo.dwCountryCode != dwCountryCode)
	{
		m_CallInfo.dwCountryCode = dwCountryCode;
		OnCallInfoChange(LINECALLINFOSTATE_OTHER);
	}

}// CTSPICallAppearance::SetDestinationCountry

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetTrunkID
//
// The number of the trunk over which the call is routed. This field 
// is used for both inbound and outgoing calls. It should be set to 
// 0xFFFFFFFFF if it is unknown.
//
VOID CTSPICallAppearance::SetTrunkID (DWORD dwTrunkID)
{
	CEnterCode sLock(this);
	if (m_CallInfo.dwTrunk != dwTrunkID)
	{
		m_CallInfo.dwTrunk = dwTrunkID;
		OnCallInfoChange(LINECALLINFOSTATE_TRUNK);
	}

}// CTSPICallAppearance::SetTrunkID

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetQualityOfService
//
// Re-negotiates the quality of service (QOS) on the call with the
// switch.  If the desired QOS is not available, then the function
// fails but the call continues with the previous QOS.
//
LONG CTSPICallAppearance::SetQualityOfService (DRV_REQUESTID dwRequestID, 
											   TSPIQOS* pQOS)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETQOS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
	// Don't allow on IDLE calls.
	if (GetCallState() == LINECALLSTATE_IDLE)
		return LINEERR_INVALCALLSTATE;
#endif
	
	// Submit the request.  The AsynchReply will copy the data
	// if successfull.
    if (AddAsynchRequest(REQUEST_SETQOS, dwRequestID, (LPCTSTR)pQOS) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::SetQualityOfService

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetQualityOfService
//
// Re-negotiates the quality of service (QOS) on the call with the
// switch.  If the desired QOS is not available, then the function
// fails but the call continues with the previous QOS.
//
VOID CTSPICallAppearance::SetQualityOfService( 
				LPVOID lpSendingFlowSpec, DWORD dwSendingFlowSpecSize,
				LPVOID lpReceivingFlowSpec, DWORD dwReceivingFlowSpecSize)
{
	CEnterCode sLock(this);  // Synch access to object

	// If there is existing FLOWSPEC data, then delete it.
	if (m_lpvSendingFlowSpec)
		FreeMem ((LPTSTR)m_lpvSendingFlowSpec);
	if (m_lpvReceivingFlowSpec)
		FreeMem ((LPTSTR)m_lpvReceivingFlowSpec);

	if (dwSendingFlowSpecSize)
	{
		m_lpvSendingFlowSpec = (LPVOID) AllocMem (dwSendingFlowSpecSize);
		if (m_lpvSendingFlowSpec != NULL)
		{
			CopyBuffer ((LPTSTR)m_lpvSendingFlowSpec, lpSendingFlowSpec, dwSendingFlowSpecSize);
			m_dwSendingFlowSpecSize = dwSendingFlowSpecSize;
		}
	}
	else
	{
		m_lpvSendingFlowSpec = NULL;
		m_dwSendingFlowSpecSize = 0L;
	}

	if (dwReceivingFlowSpecSize)
	{
		m_lpvReceivingFlowSpec = (LPVOID) AllocMem (dwReceivingFlowSpecSize);
		if (m_lpvReceivingFlowSpec != NULL)
		{
			CopyBuffer ((LPTSTR)m_lpvReceivingFlowSpec, lpReceivingFlowSpec, dwReceivingFlowSpecSize);
			m_dwReceivingFlowSpecSize = dwReceivingFlowSpecSize;
		}
	}
	else
	{
		m_lpvReceivingFlowSpec = NULL;
		m_dwReceivingFlowSpecSize = 0L;
	}

	// Tell TAPI of the change.
	OnCallInfoChange (LINECALLINFOSTATE_QOS);

}// CTSPICallAppearance::SetQualityOfService

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::CreateConsultationCall
//
// Creates a new consultation call for this call. It can be located from
// the existing object using the "GetConsultationCall" method.
//
CTSPICallAppearance* CTSPICallAppearance::CreateConsultationCall(HTAPICALL htCall, DWORD dwCallParamFlags)
{
    // Create the call appearance
    CTSPICallAppearance* pCall = (CTSPICallAppearance*) GetSP()->GetTSPICallObj()->CreateObject();
    ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    if (pCall == NULL)
        return NULL;
    
    // Ask TAPI for a new call handle to associate with this consultation call.
	if (htCall == NULL)
	{
		DWORD dwTapiCall = 0;
		GetLineOwner()->Send_TAPI_Event(NULL, LINE_NEWCALL, (DWORD)pCall, (DWORD)&dwTapiCall);
		if (dwTapiCall != 0)
			htCall = (HTAPICALL) dwTapiCall;
	}

    DTRACE(TRC_MIN, _T("CreateConsultationCall: Existing call=0x%lx, New SPcall=0x%lx, TAPI call=0x%lx\r\n"), (DWORD)this, (DWORD)pCall, (DWORD)htCall);

    // Init the call appearance with a handle.
    pCall->Init(GetAddressOwner(), htCall, GetCallInfo()->dwBearerMode, GetCallInfo()->dwRate, dwCallParamFlags, 
				LINECALLORIGIN_OUTBOUND, LINECALLREASON_DIRECT, 0xffffffff,	0, true);

	// Now set the consultation status up with this call.
	SetConsultationCall(pCall);

    // Add it to the address list list.
	CTSPIAddressInfo* pAddress = GetAddressOwner();
	CEnterCode Key (pAddress);  // Synch access to object
    pAddress->m_lstCalls.AddTail((CObject*)pCall);
	Key.Unlock();
    
    // Notify the line/address in case a derived class wants to know.
    pAddress->OnCreateCall(pCall);

    return pCall;

}// CTSPICallAppearance::CreateConsultationCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallTreatment
//
// The specified call treatment value is stored off in the
// LINECALLINFO record and TAPI is notified of the change.  If
// the call is currently in a state where the call treatment is
// relevent, then it goes into effect immediately.  Otherwise,
// the treatment will take effect the next time the call enters a
// relevent state.
//
LONG CTSPICallAppearance::SetCallTreatment(DRV_REQUESTID dwRequestID, DWORD dwCallTreatment)
{
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETTREATMENT) == 0)
		return LINEERR_OPERATIONUNAVAIL;

	// Verify that the call treatment is valid.
	if (GetAddressOwner()->GetCallTreatmentName(dwCallTreatment) == _T(""))
		return LINEERR_INVALPARAM;
	
	// Submit a request for the derived provider to handle.
	if (AddAsynchRequest(REQUEST_SETCALLTREATMENT, dwRequestID, NULL, dwCallTreatment) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::SetCallTreatment

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::IsActiveCallState
//
// Function to return whether the supplied call state is 
// considered ACTIVE to TAPI.
//  
// STATIC FUNCTION
// 
BOOL CTSPICallAppearance::IsActiveCallState(DWORD dwState)
{
    return ((dwState & 
				(LINECALLSTATE_IDLE |
				 LINECALLSTATE_UNKNOWN |
                 LINECALLSTATE_ONHOLD |
                 LINECALLSTATE_ONHOLDPENDTRANSFER |
                 LINECALLSTATE_ONHOLDPENDCONF |
                 LINECALLSTATE_CONFERENCED |
			     LINECALLSTATE_DISCONNECTED)) == 0);

}// CTSPICallAppearance::IsActiveCallState

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::IsConnectedCallState
//
// Function to return whether the supplied call state is 
// CONNECTED to a destination party or channel.
//  
// STATIC FUNCTION
// 
BOOL CTSPICallAppearance::IsConnectedCallState(DWORD dwState)
{
	return ((dwState & (LINECALLSTATE_DIALTONE |
				LINECALLSTATE_DIALING |
				LINECALLSTATE_RINGBACK |
				LINECALLSTATE_BUSY |
				LINECALLSTATE_CONNECTED |
				LINECALLSTATE_PROCEEDING)) != 0);

}// CTSPICallAppearance::IsConnectedCallState

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallStateName
//
// Returns a string indicating the current state of the connection
//
LPCTSTR CTSPICallAppearance::GetCallStateName (DWORD dwState) const
{
    switch((dwState) ? dwState : GetCallState())
    {
        case LINECALLSTATE_IDLE:		return _T("Idle");
        case LINECALLSTATE_OFFERING:	return _T("Offering");
        case LINECALLSTATE_ACCEPTED:    return _T("Accepted");
        case LINECALLSTATE_DIALTONE:    return _T("Dialtone");
        case LINECALLSTATE_DIALING:     return _T("Dialing");
        case LINECALLSTATE_RINGBACK:    return _T("Ringback");
        case LINECALLSTATE_BUSY:        return _T("Busy");
        case LINECALLSTATE_SPECIALINFO: return _T("SpecialInfo");
        case LINECALLSTATE_CONNECTED:   return _T("Connected");
        case LINECALLSTATE_PROCEEDING:  return _T("Proceeding");
        case LINECALLSTATE_ONHOLD:      return _T("OnHold");
        case LINECALLSTATE_CONFERENCED: return _T("Conferenced");
        case LINECALLSTATE_ONHOLDPENDCONF:     return _T("HoldPendConference");
        case LINECALLSTATE_ONHOLDPENDTRANSFER: return _T("HoldPendTransfer");
        case LINECALLSTATE_DISCONNECTED:       return _T("Disconnected");
    }
    return _T("Unknown");

}// CTSPICallAppearance::GetCallStateName
#endif // _DEBUG

