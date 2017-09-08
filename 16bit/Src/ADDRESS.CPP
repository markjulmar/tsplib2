/******************************************************************************/
//                                                                        
// ADDRESS.CPP - Source code for the CTSPIAddressInfo object          
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the source to manage the address objects which are 
// held by the CTSPILineConnection objects.
//
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
//                                                                        
/******************************************************************************/

#include "stdafx.h"
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////
// Run-Time class information 

IMPLEMENT_DYNCREATE( CTSPIAddressInfo, CObject )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CTSPIAddressInfo
//
// This is the constructor for the address information structure.
//
CTSPIAddressInfo::CTSPIAddressInfo()
{
    m_pLine = NULL;
    m_dwAddressID = (DWORD) -1L;
    m_dwAddressStates = 0L;
	m_dwConnectedCallCount = 0;
    
    FillBuffer (&m_AddressCaps, 0, sizeof(LINEADDRESSCAPS));
    FillBuffer (&m_AddressStatus, 0, sizeof(LINEADDRESSSTATUS));

}// CTSPIAddressInfo::CTSPIAddressInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::~CTSPIAddressInfo
//
// Address information object destructure - delete all existing
// call appearances.
//
CTSPIAddressInfo::~CTSPIAddressInfo()
{   
    // Delete the forwarding information - this will request
    // and release the mutex.
    DeleteForwardingInfo();

    // Delete all the call appearances still on the address
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        delete pCall;
    }
    
    // Remove all the array information
    m_lstCalls.RemoveAll();
    m_arrTerminals.RemoveAll();

}// CTSPIAddressInfo::~CTSPIAddressInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::Init
//
// Initialization function for dynamically created object.  This initializes
// our internal structures for this address.  It can be overriden to add new
// fields to the ADDRESSCAPS, or the lineGetAddressCaps of the CServiceProvider
// class may be overriden, or the GetAddressCaps of this class may also be
// overriden.
//
VOID CTSPIAddressInfo::Init (CTSPILineConnection* pLine, DWORD dwAddressID, LPCSTR lpszAddress,
                             LPCSTR lpszName, BOOL fSupportsIncomingCalls, 
                             BOOL fSupportsOutgoingCalls,DWORD dwAvailMediaModes, 
                             DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate,
                             DWORD dwMaxNumActiveCalls, DWORD dwMaxNumOnHoldCalls, 
                             DWORD dwMaxNumOnHoldPendCalls, DWORD dwMaxNumConference, 
                             DWORD dwMaxNumTransConf)
{
    // Fill in the passed information
    m_pLine = pLine;
    m_dwAddressID = dwAddressID;
    m_fInputAvail = fSupportsIncomingCalls;
    m_fOutputAvail = fSupportsOutgoingCalls;
    m_dwBearerMode = dwBearerMode;
    m_dwMinRateAvail = dwMinRate;
    m_dwMaxRateAvail = dwMaxRate;
	m_dwCurrRate = dwMinRate;
    m_strName = lpszName;
    
    // Move the address over - only numbers please!
    ASSERT (lpszAddress != NULL);
    m_strAddress = GetSP()->GetDialableNumber (lpszAddress);

	// Mark the available media modes for this address.
	m_dwAvailMediaModes = (dwAvailMediaModes | LINEMEDIAMODE_UNKNOWN);

    // Now gather the address capabilities, we assume the address is private - override this
    // function and change it if necessary.
    m_AddressCaps.dwAddressSharing = LINEADDRESSSHARING_PRIVATE;

    // Mark the available address state notifications the class library manages.
    m_AddressCaps.dwAddressStates = (LINEADDRESSSTATE_OTHER | LINEADDRESSSTATE_INUSEZERO | LINEADDRESSSTATE_INUSEONE | 
                                     LINEADDRESSSTATE_FORWARD | LINEADDRESSSTATE_INUSEMANY | LINEADDRESSSTATE_NUMCALLS | 
                                     LINEADDRESSSTATE_TERMINALS);
    if (GetSP()->GetSupportedVersion() >= TAPIVER_14 && GetSP()->GetSystemVersion() >= TAPIVER_14)     
        m_AddressCaps.dwAddressStates |= LINEADDRESSSTATE_CAPSCHANGE;

    // Mark the different LINE_CALLINFO messages which we can generate by our call appearances.  The only
    // field which cannot be generated by the library is DEVSPECIFIC - if this is supported by your service provider,
    // make sure to add it to this list in your derived class
    m_AddressCaps.dwCallInfoStates = (LINECALLINFOSTATE_OTHER | LINECALLINFOSTATE_BEARERMODE | 
					LINECALLINFOSTATE_RATE | LINECALLINFOSTATE_MEDIAMODE | 
					LINECALLINFOSTATE_APPSPECIFIC | LINECALLINFOSTATE_CALLID | 
					LINECALLINFOSTATE_RELATEDCALLID | LINECALLINFOSTATE_ORIGIN | 
					LINECALLINFOSTATE_REASON | LINECALLINFOSTATE_COMPLETIONID | 
					LINECALLINFOSTATE_TRUNK | LINECALLINFOSTATE_CALLERID | 
					LINECALLINFOSTATE_CALLEDID | LINECALLINFOSTATE_CONNECTEDID | 
					LINECALLINFOSTATE_REDIRECTIONID | LINECALLINFOSTATE_REDIRECTINGID |
                    LINECALLINFOSTATE_USERUSERINFO | LINECALLINFOSTATE_TERMINAL |
                    LINECALLINFOSTATE_DIALPARAMS | LINECALLINFOSTATE_MONITORMODES);

    // Mark the supported caller id fields.
    m_AddressCaps.dwCallerIDFlags = 
    m_AddressCaps.dwConnectedIDFlags = 
    m_AddressCaps.dwRedirectionIDFlags =
    m_AddressCaps.dwRedirectingIDFlags =
    m_AddressCaps.dwCalledIDFlags = (LINECALLPARTYID_BLOCKED | LINECALLPARTYID_OUTOFAREA |
                                     LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS | 
                                     LINECALLPARTYID_PARTIAL | LINECALLPARTYID_UNKNOWN | 
                                     LINECALLPARTYID_UNAVAIL);

    // Mark the call states we transition through in the class library
    m_AddressCaps.dwCallStates = (LINECALLSTATE_IDLE | LINECALLSTATE_CONNECTED | LINECALLSTATE_UNKNOWN |
                                  LINECALLSTATE_PROCEEDING | LINECALLSTATE_DISCONNECTED | 
                                  LINECALLSTATE_BUSY | LINECALLSTATE_SPECIALINFO);
    
    // Set the tone modes and disconnect information - report them all as available even though 
    // the service provider might not actually report some of them
    m_AddressCaps.dwDialToneModes = (LINEDIALTONEMODE_NORMAL | LINEDIALTONEMODE_SPECIAL |
                                    LINEDIALTONEMODE_INTERNAL | LINEDIALTONEMODE_EXTERNAL |
                                    LINEDIALTONEMODE_UNKNOWN | LINEDIALTONEMODE_UNAVAIL);
    m_AddressCaps.dwBusyModes = (LINEBUSYMODE_STATION | LINEBUSYMODE_TRUNK | LINEBUSYMODE_UNKNOWN |
                                LINEBUSYMODE_UNAVAIL);

    // Report special info as unavailable/unknown - if the service provider is to support
    // any of the special tone information, then add the appropriate fields.
    m_AddressCaps.dwSpecialInfo = LINESPECIALINFO_UNKNOWN | LINESPECIALINFO_UNAVAIL;

    // Report all the disconnect modes - some may not be reported by the service provider, but
    // its still ok to list them to TAPI.
    m_AddressCaps.dwDisconnectModes = (LINEDISCONNECTMODE_NORMAL | LINEDISCONNECTMODE_UNKNOWN |
                                       LINEDISCONNECTMODE_REJECT | LINEDISCONNECTMODE_BUSY |
                                       LINEDISCONNECTMODE_NOANSWER | LINEDISCONNECTMODE_BADADDRESS |
                                       LINEDISCONNECTMODE_UNREACHABLE | LINEDISCONNECTMODE_CONGESTION |
                                       LINEDISCONNECTMODE_INCOMPATIBLE | LINEDISCONNECTMODE_UNAVAIL);
    if (GetSP()->GetSupportedVersion() >= TAPIVER_14 &&
        GetSP()->GetSystemVersion() >= TAPIVER_14)
        m_AddressCaps.dwDisconnectModes |= LINEDISCONNECTMODE_NODIALTONE;
        
    // Set the max calls information
    m_AddressCaps.dwMaxNumActiveCalls = dwMaxNumActiveCalls;
    m_AddressCaps.dwMaxNumOnHoldCalls = dwMaxNumOnHoldCalls;
    m_AddressCaps.dwMaxNumConference = dwMaxNumConference;
    m_AddressCaps.dwMaxNumTransConf = dwMaxNumTransConf;
    m_AddressCaps.dwMaxNumOnHoldPendingCalls = dwMaxNumOnHoldPendCalls;

    // Set the address capability flags to a generic set.  Replace the flags here with the ones
    // the service provider really supports.
    m_AddressCaps.dwAddrCapFlags = LINEADDRCAPFLAGS_DIALED | LINEADDRCAPFLAGS_ORIGOFFHOOK | LINEADDRCAPFLAGS_COMPLETIONID;
    
    // Determine which capabilities this address supports.
    m_AddressCaps.dwCallFeatures = 0L;
    m_AddressCaps.dwForwardModes = 0L;

    // Add in the call features available.
    if (CanMakeCalls() && CanHandleRequest(REQUEST_MAKECALL))
    {
        m_AddressCaps.dwCallStates |= (LINECALLSTATE_DIALING | LINECALLSTATE_DIALTONE | LINECALLSTATE_RINGBACK);
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_MAKECALL;
    }        

	if (CanHandleRequest(REQUEST_DIAL))
	{
        m_AddressCaps.dwCallStates |= LINECALLSTATE_DIALING;
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_DIAL;
	}

    if (CanAnswerCalls() && CanHandleRequest(REQUEST_ANSWER))
    {
        m_AddressCaps.dwCallStates |= (LINECALLSTATE_OFFERING | LINECALLSTATE_ACCEPTED);
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_ANSWER;
    }
            
    if (CanHandleRequest(REQUEST_SETUPCONF))
    {
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETUPCONF;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_SETUPCONF;
    }   
         
    if (CanHandleRequest(REQUEST_SETUPXFER))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETUPTRANSFER;
        
    if (CanHandleRequest(REQUEST_PARK))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_PARK;
        
    if (CanHandleRequest(REQUEST_DROPCALL))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_DROP;
        
    if (CanHandleRequest(REQUEST_ADDCONF))
    {
        m_AddressCaps.dwCallStates |= (LINECALLSTATE_CONFERENCED | LINECALLSTATE_ONHOLDPENDCONF);
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
    }
        
    if (CanHandleRequest(REQUEST_BLINDXFER))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_BLINDTRANSFER;
        
    if (CanHandleRequest(REQUEST_HOLD))
    {
        m_AddressCaps.dwCallStates |= LINECALLSTATE_ONHOLD;
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_HOLD;
    }
            
    if (CanHandleRequest(REQUEST_SENDUSERINFO))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SENDUSERUSER;
        
    if (CanHandleRequest(REQUEST_SWAPHOLD))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
        
    if (CanHandleRequest(REQUEST_REDIRECT))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_REDIRECT;
        
    if (CanHandleRequest(REQUEST_ACCEPT))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_ACCEPT;
        
    if (CanHandleRequest(REQUEST_REMOVEFROMCONF))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_REMOVEFROMCONF;
        
    if (CanHandleRequest(REQUEST_UNHOLD))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_UNHOLD;
        
    if (CanHandleRequest(REQUEST_COMPLETEXFER))
    {
        m_AddressCaps.dwCallStates |= LINECALLSTATE_ONHOLDPENDTRANSFER;
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
    }
            
    if (CanHandleRequest(REQUEST_COMPLETECALL))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_COMPLETECALL;
        
    if (CanHandleRequest(REQUEST_SECURECALL))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SECURECALL;
        
    if (CanHandleRequest(REQUEST_SETCALLPARAMS))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETCALLPARAMS;
        
    if (CanHandleRequest(REQUEST_SETTERMINAL))
    {
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETTERMINAL;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_SETTERMINAL;
    }        
    
    if (CanHandleRequest(REQUEST_MEDIACONTROL))
    {
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETMEDIACONTROL;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_SETMEDIACONTROL;
    }        
    
    if (CanHandleRequest(REQUEST_FORWARD))
    {
        m_AddressCaps.dwMaxForwardEntries = 1;
        m_AddressCaps.dwMaxSpecificEntries = 0L;
        m_AddressCaps.dwMinFwdNumRings = 0L;
        m_AddressCaps.dwMaxFwdNumRings = 0L;
        m_AddressCaps.dwDisconnectModes |= LINEDISCONNECTMODE_FORWARDED;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_FORWARD; 
    }
        
    if (CanHandleRequest(REQUEST_PICKUP))
    {
        m_AddressCaps.dwDisconnectModes |= LINEDISCONNECTMODE_PICKUP;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_PICKUP; 
    }        
    
    if (CanHandleRequest(REQUEST_UNCOMPLETECALL))
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_UNCOMPLETECALL;      
    
    if (CanHandleRequest(REQUEST_UNPARK))
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_UNPARK;

    // Add the additional call features based on whether we can support
    // the request.
    if (CanHandleRequest(REQUEST_GATHERDIGITS))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_GATHERDIGITS;
    if (CanHandleRequest(REQUEST_GENERATEDIGITS))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_GENERATEDIGITS;
    if (CanHandleRequest(REQUEST_GENERATETONE))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_GENERATETONE;
    if (CanHandleRequest(REQUEST_MONITORDIGITS))        
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_MONITORDIGITS;
    if (CanHandleRequest(REQUEST_MONITORMEDIA))     
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_MONITORMEDIA;
    if (CanHandleRequest(REQUEST_MONITORTONES))     
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_MONITORTONES;
    if (CanHandleRequest(REQUEST_PREPAREADDCONF))       
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_PREPAREADDCONF;
    if (CanHandleRequest(REQUEST_MEDIACONTROL))     
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETMEDIACONTROL;
    if (CanHandleRequest(REQUEST_RELEASEUSERINFO))      
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_RELEASEUSERUSERINFO;

    // Store the additional flags for conferencing and transfer.  If transfer/pickup
    // supported, you need to alter the flags to indicate the mode the service 
    // provider works in.
    m_AddressCaps.dwRemoveFromConfCaps = LINEREMOVEFROMCONF_NONE;
    m_AddressCaps.dwRemoveFromConfState = LINECALLSTATE_UNKNOWN;
    m_AddressCaps.dwTransferModes = 0L;
    m_AddressCaps.dwParkModes = 0L;
    
    // Manage the call completion capabilities.  If the service provider supports
    // call completion, then these flags need to be altered to suite the code manager.
    m_AddressCaps.dwMaxCallCompletions = 0L;
    m_AddressCaps.dwCallCompletionConds = 0L;
    m_AddressCaps.dwCallCompletionModes = 0L;

    // The messages are managed automatically through the AddCompletionMessage API.
    m_AddressCaps.dwNumCompletionMessages = 0L;
    m_AddressCaps.dwCompletionMsgTextEntrySize = 0L;

    // Grab the terminal information from our parent line.  This information
    // will then be applied to any of our calls.
    for (int i = 0; i < m_pLine->GetTerminalCount(); i++)
        m_arrTerminals.Add(m_pLine->GetTerminalInformation(i));     

    // Setup the initial address status.  We assume no calls are currently on
    // the line.
    m_AddressStatus.dwTotalSize = sizeof(LINEADDRESSSTATUS);
    m_AddressStatus.dwAddressFeatures = (m_AddressCaps.dwAddressFeatures & 
                               (LINEADDRFEATURE_SETMEDIACONTROL |
                                LINEADDRFEATURE_SETTERMINAL |
                                LINEADDRFEATURE_FORWARD | 
                                LINEADDRFEATURE_PICKUP |
                                LINEADDRFEATURE_MAKECALL));

}// CTSPIAddressInfo::Init

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddAsynchRequest
//
// This method adds a request to a particular connection.  It
// will add the request to the device list this connection belongs
// to.
//
CTSPIRequest* CTSPIAddressInfo::AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId, LPCVOID lpBuff, DWORD dwSize)
{
    return GetLineOwner()->AddAsynchRequest(this, NULL, wReqId, dwReqId, lpBuff, dwSize);

}// CTSPIAddressInfo::AddAsynchRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanHandleRequest
//
// Determine if our service provider is capable of handling the request.
// 
BOOL CTSPIAddressInfo::CanHandleRequest(WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest(this, wRequest, dwData);                

}// CTSPIAddressInfo::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnAddressStateChange
//
// Send a TAPI event for this address.
//
VOID CTSPIAddressInfo::OnAddressStateChange (DWORD dwAddressState)
{
    if ((m_dwAddressStates & dwAddressState) == dwAddressState)
        GetLineOwner()->Send_TAPI_Event (NULL, LINE_ADDRESSSTATE, GetAddressID(), dwAddressState);

}// CTSPIAddressInfo::Send_TAPI_Event

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAddressID
//
// Return the address identifier for this address.  This is always a
// numeric number from 0-numAddr on the line owner.
//
DWORD CTSPIAddressInfo::GetAddressID() const
{
    return m_dwAddressID;

}// CTSPIAddressInfo::GetAddressID

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetLineOwner
//
// Return the line owner for this address
//
CTSPILineConnection* CTSPIAddressInfo::GetLineOwner() const
{
    return m_pLine;

}// CTSPIAddressInfo::GetLineOwner

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CreateConferenceCall
//
// This method creates a call appearance which will be used as a
// conference call.
//
CTSPIConferenceCall* CTSPIAddressInfo::CreateConferenceCall(HTAPICALL hCall)
{
    CTSPIConferenceCall* pCall = NULL;

    // See if the call appearance already exists.
    if (hCall != NULL)
    {
        pCall = (CTSPIConferenceCall*) FindCallByHandle (hCall);
        if (pCall)
            return pCall;
    }

    // Create the call appearance
    pCall = (CTSPIConferenceCall*) GetSP()->GetTSPIConferenceCallObj()->CreateObject();
    ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPIConferenceCall)));
    if (pCall == NULL)
        return NULL;

    // Increment our total call count.
    m_AddressStatus.dwNumInUse++;

    // If we don't have a call handle, then ask TAPI for one.
    BOOL fNewCall = FALSE;
    if (hCall == NULL)
    {
        DWORD dwTapiCall;
        GetLineOwner()->Send_TAPI_Event (NULL, LINE_NEWCALL, (DWORD)pCall, (DWORD)&dwTapiCall);
        if (dwTapiCall != 0)
            hCall = (HTAPICALL) dwTapiCall;
        fNewCall = TRUE;            
    }

    TRACE("CreateConferenceCall: SP call=0x%lx, TAPI call=0x%lx\r\n", (DWORD)pCall, (DWORD)hCall);

    // Init the call appearance with a handle.
    pCall->Init(this, hCall, m_dwBearerMode, m_dwCurrRate, 0, LINECALLORIGIN_CONFERENCE, 
                LINECALLREASON_DIRECT, 0xffff, 0, fNewCall);

    // Add it to our list.
    m_lstCalls.AddTail((CObject*)pCall);
    ASSERT (m_lstCalls.GetCount() == (int) m_AddressStatus.dwNumInUse);
    
    // Send out a notification about the number of calls on the address changing.
    OnAddressStateChange (LINEADDRESSSTATE_NUMCALLS);

    // Notify ourselves in case a derived class wants to know.
    OnCreateCall (pCall);

    return pCall;

}// CTSPIAddressInfo::CreateConferenceCall

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CreateCallAppearance
//
// This method creates a new call appearance.  The call appearance
// object is returned to the caller.
//
CTSPICallAppearance* CTSPIAddressInfo::CreateCallAppearance(HTAPICALL hCall,
                                DWORD dwCallParamFlags, DWORD dwOrigin, 
                                DWORD dwReason, DWORD dwTrunk, DWORD dwCompletionID)
{
    CTSPICallAppearance* pCall = NULL;

    // See if the call appearance already exists.
    if (hCall != NULL)
    {
        pCall = FindCallByHandle (hCall);
        if (pCall)
            return pCall;
    }

    // Create the call appearance
    pCall = (CTSPICallAppearance*) GetSP()->GetTSPICallObj()->CreateObject();
    ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

    if (pCall == NULL)
        return NULL;
    
    // Increment our total call count.
    m_AddressStatus.dwNumInUse++;
    
    // If we don't have a call handle, then ask TAPI for one.
    BOOL fNewCall = FALSE;
    if (hCall == NULL)
    {
        DWORD dwTapiCall;
        GetLineOwner()->Send_TAPI_Event (NULL, LINE_NEWCALL, (DWORD)pCall, (DWORD)&dwTapiCall);
        if (dwTapiCall != 0)
            hCall = (HTAPICALL) dwTapiCall;
        fNewCall = TRUE;            
    }

    TRACE("CreateCallAppearance: SP call=0x%lx, TAPI call=0x%lx\r\n", (DWORD)pCall, (DWORD)hCall);

	// If the completion ID is non-zero, then use the appropriate reason.
	if (dwCompletionID > 0 && dwReason == LINECALLREASON_UNKNOWN)
		dwReason = LINECALLREASON_CALLCOMPLETION;

    // Init the call appearance with a handle.
    pCall->Init(this, hCall, m_dwBearerMode, m_dwCurrRate, 
                dwCallParamFlags, dwOrigin, dwReason, dwTrunk, dwCompletionID, fNewCall);

    // Add it to our list.
    m_lstCalls.AddTail((CObject*)pCall);
    ASSERT (m_lstCalls.GetCount() == (int) m_AddressStatus.dwNumInUse);
    
    // Send out a notification about the number of calls on the address changing.
    OnAddressStateChange (LINEADDRESSSTATE_NUMCALLS);

    // Notify ourselves in case a derived class wants to know.
    OnCreateCall (pCall);

    return pCall;

}// CTSPILineConnection::CreateCallAppearance

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::RemoveCallAppearance
//
// Remove a call appearance from our list and delete it.
//
VOID CTSPIAddressInfo::RemoveCallAppearance (CTSPICallAppearance* pCall)
{
    // If it is null, ignore it.
    if (pCall == NULL)
        return;

	// If it has already been deleted, ignore it.
	if (pCall->m_dwFlags & CTSPICallAppearance::IsDeleted)
		return;

	// If it has an attached call, notify a change.  This will cause us
	// to re-attach any other call or zero out the attachment.
	if (pCall->GetAttachedCall() != NULL)
	{
		pCall->GetAttachedCall()->
			OnRelatedCallStateChange(pCall, LINECALLSTATE_IDLE, LINECALLSTATE_UNKNOWN);
	}
                                    
    // First locate and remove it from our array
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        POSITION posCurr = pos;
        CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pTest == pCall)
        {
            // Decrement our total call count.
            m_AddressStatus.dwNumInUse--;
            if (m_AddressStatus.dwNumInUse > 0x80000000)
                m_AddressStatus.dwNumInUse = 0;
            m_lstCalls.RemoveAt(posCurr);
            break;
        }
    }

	// Make sure it goes IDLE for our active call counts.
	pCall->SetCallState(LINECALLSTATE_IDLE, 0, 0, FALSE);
    
    // Make sure the call isn't referenced in a request packet anywhere.
    GetLineOwner()->OnCallDeleted (pCall);
    
	// Mark the call deleted
	pCall->m_dwFlags |= CTSPICallAppearance::IsDeleted;
    pCall->DecRefCount();

    // Send out a notification about the number of calls on the address changing.
    OnAddressStateChange (LINEADDRESSSTATE_NUMCALLS);

}// CTSPIAddressInfo::RemoveCallAppearance

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCallCount
//
// Return the total number of allocated CTSPICallAppearance object in
// the call array.  Note that some of the objects may be idle.
//
int CTSPIAddressInfo::GetCallCount() const
{
	return m_lstCalls.GetCount();
	
}// CTSPIAddressInfo::GetCallCount
                     
