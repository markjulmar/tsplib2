/******************************************************************************/
//                                                                        
// CALL.CPP - Source code for the CTSPICallAppearance object.
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code for managing the call appearance       
// objects which are controlled by the CTSPIAddressInfo object.
//
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
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
	m_pAddr(NULL), m_htCall(0), m_iCallType(CALLTYPE_NORMAL),
	m_lpGather(NULL), m_lpMediaControl(NULL), m_pConsult(NULL),
	m_pConf(NULL), m_dwFlags(0), m_dwRefCount(1)
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
    // Delete any outstanding gathering or monitoring buffers.
    delete m_lpGather;
    delete m_lpMediaControl;

    DeleteToneMonitorList();

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
    m_CallInfo.dwLineDeviceID = GetLineConnectionInfo()->GetDeviceID();
    m_CallInfo.dwAddressID = GetAddressInfo()->GetAddressID();
    m_CallInfo.dwBearerMode = dwBearerMode;
    m_CallInfo.dwRate = dwRate;
    m_CallInfo.dwMediaMode = 0;     // Will be set by 1st SetCallState
    m_CallInfo.dwCallID = 0;        // This may be used by the derived service provider.
    m_CallInfo.dwRelatedCallID = 0; // This may be used by the derived service provider.
    
    // Available Call states
    m_CallInfo.dwCallStates = (LINECALLSTATE_IDLE | LINECALLSTATE_CONNECTED | LINECALLSTATE_UNKNOWN |
                                  LINECALLSTATE_PROCEEDING | LINECALLSTATE_DISCONNECTED | 
                                  LINECALLSTATE_BUSY);
    if (GetAddressInfo()->CanMakeCalls() && CanHandleRequest(REQUEST_MAKECALL))
        m_CallInfo.dwCallStates |= (LINECALLSTATE_DIALING | LINECALLSTATE_DIALTONE | LINECALLSTATE_RINGBACK);
    if (GetAddressInfo()->CanAnswerCalls() && CanHandleRequest(REQUEST_ANSWER))
        m_CallInfo.dwCallStates |= (LINECALLSTATE_OFFERING | LINECALLSTATE_ACCEPTED);
    if (CanHandleRequest(REQUEST_ADDCONF))
        m_CallInfo.dwCallStates |= (LINECALLSTATE_CONFERENCED | LINECALLSTATE_ONHOLDPENDCONF);
    if (CanHandleRequest(REQUEST_COMPLETEXFER))
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
    for (int i = 0; i < GetLineConnectionInfo()->GetTerminalCount(); i++)
        m_arrTerminals.Add (m_pAddr->GetTerminalInformation(i));

}// CTSPICallAppearance::Init

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::IncRefCount
//
// This method is called to increment the reference count associated
// with the call.
//
void CTSPICallAppearance::IncRefCount()
{   
	m_dwRefCount++;

}// CTSPICallAppearance::IncRefCount

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DecRefCount
//
// This method is called to decrement the reference count associated
// with the call.  If it hits zero, the call is deleted.
//
void CTSPICallAppearance::DecRefCount()
{                                   
	if (--m_dwRefCount == 0)
	{
		ASSERT ((m_dwFlags & IsDeleted) == IsDeleted);
		TRACE("Deleting call appearance 0x%lx\r\n", (DWORD)this);
		delete this;
	}

}// CTSPICallAppearance::DecRefCount

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
			CTSPICallAppearance* pCall = (CTSPICallAppearance*) pReq->GetDataPtr();
			if (pCall && GetAttachedCall() == pCall)
			{
				int iType = GetCallType();
				SetCallType(pCall->GetCallType());
				pCall->SetCallType(iType);
			}
		}
	}
    
    // If this is a DROP request, and the callstate is not idle, then change it to idle
    // so that TAPI knows it may deallocate the call handle.
    else if (wCommand == REQUEST_DROPCALL || wCommand == REQUEST_DROPNOOWNER ||
             wCommand == REQUEST_DROPCALLONCLOSE)
    {
		if (lResult == 0)
		{
			// Remove all pending requests for the call.
			// This can be called to close a call handle which has not been 
			// completely setup (ie: the original asynch request which created the
			// call hasn't completely finished).  In this case, we need to return
			// LINEERR_OPERATIONFAILED for each pending request.
			GetLineConnectionInfo()->RemovePendingRequests (this, REQUEST_ALL, LINEERR_OPERATIONFAILED);
		}
		else
		{
			// Unmark the drop flags
			m_dwFlags &= ~IsDropped;
		}
    }    
    
    // If this is a GenerateDigit or GenerateTone request, then complete it
    // with TAPI.
    else if (wCommand == REQUEST_GENERATEDIGITS || wCommand == REQUEST_GENERATETONE)
    {   
        CTSPILineConnection* pLine = GetLineConnectionInfo();
        ASSERT (pLine != NULL);
        ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));

        TSPIGENERATE* pInfo = (TSPIGENERATE*) pReq->GetDataPtr();
        if (pLine != NULL)
            pLine->Send_TAPI_Event(this, LINE_GENERATE, (lResult == 0) ?
                           LINEGENERATETERM_DONE : LINEGENERATETERM_CANCEL,
                           pInfo->dwEndToEndID);
    }
    

}// CTSPICallAppearance::OnRequestComplete

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallType
//
// Return a unique call type which can identify conference and
// consultant calls
//
int CTSPICallAppearance::GetCallType() const
{
    return m_iCallType;

}// CTSPICallAppearance::GetCallType

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallType
//
// Set the call type for this call.
//
VOID CTSPICallAppearance::SetCallType(int iCallType)
{
    m_iCallType = iCallType;

}// CTSPICallAppearance::SetCallType

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnCallInfoChange
//
// This method is called when any of the information in our call 
// information record has changed.  It informs TAPI through the
// asynch. callback function in our line.
//
VOID CTSPICallAppearance::OnCallInfoChange (DWORD dwCallInfo)
{
    CTSPILineConnection* pLine = GetLineConnectionInfo();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));

	if (GetCallHandle() == 0)
		return;

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
    CTSPIAddressInfo* pAddr = GetAddressInfo();
    ASSERT (pAddr != NULL);
    ASSERT (pAddr->IsKindOf (RUNTIME_CLASS(CTSPIAddressInfo)));
    
    LPLINEADDRESSCAPS lpAddrCaps = pAddr->GetAddressCaps();
    DWORD dwAddrCallFeatures = lpAddrCaps->dwCallFeatures;

    // Get the call features which are available at most any time.    
    dwCallFeatures |= (dwAddrCallFeatures &
    					(LINECALLFEATURE_SECURECALL |
    					 LINECALLFEATURE_SETCALLPARAMS |
    					 LINECALLFEATURE_SETMEDIACONTROL |
    					 LINECALLFEATURE_MONITORDIGITS |
    					 LINECALLFEATURE_MONITORMEDIA |
						 LINECALLFEATURE_MONITORTONES |
						 LINECALLFEATURE_DROP));

	// Add RELEASE user information if we have some,
	if (m_arrUserUserInfo.GetSize() > 0)
		dwCallFeatures |= (dwCallFeatures & LINECALLFEATURE_RELEASEUSERUSERINFO);
	
	// Add terminal support
  	if (dwAddrCallFeatures & LINECALLFEATURE_SETTERMINAL && m_arrTerminals.GetSize() > 0)
        dwCallFeatures |= LINECALLFEATURE_SETTERMINAL;
    
    // Add features available when call is active.
	if (IsConnectedCallState(dwCallState))
	{
    	// Add dialing capabilities
    	if (pAddr->CanMakeCalls() && (dwAddrCallFeatures & LINECALLFEATURE_DIAL) &&
        	(lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_DIALED))
         	dwCallFeatures |= LINECALLFEATURE_DIAL;
         
        dwCallFeatures |= (dwAddrCallFeatures & 
        					(LINECALLFEATURE_GATHERDIGITS |
        					 LINECALLFEATURE_GENERATEDIGITS |
        					 LINECALLFEATURE_GENERATETONE));
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
            delete m_lpMediaControl;
            m_lpMediaControl = NULL;
            // Delete any caller id information.
            m_CallInfo.dwCallerIDFlags = 0;
            m_CallerID.strPartyId.Empty();
            m_CallerID.strPartyName.Empty();
            m_CallInfo.dwCalledIDFlags = 0;
            m_CalledID.strPartyId.Empty();
            m_CalledID.strPartyName.Empty();
            m_CallInfo.dwConnectedIDFlags = 0;
            m_ConnectedID.strPartyId.Empty();
            m_ConnectedID.strPartyName.Empty();
            m_CallInfo.dwRedirectionIDFlags = 0;
            m_RedirectionID.strPartyId.Empty();
            m_RedirectionID.strPartyName.Empty();
            m_CallInfo.dwRedirectingIDFlags = 0;
            m_RedirectingID.strPartyId.Empty();
            m_RedirectingID.strPartyName.Empty();

            // Delete origin/reason information
            m_CallInfo.dwOrigin = LINECALLORIGIN_UNAVAIL;
            m_CallInfo.dwReason = LINECALLREASON_UNAVAIL;            
            
            // Remove any pending requests which have NOT started.
			GetLineConnectionInfo()->RemovePendingRequests (this, REQUEST_ALL, LINEERR_INVALCALLSTATE, TRUE);            
            
            // Fall through intentional.
            
        case LINECALLSTATE_UNKNOWN:
            m_CallStatus.dwCallFeatures = 0L;
            break;

        case LINECALLSTATE_DISCONNECTED:
            m_CallStatus.dwCallFeatures = LINECALLSTATE_IDLE;
            break;

        case LINECALLSTATE_OFFERING:
        	dwCallFeatures |= (dwAddrCallFeatures & (LINECALLFEATURE_REDIRECT | LINECALLFEATURE_ACCEPT));
            if ((dwAddrCallFeatures & LINECALLFEATURE_ANSWER) && pAddr->CanAnswerCalls())
                dwCallFeatures |= LINECALLFEATURE_ANSWER;
            break;

        case LINECALLSTATE_CONFERENCED:
            if (dwAddrCallFeatures & LINECALLFEATURE_REMOVEFROMCONF)
            {
                CTSPIConferenceCall* pConfCall = GetConferenceOwner();
                if (pConfCall->CanRemoveFromConference(this))
                    dwCallFeatures |= LINECALLFEATURE_REMOVEFROMCONF;
            }                   
            break;

        case LINECALLSTATE_DIALTONE:
        case LINECALLSTATE_DIALING:
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_ONHOLD | 
                        LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLDPENDCONF))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            break;

        case LINECALLSTATE_ONHOLD:
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_CONNECTED | 
                        LINECALLSTATE_DIALTONE | LINECALLSTATE_PROCEEDING | 
                        LINECALLSTATE_DIALING | LINECALLSTATE_RINGBACK |
                        LINECALLSTATE_BUSY))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            if (dwAddrCallFeatures & LINECALLFEATURE_UNHOLD)
                dwCallFeatures |= LINECALLFEATURE_UNHOLD;
            if ((dwAddrCallFeatures & LINECALLFEATURE_ADDTOCONF) &&
                (lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_CONFERENCEHELD) &&
                 GetLineConnectionInfo()->IsConferenceAvailable(this))
                dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
            if ((dwAddrCallFeatures & LINECALLFEATURE_COMPLETETRANSF) &&
                (lpAddrCaps->dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERHELD))
                dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
            break;

        case LINECALLSTATE_ONHOLDPENDCONF:
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_CONNECTED | 
                        LINECALLSTATE_DIALTONE | LINECALLSTATE_PROCEEDING | 
                        LINECALLSTATE_DIALING | LINECALLSTATE_RINGBACK |
                        LINECALLSTATE_BUSY))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            if (dwAddrCallFeatures & LINECALLFEATURE_UNHOLD)
                dwCallFeatures |= LINECALLFEATURE_UNHOLD;
            break;

        case LINECALLSTATE_ONHOLDPENDTRANSFER:
            if ((dwAddrCallFeatures & LINECALLFEATURE_COMPLETETRANSF) &&
                GetLineConnectionInfo()->IsTransferConsultAvailable(this))
                dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_CONNECTED | 
                        LINECALLSTATE_DIALTONE | LINECALLSTATE_PROCEEDING | 
                        LINECALLSTATE_DIALING | LINECALLSTATE_RINGBACK |
                        LINECALLSTATE_BUSY))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            if (dwAddrCallFeatures & LINECALLFEATURE_UNHOLD)
                dwCallFeatures |= LINECALLFEATURE_UNHOLD;
            break;

        case LINECALLSTATE_RINGBACK:
            if ((dwAddrCallFeatures & LINECALLFEATURE_COMPLETETRANSF) &&
                GetLineConnectionInfo()->IsTransferConsultAvailable(this))
                dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
            if (dwAddrCallFeatures & LINECALLFEATURE_COMPLETECALL)
                dwCallFeatures |= LINECALLFEATURE_COMPLETECALL;
            if ((dwAddrCallFeatures & LINECALLFEATURE_ADDTOCONF) &&
                GetLineConnectionInfo()->IsConferenceAvailable(this))
                dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;                
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_ONHOLD | 
                        LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLDPENDCONF))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            break;

        case LINECALLSTATE_BUSY:
            if ((dwAddrCallFeatures & LINECALLFEATURE_COMPLETETRANSF) &&
                GetLineConnectionInfo()->IsTransferConsultAvailable(this))
                dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
            if (dwAddrCallFeatures & LINECALLFEATURE_COMPLETECALL)
                dwCallFeatures |= LINECALLFEATURE_COMPLETECALL;
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_ONHOLD | 
                        LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLDPENDCONF))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            break;

        case LINECALLSTATE_PROCEEDING:
            if ((dwAddrCallFeatures & LINECALLFEATURE_COMPLETETRANSF) &&
                GetLineConnectionInfo()->IsTransferConsultAvailable(this))
                dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
            if ((dwAddrCallFeatures & LINECALLFEATURE_ADDTOCONF) &&
                GetLineConnectionInfo()->IsConferenceAvailable(this))
                dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
            if (dwAddrCallFeatures & LINECALLFEATURE_BLINDTRANSFER)
                dwCallFeatures |= LINECALLFEATURE_BLINDTRANSFER;
            if (dwAddrCallFeatures & LINECALLFEATURE_HOLD)
                dwCallFeatures |= LINECALLFEATURE_HOLD;
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_ONHOLD | 
                        LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLDPENDCONF))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            break;

        case LINECALLSTATE_CONNECTED:
            if ((dwAddrCallFeatures &  LINECALLFEATURE_SETUPCONF) &&
                GetCallType() != CALLTYPE_CONFERENCE)
                dwCallFeatures |= LINECALLFEATURE_SETUPCONF;
            if (dwAddrCallFeatures & LINECALLFEATURE_SETUPTRANSFER)
			{
                dwCallFeatures |= LINECALLFEATURE_SETUPTRANSFER;
                if (GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_ONHOLDPENDTRANSFER))
                	dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
			}
            if (dwAddrCallFeatures & LINECALLFEATURE_PARK)
                dwCallFeatures |= LINECALLFEATURE_PARK;
            if ((dwAddrCallFeatures & LINECALLFEATURE_ADDTOCONF) &&
                (GetLineConnectionInfo()->IsConferenceAvailable(this) ||
                 GetCallType() == CALLTYPE_CONFERENCE))
                dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
            if ((dwAddrCallFeatures & LINECALLFEATURE_PREPAREADDCONF) &&
                 GetCallType() == CALLTYPE_CONFERENCE)
                dwCallFeatures |= LINECALLFEATURE_PREPAREADDCONF;
            if (dwAddrCallFeatures & LINECALLFEATURE_BLINDTRANSFER)
                dwCallFeatures |= LINECALLFEATURE_BLINDTRANSFER;
            if (dwAddrCallFeatures & LINECALLFEATURE_HOLD)
                dwCallFeatures |= LINECALLFEATURE_HOLD;
            if (dwAddrCallFeatures & LINECALLFEATURE_SENDUSERUSER)
                dwCallFeatures |= LINECALLFEATURE_SENDUSERUSER;
            if ((dwAddrCallFeatures & LINECALLFEATURE_SWAPHOLD) &&
                GetLineConnectionInfo()->FindCallByState(LINECALLSTATE_ONHOLD | 
                        LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLDPENDCONF))
                dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
            break;

        default:
            break;
    }

    // Allow the derived service provider to adjust this list.
	SetCallFeatures (pAddr->OnCallFeaturesChanged(this, dwCallFeatures));
    
}// CTSPICallAppearance::OnCallStatusChange

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallFeatures
//
// Set the current features of the call w/o callng OnCallFeaturesChanged.
//
VOID CTSPICallAppearance::SetCallFeatures (DWORD dwFeatures)
{
	// Make sure the features are in the ADDRESSCAPS.
    CTSPIAddressInfo* pAddr = GetAddressInfo();
    if ((pAddr->GetAddressCaps()->dwCallFeatures & dwFeatures) != dwFeatures)
	{
		TRACE("LINEADDRESSCAPS.dwCallFeatures doesn't have 0x%lx in it.\r\n");
		ASSERT (FALSE);
		dwFeatures &= pAddr->GetAddressCaps()->dwCallFeatures;
	}
	m_CallStatus.dwCallFeatures = dwFeatures;

}// CTSPICallAppearance::SetCallFeatures

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DeleteToneMonitorList
//
// Delete the list of tones we are monitoring for.     
//
VOID CTSPICallAppearance::DeleteToneMonitorList()
{
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
            
///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::CanHandleRequest
//
// Determine dynamically what the service provider can support
//
BOOL CTSPICallAppearance::CanHandleRequest (WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest (this, wRequest, dwData);

}// CTSPICallAppearance::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AddAsynchRequest
//
// This method adds a request to a particular connection.  It
// will add the request to the device list this connection belongs
// to.
//
CTSPIRequest* CTSPICallAppearance::AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId, LPCVOID lpBuff, DWORD dwSize)
{
    return GetLineConnectionInfo()->AddAsynchRequest(this, wReqId, dwReqId, lpBuff, dwSize);

}// CTSPICallAppearance::AddAsynchRequest

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Close
//
// This method is called to close the call appearance.  Once this is
// completed, the call appearance will be invalid (and deleted).
// This is called during lineCloseCall.
//
LONG CTSPICallAppearance::Close()
{           
    // Wait for any pending DROPs to occur.  The call should have been dropped
    // already.
    CTSPILineConnection* pLine = GetLineConnectionInfo();

	// If the call hasn't been dropped yet, then issue a DROP request so the
	// provider always sees a DROP.
	if ((m_dwFlags & IsDropped) == 0 && 
		pLine->FindRequest(this, REQUEST_DROPCALL) == NULL &&
		pLine->FindRequest(this, REQUEST_DROPCALLONCLOSE) == NULL &&
		pLine->FindRequest(this, REQUEST_DROPNOOWNER) == NULL &&
		GetCallState() != LINECALLSTATE_IDLE)
		Drop (0, NULL, 0);

	// Wait for the drop to complete.
    pLine->WaitForAllRequests (this, REQUEST_DROPCALLONCLOSE);
    pLine->WaitForAllRequests (this, REQUEST_DROPCALL);
    pLine->WaitForAllRequests (this, REQUEST_DROPNOOWNER);

    // Deallocate the call appearance from the address.
    GetAddressInfo()->RemoveCallAppearance(this);

	// Close the handle
	m_htCall = (HTAPICALL) 0;

    // 'this' pointer might now be invalid!
    return FALSE;

}// CTSPICallAppearance::Close

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DropOnClose
//
// This method will be called for each call owned by an application which
// called the TAPI lineClose() function, if the application is the sole
// owner of the call.  This is called in place of lineDrop when there
// are no more owners, but still monitors active (therefore TSPI_lineClose
// cannot be invoked).
//
LONG CTSPICallAppearance::DropOnClose()
{
    return PerformDropCall (REQUEST_DROPCALLONCLOSE);

}// CTSPICallAppearance::DropOnClose

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DropNoOwner
//                                 
// This method is called by TAPI when a new call is being offered in a
// state other than OFFERING or IDLE (ie: the provider answered the call,
// or direct-connect), but no application is available to take ownership 
// of the call.  This would leave the call present in the system with nobody
// to drop the call.  TAPI will automatically invoke this function to handle
// this case.
//
LONG CTSPICallAppearance::DropNoOwner()
{
    return PerformDropCall (REQUEST_DROPNOOWNER);

}// CTSPICallAppearance::DropNoOwner

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Drop
//
// Drop the call appearance and transition the call to IDLE.  The
// USERUSER info is local to our SP.  This is called by lineDrop and
// the Close() method.
//
LONG CTSPICallAppearance::Drop(DRV_REQUESTID dwRequestId, LPCSTR lpsUserUserInfo, DWORD dwSize)
{
    return PerformDropCall (REQUEST_DROPCALL, dwRequestId, lpsUserUserInfo, dwSize);
    
}// CTSPICallAppearance::Drop

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::PerformDropCall
//
// Worker function to drop a call and transition the callstate to
// IDLE.
//
LONG CTSPICallAppearance::PerformDropCall (WORD wRequest, DRV_REQUESTID dwRequestId, LPCSTR lpsUserUserInfo, DWORD dwSize)
{                              
	// If the function cannot be performed..
	if (dwRequestId > 0 && (GetCallStatus()->dwCallFeatures & LINECALLFEATURE_DROP) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    CTSPILineConnection* pLine = GetLineConnectionInfo();

	// If the DROP flag is already set, the call is probably in the process of
	// being dropped - otherwise it would be in a state reflective of a dropped
	// call (Idle).
	if (m_dwFlags & IsDropped)
		return LINEERR_OPERATIONUNAVAIL;

	// Mark the call as dropped so we never allow a second drop.
	m_dwFlags |= IsDropped;

    // Make sure the call state allows this call to be dropped.
    // According to the TAPI spec, any state except IDLE can
    // be dropped.
    if (GetCallState() != LINECALLSTATE_IDLE)
    {
        // Submit the drop request
        if (!AddAsynchRequest(wRequest, dwRequestId, lpsUserUserInfo, dwSize))
            return LINEERR_OPERATIONFAILED;
        return dwRequestId;
    }
    return LINEERR_INVALCALLSTATE;
    
}// CTSPICallAppearance::PerformDropCall

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_ACCEPT) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_OFFERING)
        return LINEERR_INVALCALLSTATE;

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SENDUSERUSER) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Check the call state of this call and make sure it is connected
    // to a call.  Added new callstates (TAPI bakeoff fix).
    if (GetCallState() != LINECALLSTATE_CONNECTED &&
        GetCallState() != LINECALLSTATE_OFFERING &&
        GetCallState() != LINECALLSTATE_ACCEPTED &&
        GetCallState() != LINECALLSTATE_RINGBACK)
        return LINEERR_INVALCALLSTATE;

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SECURECALL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If this call is ALREADY secure, then ignore the request.
    DWORD dwCallFlags = m_CallInfo.dwCallParamFlags;
    if (dwCallFlags & LINECALLPARAMFLAGS_SECURE)
        return 0L;

    // Check the call state of this call and make sure it is not idle.
    if (GetCallState() == LINECALLSTATE_IDLE)
        return LINEERR_INVALCALLSTATE;

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_ANSWER) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If this call appearance cannot be answered (outgoing only) then report unavailable.
    if (!GetAddressInfo()->CanAnswerCalls())
        return LINEERR_OPERATIONUNAVAIL;

    // Make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_OFFERING && GetCallState() != LINECALLSTATE_ACCEPTED)
        return LINEERR_INVALCALLSTATE;

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_BLINDTRANSFER) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Ok, make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_CONNECTED)
        return LINEERR_INVALCALLSTATE;
    
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_COMPLETECALL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Ok, make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_BUSY && 
        GetCallState() != LINECALLSTATE_RINGBACK &&
        GetCallState() != LINECALLSTATE_PROCEEDING)
        return LINEERR_INVALCALLSTATE;

    // Validate the message id and completion mode
    if (lpCompCall->dwMessageId > GetAddressInfo()->GetAddressCaps()->dwNumCompletionMessages)
        return LINEERR_INVALMESSAGEID;
    if ((lpCompCall->dwCompletionMode & GetAddressInfo()->GetAddressCaps()->dwCallCompletionModes) == 0)
        return LINEERR_INVALCALLCOMPLMODE;
    
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_DIAL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Make sure we are in the appropriate connection state.
    if (GetCallState() == LINECALLSTATE_IDLE ||
        GetCallState() == LINECALLSTATE_DISCONNECTED)
        return LINEERR_INVALCALLSTATE;
    
    // Set the CALLER id information (us)
	if (m_CallerID.strPartyId.IsEmpty() || m_CallerID.strPartyName.IsEmpty())
		SetCallerIDInformation (LINECALLPARTYID_ADDRESS | LINECALLPARTYID_NAME, 
			GetAddressInfo()->GetDialableAddress(), GetAddressInfo()->GetName());

	// Only set called id information if the call is still dialing.  Otherwise,
	// the digits dialed shouldn't be part of the telephone number and therefore
	// shouldn't be reported.  (v2.04 09/16/97 MCS)
	if (GetCallState() == LINECALLSTATE_DIALTONE || GetCallState() == LINECALLSTATE_DIALING)
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_HOLD) == 0)
		return LINEERR_OPERATIONUNAVAIL;

   // Check the call state and make sure this call may be placed
   // on hold.
   if (GetCallState() != LINECALLSTATE_CONNECTED &&
       GetCallState() != LINECALLSTATE_PROCEEDING &&
       GetCallState() != LINECALLSTATE_DIALING &&
       GetCallState() != LINECALLSTATE_DIALTONE)
      return LINEERR_INVALCALLSTATE;

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_PARK) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Make sure we are in the appropriate connection state.
    if (GetCallState() != LINECALLSTATE_CONNECTED)
        return LINEERR_INVALCALLSTATE;
    
    // Check whether or not our address supports the park mode requested.
    if ((lpPark->dwParkMode & GetAddressInfo()->GetAddressCaps()->dwParkModes) == 0)
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
    // If the call is in the wrong call state or not available, return an error.
    if (GetCallState() != LINECALLSTATE_UNKNOWN)
        return LINEERR_INVALCALLSTATE;                
         
    // If we cannot dial out, give an error
    if (!GetAddressInfo()->CanMakeCalls())
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
        ASSERT (m_CallInfo.dwBearerMode == GetAddressInfo()->GetBearerMode() ||
                m_CallInfo.dwBearerMode == LINEBEARERMODE_PASSTHROUGH);
        m_CallInfo.dwCallParamFlags = lpMakeCall->lpCallParams->dwCallParamFlags;
        CopyBuffer (&m_CallInfo.DialParams, &lpMakeCall->lpCallParams->DialParams, sizeof(LINEDIALPARAMS));
    }   
    else
    {   
        // Otherwise default to the highest priority for a call which is voice.
        m_CallInfo.dwMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
        m_CallInfo.dwBearerMode = GetAddressInfo()->GetBearerMode();
    }

    // Mark our (the originator) phone as automatically being taken offhook.
    m_CallInfo.dwCallParamFlags |= LINECALLPARAMFLAGS_ORIGOFFHOOK;

    // Set the CALLER id information (us)
    SetCallerIDInformation (LINECALLPARTYID_ADDRESS | LINECALLPARTYID_NAME, GetAddressInfo()->GetDialableAddress(), GetAddressInfo()->GetName());

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_REDIRECT) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Make sure we are in the appropriate connection state.
    if (GetCallState() != LINECALLSTATE_OFFERING)
        return LINEERR_INVALCALLSTATE;

    // Store off the redirecting ID (us)
    SetRedirectingIDInformation (LINECALLPARTYID_ADDRESS | LINECALLPARTYID_NAME, GetAddressInfo()->GetDialableAddress(), GetAddressInfo()->GetName());

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
// CTSPICallAppearance::SetTerminal
//
// Redirect the flow of specifiec terminal events to a destination
// terminal for this specific call.
//
LONG CTSPICallAppearance::SetTerminal (DRV_REQUESTID dwRequestID, 
									   TSPILINESETTERMINAL* lpLine)
{
    // WARNING: DO NOT SET THE TERMINAL IDENTIFIER INTO THE CALL APPEARANCE ARRAY
    // HERE - THE DERIVED CLASS MUST DO THIS WHEN THE HARDWARE COMPLETES THE
    // EVENT OR WHEN IT IS CAPABLE OF PERFORMING THE ACTION!

    // Submit the request.
    if (AddAsynchRequest(REQUEST_SETTERMINAL, dwRequestID, lpLine) != NULL)
		return dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::SetTerminal

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETCALLPARAMS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Make sure the call state allows changes.
    if (GetCallState() == LINECALLSTATE_IDLE ||
        GetCallState() == LINECALLSTATE_DISCONNECTED)
        return LINEERR_INVALCALLSTATE;

    if (AddAsynchRequest(REQUEST_SETCALLPARAMS, dwRequestID, lpCallParams) != NULL)
            return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::SetCallParams

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetLineConnectionInfo
//
// This returns the line connection information for this call
// appearance.
//
CTSPILineConnection* CTSPICallAppearance::GetLineConnectionInfo() const
{
    return GetAddressInfo()->GetLineOwner();

}// CTSPICallAppearance::GetLineConnectionInfo

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetAddressInfo
//
// Return the address information for this call appearance.
//
CTSPIAddressInfo* CTSPICallAppearance::GetAddressInfo() const
{
    ASSERT (m_pAddr != NULL);
    return m_pAddr;

}// CTSPICallAppearance::GetAddressInfo

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallHandle
//
// Return the TAPI call handle for this call appearance.
//
HTAPICALL CTSPICallAppearance::GetCallHandle() const
{
    return m_htCall;

}// CTSPICallAppearance::GetCallHandle

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetBearerMode
//
// This function sets the current bearer mode for this call.
//
VOID CTSPICallAppearance::SetBearerMode(DWORD dwBearerMode)
{
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
	if (m_CallInfo.dwRate != dwRateBps)
	{
		m_CallInfo.dwRate = dwRateBps;
		OnCallInfoChange (LINECALLINFOSTATE_RATE);
	}

}// CTSPICallAppearance::SetDataRate

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
    CTSPILineConnection* pLine = GetLineConnectionInfo();

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
    if (!GetAddressInfo()->CanSupportMediaModes (dwMediaMode))
        return LINEERR_INVALMEDIAMODE;

    // Adjust the media mode in the call record.  This function is designed to be
    // simply "advisory".  The media mode is not FORCED to be this new mode.
	if (m_CallInfo.dwMediaMode != dwMediaMode)
	{
		m_CallInfo.dwMediaMode = dwMediaMode;
		OnCallInfoChange (LINECALLINFOSTATE_MEDIAMODE);
	}

    return FALSE;

}// CTSPICallAppearance::SetMediaMode

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetAppSpecificData
//
// Set the application specific data for the call appearance.  This
// will be visible ACROSS applications.
//
VOID CTSPICallAppearance::SetAppSpecificData(DWORD dwAppSpecific)
{
	if (m_CallInfo.dwAppSpecific != dwAppSpecific)
	{
		m_CallInfo.dwAppSpecific = dwAppSpecific; 
		OnCallInfoChange(LINECALLINFOSTATE_APPSPECIFIC);
	}

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
	if (m_CallInfo.dwRelatedCallID != dwCallID)
	{
		m_CallInfo.dwRelatedCallID = dwCallID;
		OnCallInfoChange(LINECALLINFOSTATE_RELATEDCALLID);
	}

}// CTSPICallAppearance::SetRelatedCallID

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
	if (memcmp(&m_CallInfo.DialParams, &dp, sizeof(LINEDIALPARAMS)))
	{
		memcpy (&m_CallInfo.DialParams, &dp, sizeof(LINEDIALPARAMS));
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
// 0xFFFFFFFF if it is unknown.
//
VOID CTSPICallAppearance::SetTrunkID (DWORD dwTrunkID)
{
	if (m_CallInfo.dwTrunk != dwTrunkID)
	{
		m_CallInfo.dwTrunk = dwTrunkID;
		OnCallInfoChange(LINECALLINFOSTATE_TRUNK);
	}

}// CTSPICallAppearance::SetTrunkID

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallerIDInformation
//
// Determines the validity and content of the caller party ID 
// information. The caller is the originator of the call.
//
VOID CTSPICallAppearance::SetCallerIDInformation (DWORD dwFlags, LPCSTR lpszPartyID, LPCSTR lpszName, DWORD dwCountryCode)
{
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
VOID CTSPICallAppearance::SetCalledIDInformation (DWORD dwFlags, LPCSTR lpszPartyID, LPCSTR lpszName, DWORD dwCountryCode)
{
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

}// CTSPICallAppearance::SetCallerIDInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetConnectedIDInformation
//
// Determines the validity and content of the connected party ID 
// information. The connected party is the party that was actually 
// connected to. This may be different from the called-party ID if the 
// call was diverted.
//
VOID CTSPICallAppearance::SetConnectedIDInformation (DWORD dwFlags, LPCSTR lpszPartyID, LPCSTR lpszName, DWORD dwCountryCode)
{
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
VOID CTSPICallAppearance::SetRedirectionIDInformation (DWORD dwFlags, LPCSTR lpszPartyID, LPCSTR lpszName, DWORD dwCountryCode)
{
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
VOID CTSPICallAppearance::SetRedirectingIDInformation (DWORD dwFlags, LPCSTR lpszPartyID, LPCSTR lpszName, DWORD dwCountryCode)
{
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
    DWORD dwCurrState = m_CallStatus.dwCallState;
    DWORD dwCurrMode = m_CallStatus.dwCallStateMode;

	// If the current call state is IDLE, and we are moving out of IDLE, report an error.
	// According to information obtained at the TAPI bakeoff, a call appearances cannot
	// transition out of the idle state.
	if (dwCurrState == LINECALLSTATE_IDLE && dwState != LINECALLSTATE_IDLE)
	{
		TRACE (_T("WARNING: Call appearance cannot transition out of the IDLE state!\r\n"));
		ASSERT (FALSE);
		return;
	}
    
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
		CTSPILineConnection* pLine = GetLineConnectionInfo();
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
        // Initial callstate MUST supply a media mode!!
        ASSERT (m_CallInfo.dwMediaMode > 0);
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
		TRACE (_T("Call=0x%lx, CallStateChange Notify=%d, FROM <%s> to <%s>\r\n"), (DWORD)this, fTellTapi, GetCallStateName(), GetCallStateName(dwState));
#endif
		// Save off the new state/mode.
		m_CallStatus.dwCallState = dwState;
		if (dwMode > 0)
			m_CallStatus.dwCallStateMode = dwMode;

    	// Tell the address, and then the call appearance that the call
    	// status record has changed.  This order will allow the line/address or call
    	// features to be adjusted through the CServiceProvider::CheckCallFeatures
    	// method.
        m_pAddr->OnCallStateChange(this, dwState, dwCurrState);
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
        
        // Pass the notification to TAPI if necessary.  This is always the last
        // step performed so each object gets a chance to look at the new state before
        // we tell TAPI.
        if (fTellTapi)
        {
            CTSPILineConnection* pLine = GetLineConnectionInfo();
            ASSERT (pLine != NULL);
            ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));
            
            // Determine what the parameters are for this callstate.
            DWORD dwP2 = 0, dwP3 = dwMediaMode;
            switch (dwState)
            {   
                case LINECALLSTATE_BUSY:
                case LINECALLSTATE_DIALTONE:
                case LINECALLSTATE_SPECIALINFO:
                case LINECALLSTATE_DISCONNECTED:
                    dwP2 = m_CallStatus.dwCallStateMode;
                    break;
                
                // TAPI 1.4 extension
                case LINECALLSTATE_CONFERENCED:               
                    // If the call was CREATED by us (i.e. a new call) which
                    // TAPI may not know the conference owner of, then send the
                    // conference owner in dwP2.
                    if (GetSP()->GetSupportedVersion() >= TAPIVER_14 &&
                        GetSP()->GetSystemVersion() >= TAPIVER_14)
                    {   
                        CTSPIConferenceCall* pConf = GetConferenceOwner();
                        ASSERT (pConf != NULL);
                        ASSERT (pConf->GetCallType() == CALLTYPE_CONFERENCE);
                        if ((m_dwFlags | pConf->m_dwFlags) & IsNewCall)
                            dwP2 = (DWORD) pConf->GetCallHandle();
                    }                       
                    break;
                    
                default:
                    break;
            }
            
            pLine->Send_TAPI_Event(this, LINE_CALLSTATE, dwState, dwP2, dwP3);
        }            
    }

}// CTSPICallAppearance::SetCallState

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnRelatedCallStateChange
//
// A related call has changed state
//
VOID CTSPICallAppearance::OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD /*dwOldState*/)
{
    // We should only get related call information from attached calls.
    if (dwState == LINECALLSTATE_IDLE)
    {   
        // Call has gone idle, automatically detach it.
        DetachCall();
        pCall->DetachCall();

		// Now run through and see if any other call was attached to this call
		// appearance if WE are not idle.
		if (GetCallState() != LINECALLSTATE_IDLE)
		{
			CTSPICallAppearance* pNewCall = GetAddressInfo()->FindAttachedCall(this);
			if (pNewCall != NULL)
				AttachCall(pNewCall);
		}
    }

}// CTSPICallAppearance::OnRelatedCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallState
//
// Return the call state of the call appearance from our LINECALLSTATE
// structure.
//
DWORD CTSPICallAppearance::GetCallState() const
{ 
    return m_CallStatus.dwCallState;

}// CTSPICallAppearance::GetCallState

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallInfo
//
// Return a pointer to the LINECALLINFO record
//
LPLINECALLINFO CTSPICallAppearance::GetCallInfo()
{                                     
    return &m_CallInfo;
    
}// CTSPICallAppearance::GetCallInfo

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallStatus
//                                   
// Return a pointer to the LINECALLSTATUS record
//
LPLINECALLSTATUS CTSPICallAppearance::GetCallStatus()
{   
    return &m_CallStatus;
    
}// CTSPICallAppearance::GetCallStatus

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GatherCallInformation
//
// Gather the LINECALLINFO information for this call appearance
//
LONG CTSPICallAppearance::GatherCallInformation (LPLINECALLINFO lpCallInfo)    
{   
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
    
#ifdef _DEBUG
    // TAPI is supposed to verify the structure size for us.  Check it in DEBUG
    // builds just to be certain, but count on it in the retail version.
    if (lpCallInfo->dwTotalSize < sizeof(LINECALLINFO))
        return LINEERR_STRUCTURETOOSMALL;
#endif

    // Copy our basic structure over
    CopyBuffer (lpCallInfo, &m_CallInfo, sizeof(LINECALLINFO));
    lpCallInfo->dwUsedSize = sizeof(LINECALLINFO);

    // Fill in the caller id information.
    if (!m_CallerID.strPartyId.IsEmpty())    
    {
        int cbSize = m_CallerID.strPartyId.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwCallerIDSize = cbSize;
            lpCallInfo->dwCallerIDOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwCallerIDOffset, (LPCSTR)m_CallerID.strPartyId, cbSize);
        }
    }

    if (!m_CallerID.strPartyName.IsEmpty())    
    {
        int cbSize = m_CallerID.strPartyName.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwCallerIDNameSize = cbSize;
            lpCallInfo->dwCallerIDNameOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwCallerIDNameOffset, (LPCSTR)m_CallerID.strPartyName, cbSize);
        }
    }
    
    if (!m_CalledID.strPartyId.IsEmpty())    
    {
        int cbSize = m_CalledID.strPartyId.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwCalledIDSize = cbSize;
            lpCallInfo->dwCalledIDOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwCalledIDOffset, (LPCSTR)m_CalledID.strPartyId, cbSize);
        }
    }

    if (!m_CalledID.strPartyName.IsEmpty())    
    {
        int cbSize = m_CalledID.strPartyName.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwCalledIDNameSize = cbSize;
            lpCallInfo->dwCalledIDNameOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwCalledIDNameOffset, (LPCSTR)m_CalledID.strPartyName, cbSize);
        }
    }

    if (!m_ConnectedID.strPartyId.IsEmpty())    
    {
        int cbSize = m_ConnectedID.strPartyId.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwConnectedIDSize = cbSize;
            lpCallInfo->dwConnectedIDOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwConnectedIDOffset, (LPCSTR)m_ConnectedID.strPartyId, cbSize);
        }
    }

    if (!m_ConnectedID.strPartyName.IsEmpty())    
    {
        int cbSize = m_ConnectedID.strPartyName.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwConnectedIDNameSize = cbSize;
            lpCallInfo->dwConnectedIDNameOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwConnectedIDNameOffset, (LPCSTR)m_ConnectedID.strPartyName, cbSize);
        }
    }

    if (!m_RedirectionID.strPartyId.IsEmpty())    
    {
        int cbSize = m_RedirectionID.strPartyId.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwRedirectionIDSize = cbSize;
            lpCallInfo->dwRedirectionIDOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwRedirectionIDOffset, (LPCSTR)m_RedirectionID.strPartyId, cbSize);
        }
    }

    if (!m_RedirectionID.strPartyName.IsEmpty())    
    {
        int cbSize = m_RedirectionID.strPartyName.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwRedirectionIDNameSize = cbSize;
            lpCallInfo->dwRedirectionIDNameOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwRedirectionIDNameOffset, (LPCSTR)m_RedirectionID.strPartyName, cbSize);
        }
    }

    if (!m_RedirectingID.strPartyId.IsEmpty())    
    {
        int cbSize = m_RedirectingID.strPartyId.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwRedirectingIDSize = cbSize;
            lpCallInfo->dwRedirectingIDOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwRedirectingIDOffset, (LPCSTR)m_RedirectingID.strPartyId, cbSize);
        }
    }

    if (!m_RedirectingID.strPartyName.IsEmpty())    
    {
        int cbSize = m_RedirectingID.strPartyName.GetLength()+1;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwUsedSize+cbSize)
        {
            lpCallInfo->dwRedirectingIDNameSize = cbSize;
            lpCallInfo->dwRedirectingIDNameOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpCallInfo + lpCallInfo->dwRedirectingIDNameOffset, (LPCSTR)m_RedirectingID.strPartyName, cbSize);
        }
    }
    
    // If we have room for terminal entries, then include them.
    if (m_arrTerminals.GetSize() > 0)
    {
        DWORD dwReqSize = m_arrTerminals.GetSize() * sizeof(DWORD);
        lpCallInfo->dwNeededSize += dwReqSize;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwNeededSize)
        {
            lpCallInfo->dwTerminalModesOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += dwReqSize;
            lpCallInfo->dwTerminalModesSize = dwReqSize;
            LPDWORD lpDW = (LPDWORD)((LPSTR)lpCallInfo+lpCallInfo->dwTerminalModesOffset);
            for (int i = 0; i < m_arrTerminals.GetSize(); i++)
                *lpDW++ = m_arrTerminals[i];
        }
    }
    
    // If we have room for the UserUser information, then copy it over.
    if (m_arrUserUserInfo.GetSize() > 0)
	{
		USERUSERINFO* pInfo = (USERUSERINFO*) m_arrUserUserInfo[0];
        lpCallInfo->dwNeededSize += pInfo->dwSize;
        if (lpCallInfo->dwTotalSize >= lpCallInfo->dwNeededSize)
        {
            lpCallInfo->dwUserUserInfoSize = pInfo->dwSize;
            lpCallInfo->dwUserUserInfoOffset = lpCallInfo->dwUsedSize;
            lpCallInfo->dwUsedSize += pInfo->dwSize;
            CopyBuffer ((LPSTR)lpCallInfo+lpCallInfo->dwUserUserInfoOffset, (LPCSTR)pInfo->lpvBuff, pInfo->dwSize);
        }
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
    // Save off what TAPI provides
    m_CallStatus.dwTotalSize = lpCallStatus->dwTotalSize;
    m_CallStatus.dwCallPrivilege = lpCallStatus->dwCallPrivilege;
    
    // Now fill in the other fields.
    lpCallStatus->dwNeededSize = sizeof(LINECALLSTATUS);
    
#ifdef _DEBUG
    // TAPI is supposed to verify the structure size for us.  Check it in DEBUG
    // builds just to be certain, but count on it in the retail version.
    if (lpCallStatus->dwTotalSize < lpCallStatus->dwNeededSize)
        return LINEERR_STRUCTURETOOSMALL;
#endif

    // Copy our static structure into this one.
    CopyBuffer (lpCallStatus, &m_CallStatus, sizeof(LINECALLSTATUS));
    lpCallStatus->dwUsedSize = lpCallStatus->dwNeededSize;
    
    return 0L;

}// CTSPICallAppearance::GatherStatusInformation

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::Unhold
//
// Take the call appearance off hold
//
LONG CTSPICallAppearance::Unhold (DRV_REQUESTID dwRequestID)
{                       
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_UNHOLD) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Make sure the call is in the HOLD state.
    if (GetCallState() != LINECALLSTATE_ONHOLD &&
        GetCallState() != LINECALLSTATE_ONHOLDPENDTRANSFER &&
        GetCallState() != LINECALLSTATE_ONHOLDPENDCONF)
        return LINEERR_INVALCALLSTATE;

    // Submit the request
    if (AddAsynchRequest(REQUEST_UNHOLD, dwRequestID) != NULL)
        return (LONG) dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPICallAppearance::Unhold

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::CompleteDigitGather
//
// This function completes a digit gathering session and optionally
// deletes and resets our digit gather.  It is called when a termination
// digit is found, the buffer is full, or a gather is cancelled.
//
VOID CTSPICallAppearance::CompleteDigitGather (DWORD dwReason)
{
    if (m_lpGather != NULL)
    {
        ASSERT (m_lpGather->lpBuffer != NULL);
        CTSPILineConnection* pLine = GetLineConnectionInfo();
        ASSERT (pLine != NULL);
        ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));
        pLine->Send_TAPI_Event(this, LINE_GATHERDIGITS, dwReason, m_lpGather->dwEndToEndID, GetTickCount());

        delete m_lpGather;
        m_lpGather = NULL;
    }

}// CTSPICallAppearance::CompleteDigitGather

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnTimer
//
// This method is called by the address when our interval timer is
// set.  It allows us to check our digit gathering process.
//
VOID CTSPICallAppearance::OnTimer()
{                           
    DWORD dwCurr = GetTickCount();
    if (m_lpGather != NULL)
    {
        if (m_lpGather->dwCount == 0L)
        {
            if (m_lpGather->dwFirstDigitTimeout > 0L &&
                m_lpGather->dwFirstDigitTimeout + m_lpGather->dwLastTime < dwCurr)
                CompleteDigitGather (LINEGATHERTERM_FIRSTTIMEOUT);
        }                
        else // Have at least one character
        {
             if (m_lpGather->dwInterDigitTimeout > 0L &&
                 m_lpGather->dwInterDigitTimeout + m_lpGather->dwLastTime < dwCurr)
                 CompleteDigitGather (LINEGATHERTERM_INTERTIMEOUT);
        }                 
    }
    
    // See if we have any pending events we need to check on.
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

}// CTSPICallAppearance::OnTimer

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
                    }
                }
            }
        }
    }

}// CTSPICallAppearance::OnTone

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnDigit
//
// This method should be called by the address when a digit is detected
// on the address this call appearance is attached to.  It may be called
// directly by the derived class if digit detection can be isolated to 
// a particular call (not always possible).
//
VOID CTSPICallAppearance::OnDigit (DWORD dwType, char cDigit)
{                               
    // If we are monitoring for this type of digit, send a 
    // digit monitor message to TAPI.
    if (m_CallInfo.dwMonitorDigitModes & dwType)
    {
        CTSPILineConnection* pLine = GetLineConnectionInfo();
        ASSERT (pLine != NULL);
        ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));
        pLine->Send_TAPI_Event(this, LINE_MONITORDIGITS, (DWORD)cDigit, dwType, 0L);
    }

    // Add the character to our buffer.
    if (m_lpGather != NULL)
    {   
        if (m_lpGather->dwDigitModes & dwType)
        {
            *(m_lpGather->lpBuffer+m_lpGather->dwCount) = cDigit;
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
            if (LOBYTE(LOWORD(lpDigit->dwDigit)) == cDigit)
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
	// If the function cannot be performed..
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
    CTSPILineConnection* pLine = GetLineConnectionInfo();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    
    // Digit timeout values.
    if (lpGather->dwFirstDigitTimeout > 0 &&
        (lpGather->dwFirstDigitTimeout < pLine->GetLineDevCaps()->dwGatherDigitsMinTimeout ||
         lpGather->dwFirstDigitTimeout > pLine->GetLineDevCaps()->dwGatherDigitsMaxTimeout) ||
        lpGather->dwInterDigitTimeout > 0 &&
         (lpGather->dwInterDigitTimeout < pLine->GetLineDevCaps()->dwGatherDigitsMinTimeout ||
         lpGather->dwInterDigitTimeout > pLine->GetLineDevCaps()->dwGatherDigitsMaxTimeout))
        return LINEERR_INVALTIMEOUT;
    
    // If we cannot detect the type of digits requested, return an error.
    if ((lpGather->dwDigitModes & pLine->GetLineDevCaps()->dwMonitorDigitModes) != lpGather->dwDigitModes)
        return LINEERR_INVALDIGITMODE;
                                     
    // Validate the termination digits.
    if (lpGather->dwDigitModes & LINEDIGITMODE_PULSE)
    {
        if (!lpGather->strTerminationDigits.SpanExcluding("0123456789").IsEmpty())
            return LINEERR_INVALDIGITS;
    }
    
    if (lpGather->dwDigitModes & LINEDIGITMODE_DTMF)
    {
        if (!lpGather->strTerminationDigits.SpanExcluding("0123456789ABCD*#").IsEmpty())
            return LINEERR_INVALDIGITS;
    }
                                     
    // Everything looks ok, setup the new digit gathering.
    if (m_lpGather != NULL)
        delete m_lpGather;
    
    m_lpGather = lpGather;
    m_lpGather->dwCount = 0L;
    m_lpGather->dwLastTime = GetTickCount();
        
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_GENERATEDIGITS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    CTSPILineConnection* pLine = GetLineConnectionInfo();
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
        if (!lpGenerate->strDigits.SpanExcluding("!0123456789").IsEmpty())
            return LINEERR_INVALDIGITS;
    }
    else if (lpGenerate->dwMode == LINEDIGITMODE_DTMF)
    {
        if (!lpGenerate->strDigits.SpanExcluding("!0123456789ABCD*#").IsEmpty())
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_GENERATETONE) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    CTSPILineConnection* pLine = GetLineConnectionInfo();
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_MONITORDIGITS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Validate the call state.                             
    if (GetCallState() == LINECALLSTATE_IDLE || GetCallState() == LINECALLSTATE_DISCONNECTED)
        return LINEERR_INVALCALLSTATE;

    CTSPILineConnection* pLine = GetLineConnectionInfo();
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_MONITORMEDIA) == 0)
		return LINEERR_OPERATIONUNAVAIL;
                                 
    // Validate the call state.                             
    if (GetCallState() == LINECALLSTATE_IDLE)
        return LINEERR_INVALCALLSTATE;

    CTSPILineConnection* pLine = GetLineConnectionInfo();
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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_MONITORTONES) == 0)
		return LINEERR_OPERATIONUNAVAIL;
    
    // Validate the call state.                             
    if (GetCallState() == LINECALLSTATE_IDLE)
        return LINEERR_INVALCALLSTATE;
    
    // Validate the tone monitor list.
    CTSPILineConnection* pLine = GetLineConnectionInfo();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
                                        
    // If it is disabled in the line device capabilities.                                        
    if (pLine->GetLineDevCaps()->dwMonitorToneMaxNumFreq == 0)
        return LINEERR_OPERATIONUNAVAIL;

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
// CTSPICallAppearance::SetMediaControl
//
// This function enables and disables control actions on the media stream 
// associated with the specified line, address, or call. Media control 
// actions can be triggered by the detection of specified digits, media modes, 
// custom tones, and call states.  The new specified media controls replace 
// all the ones that were in effect for this line, address, or call prior 
// to this request.
//
LONG CTSPICallAppearance::SetMediaControl (TSPIMEDIACONTROL* lpMediaControl)
{   
    m_lpMediaControl->DecUsage();  // Will auto-delete if last one using this data.
    m_lpMediaControl = lpMediaControl;    
    m_lpMediaControl->IncUsage();
        
    return FALSE;    

}// CTSPICallAppearance::SetMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnToneMonitorDetect
//
// A monitored tone has been detected, inform TAPI.
//
VOID CTSPICallAppearance::OnToneMonitorDetect (DWORD dwToneListID, DWORD dwAppSpecific)
{                                           
    CTSPILineConnection* pLine = GetLineConnectionInfo();
    ASSERT (pLine != NULL);
    ASSERT (pLine->IsKindOf (RUNTIME_CLASS(CTSPILineConnection)));

    if (pLine != NULL)
        pLine->Send_TAPI_Event(this, LINE_MONITORTONE, dwAppSpecific, dwToneListID);

}// CTSPICallAppearance::OnToneMonitorDetect

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::OnMediaControl
//
// This method is called when a media control event was activated
// due to a media monitoring event being caught.
//
VOID CTSPICallAppearance::OnMediaControl (DWORD dwMediaControl)
{                                      
    // Add a request to perform the media control active for this call.
    AddAsynchRequest (REQUEST_MEDIACONTROL, 0, NULL, dwMediaControl);

}// CTSPICallAppearance::OnMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SwapHold
//
// Swap this active call with another call on some type of
// hold
//
LONG CTSPICallAppearance::SwapHold(DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall)
{  
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SWAPHOLD) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Make sure the call state allows this.
    if (GetCallState() != LINECALLSTATE_CONNECTED &&
        GetCallState() != LINECALLSTATE_DIALTONE &&
        GetCallState() != LINECALLSTATE_DIALING &&
        GetCallState() != LINECALLSTATE_PROCEEDING)
        return LINEERR_INVALCALLSTATE;
        
    // Make sure the other call is onHold.
    if (pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDTRANSFER &&
        pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDCONF &&
        pCall->GetCallState() != LINECALLSTATE_ONHOLD)
        return LINEERR_INVALCALLSTATE;

    // Everything seems ok, submit the accept request.
    if (AddAsynchRequest(REQUEST_SWAPHOLD, dwRequestID, (LPCSTR)pCall) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;
    
}// CTSPICallAppearance::SwapHold

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetID
//
// Manage device-level requests for information based on a device id.
//
LONG CTSPICallAppearance::GetID (CString& /*strDevClass*/, LPVARSTRING /*lpDeviceID*/)
{
    // Additional support must be provided by derived class
    return LINEERR_OPERATIONUNAVAIL;
    
}// CTSPICallAppearance::GetID

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
		m_arrUserUserInfo.Add(pInfo);
    	if (GetAddressInfo()->GetAddressCaps()->dwCallFeatures & LINECALLFEATURE_RELEASEUSERUSERINFO)
			m_CallStatus.dwCallFeatures	|= LINECALLFEATURE_RELEASEUSERUSERINFO;

		// Send out a notification about the change in user information.
		OnCallInfoChange (LINECALLINFOSTATE_USERUSERINFO);
	}                      

}// CTSPICallAppearance::OnReceivedUserUserInfo
    
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
    return (dwState != LINECALLSTATE_IDLE &&
            dwState != LINECALLSTATE_UNKNOWN &&
            dwState != LINECALLSTATE_ONHOLD &&
            dwState != LINECALLSTATE_ONHOLDPENDTRANSFER &&
            dwState != LINECALLSTATE_ONHOLDPENDCONF &&
            dwState != LINECALLSTATE_CONFERENCED &&
			dwState != LINECALLSTATE_DISCONNECTED);

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
	return (dwState == LINECALLSTATE_DIALTONE ||
		    dwState == LINECALLSTATE_DIALING ||
			dwState == LINECALLSTATE_RINGBACK ||
			dwState == LINECALLSTATE_BUSY ||
			dwState == LINECALLSTATE_CONNECTED ||
			dwState == LINECALLSTATE_PROCEEDING);

}// CTSPICallAppearance::IsConnectedCallState

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AttachCall
//
// Attach a call to this call appearance
//
void CTSPICallAppearance::AttachCall (CTSPICallAppearance* pCall)
{                                  
    m_pConsult = pCall;
    
}// CTSPICallAppearance::AttachCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetAttachedCall
//
// Return the attached call appearance
//
CTSPICallAppearance* CTSPICallAppearance::GetAttachedCall()
{
    return m_pConsult;
    
}// CTSPICallAppearance::GetAttachedCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DetachCall
//
// Detach a call from this call appearance
//
void CTSPICallAppearance::DetachCall()
{                                  
    m_pConsult = NULL;

}// CTSPICallAppearance::DetachCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetConferenceOwner
//
// Set the attached conference owner
//
void CTSPICallAppearance::SetConferenceOwner(CTSPIConferenceCall* pConf)
{
	m_pConf = pConf;

}// CTSPICallAppearance::SetConferenceOwner

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetConferenceOwner
//
// Return the attached conference call appearance
//
CTSPIConferenceCall* CTSPICallAppearance::GetConferenceOwner() const
{
    return m_pConf;
    
}// CTSPICallAppearance::GetConferenceOwner

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallStateName
//
// Returns a string indicating the current state of the connection
//
const char * CTSPICallAppearance::GetCallStateName (DWORD dwState) const
{
    int iPos = 15;
    static const char * pszLineStates[] = { "Idle", "Offering", "Accepted", "Dialtone", "Dialing", "Ringback",
                                            "Busy", "SpecialInfo", "Connected", "Proceeding", "Onhold",
                                            "Conferenced", "OnHoldPendConf", "OnHoldPendTransfer", "Disconnected",
                                            "Unknown" };

    if (dwState == 0)
        dwState = GetCallState();

    switch(dwState)
    {
        case LINECALLSTATE_IDLE:               iPos = 0; break;
        case LINECALLSTATE_OFFERING:           iPos = 1; break;
        case LINECALLSTATE_ACCEPTED:           iPos = 2; break;
        case LINECALLSTATE_DIALTONE:           iPos = 3; break;
        case LINECALLSTATE_DIALING:            iPos = 4; break;
        case LINECALLSTATE_RINGBACK:           iPos = 5; break;
        case LINECALLSTATE_BUSY:               iPos = 6; break;
        case LINECALLSTATE_SPECIALINFO:        iPos = 7; break;
        case LINECALLSTATE_CONNECTED:          iPos = 8; break;
        case LINECALLSTATE_PROCEEDING:         iPos = 9; break;
        case LINECALLSTATE_ONHOLD:             iPos = 10; break;
        case LINECALLSTATE_CONFERENCED:        iPos = 11; break;
        case LINECALLSTATE_ONHOLDPENDCONF:     iPos = 12; break;
        case LINECALLSTATE_ONHOLDPENDTRANSFER: iPos = 13; break;
        case LINECALLSTATE_DISCONNECTED:       iPos = 14; break;
        case LINECALLSTATE_UNKNOWN:            iPos = 15; break;
    }

    ASSERT( iPos >= 0 && iPos <= 15 );
    return pszLineStates[iPos];

}// CTSPICallAppearance::GetCallStateName
#endif // _DEBUG