///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::FindAttachedCall
//
// Returns a pointer to the call which is attached to the specified
// call.  This allows a link of attached calls.
//
CTSPICallAppearance* CTSPIAddressInfo::FindAttachedCall(CTSPICallAppearance* pSCall) const
{
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pCall->GetAttachedCall() == pSCall)
            return pCall;
    }
    return NULL;

}// CTSPIAddressInfo::FindAttachedCall
                     
///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCallInfo
//
// Return the call information object based on a position in our
// list.
//
CTSPICallAppearance* CTSPIAddressInfo::GetCallInfo(int iPos) const
{
    int iCount = 0;
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (iCount == iPos)
            return pCall;
        iCount++;
    }
    return NULL;

}// CTSPIAddressInfo::GetCallInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::FindCallByState
//
// Returns a pointer to the call with the specified call state
//
CTSPICallAppearance* CTSPIAddressInfo::FindCallByState(DWORD dwState) const
{
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if ((pCall->GetCallState() & dwState) == dwState)
            return pCall;
    }
    return NULL;

}// CTSPIAddressInfo::FindCallByState

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::FindCallByCallID
//
// Locate a call by the dwCallID field in CALLINFO.  This function may
// be used by providers to match up a call appearance on an address to
// a device call which has been identified and placed into the dwCallID field
// of the CALLINFO record.
//
CTSPICallAppearance* CTSPIAddressInfo::FindCallByCallID (DWORD dwCallID) const
{
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pCall->GetCallInfo()->dwCallID == dwCallID)
            return pCall;
    }
    return NULL;

}// CTSPIAddressInfo::FindCallByCallID

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnCallStateChange
//
// A call appearance on this address has changed states.  Potentially
// send out an address state change.
//
VOID CTSPIAddressInfo::OnCallStateChange (CTSPICallAppearance* pCall, DWORD dwNewState, DWORD dwOldState)
{
    // Determine if the number of active calls has changed.
    BOOL fWasActive = CTSPICallAppearance::IsActiveCallState(dwOldState);
    BOOL fIsActive  = CTSPICallAppearance::IsActiveCallState(dwNewState);
	BOOL fWasConnected = CTSPICallAppearance::IsConnectedCallState(dwOldState);
	BOOL fIsConnected = CTSPICallAppearance::IsConnectedCallState(dwNewState);
    BOOL fSendNumCalls = FALSE;
    
    // If the call is now active, but wasn't before, then our active
    // call count is up by one.
    if (fWasActive == FALSE && fIsActive == TRUE)
    {                                   
        m_AddressStatus.dwNumActiveCalls++;
        fSendNumCalls = TRUE;
    }

	// Or if the number of active calls has gone down
    else if (fWasActive == TRUE && fIsActive == FALSE)
    {
        m_AddressStatus.dwNumActiveCalls--;
        if (m_AddressStatus.dwNumActiveCalls & 0x80000000)
            m_AddressStatus.dwNumActiveCalls = 0L;
        fSendNumCalls = TRUE;
    }       

	// Count the CONNECTED calls.  This is used to determine
	// the bandwidth of the TSP.
	if (fWasConnected == FALSE && fIsConnected == TRUE)
	{
		m_dwConnectedCallCount++;
		GetLineOwner()->OnConnectedCallCountChange(this, 1);
	}
	else if (fWasConnected == TRUE && fIsConnected == FALSE)
	{
		if (--m_dwConnectedCallCount & 0x80000000)
			m_dwConnectedCallCount = 0;
		GetLineOwner()->OnConnectedCallCountChange(this, -1);
	}

    // Determine if the HOLD status has changed.        
    if (dwNewState == LINECALLSTATE_ONHOLD && dwOldState != dwNewState)
    {
        m_AddressStatus.dwNumOnHoldCalls++;
        fSendNumCalls = TRUE;
    }
    else if ((dwNewState == LINECALLSTATE_ONHOLDPENDTRANSFER ||
              dwNewState == LINECALLSTATE_ONHOLDPENDCONF) &&
             (dwOldState != LINECALLSTATE_ONHOLDPENDTRANSFER &&
              dwOldState != LINECALLSTATE_ONHOLDPENDCONF))
    {
        m_AddressStatus.dwNumOnHoldPendCalls++;
        fSendNumCalls = TRUE;
    }

    if (dwOldState == LINECALLSTATE_ONHOLD && dwNewState != dwOldState)
    {
        m_AddressStatus.dwNumOnHoldCalls--;
        if (m_AddressStatus.dwNumOnHoldCalls & 0x80000000)
            m_AddressStatus.dwNumOnHoldCalls = 0L;
        fSendNumCalls = TRUE;
    }

    else if ((dwOldState == LINECALLSTATE_ONHOLDPENDTRANSFER ||
              dwOldState == LINECALLSTATE_ONHOLDPENDCONF) &&
             (dwNewState != LINECALLSTATE_ONHOLDPENDTRANSFER &&
              dwNewState != LINECALLSTATE_ONHOLDPENDCONF))
    {
        m_AddressStatus.dwNumOnHoldPendCalls--;
        if (m_AddressStatus.dwNumOnHoldPendCalls & 0x80000000)
            m_AddressStatus.dwNumOnHoldPendCalls = 0L;
        fSendNumCalls = TRUE;
    }

    if (fSendNumCalls)
    {
        TRACE(_T("Address Call Counts NewCallState=0x%lx, OldCallState=0x%lx,  Active=%ld, OnHold=%ld,  OnHoldPend=%ld\r\n"),
            dwNewState, dwOldState,
            m_AddressStatus.dwNumActiveCalls,
            m_AddressStatus.dwNumOnHoldCalls,
            m_AddressStatus.dwNumOnHoldPendCalls);            
        OnAddressStateChange (LINEADDRESSSTATE_NUMCALLS);
    }        

    // Determine how many are not idle.
    DWORD dwCount = m_AddressStatus.dwNumActiveCalls + 
					m_AddressStatus.dwNumOnHoldCalls + 
					m_AddressStatus.dwNumOnHoldPendCalls;
    switch (dwCount)
    {
        case 0L:  OnAddressStateChange (LINEADDRESSSTATE_INUSEZERO); break;
        case 1L:  OnAddressStateChange (LINEADDRESSSTATE_INUSEONE); break;
        default:  OnAddressStateChange (LINEADDRESSSTATE_INUSEMANY); break;
    }

	// Adjust our address features based on the call counts we now have.
	DWORD dwAddressFeatures = m_AddressStatus.dwAddressFeatures;

	// If there are active calls, and we support conferencing, then show SetupConf as a feature
	if (m_dwConnectedCallCount > 0 && (m_AddressCaps.dwAddressFeatures & LINEADDRFEATURE_SETUPCONF))
    	dwAddressFeatures |= LINEADDRFEATURE_SETUPCONF;
	else
		dwAddressFeatures &= ~LINEADDRFEATURE_SETUPCONF;

    // Determine if any calls are waiting (camped on) pending call completions.
    if (m_AddressCaps.dwAddressFeatures & LINEADDRFEATURE_UNCOMPLETECALL)
    {
    	dwAddressFeatures &= ~LINEADDRFEATURE_UNCOMPLETECALL;
    	for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    	{
    		CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
    		if (GetLineOwner()->FindCallCompletionRequest(pCall))
    		{
        		dwAddressFeatures |= LINEADDRFEATURE_UNCOMPLETECALL;
    		    break;
    		}
    	}
    }
    
    // If we have the bandwidth for another active call, then setup some of the
    // features which create a new call appearance.
    if (m_AddressStatus.dwNumActiveCalls < m_AddressCaps.dwMaxNumActiveCalls)
    {
		if (CanMakeCalls())
			dwAddressFeatures |= LINEADDRFEATURE_MAKECALL;
        if (CanAnswerCalls() && (m_AddressCaps.dwAddressFeatures & LINEADDRFEATURE_PICKUP))
            dwAddressFeatures |= LINEADDRFEATURE_PICKUP;
        if (m_AddressCaps.dwAddressFeatures & LINEADDRFEATURE_UNPARK)
            dwAddressFeatures |= LINEADDRFEATURE_UNPARK;
    }
    else
   		dwAddressFeatures &= ~(LINEADDRFEATURE_MAKECALL | 
					LINEADDRFEATURE_PICKUP | LINEADDRFEATURE_UNPARK);

	// Set our address features
	m_AddressStatus.dwAddressFeatures = OnAddressFeaturesChanged(dwAddressFeatures);

    // Tell our line owner about the call changing state.
    GetLineOwner()->OnCallStateChange(this, pCall, dwNewState, dwOldState);

}// CTSPIAddressInfo::OnCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::FindCallByHandle
//
// Locate a call appearance by the handle used in TAPI
//
CTSPICallAppearance* CTSPIAddressInfo::FindCallByHandle(HTAPICALL htCall) const
{
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pCall->GetCallHandle() == htCall)
            return pCall;
    }
    return NULL;

}// CTSPIAddressInfo::FindCallByHandle

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GatherCapabilities
//
// Gather all the address capabilities and store them off into an
// ADDRESSCAPS structure for TAPI.
//
LONG CTSPIAddressInfo::GatherCapabilities (
DWORD dwTSPIVersion,                   // Version of SPI expected
DWORD /*dwExtVersion*/,                // Driver specific version
LPLINEADDRESSCAPS lpAddressCaps)       // Address CAPS structure to fill in
{
    // Grab the dialable address
    CString strAddr = GetDialableAddress();
    int cbLineAddr = 0;
    if (!strAddr.IsEmpty())
        cbLineAddr = strAddr.GetLength() + 1;

    // Grab our device id.  We do this at the time of the call to insure that
    // we always get the current line id in case this line was dynamically added -
    // it can change.
    m_AddressCaps.dwTotalSize = lpAddressCaps->dwTotalSize;
    m_AddressCaps.dwLineDeviceID = GetLineOwner()->GetDeviceID();
    m_AddressCaps.dwNeededSize = sizeof(LINEADDRESSCAPS) + cbLineAddr + lpAddressCaps->dwCompletionMsgTextSize;
    
    // Make sure we have enough space.  Note that we must subtract the address
    // features which were added in TAPI 1.4 for backward compatibility.
    DWORD dwReqSize;
    if (dwTSPIVersion >= TAPIVER_14)
        dwReqSize = sizeof(LINEADDRESSCAPS);
    else
        dwReqSize = (sizeof(LINEADDRESSCAPS) - sizeof(DWORD));

#ifdef _DEBUG
    // TAPI is supposed to verify the structure size for us.  Check it in DEBUG
    // builds just to be certain, but count on it in the retail version.
    if (lpAddressCaps->dwTotalSize < dwReqSize)
    {
        lpAddressCaps->dwNeededSize = m_AddressCaps.dwNeededSize;
        return LINEERR_STRUCTURETOOSMALL;
    }
#endif

    // Copy over our STATIC version of the capabilities.
    CopyBuffer (lpAddressCaps, &m_AddressCaps, dwReqSize);
    lpAddressCaps->dwUsedSize = dwReqSize;
    
    // Remove the things which are not available previous to TAPI 1.4
    if (dwTSPIVersion < TAPIVER_14)
    {
        lpAddressCaps->dwAddressStates &= ~(LINEADDRESSSTATE_CAPSCHANGE);
        lpAddressCaps->dwDisconnectModes &= ~(LINEDISCONNECTMODE_NODIALTONE);
    }
    else
    {
        // If the structure is big enough to support the new features
        // added in TAPI 1.4 then add the address features
        if (lpAddressCaps->dwTotalSize >= sizeof(LINEADDRESSCAPS))
        {
            lpAddressCaps->dwUsedSize = sizeof(LINEADDRESSCAPS);
            lpAddressCaps->dwAddressFeatures = m_AddressCaps.dwAddressFeatures;
        }
    }       

    // Add the string if we have the space.
    if (lpAddressCaps->dwTotalSize >= lpAddressCaps->dwUsedSize + cbLineAddr)
    {
        lpAddressCaps->dwAddressOffset = lpAddressCaps->dwUsedSize;
        lpAddressCaps->dwAddressSize = cbLineAddr;
        CopyBuffer((LPSTR)lpAddressCaps + lpAddressCaps->dwUsedSize, (LPVOID)(LPCSTR)strAddr, cbLineAddr);
        lpAddressCaps->dwUsedSize += cbLineAddr;
    }

    // Add the completion messages if we have the space
    if (lpAddressCaps->dwNumCompletionMessages > 0)
    {   
        // See if we have the space
        if (lpAddressCaps->dwTotalSize >= lpAddressCaps->dwUsedSize + lpAddressCaps->dwCompletionMsgTextSize)
        {
            LPSTR lpszBuff = (LPSTR)lpAddressCaps + lpAddressCaps->dwUsedSize;
            lpAddressCaps->dwCompletionMsgTextOffset = lpAddressCaps->dwUsedSize;
            lpAddressCaps->dwUsedSize += lpAddressCaps->dwCompletionMsgTextSize;
            for (int i = 0; i < m_arrCompletionMsgs.GetSize(); i++)
            {   
                CString strBuff = m_arrCompletionMsgs[i];
                FillBuffer (lpszBuff, ' ', lpAddressCaps->dwCompletionMsgTextEntrySize-1);
                CopyBuffer (lpszBuff, (LPVOID)(LPCSTR)strBuff, strBuff.GetLength());
                lpszBuff += lpAddressCaps->dwCompletionMsgTextEntrySize-1;
                *lpszBuff++ = '\0';
            }
        }
    }

    return FALSE;

}// CTSPIAddressInfo::GatherCapabilities

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnAddressCapabiltiesChanged
//
// The address capabilities have changed (ADDRESSCAPS), tell TAPI 
// about it.
//
VOID CTSPIAddressInfo::OnAddressCapabiltiesChanged()
{
    // Tell TAPI about the change.
    OnAddressStateChange (LINEADDRESSSTATE_CAPSCHANGE);

}// CTSPIAddressInfo::OnAddressCapabiltiesChanged

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GatherStatusInformation
//
// Gather all the status information for this address.
//
LONG CTSPIAddressInfo::GatherStatusInformation (LPLINEADDRESSSTATUS lpAddressStatus)
{
    m_AddressStatus.dwTotalSize = lpAddressStatus->dwTotalSize;
    m_AddressStatus.dwNeededSize = sizeof(LINEADDRESSSTATUS);

#ifdef _DEBUG
    // TAPI is supposed to verify the structure size for us.  Check it in DEBUG
    // builds just to be certain, but count on it in the retail version.
    if (lpAddressStatus->dwTotalSize < m_AddressStatus.dwNeededSize)
        return LINEERR_STRUCTURETOOSMALL;
#endif
    
    m_AddressStatus.dwUsedSize = m_AddressStatus.dwNeededSize;
    CopyBuffer (lpAddressStatus, &m_AddressStatus, m_AddressStatus.dwUsedSize);

    // If we have room for forwarding information, then include it.
    lpAddressStatus->dwForwardNumEntries = m_arrForwardInfo.GetSize();
    if (lpAddressStatus->dwForwardNumEntries > 0)
    {
        DWORD dwReqSize = 0;
        for (int i = 0; i < m_arrForwardInfo.GetSize(); i++)
        {
            TSPIFORWARDINFO* pInfo = (TSPIFORWARDINFO*) m_arrForwardInfo[i];
            dwReqSize += sizeof(LINEFORWARD);
            // Add all the addresses together.
            for (int j = 0; j < pInfo->arrCallerAddress.GetSize(); j++)
            {
                DIALINFO* pDialInfo = (DIALINFO*) pInfo->arrCallerAddress[j];
                if (!pDialInfo->strNumber.IsEmpty())
                {
                    dwReqSize += pDialInfo->strNumber.GetLength();
                    if (j == 0)
                        dwReqSize++;
                    else
                        dwReqSize += 2;  // CRLF
                }
            }
            for (j = 0; j < pInfo->arrDestAddress.GetSize(); j++)
            {
                DIALINFO* pDialInfo = (DIALINFO*) pInfo->arrCallerAddress[j];
                if (!pDialInfo->strNumber.IsEmpty())
                {
                    dwReqSize += pDialInfo->strNumber.GetLength();
                    if (j == 0)
                        dwReqSize++;
                    else
                        dwReqSize += 2;  // CRLF
                }
            }                
        }

        lpAddressStatus->dwNeededSize += dwReqSize;
        if (lpAddressStatus->dwTotalSize >= (dwReqSize + lpAddressStatus->dwUsedSize))
        {
            lpAddressStatus->dwForwardSize = dwReqSize;
            lpAddressStatus->dwForwardOffset = lpAddressStatus->dwUsedSize;
            lpAddressStatus->dwUsedSize += dwReqSize;
            
            // Now get a pointer to the bottom of the actual LINEFORWARD structures.
            // The addresses will get stored after all the structures are filled.
            LPSTR lpBuff = (LPSTR)lpAddressStatus;
            lpBuff += lpAddressStatus->dwForwardOffset;
            LPLINEFORWARD lpForward = (LPLINEFORWARD)lpBuff;
            lpBuff += (lpAddressStatus->dwForwardNumEntries * sizeof(LINEFORWARD));
            
            for (i = 0; i < m_arrForwardInfo.GetSize(); i++)
            {
                TSPIFORWARDINFO* pInfo = (TSPIFORWARDINFO*) m_arrForwardInfo[i];
                lpForward->dwForwardMode = pInfo->dwForwardMode;
                lpForward->dwDestCountryCode = pInfo->dwDestCountry;
                
                // Copy the caller information if available.  Multiple addresses
                // may be strung together in the standard dialable format.  We
                // don't include the NAME or ISDN sub address information we might
                // have pulled out of the original request - only the dialable string.
                CString strFinalBuffer;
                for (int j = 0; j < pInfo->arrCallerAddress.GetSize(); j++)
                {
                    DIALINFO* pDialInfo = (DIALINFO*) pInfo->arrCallerAddress[j];
                    if (!pDialInfo->strNumber.IsEmpty())
                    {
                        if (!strFinalBuffer.IsEmpty())   
                            strFinalBuffer += "\r\n";
                        strFinalBuffer += GetSP()->GetDialableNumber(pDialInfo->strNumber);
                    }
                }
                
                if (!strFinalBuffer.IsEmpty())
                {
                    lpForward->dwCallerAddressOffset = (DWORD)((DWORD)lpBuff - (DWORD)lpAddressStatus);
                    lpForward->dwCallerAddressSize = strFinalBuffer.GetLength()+1;
                    CopyBuffer(lpBuff, (LPCSTR)strFinalBuffer, lpForward->dwCallerAddressSize);
                    lpBuff += lpForward->dwCallerAddressSize;
                }                    
                else
                {
                    lpForward->dwCallerAddressSize = 0;
                    lpForward->dwCallerAddressOffset = 0;
                } 
                
                // Copy the destination if available.
                for (j = 0; j < pInfo->arrDestAddress.GetSize(); j++)
                {
                    DIALINFO* pDialInfo = (DIALINFO*) pInfo->arrDestAddress[j];
                    if (!pDialInfo->strNumber.IsEmpty())
                    {
                        if (!strFinalBuffer.IsEmpty())   
                            strFinalBuffer += "\r\n";
                        strFinalBuffer += GetSP()->GetDialableNumber(pDialInfo->strNumber);
                    }
                }
                
                if (!strFinalBuffer.IsEmpty())
                {
                    lpForward->dwDestAddressSize = strFinalBuffer.GetLength()+1;
                    lpForward->dwDestAddressOffset = (DWORD)((DWORD)lpBuff - (DWORD)lpAddressStatus);
                    CopyBuffer (lpBuff, (LPCSTR)strFinalBuffer, lpForward->dwDestAddressSize);
                    lpBuff += lpForward->dwDestAddressSize;
                }
                else
                {
                    lpForward->dwDestAddressSize = 0;
                    lpForward->dwDestAddressOffset = 0;
                } 
            }
        }    
    }
    else
    {
        lpAddressStatus->dwForwardSize =
        lpAddressStatus->dwForwardOffset = 0L;
    }

    // If we have room for terminal entries, then include them.
    if (m_arrTerminals.GetSize() > 0)
    {
        DWORD dwReqSize = m_arrTerminals.GetSize() * sizeof(DWORD);
        lpAddressStatus->dwNeededSize += dwReqSize;

        if (lpAddressStatus->dwTotalSize >= (dwReqSize + lpAddressStatus->dwUsedSize))
        {
            lpAddressStatus->dwTerminalModesOffset = lpAddressStatus->dwUsedSize;
            lpAddressStatus->dwUsedSize += dwReqSize;
            lpAddressStatus->dwTerminalModesSize = dwReqSize;
            LPDWORD lpDW = (LPDWORD)((LPSTR)lpAddressStatus+lpAddressStatus->dwTerminalModesOffset);
            for (int i = 0; i < m_arrTerminals.GetSize(); i++)
                *lpDW++ = m_arrTerminals[i];
        }
    }
    else
    {
        lpAddressStatus->dwTerminalModesSize =
        lpAddressStatus->dwTerminalModesOffset = 0L;
    }

    // Determine the current address features
    

    return 0L;

}// CTSPIAddressInfo::GatherStatusInformation

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetStatusMessages
//
// Set the state notifications to tell TAPI about
//
VOID CTSPIAddressInfo::SetStatusMessages (DWORD dwStates)
{ 
    m_dwAddressStates = dwStates;

}// CTSPIAddressInfo::SetStatusMessages

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetDialableAddress
//
// Return the dialable address (phone#) for this address
//
const char * CTSPIAddressInfo::GetDialableAddress() const
{ 
    return m_strAddress;

}// CTSPIAddressInfo::GetDialableAddress

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetName
//
// Return the name given to us when the address was created.
//
const char * CTSPIAddressInfo::GetName() const
{ 
    return m_strName;

}// CTSPIAddressInfo::GetName

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetName
//
// Changes the name of the address object internally.
//
VOID CTSPIAddressInfo::SetName (LPCSTR pwszName)
{
	m_strName = pwszName;
	OnAddressCapabiltiesChanged();

}// CTSPIAddressInfo::SetName

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetDialableAddress
//
// Changes the address of the address object internally.
//
VOID CTSPIAddressInfo::SetDialableAddress (LPCSTR pwszAddr)
{
	m_strAddress = pwszAddr;
	OnAddressCapabiltiesChanged();

}// CTSPIAddressInfo::SetDialableAddress

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnCreateCall
//
// This method gets called whenever a new call appearance is created
// on this address.
//
VOID CTSPIAddressInfo::OnCreateCall (CTSPICallAppearance* /*pCall*/)
{ 
    /* Do nothing */

}// CTSPIAddressInfo::OnCreateCall

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnCallFeaturesChanged
//
// This method gets called whenever a call changes its currently
// available features in the CALLINFO structure.
//
DWORD CTSPIAddressInfo::OnCallFeaturesChanged (CTSPICallAppearance* pCall, DWORD dwFeatures)
{ 
	// Let the line adjust its counts based on the changing call.
	return GetLineOwner()->OnCallFeaturesChanged(pCall, dwFeatures);

}// CTSPIAddressInfo::OnCallFeaturesChanged

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetNumRingsNoAnswer
//
// Set the number of rings before a call is considered a "no answer".
//
VOID CTSPIAddressInfo::SetNumRingsNoAnswer (DWORD dwNumRings)
{ 
    m_AddressStatus.dwNumRingsNoAnswer = dwNumRings; 
    OnAddressStateChange (LINEADDRESSSTATE_FORWARD);

}// CTSPIAddressInfo::SetNumRingsNoAnswer

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAddressCaps
//
// Return our "partial" address caps structure.
//
LPLINEADDRESSCAPS CTSPIAddressInfo::GetAddressCaps()
{ 
    return &m_AddressCaps;

}// CTSPIAddressInfo::GetAddressCaps

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAddressStatus
//
// Return our "partial" address status structure.
//
LPLINEADDRESSSTATUS CTSPIAddressInfo::GetAddressStatus()
{ 
    return &m_AddressStatus;

}// CTSPIAddressInfo::GetAddressStatus

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::Unpark
//
// This function retrieves the call parked at the specified address and
// returns a call handle to it.
//
LONG CTSPIAddressInfo::Unpark (
DRV_REQUESTID dwRequestID,          // Asynch. request id.
HTAPICALL htCall,                   // New unparked call
LPHDRVCALL lphdCall,                // Return address for unparked call
CADObArray* parrAddresses)          // Array of addresses to unpark from
{                      
	// If the address cannot current make a call then fail.
	if ((GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_UNPARK) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If we have no available call appearances, then error out.             
    if (m_AddressStatus.dwNumActiveCalls >= m_AddressCaps.dwMaxNumActiveCalls)
        return LINEERR_CALLUNAVAIL;

    // Create a new call on the address specified.
    CTSPICallAppearance* pCall = CreateCallAppearance(htCall, 0, LINECALLORIGIN_UNKNOWN,
                                             LINECALLREASON_UNPARK);
    ASSERT (pCall != NULL);

    // Pass the request down to the new call.
    LONG lResult = pCall->Unpark (dwRequestID, parrAddresses);
    if (!ReportError (lResult))
        *lphdCall = (HDRVCALL) pCall;
    else
        RemoveCallAppearance(pCall);
    return lResult;

}// CTSPIAddressInfo::Unpark

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::Pickup
//
// This function picks up a call alerting at the specified destination
// address and returns a call handle for the picked up call.  If invoked
// with a NULL for the 'lpszDestAddr' parameter, a group pickup is performed.
// If required by the device capabilities, 'lpszGroupID' specifies the
// group ID to which the alerting station belongs.
//
LONG CTSPIAddressInfo::Pickup (
DRV_REQUESTID dwRequestID,       // Asynch request id.   
HTAPICALL htCall,                // New call handle
LPHDRVCALL lphdCall,             // Return address for call handle
TSPILINEPICKUP* lpPickup)        // Pickup structure
{   
	// If the address cannot current make a call then fail.
	if ((GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_PICKUP) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If we have no available call appearances, then error out.             
    if (m_AddressStatus.dwNumActiveCalls > m_AddressCaps.dwMaxNumActiveCalls)
        return LINEERR_CALLUNAVAIL;

    // If we require a group id, and one is not supplied, give an error.
    if ((m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_PICKUPGROUPID) &&
         lpPickup->strGroupID.IsEmpty())                                  
        return LINEERR_INVALGROUPID;         

    // Create or locate existing call appearance on this address.
    CTSPICallAppearance* pCall = CreateCallAppearance(htCall, 0, LINECALLORIGIN_UNKNOWN,
                                     LINECALLREASON_PICKUP);
    ASSERT (pCall != NULL);

    // Pass the request down to the new call.
    LONG lResult = pCall->Pickup (dwRequestID, lpPickup);
    if (!ReportError(lResult))
        *lphdCall = (HDRVCALL) pCall;
    else
        RemoveCallAppearance(pCall);

    return lResult;

}// CTSPIAddressInfo::Pickup

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetTerminal
//
// Redirect the flow of specifiec terminal events to a destination
// terminal for this specific call.
//
LONG CTSPIAddressInfo::SetTerminal (DRV_REQUESTID dwRequestID, 
									TSPILINESETTERMINAL* lpLine)
{
	// If the address cannot current make a call then fail.
	if ((GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_SETTERMINAL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Submit the request.
    if (AddAsynchRequest(REQUEST_SETTERMINAL, dwRequestID, lpLine) != NULL)
        return dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPIAddressInfo::SetTerminal

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetTerminalModes
//
// This is the function which will be called when a lineSetTerminal is
// completed by the derived service provider class.
// This stores or removes the specified terminal from the terminal modes 
// given, and then forces it to happen for any existing calls on the 
// address.
//
VOID CTSPIAddressInfo::SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal)
{
    if (iTerminalID < m_arrTerminals.GetSize())
    {
        DWORD dwCurrMode = m_arrTerminals[iTerminalID];
        if (fRouteToTerminal)
            dwCurrMode |= dwTerminalModes;
        else
            dwCurrMode &= ~dwTerminalModes;
        m_arrTerminals.SetAt(iTerminalID, dwCurrMode);
    }

    // Notify TAPI about our address state change
    OnAddressStateChange (LINEADDRESSSTATE_TERMINALS);

    // Run through all our call appearances and force them to update their terminal
    // maps as well.  It is assumed that the service provider code already performed
    // the REAL transfer in H/W.
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        pCall->SetTerminalModes (iTerminalID, dwTerminalModes, fRouteToTerminal);
    }

}// CTSPIAddressInfo::SetTerminalModes

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnTerminalCountChanged
//
// The terminal count has changed, either add or remove a terminal
// entry from our array
//
VOID CTSPIAddressInfo::OnTerminalCountChanged (BOOL fAdded, int iPos, DWORD dwMode)
{
    if (fAdded)
        VERIFY (m_arrTerminals.Add (dwMode) == iPos);
    else
        m_arrTerminals.RemoveAt(iPos);
    
    // Notify TAPI about our address state change
    OnAddressStateChange (LINEADDRESSSTATE_TERMINALS);

    // Run through all our call appearances and force them to update their terminal
    // maps as well.
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        pCall->OnTerminalCountChanged (fAdded, iPos, dwMode);
    }

}// CTSPIAddressInfo::OnTerminalCountChanged

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetupTransfer
//
// Setup the passed call appearance for a consultation transfer.  This
// allocates a new consultation call and associates it with the passed
// call.
//
LONG CTSPIAddressInfo::SetupTransfer(
DRV_REQUESTID dwRequestID,          // Asynch. request id
TSPITRANSFER* lpTransfer,           // Transfer block
HTAPICALL htConsultCall,            // Consultant call to create
LPHDRVCALL lphdConsultCall)         // Return handle for call to create  
{                                 
	// If the function cannot be performed..
	if ((lpTransfer->pCall->GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETUPTRANSFER) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If the call is unavailable because it is a conference, error it.
    if (lpTransfer->pCall->GetCallType() == CALLTYPE_CONFERENCE)
        return LINEERR_OPERATIONUNAVAIL;   

    // Ok, make sure the call state allows this.
    if (lpTransfer->pCall->GetCallState() != LINECALLSTATE_CONNECTED)
        return LINEERR_INVALCALLSTATE;

    // Create the consultation call and associate it with this call appearance.
    DWORD dwCallParamFlags = 0;
    if (lpTransfer->lpCallParams != NULL)
        dwCallParamFlags = lpTransfer->lpCallParams->dwCallParamFlags;
    CTSPICallAppearance* pConsultCall = CreateCallAppearance(htConsultCall, 
                                                dwCallParamFlags, 
                                                LINECALLORIGIN_OUTBOUND,
                                                LINECALLREASON_DIRECT);
    ASSERT(pConsultCall != NULL);
    pConsultCall->SetCallType (CALLTYPE_CONSULTANT);
    lpTransfer->pConsult = pConsultCall;
    lpTransfer->pCall->AttachCall(pConsultCall);
    pConsultCall->AttachCall (lpTransfer->pCall);

    // Submit the request.  The worker should look at the related call field
    // and transition the consultant call to the DIALTONE state.   
    if (lpTransfer->pCall->AddAsynchRequest(REQUEST_SETUPXFER, dwRequestID, lpTransfer))
    {
        *lphdConsultCall = (HDRVCALL) pConsultCall;
        return (LONG) dwRequestID;
    }

    // It failed, kill the new call.
    RemoveCallAppearance (pConsultCall);
    return LINEERR_OPERATIONFAILED;

}// CTSPIAddressInfo::SetupTransfer

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CompleteTransfer
//
// Complete the pending transfer using the specified consultant
// call as the destination.  This can create an additional call appearance
// which then becomes a conference call.
//
LONG CTSPIAddressInfo::CompleteTransfer (
DRV_REQUESTID dwRequestId,          // Asynch, request id
TSPITRANSFER* lpTransfer,           // Transfer block
HTAPICALL htConfCall,               // Conference call handle if needed.
LPHDRVCALL lphdConfCall)            // Return SP handle for conference
{                                   
	// If the function cannot be performed..
	if ((lpTransfer->pConsult->GetCallStatus()->dwCallFeatures & LINECALLFEATURE_COMPLETETRANSF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Verify the call state of the consultant call.  Note that the call may not
    // be the same as the one we created during SetupTransfer.  It is possible
    // to drop/deallocate the original call and release a held call in order to
    // transfer to another line.
    if (lpTransfer->pConsult->GetCallState() != LINECALLSTATE_CONNECTED &&
        lpTransfer->pConsult->GetCallState() != LINECALLSTATE_RINGBACK &&
        lpTransfer->pConsult->GetCallState() != LINECALLSTATE_BUSY &&
        lpTransfer->pConsult->GetCallState() != LINECALLSTATE_PROCEEDING)
        return LINEERR_INVALCALLSTATE;
                                
    // Make sure it is the call we created if we don't support the creation of
    // a new call.
    if ((m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERMAKE) == 0 &&
         (lpTransfer->pConsult->GetCallType() != CALLTYPE_CONSULTANT ||
          lpTransfer->pConsult->GetAttachedCall() != lpTransfer->pCall))
         return LINEERR_INVALCONSULTCALLHANDLE;
                                
    // Make sure the target call is setup.
    if (lpTransfer->dwTransferMode == LINETRANSFERMODE_TRANSFER)
	{
		if ((lpTransfer->pCall->GetCallState() == LINECALLSTATE_ONHOLD &&
			m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERHELD) ||
			lpTransfer->pCall->GetCallState() == LINECALLSTATE_ONHOLDPENDTRANSFER)
			;
		else
			return LINEERR_INVALCALLSTATE;
	}

    // If the transfer mode is conference, then create a conference call
    // to return.  This indicates to transition the call into a three-way
    // conference call.
    CTSPIConferenceCall* pConfCall = NULL;
    if (lpTransfer->dwTransferMode == LINETRANSFERMODE_CONFERENCE)
    {
        if ((m_AddressCaps.dwTransferModes & LINETRANSFERMODE_CONFERENCE) == 0)
            return LINEERR_OPERATIONUNAVAIL;
        if (lphdConfCall == NULL)
            return LINEERR_INVALPOINTER;
            
        pConfCall = CreateConferenceCall(htConfCall);
        ASSERT(pConfCall != NULL);
        
        // Attach both calls to the conference call.  They should both transition
        // to CONFERENCED and be automatically added to our conference.
        lpTransfer->pCall->SetConferenceOwner(pConfCall);
        lpTransfer->pConsult->SetConferenceOwner(pConfCall);
    }
    // Or a simple transfer - make sure it can be performed.
    else if (lpTransfer->dwTransferMode == LINETRANSFERMODE_TRANSFER)
    {                                                            
        if ((m_AddressCaps.dwTransferModes & LINETRANSFERMODE_TRANSFER) == 0)
            return LINEERR_OPERATIONUNAVAIL;
    }
    
    lpTransfer->pConf = pConfCall;

    // Submit the request.
    if (lpTransfer->pConsult->AddAsynchRequest(REQUEST_COMPLETEXFER, dwRequestId, lpTransfer))
    {   
        if (lphdConfCall != NULL && pConfCall)
            *lphdConfCall = (HDRVCALL)pConfCall;
        return (LONG) dwRequestId;
    }

    // Remove it if we failed.
    if (pConfCall)
        RemoveCallAppearance (pConfCall);
    return LINEERR_OPERATIONUNAVAIL;

}// CTSPIAddressInfo::CompleteTransfer

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetupConference
//
// This function creates a new conference either from an existing 
// call or a new call.
//
LONG CTSPIAddressInfo::SetupConference (                                       
DRV_REQUESTID dwRequestID,          // Asynch Request id.
TSPICONFERENCE* lpConf,             // Conference data
HTAPICALL htConfCall,               // New conference call TAPI handle
LPHDRVCALL lphdConfCall,            // Returning call handle
HTAPICALL htConsultCall,            // New consultation call TAPI handle
LPHDRVCALL lphdConsultCall)         // Returning call handle
{                    
	// If the address cannot current make a call then fail.
	if ((GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_SETUPCONF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If we have an initial call, then it must be connected.
    if (lpConf->pCall != NULL)
    {   
        // If we are not supposed to have a call appearance during setup, then fail this
        // call.
        if (m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_SETUPCONFNULL)
            return LINEERR_INVALCALLHANDLE;
    
        if (lpConf->pCall->GetCallState() != LINECALLSTATE_CONNECTED)
            return LINEERR_INVALCALLSTATE;
    }                               
    // Or if we don't have one, but we need one, fail it.       
    else if ((m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_SETUPCONFNULL) == 0)
        return LINEERR_INVALCALLHANDLE;
    
    // Verify the count we are getting against our maximum allowed in a conference.
    if (lpConf->dwPartyCount > m_AddressCaps.dwMaxNumConference)
        return LINEERR_CONFERENCEFULL;
    
    // Create the conference call we are going to master this with.
    CTSPIConferenceCall* pConfCall = CreateConferenceCall (htConfCall);
    ASSERT (pConfCall != NULL);

    // Setup the confererence call
    lpConf->pConfCall = pConfCall;

    // Create the consultation call which will be associated with the
    // initial call -OR- be a new call altogether on the address.
    CTSPICallAppearance* pConsultCall = CreateCallAppearance (htConsultCall);
    ASSERT (pConsultCall != NULL);
    
    // Setup the consultation call.
    pConsultCall->SetCallType (CALLTYPE_CONSULTANT);
    lpConf->pConsult = pConsultCall;

    // Attach the original call to the conference call.  With this attachment,
    // when the call enters the "CONFERENCED" state, the conference call will
    // automatically add it to the conferencing array.
    if (lpConf->pCall != NULL)
    {
        lpConf->pCall->SetConferenceOwner(pConfCall);
        // Attach the consultation call to the conference.
        pConfCall->AttachCall(pConsultCall);
        pConsultCall->AttachCall(pConfCall);
    }        
    
    // Submit the request via the conference call.
    if (pConfCall->AddAsynchRequest (REQUEST_SETUPCONF, dwRequestID, lpConf))
    {
        *lphdConfCall = (HDRVCALL) pConfCall;
        *lphdConsultCall = (HDRVCALL) pConsultCall;
        return (LONG) dwRequestID;
    }

    return LINEERR_OPERATIONFAILED;

}// CTSPIAddressInfo::SetupConference

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddCompletionMessage
//
// Add a new completion message to the address information.
// TAPI is NOT notified.  This should be done at INIT time when each
// of the addresses are being added.
//
int CTSPIAddressInfo::AddCompletionMessage (const char *pszBuff)
{   
    int cbLen = strlen (pszBuff)+1;
    if (m_AddressCaps.dwCompletionMsgTextEntrySize < (DWORD) cbLen)
        m_AddressCaps.dwCompletionMsgTextEntrySize = (DWORD) cbLen;
        
    m_AddressCaps.dwNumCompletionMessages++;                   
    m_AddressCaps.dwCompletionMsgTextSize = (m_AddressCaps.dwCompletionMsgTextEntrySize * m_AddressCaps.dwNumCompletionMessages);
        
    return m_arrCompletionMsgs.Add (pszBuff);

}// CTSPIAddressInfo::AddCompletionMessage

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCompletionMessageCount
//
// Return the total number of completion messages available.
//
int CTSPIAddressInfo::GetCompletionMessageCount() const
{          
    ASSERT (m_AddressCaps.dwNumCompletionMessages == (DWORD) m_arrCompletionMsgs.GetSize());
    return (int) m_AddressCaps.dwNumCompletionMessages;
    
}// CTSPIAddressInfo::GetCompletionMessageCount

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCompletionMessage
//
// Return a particular completion message.
//
const char * CTSPIAddressInfo::GetCompletionMessage (int iPos) const
{
    if (iPos >= 0 && iPos < m_arrCompletionMsgs.GetSize())
        return m_arrCompletionMsgs[iPos];

    return NULL;
                                          
}// CTSPIAddressInfo::GetCompletionMessage

////////////////////////////////////////////////////////////////////////////
// CTSPIAddress::CanForward
//
// This function is called to verify that this address can forward
// given the specified forwarding information.  All addresses being
// forwarded in a group will be given a chance to check the forwarding
// request before the "Forward" function is actually invoked to insert
// the asynch. request.
//
LONG CTSPIAddressInfo::CanForward(TSPILINEFORWARD* lpForwardInfo, int iCount)
{
    // If we need to establish a consultation call, and this is an
    // "all forward" request, then fail it since each address will require
    // its own consultation call.  The only exception is if this is the
    // only address.
    if (iCount > 1 && m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_FWDCONSULT) 
        return LINEERR_RESOURCEUNAVAIL;

    // If the forwarding list exceeds what our derived class expects, fail it.
    if (lpForwardInfo->arrForwardInfo.GetSize() > (int) m_AddressCaps.dwMaxForwardEntries)
        return LINEERR_INVALPARAM;

    // Adjust the ring count if it falls outside our range.        
    if ((m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_FWDNUMRINGS) == 0)
        lpForwardInfo->dwNumRings = 0;
    else if (lpForwardInfo->dwNumRings < m_AddressCaps.dwMinFwdNumRings)
        lpForwardInfo->dwNumRings = m_AddressCaps.dwMinFwdNumRings;
    else if (lpForwardInfo->dwNumRings > m_AddressCaps.dwMaxFwdNumRings)
        lpForwardInfo->dwNumRings = m_AddressCaps.dwMaxFwdNumRings;
        
    // Run through the forwarding list and verify that we support the different
    // forwarding modes being asked for.
    int iFwdCount = 0;
    for (int i = 0; i < lpForwardInfo->arrForwardInfo.GetSize(); i++)
    {
        TSPIFORWARDINFO* lpForward = (TSPIFORWARDINFO*) lpForwardInfo->arrForwardInfo[i];
        if ((lpForward->dwForwardMode & m_AddressCaps.dwForwardModes) == 0)
            return LINEERR_INVALPARAM;
        iFwdCount += lpForward->arrCallerAddress.GetSize();
    }                
                                  
    // If the specific entries exceeds our list, fail it.                                  
    if (iFwdCount > (int) m_AddressCaps.dwMaxSpecificEntries)
        return LINEERR_INVALPARAM;                       
    
    // Everything looks ok.
    return 0L;        

}// CTSPIAddress::CanForward

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::Forward
//
// Forward this address to another address.  This can also UNFORWARD.
// This list of forwarding instructions REPLACES any existing forwarding
// information.
//
LONG CTSPIAddressInfo::Forward (
DRV_REQUESTID dwRequestID,             // Asynchronous request id
TSPILINEFORWARD* lpForwardInfo,        // Local Forward information
HTAPICALL htConsultCall,               // New TAPI call handle if necessary
LPHDRVCALL lphdConsultCall)            // Our return call handle if needed
{                          
	if ((GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_FORWARD) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If we need to establish a consultation call for forwarding, then do so.
    CTSPICallAppearance* pCall = NULL;
    if (m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_FWDCONSULT) 
    {                                                              
        ASSERT (*lphdConsultCall == NULL);
        
        // If call capabilities were supplied, then use them to create the
        // consultation call.
        DWORD dwCallParamFlags = 0;
        if (lpForwardInfo->lpCallParams != NULL)
            dwCallParamFlags = lpForwardInfo->lpCallParams->dwCallParamFlags;
        pCall = CreateCallAppearance(htConsultCall, dwCallParamFlags, 
                                     LINECALLORIGIN_OUTBOUND, LINECALLREASON_DIRECT);
        ASSERT (pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
        pCall->SetCallType (CALLTYPE_CONSULTANT);                           
        *lphdConsultCall = (HDRVCALL) pCall;
        lpForwardInfo->pCall = pCall;
    }

    // Insert the request into our request list.
    if (AddAsynchRequest (REQUEST_FORWARD, dwRequestID, lpForwardInfo))
        return (LONG) dwRequestID;

    // If it fails, remove the call appearance.        
    if (pCall != NULL)
    {
        RemoveCallAppearance (pCall);
        *lphdConsultCall = NULL;
    }
    return LINEERR_OPERATIONFAILED;

}// CTSPIAddressInfo::Forward

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAvailableMediaModes
//
// Return the available media modes for this address.
//
DWORD CTSPIAddressInfo::GetAvailableMediaModes () const
{
    return m_dwAvailMediaModes;

}// CTSPIAddressInfo::GetAvailableMediaModes

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanSupportMediaModes
//
// Return whether this address can support the requested media modes.
// The parameter passed is a collection of LINEMEDIAMODE_xxxx constants.
//
BOOL CTSPIAddressInfo::CanSupportMediaModes (DWORD dwMediaModes) const
{         
    dwMediaModes &= ~LINEMEDIAMODE_UNKNOWN;
    return ((GetAvailableMediaModes() & dwMediaModes) == dwMediaModes);

}// CTSPIAddressInfo::CanSupportMediaModes

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnTimer
//
// This method is called by the line whenever our interval timer fires.
//
VOID CTSPIAddressInfo::OnTimer()
{                            
    // Wait for our address exclusive mutex
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        pCall->OnTimer();
    }

}// CTSPIAddressInfo::OnTimer

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanSupportCall
//
// Return zero or an error if the call cannot be supported on this
// address.
//
LONG CTSPIAddressInfo::CanSupportCall (LPLINECALLPARAMS const lpCallParams) const
{                                 
    if (!CanSupportMediaModes(lpCallParams->dwMediaMode))
        return LINEERR_INVALMEDIAMODE;
    
    if ((lpCallParams->dwCallParamFlags & LINECALLPARAMFLAGS_SECURE) &&
            (m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_SECURE) == 0)
        return LINEERR_INVALCALLPARAMS;
            
    if ((lpCallParams->dwCallParamFlags & LINECALLPARAMFLAGS_BLOCKID) &&
            (m_AddressCaps.dwAddrCapFlags & (LINEADDRCAPFLAGS_BLOCKIDOVERRIDE | LINEADDRCAPFLAGS_BLOCKIDDEFAULT)) == 0)
        return LINEERR_INVALCALLPARAMS;
            
    if ((lpCallParams->dwCallParamFlags & LINECALLPARAMFLAGS_ORIGOFFHOOK) &&
            (m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_ORIGOFFHOOK) == 0)
        return LINEERR_INVALCALLPARAMS;
            
    if ((lpCallParams->dwCallParamFlags & LINECALLPARAMFLAGS_DESTOFFHOOK) &&
            (m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_DESTOFFHOOK) == 0)
        return LINEERR_INVALCALLPARAMS;
            
    return FALSE;

}// CTSPIAddressInfo::CanSupportCall

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetMediaControl
//
// This function enables and disables control actions on the media stream 
// associated with this address and all calls present here. Media control 
// actions can be triggered by the detection of specified digits, media modes, 
// custom tones, and call states.  The new specified media controls replace 
// all the ones that were in effect for this line, address, or call prior 
// to this request.
//
LONG CTSPIAddressInfo::SetMediaControl (TSPIMEDIACONTROL* lpMediaControl)
{   
    m_lpMediaControl->DecUsage();  // Will auto-delete if last one using this data.
    m_lpMediaControl = lpMediaControl;    
    m_lpMediaControl->IncUsage();

    // Go through all calls and tell them about this new media monitoring.
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        pCall->SetMediaControl (lpMediaControl);
    }
    return FALSE;    

}// CTSPIAddressInfo::SetMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::DeleteForwardingInfo
//
// Delete the information in our forwarding array.  This array
// holds the forwarding information reported to TAPI.
//
VOID CTSPIAddressInfo::DeleteForwardingInfo()
{   
    for (int i = 0; i < m_arrForwardInfo.GetSize(); i++)
    {
        TSPIFORWARDINFO* pInfo = (TSPIFORWARDINFO*) m_arrForwardInfo[i];
        pInfo->DecUsage();
    }
    m_arrForwardInfo.RemoveAll();

}// CTSPIAddressInfo::DeleteForwardingInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnRequestComplete
//
// This virtual method is called when an outstanding request on this
// line address has completed.  The return code indicates the success
// or failure of the request.  Note that the request will filter to
// the call appearance.
//
VOID CTSPIAddressInfo::OnRequestComplete(CTSPIRequest* pReq, LONG lResult)
{                                         
    WORD wRequest = pReq->GetCommand();
    
    // On a set terminal request, if it is successful, then go ahead and set the
    // terminal identifiers up inside our class.  This information can then be
    // retrieved by TAPI through the GetAddressStatus/Caps methods.
    if (wRequest == REQUEST_SETTERMINAL)
    {
        if (lResult == 0)
        {
            TSPILINESETTERMINAL* pTermStruct = (TSPILINESETTERMINAL*) pReq->GetDataPtr();
            ASSERT (pTermStruct != NULL);
            if (pTermStruct->pAddress != NULL)
                SetTerminalModes ((int)pTermStruct->dwTerminalID, pTermStruct->dwTerminalModes, pTermStruct->bEnable);
        }
    } 
    
    // On a FORWARD request, note the forwarding information internally so we
    // may return it to TAPI if requested.
    else if (wRequest == REQUEST_FORWARD)
    {
        TSPILINEFORWARD* pForward = (TSPILINEFORWARD*) pReq->GetDataPtr();

        if (lResult == 0)
        {            
            // Derived class should have changed this in the TSPILINEFORWARD structure if invalid.
            m_AddressStatus.dwNumRingsNoAnswer = pForward->dwNumRings;         
            // Move the FORWARDINFO pointers over to our array
            DeleteForwardingInfo();
            for (int i = 0; i < pForward->arrForwardInfo.GetSize(); i++)
            {
                TSPIFORWARDINFO* pInfo = (TSPIFORWARDINFO*) pForward->arrForwardInfo[i];
                pInfo->IncUsage();
                m_arrForwardInfo.Add (pInfo);
            }                                        
            OnAddressStateChange (LINEADDRESSSTATE_FORWARD);
        }            
        else
        {
            if (pForward->pCall != NULL)
            {
				if ((pForward->pCall->m_dwFlags & CTSPICallAppearance::InitNotify) == 0)
                {
                    TRACE("Deleting invalid consultation call <0x%lx>\r\n", (DWORD) pForward->pCall);
                    RemoveCallAppearance(pForward->pCall);
                }
            }
        }
    }                                      
    // If a transfer request setup failed, then remove the call appearance
    // which represents the consultant call.
    else if (wRequest == REQUEST_SETUPXFER) 
    {
        if (lResult != 0)
        {
            TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
            if (pTrans->pConsult != NULL)
            {
				if ((pTrans->pConsult->m_dwFlags & CTSPICallAppearance::InitNotify) == 0)
                {
                    TRACE("Deleting invalid consultation call <0x%lx>\r\n", (DWORD) pTrans->pConsult);
                    RemoveCallAppearance(pTrans->pConsult);               
                }
            }
        }
    }   
    // If this is a COMPLETE transfer request, and it failed, remove the 
    // consultation call appearance created.
    else if (wRequest == REQUEST_COMPLETEXFER)
    {
        if (lResult != 0)
        {
            TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
            if (pTrans->pConf != NULL)
            {
    			if ((pTrans->pConf->m_dwFlags & CTSPICallAppearance::InitNotify) == 0)
                {
                    TRACE("Deleting invalid conference call <0x%lx>\r\n", (DWORD) pTrans->pConf);
                    RemoveCallAppearance(pTrans->pConf);
                }
            }            

			// Reset the related call ids of the other two calls.
			pTrans->pCall->SetConferenceOwner(NULL);
			pTrans->pConsult->SetConferenceOwner(NULL);
        }
    }
    // Or a conference setup request - same as transfer.
    else if (wRequest == REQUEST_SETUPCONF || wRequest == REQUEST_PREPAREADDCONF)
    {
        if (lResult != 0)
        {
            TSPICONFERENCE* pConf = (TSPICONFERENCE*) pReq->GetDataPtr();
            if (pConf->pConsult != NULL)
            {
        		if ((pConf->pConsult->m_dwFlags & CTSPICallAppearance::InitNotify) == 0)
                {
                    TRACE("Deleting invalid consultation call <0x%lx>\r\n", (DWORD) pConf->pConsult);
                    RemoveCallAppearance(pConf->pConsult);
                }
            }
        }
    } 
    // Or if this is a MAKECALL request which has failed, remove the
    // call appearance.  Do the same for pickup and unpark.
    else if (wRequest == REQUEST_MAKECALL || wRequest == REQUEST_PICKUP ||
             wRequest == REQUEST_UNPARK)
    {
        if (lResult != 0)
        {
            CTSPICallAppearance* pCall = pReq->GetCallInfo();
            if (pCall != NULL)
            {
    			if ((pCall->m_dwFlags & CTSPICallAppearance::InitNotify) == 0)
                {
                    TRACE("Deleting invalid consultation call <0x%lx>\r\n", (DWORD) pCall);
                    RemoveCallAppearance(pCall);
                    pReq->m_pCall = NULL;
                }
            }
        }
    }

}// CTSPIAddressInfo::OnRequestComplete

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddForwardEntry
//
// This method allows the direct addition of forwarding information
// on this address.  This should only be used if the service provider
// can detect that the address is already forwarded on initialization.
//
// To delete the forwarding information, pass a zero in for forward mode.
//
int CTSPIAddressInfo::AddForwardEntry (DWORD dwForwardMode, const char *pszCaller, const char *pszDestination, DWORD dwDestCountry)
{   
    int iPos = -1;
    if (dwForwardMode == 0)
        DeleteForwardingInfo();
    else
    {
        TSPIFORWARDINFO* pForward = new TSPIFORWARDINFO;
        if (pForward != NULL)
        {
            pForward->dwForwardMode = dwForwardMode;
            pForward->dwDestCountry = dwDestCountry;
            
            if (pszCaller != NULL && *pszCaller != '\0')
                GetSP()->CheckDialableNumber(GetLineOwner(), this, pszCaller, &pForward->arrCallerAddress, dwDestCountry);
            
            if (pszDestination != NULL && *pszDestination != '\0')
                GetSP()->CheckDialableNumber(GetLineOwner(), this, pszDestination, &pForward->arrDestAddress, dwDestCountry);

            iPos = m_arrForwardInfo.Add (pForward);
            OnAddressStateChange (LINEADDRESSSTATE_FORWARD);
        }            
    }           
    
    return iPos;

}// CTSPIAddressInfo::AddForwardEntry

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanMakeCalls
//
// Return whether this address can make calls EVER
//
BOOL CTSPIAddressInfo::CanMakeCalls() const
{                                 
    return (m_fOutputAvail != 0);
    
}// CTSPIAddressInfo::CanMakeCalls

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanAnswerCalls
//
// Return whether this address can answer calls EVER
//
BOOL CTSPIAddressInfo::CanAnswerCalls() const
{                                 
    return (m_fInputAvail != 0);
    
}// CTSPIAddressInfo::CanAnswerCalls

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetCurrentRate
//
// Set the current data rate
//
VOID CTSPIAddressInfo::SetCurrentRate (DWORD dwRate)
{                                   
    m_dwCurrRate = dwRate;
    
}// CTSPIAddressInfo::SetCurrentRate

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCurrentRate
//
// Set the current data rate
//
DWORD CTSPIAddressInfo::GetCurrentRate() const
{                                   
    return m_dwCurrRate;
    
}// CTSPIAddressInfo::GetCurrentRate

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCurrentBearerMode
//
// Get the current bearer mode for this address.  This cooresponds to
// the Quality of Service (QOS) for the address.
//
DWORD CTSPIAddressInfo::GetBearerMode() const
{   
    return m_dwBearerMode;
    
}// CTSPIAddressInfo::GetCurrentBearerMode

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetID
//
// Manage device-level requests for information based on a device id.
//
LONG CTSPIAddressInfo::GetID (CString& /*strDevClass*/, LPVARSTRING /*lpDeviceID*/)
{
    // Additional support must be provided by derived class
    return LINEERR_OPERATIONUNAVAIL;
    
}// CTSPIAddressInfo::GetID

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetTerminalInformation
//
// Return the terminal information for the specified terminal.
//
DWORD CTSPIAddressInfo::GetTerminalInformation (int iTerminalID) const
{                                           
    if (iTerminalID >= 0 && iTerminalID < m_arrTerminals.GetSize())
        return m_arrTerminals[iTerminalID];
    return 0L;

}// CTSPIAddressInfo::GetTerminalInformation

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetAddressFeatures
//
// This method sets the current features available on the address
// It does NOT invoke the OnAddressFeaturesChanged.
//
VOID CTSPIAddressInfo::SetAddressFeatures(DWORD dwFeatures)
{
	// Make sure the capabilities structure reflects this ability.
	if (m_AddressCaps.dwAddressFeatures & dwFeatures == 0)
	{
		// If you get this, then you need to update the object dwAddressFeatures
		// in the Init method of the address or in the line while adding it.
		TRACE(_T("LINEADDRCAPS.dwAddressFeatures missing 0x%lx bit\r\n"), dwFeatures);
		m_AddressCaps.dwAddressFeatures |= dwFeatures;	
		OnAddressCapabiltiesChanged();
	}

	// Update it only if it has changed.
	if (m_AddressStatus.dwAddressFeatures != dwFeatures)
	{
		m_AddressStatus.dwAddressFeatures = dwFeatures;
		OnAddressStateChange (LINEADDRESSSTATE_OTHER);
	}
		
}// CTSPIAddressInfo::SetAddressFeatures

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnAddressFeaturesChanged
//
// This method gets called whenever our address changes its features.
// It is only called when the LIBRARY changes the features.
//
DWORD CTSPIAddressInfo::OnAddressFeaturesChanged (DWORD dwFeatures)
{ 
	return GetLineOwner()->OnAddressFeaturesChanged(this, dwFeatures);

}// CTSPIAddressInfo::OnAddressFeaturesChanged

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::Dump
//
// Debug method to dump out the Address object
//
void CTSPIAddressInfo::Dump (CDumpContext& /*dc*/) const
{
	TRACE("Address ID:0x%lx [%s] %s\r\n", m_dwAddressID, (LPCSTR) m_strName, (LPCSTR) m_strAddress);

}// CTSPIAddressInfo::Dump
#endif // _DEBUG
