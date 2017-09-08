/******************************************************************************/
//                                                                        
// ADDRESS.CPP - Source code for the CTSPIAddressInfo object          
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the source to manage the address objects which are 
// held by the CTSPILineConnection objects.
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
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////
// Run-Time class information 

IMPLEMENT_DYNCREATE( CTSPIAddressInfo, CTSPIBaseObject )

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
CTSPIAddressInfo::CTSPIAddressInfo() : 
	m_pLine(0), m_dwAddressID(0), m_dwAddressStates(0),
	m_dwConnectedCallCount(0), m_lpMediaControl(0), m_dwFlags(0)
{
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

	// If we have media control information, decrement it.
	if (m_lpMediaControl != NULL)
		m_lpMediaControl->DecUsage();

    // Delete all the call appearances still on the address
	CEnterCode sLock (this, TRUE);  // Synch access to object
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
void CTSPIAddressInfo::Init (CTSPILineConnection* pLine, DWORD dwAddressID, LPCTSTR lpszAddress,
                             LPCTSTR lpszName, BOOL fSupportsIncomingCalls, 
                             BOOL fSupportsOutgoingCalls,DWORD dwAvailMediaModes, 
                             DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate,
                             DWORD dwMaxNumActiveCalls, DWORD dwMaxNumOnHoldCalls, 
                             DWORD dwMaxNumOnHoldPendCalls, DWORD dwMaxNumConference, 
                             DWORD dwMaxNumTransConf)
{
    // Fill in the passed information
	m_pLine = pLine;
    m_dwAddressID = dwAddressID;
    m_dwBearerMode = dwBearerMode;
    m_dwMinRateAvail = dwMinRate;
    m_dwMaxRateAvail = dwMaxRate;
	m_dwCurrRate = dwMinRate;

	// Mark the type of input available.
	if (fSupportsIncomingCalls)
		m_dwFlags |= InputAvail;
	if (fSupportsOutgoingCalls)
		m_dwFlags |= OutputAvail;

	// Move the name over.
	if (lpszName != NULL)
		m_strName = lpszName;
    
    // Move the address over - only numbers please!
	if (lpszAddress != NULL)
		m_strAddress = GetSP()->GetDialableNumber(lpszAddress);

	// Mark the available media modes for this address.
    m_AddressCaps.dwAvailableMediaModes = (dwAvailMediaModes | LINEMEDIAMODE_UNKNOWN);

    // Now gather the address capabilities, we assume the address is private - override this
    // function and change it if necessary.
    m_AddressCaps.dwAddressSharing = LINEADDRESSSHARING_PRIVATE;

    // Mark the available address state notifications the class library manages.
    m_AddressCaps.dwAddressStates = (LINEADDRESSSTATE_OTHER | LINEADDRESSSTATE_INUSEZERO | LINEADDRESSSTATE_INUSEONE | 
                                     LINEADDRESSSTATE_FORWARD | LINEADDRESSSTATE_INUSEMANY | LINEADDRESSSTATE_NUMCALLS | 
									 LINEADDRESSSTATE_CAPSCHANGE);		// TAPI v1.4

    // Mark the different LINE_CALLINFO messages which we can generate by our call appearances.  The only
    // field which cannot be generated by the library is DEVSPECIFIC - if this is supported by your service provider,
    // make sure to add it to this list in your derived class
    m_AddressCaps.dwCallInfoStates = (LINECALLINFOSTATE_OTHER | LINECALLINFOSTATE_BEARERMODE | 
                                      LINECALLINFOSTATE_RATE |
                                      LINECALLINFOSTATE_MEDIAMODE | LINECALLINFOSTATE_APPSPECIFIC |
                                      LINECALLINFOSTATE_CALLID | LINECALLINFOSTATE_RELATEDCALLID | 
                                      LINECALLINFOSTATE_ORIGIN | LINECALLINFOSTATE_REASON |
                                      LINECALLINFOSTATE_COMPLETIONID | LINECALLINFOSTATE_TRUNK |
                                      LINECALLINFOSTATE_CALLERID | LINECALLINFOSTATE_CALLEDID |
                                      LINECALLINFOSTATE_CONNECTEDID | LINECALLINFOSTATE_REDIRECTIONID |
                                      LINECALLINFOSTATE_REDIRECTINGID | LINECALLINFOSTATE_USERUSERINFO | 
                                      LINECALLINFOSTATE_DIALPARAMS);

	// Add the states which are set by TAPI apis.
	if (CanHandleRequest(TSPI_LINEMONITORMEDIA))
		m_AddressCaps.dwCallInfoStates |= LINECALLINFOSTATE_MONITORMODES;
	if (CanHandleRequest(TSPI_LINESETCALLTREATMENT))
		m_AddressCaps.dwCallInfoStates |= LINECALLINFOSTATE_TREATMENT;
	if (CanHandleRequest(TSPI_LINESETCALLQUALITYOFSERVICE))
		m_AddressCaps.dwCallInfoStates |= LINECALLINFOSTATE_QOS;
	if (CanHandleRequest(TSPI_LINESETCALLDATA))
		m_AddressCaps.dwCallInfoStates |= LINECALLINFOSTATE_CALLDATA;
	if (CanHandleRequest(TSPI_LINESETTERMINAL))
	{
		m_AddressCaps.dwAddressStates |= LINEADDRESSSTATE_TERMINALS;
		m_AddressCaps.dwCallInfoStates |= LINECALLINFOSTATE_TERMINAL;
	}

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

	// Report the new TAPI 2.0 mode information - again, these might not all be reported by this
	// provider, but return them anyway.
	m_AddressCaps.dwConnectedModes = (LINECONNECTEDMODE_ACTIVE | LINECONNECTEDMODE_INACTIVE |
								      LINECONNECTEDMODE_ACTIVEHELD | LINECONNECTEDMODE_INACTIVEHELD | 
								      LINECONNECTEDMODE_CONFIRMED);
	m_AddressCaps.dwOfferingModes = (LINEOFFERINGMODE_ACTIVE | LINEOFFERINGMODE_INACTIVE);

    // Report special info as unavailable/unknown - if the service provider is to support
    // any of the special tone information, then add the appropriate fields.
    m_AddressCaps.dwSpecialInfo = (LINESPECIALINFO_UNKNOWN | LINESPECIALINFO_UNAVAIL);

    // Report all the disconnect modes - some may not be reported by the service provider, but
    // its still ok to list them to TAPI.
    m_AddressCaps.dwDisconnectModes = (LINEDISCONNECTMODE_NORMAL | LINEDISCONNECTMODE_UNKNOWN |
                                       LINEDISCONNECTMODE_REJECT | LINEDISCONNECTMODE_BUSY |
                                       LINEDISCONNECTMODE_NOANSWER | LINEDISCONNECTMODE_BADADDRESS |
                                       LINEDISCONNECTMODE_UNREACHABLE | LINEDISCONNECTMODE_CONGESTION |
                                       LINEDISCONNECTMODE_INCOMPATIBLE | LINEDISCONNECTMODE_UNAVAIL | 
								LINEDISCONNECTMODE_NODIALTONE    | // TAPI 1.4
								LINEDISCONNECTMODE_NUMBERCHANGED | // TAPI v2.0
								LINEDISCONNECTMODE_OUTOFORDER    | // TAPI v2.0
								LINEDISCONNECTMODE_TEMPFAILURE   | // TAPI v2.0
								LINEDISCONNECTMODE_QOSUNAVAIL    | // TAPI v2.0
								LINEDISCONNECTMODE_BLOCKED       | // TAPI v2.0
								LINEDISCONNECTMODE_DONOTDISTURB);  // TAPI v2.0

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
    m_AddressCaps.dwCallFeatures = LINECALLFEATURE_DROP;
    m_AddressCaps.dwForwardModes = 0L;

    // Add in the call features available.
    if (CanMakeCalls() && CanHandleRequest(TSPI_LINEMAKECALL))
    {
        m_AddressCaps.dwCallStates |= (LINECALLSTATE_DIALING | LINECALLSTATE_DIALTONE | LINECALLSTATE_RINGBACK);
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_MAKECALL;
	}        

	if (CanHandleRequest(TSPI_LINEDIAL))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_DIAL;

    if (CanAnswerCalls() && CanHandleRequest(TSPI_LINEANSWER))
    {
        m_AddressCaps.dwCallStates |= (LINECALLSTATE_OFFERING | LINECALLSTATE_ACCEPTED);
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_ANSWER;
	}
	        
    if (CanHandleRequest(TSPI_LINESETUPCONFERENCE))
    {
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETUPCONF;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_SETUPCONF;
	}   
	     
    if (CanHandleRequest(TSPI_LINESETUPTRANSFER))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETUPTRANSFER;
        
    if (CanHandleRequest(TSPI_LINEPARK))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_PARK;
        
    if (CanHandleRequest(TSPI_LINEADDTOCONFERENCE))
    {
        m_AddressCaps.dwCallStates |= (LINECALLSTATE_CONFERENCED | LINECALLSTATE_ONHOLDPENDCONF);
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_ADDTOCONF;
    }
        
    if (CanHandleRequest(TSPI_LINEBLINDTRANSFER))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_BLINDTRANSFER;
        
    if (CanHandleRequest(TSPI_LINEHOLD))
    {
        m_AddressCaps.dwCallStates |= LINECALLSTATE_ONHOLD;
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_HOLD;
	}
	        
    if (CanHandleRequest(TSPI_LINESENDUSERUSERINFO))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SENDUSERUSER;
        
    if (CanHandleRequest(TSPI_LINESWAPHOLD))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SWAPHOLD;
        
    if (CanHandleRequest(TSPI_LINEREDIRECT))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_REDIRECT;
        
    if (CanHandleRequest(TSPI_LINEACCEPT))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_ACCEPT;
        
    if (CanHandleRequest(TSPI_LINEREMOVEFROMCONFERENCE))
	{
		m_AddressCaps.dwRemoveFromConfCaps = LINEREMOVEFROMCONF_ANY;
		m_AddressCaps.dwRemoveFromConfState = LINECALLSTATE_IDLE;
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_REMOVEFROMCONF;
	}
	else
		m_AddressCaps.dwRemoveFromConfCaps = LINEREMOVEFROMCONF_NONE;
        
    if (CanHandleRequest(TSPI_LINEUNHOLD))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_UNHOLD;
        
    if (CanHandleRequest(TSPI_LINECOMPLETETRANSFER))
    {
        m_AddressCaps.dwCallStates |= LINECALLSTATE_ONHOLDPENDTRANSFER;
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_COMPLETETRANSF;
	}
	        
    if (CanHandleRequest(TSPI_LINECOMPLETECALL))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_COMPLETECALL;
        
    if (CanHandleRequest(TSPI_LINESECURECALL))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SECURECALL;
        
    if (CanHandleRequest(TSPI_LINESETCALLPARAMS))
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETCALLPARAMS;
        
    if (CanHandleRequest(TSPI_LINESETTERMINAL))
    {
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETTERMINAL;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_SETTERMINAL;
	}        
    
    if (CanHandleRequest(TSPI_LINESETMEDIACONTROL))
    {
        m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETMEDIACONTROL;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_SETMEDIACONTROL;
	}        
    
    if (CanHandleRequest(TSPI_LINEFORWARD))
    {
        m_AddressCaps.dwMaxForwardEntries = 1;
        m_AddressCaps.dwMaxSpecificEntries = 0L;
        m_AddressCaps.dwMinFwdNumRings = 0L;
        m_AddressCaps.dwMaxFwdNumRings = 0L;
        m_AddressCaps.dwDisconnectModes |= LINEDISCONNECTMODE_FORWARDED;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_FORWARD; 
	}
	    
    if (CanHandleRequest(TSPI_LINEPICKUP))
    {
        m_AddressCaps.dwDisconnectModes |= LINEDISCONNECTMODE_PICKUP;
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_PICKUP; 
	}        
    
    if (CanHandleRequest(TSPI_LINEUNCOMPLETECALL))
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_UNCOMPLETECALL;      
    
    if (CanHandleRequest(TSPI_LINEUNPARK))
        m_AddressCaps.dwAddressFeatures |= LINEADDRFEATURE_UNPARK;

	// Add the additional call features based on whether we can support
	// the request.
	if (CanHandleRequest(TSPI_LINEGATHERDIGITS))
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_GATHERDIGITS;
	if (CanHandleRequest(TSPI_LINEGENERATEDIGITS))
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_GENERATEDIGITS;
	if (CanHandleRequest(TSPI_LINEGENERATETONE))
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_GENERATETONE;
	if (CanHandleRequest(TSPI_LINEMONITORDIGITS))		
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_MONITORDIGITS;
	if (CanHandleRequest(TSPI_LINEMONITORMEDIA))		
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_MONITORMEDIA;
	if (CanHandleRequest(TSPI_LINEMONITORTONES))		
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_MONITORTONES;
	if (CanHandleRequest(TSPI_LINEPREPAREADDTOCONFERENCE))		
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_PREPAREADDCONF;
	if (CanHandleRequest(TSPI_LINESETMEDIACONTROL))		
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETMEDIACONTROL;
	// TAPI 1.4
	if (CanHandleRequest(TSPI_LINERELEASEUSERUSERINFO))		
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_RELEASEUSERUSERINFO;
	// TAPI 2.0
	if (CanHandleRequest(TSPI_LINESETCALLTREATMENT))
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETTREATMENT;
	if (CanHandleRequest(TSPI_LINESETCALLQUALITYOFSERVICE))
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETQOS;
	if (CanHandleRequest(TSPI_LINESETCALLDATA))
		m_AddressCaps.dwCallFeatures |= LINECALLFEATURE_SETCALLDATA;

    // Store the additional flags for transfer.  If transfer/pickup
    // supported, you need to alter the flags to indicate the mode the service 
    // provider works in.
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

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanSupportMediaModes
//
// Return whether this address can support the requested media modes.
// The parameter passed is a collection of LINEMEDIAMODE_xxxx constants.
//
BOOL CTSPIAddressInfo::CanSupportMediaModes (DWORD dwMediaModes) const
{   
	if (GetAvailableMediaModes() == 0)
		return TRUE;
    dwMediaModes &= ~LINEMEDIAMODE_UNKNOWN;
    return ((GetAvailableMediaModes() & dwMediaModes) == dwMediaModes);

}// CTSPIAddressInfo::CanSupportMediaModes

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

    // If we don't have a call handle, then ask TAPI for one.
    BOOL fNewCall = FALSE;
    if (hCall == NULL)
    {
        DWORD dwTapiCall = 0;
        GetLineOwner()->Send_TAPI_Event (NULL, LINE_NEWCALL, (DWORD)pCall, (DWORD)&dwTapiCall);
        if (dwTapiCall != 0)
            hCall = (HTAPICALL) dwTapiCall;
        fNewCall = TRUE;            
    }

    DTRACE(TRC_MIN, _T("CreateConferenceCall: SP call=0x%lx, TAPI call=0x%lx\r\n"), (DWORD)pCall, (DWORD)hCall);

    // Init the call appearance with a handle.
    pCall->Init(this, hCall, m_dwBearerMode, m_dwCurrRate, 0, LINECALLORIGIN_CONFERENCE, 
                LINECALLREASON_DIRECT, 0xffffffff, 0, fNewCall);

    // Add it to our list.
	CEnterCode Key (this);  // Synch access to object
    m_lstCalls.AddTail((CObject*)pCall);
	Key.Unlock();
    
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
    
    // If we don't have a call handle, then ask TAPI for one.
    BOOL fNewCall = FALSE;
    if (hCall == NULL)
    {
        DWORD dwTapiCall = 0;
        GetLineOwner()->Send_TAPI_Event (NULL, LINE_NEWCALL, (DWORD)pCall, (DWORD)&dwTapiCall);
        if (dwTapiCall != 0)
            hCall = (HTAPICALL) dwTapiCall;
        fNewCall = TRUE;            
    }

    DTRACE(TRC_MIN, _T("CreateCallAppearance: SP call=0x%lx, TAPI call=0x%lx\r\n"), (DWORD)pCall, (DWORD)hCall);

	// If the completion ID is non-zero, then use the appropriate reason.
	if (dwCompletionID > 0 && dwReason == LINECALLREASON_UNKNOWN)
		dwReason = LINECALLREASON_CALLCOMPLETION;

    // Init the call appearance with a handle.
    pCall->Init(this, hCall, m_dwBearerMode, m_dwCurrRate, 
                dwCallParamFlags, dwOrigin, dwReason, dwTrunk, dwCompletionID, fNewCall);

    // Add it to our list.
	CEnterCode Key (this);  // Synch access to object
    m_lstCalls.AddTail((CObject*)pCall);
	Key.Unlock();
    
    // Notify ourselves in case a derived class wants to know.
    OnCreateCall (pCall);

    return pCall;

}// CTSPIAddressInfo::CreateCallAppearance

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::RemoveCallAppearance
//
// Remove a call appearance from our list and delete it.
//
void CTSPIAddressInfo::RemoveCallAppearance (CTSPICallAppearance* pCall)
{
    // If it is null or deleted, ignore it.
    if (pCall == NULL || (pCall->m_dwFlags & CTSPICallAppearance::IsDeleted))
        return;

	// If it has an attached call, notify a change.  This will cause us
	// to re-attach any other call or zero out the attachment.
	if (pCall->GetAttachedCall() != NULL)
	{
		pCall->GetAttachedCall()->
			OnRelatedCallStateChange(pCall, LINECALLSTATE_IDLE, LINECALLSTATE_UNKNOWN);
	}

    // First locate and remove it from our array
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        POSITION posCurr = pos;
        CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pTest == pCall)
        {
            m_lstCalls.RemoveAt(posCurr);
            break;
        }
    }

	// Make sure it goes IDLE for our active call counts.
	pCall->SetCallState(LINECALLSTATE_IDLE, 0, 0, FALSE);

    // Make sure the call isn't referenced in a request packet anywhere.
    GetLineOwner()->OnCallDeleted (pCall);
    
	// Mark the call deleted and unavailable to TAPI.
	pCall->m_dwFlags |= CTSPICallAppearance::IsDeleted;
	pCall->m_htCall = 0;

	// Decrement the reference count.  This will eventually delete the
	// call object when all requests which are associated with the call 
	// are deleted.
    pCall->DecRefCount();

}// CTSPIAddressInfo::RemoveCallAppearance

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::NotifyInUseZero
//
// This checks to see whether any calls exist in the non-IDLE state.
// It is called as each call is removed from the address
//
BOOL CTSPIAddressInfo::NotifyInUseZero()
{
	CEnterCode sLock(this);  // Synch access to object

	// If we already have reported no non-IDLE calls, don't do it again.
	if (m_AddressStatus.dwNumInUse == 0)
		return FALSE;

    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pCall->GetCallState() != LINECALLSTATE_IDLE)
            return FALSE;
    }

	m_AddressStatus.dwNumInUse = 0;
	OnAddressStateChange(LINEADDRESSSTATE_INUSEZERO);
	return TRUE;

}// CTSPIAddressInfo::NotifyInUseZero

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCallInfo
//
// Return the call information object based on a position in our
// list.
//
CTSPICallAppearance* CTSPIAddressInfo::GetCallInfo(int iPos) const
{
    int iCount = 0;
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL; iCount++)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (iCount == iPos)
            return pCall;
    }
    return NULL;

}// CTSPIAddressInfo::GetCallInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::FindAttachedCall
//
// Returns a pointer to the call which is attached to the specified
// call.  This allows a link of attached calls.
//
CTSPICallAppearance* CTSPIAddressInfo::FindAttachedCall(CTSPICallAppearance* pSCall) const
{
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pCall->GetAttachedCall() == pSCall)
            return pCall;
    }
    return NULL;

}// CTSPIAddressInfo::FindAttachedCall

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::FindCallByState
//
// Returns a pointer to the call with the specified call state
//
CTSPICallAppearance* CTSPIAddressInfo::FindCallByState(DWORD dwState) const
{
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if ((pCall->GetCallState() & dwState) != 0)
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
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pCall->GetCallInfo()->dwCallID == dwCallID)
            return pCall;
    }
    return NULL;

}// CTSPIAddressInfo::FindCallByCallID

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnPreCallStateChange
//
// A call appearance on this address is about to change states.
//
void CTSPIAddressInfo::OnPreCallStateChange (CTSPICallAppearance* pCall, DWORD dwNewState, DWORD dwOldState)
{
	// If the state has not changed, ignore.
	if (dwNewState == dwOldState)
		return;

	CEnterCode sLock(this);  // Synch access to object

    // Determine if the number of active calls has changed.
    BOOL fWasActive = CTSPICallAppearance::IsActiveCallState(dwOldState);
    BOOL fIsActive  = CTSPICallAppearance::IsActiveCallState(dwNewState);
	BOOL fWasConnected = CTSPICallAppearance::IsConnectedCallState(dwOldState);
	BOOL fIsConnected = CTSPICallAppearance::IsConnectedCallState(dwNewState);

	// If the call is now active, but wasn't before, then our active
	// call count is up by one.
	if (fWasActive == FALSE && fIsActive == TRUE)
	{                                   
		m_AddressStatus.dwNumActiveCalls++;
		m_dwFlags |= NotifyNumCalls;
	}

	// Or if the number of active calls has gone down
	else if (fWasActive == TRUE && fIsActive == FALSE)
	{
		m_AddressStatus.dwNumActiveCalls--;
		if (m_AddressStatus.dwNumActiveCalls & 0x80000000)
			m_AddressStatus.dwNumActiveCalls = 0L;
		m_dwFlags |= NotifyNumCalls;
	}       

	// Count the active calls.  This is used to determine the bandwidth of the TSP.
	if (fWasConnected == FALSE && fIsConnected == TRUE)
	{
		++m_dwConnectedCallCount;
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
		++m_AddressStatus.dwNumOnHoldCalls;
		m_dwFlags |= NotifyNumCalls;
	}
	else if ((dwNewState == LINECALLSTATE_ONHOLDPENDTRANSFER ||
			  dwNewState == LINECALLSTATE_ONHOLDPENDCONF) &&
			 (dwOldState != LINECALLSTATE_ONHOLDPENDTRANSFER &&
			  dwOldState != LINECALLSTATE_ONHOLDPENDCONF))
	{
		++m_AddressStatus.dwNumOnHoldPendCalls;
		m_dwFlags |= NotifyNumCalls;
	}

	if (dwOldState == LINECALLSTATE_ONHOLD && dwNewState != dwOldState)
	{
		if (--m_AddressStatus.dwNumOnHoldCalls & 0x80000000)
			m_AddressStatus.dwNumOnHoldCalls = 0L;
		m_dwFlags |= NotifyNumCalls;
	}
	else if ((dwOldState == LINECALLSTATE_ONHOLDPENDTRANSFER ||
			  dwOldState == LINECALLSTATE_ONHOLDPENDCONF) &&
			 (dwNewState != LINECALLSTATE_ONHOLDPENDTRANSFER &&
			  dwNewState != LINECALLSTATE_ONHOLDPENDCONF))
	{
		if (--m_AddressStatus.dwNumOnHoldPendCalls & 0x80000000)
			m_AddressStatus.dwNumOnHoldPendCalls = 0L;
		m_dwFlags |= NotifyNumCalls;
	}

	// Unlock our address
	sLock.Unlock();

#ifdef _DEBUG	
	if (dwNewState != dwOldState)
		TRACE(_T("Address Call Counts NewCallState=0x%lx, OldCallState=0x%lx,  Active=%ld, OnHold=%ld,  OnHoldPend=%ld\n"),
				dwNewState, dwOldState, m_AddressStatus.dwNumActiveCalls, m_AddressStatus.dwNumOnHoldCalls, m_AddressStatus.dwNumOnHoldPendCalls);            
#endif

    // Tell our line owner about the call changing state.
    GetLineOwner()->OnPreCallStateChange(this, pCall, dwNewState, dwOldState);

}// CTSPIAddressInfo::OnPreCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnCallStateChange
//
// A call appearance on this address has changed states.  Potentially
// send out an address state change.
//
void CTSPIAddressInfo::OnCallStateChange (CTSPICallAppearance* pCall, DWORD dwNewState, DWORD dwOldState)
{
	// The LINEADDRESSSTATE_NUMCALLS should be sent whenever any of the LINEADDRESSSTATUS
	// dwNumXXXX fields have changed. If this has happened, send the notification.
    if (m_dwFlags & NotifyNumCalls)
	{
        OnAddressStateChange (LINEADDRESSSTATE_NUMCALLS);
		m_dwFlags &= ~NotifyNumCalls;
	}

	// Recalculate our address features if this isn't a monitored address
	if (m_AddressCaps.dwAddressSharing != LINEADDRESSSHARING_MONITORED)
		RecalcAddrFeatures();

    // Tell our line owner about the call changing state.
    GetLineOwner()->OnCallStateChange(this, pCall, dwNewState, dwOldState);

	// Send the INUSEZERO notification if necessary
	NotifyInUseZero();

}// CTSPIAddressInfo::OnCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::FindCallByHandle
//
// Locate a call appearance by the handle used in TAPI
//
CTSPICallAppearance* CTSPIAddressInfo::FindCallByHandle(HTAPICALL htCall) const
{
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        if (pCall->GetCallHandle() == htCall && 
			(pCall->m_dwFlags & CTSPICallAppearance::IsDeleted) == 0)
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
	// Adjust the TSPI version if this line was opened at a lower TSPI version.
	if (dwTSPIVersion > GetLineOwner()->GetNegotiatedVersion())
		dwTSPIVersion = GetLineOwner()->GetNegotiatedVersion();

	// Synch access to object
	CEnterCode sLock(this);  

	// Grab the dialable address.  If there isn't one,
	// or we are not to report the DN to the upper level
	// (possibly because it is dynamic), then report a
	// static entity based on our address position.
	CString strAddr = GetDialableAddress();
	if (strAddr.IsEmpty())
	{
		TCHAR chBuff[20];
		wsprintf(chBuff, _T("Address %ld"), m_dwAddressID);
		strAddr = chBuff;
	}
	int cbLineAddr = (strAddr.IsEmpty()) ? 0 : (strAddr.GetLength()+1) * sizeof(TCHAR);

	// Get the length of the device classes we support.
	CString strDeviceNames;
	int cbDeviceNameLen = 0;
	for (int i = 0; i < m_arrDeviceClass.GetSize(); i++)
	{
		DEVICECLASSINFO* pDevClass = (DEVICECLASSINFO*) m_arrDeviceClass[i];
		strDeviceNames += pDevClass->strName + _T('~');
	}

	// Add the DEVICECLASS entries from the owner line object.
	for (i = 0; i < GetLineOwner()->m_arrDeviceClass.GetSize(); i++)
	{
		DEVICECLASSINFO* pDevClass = (DEVICECLASSINFO*) GetLineOwner()->m_arrDeviceClass[i];
		if (strDeviceNames.Find(pDevClass->strName) == -1)
			strDeviceNames += pDevClass->strName + _T('~');
	}
	if (!strDeviceNames.IsEmpty())
	{
		strDeviceNames += _T('~');
		cbDeviceNameLen = (strDeviceNames.GetLength()+1) * sizeof(TCHAR);
	}

	// Determine the size of all the call treatment text sizes.
	// Make sure to DWORD align each string size since it will be OFFSETS
	// placed into the LINECALLENTRY array.
	DWORD dwCallTextSize = 0L;
	POSITION pos = m_mapCallTreatment.GetStartPosition();
	while (pos != NULL)
	{
		DWORD dwKey;
		CString strValue;
		m_mapCallTreatment.GetNextAssoc (pos, dwKey, strValue);
		int iLen = (strValue.GetLength()+1) * sizeof(TCHAR);

#ifndef _X86_
		// Force it to be DWORD aligned.
		while ((iLen % 4) != 0)
			iLen++;
#endif
		dwCallTextSize += iLen;
	}

	// Determine how much space we must have for this structure.  This is the
	// amount of space required to retrieve ALL information - including all
	// TAPI 2.0 fields.
    m_AddressCaps.dwTotalSize = lpAddressCaps->dwTotalSize;
    m_AddressCaps.dwLineDeviceID = GetLineOwner()->GetDeviceID();
    m_AddressCaps.dwNeededSize = sizeof(LINEADDRESSCAPS) + cbLineAddr + 
						m_AddressCaps.dwCompletionMsgTextSize + dwCallTextSize +
						cbDeviceNameLen;
    
    // Make sure we have enough space.  Note that we must subtract the address
    // features which were added in TAPI 2.0 for backward compatibility.
    DWORD dwReqSize = sizeof(LINEADDRESSCAPS);

    if (dwTSPIVersion < TAPIVER_20)
		dwReqSize -= sizeof(DWORD) * 12;
	if (dwTSPIVersion < TAPIVER_14)
		dwReqSize -= sizeof(DWORD);

#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
	if (lpAddressCaps->dwTotalSize < dwReqSize)
    {
        lpAddressCaps->dwNeededSize = m_AddressCaps.dwNeededSize;
        return LINEERR_STRUCTURETOOSMALL;
    }
#endif

    // Copy over our STATIC version of the capabilities.
    CopyBuffer (lpAddressCaps, &m_AddressCaps, dwReqSize);
    lpAddressCaps->dwUsedSize = dwReqSize;

    // Remove the things which are not available in previous versions depending on 
	// the negotiation level for the calling application.  Note that we can get older
	// TAPI apps calling us.
	if (dwTSPIVersion < TAPIVER_20)
	{
		lpAddressCaps->dwAddrCapFlags &= ~(LINEADDRCAPFLAGS_PREDICTIVEDIALER | 
			                               LINEADDRCAPFLAGS_QUEUE | 
										   LINEADDRCAPFLAGS_ROUTEPOINT |
										   LINEADDRCAPFLAGS_HOLDMAKESNEW |
										   LINEADDRCAPFLAGS_NOINTERNALCALLS |
										   LINEADDRCAPFLAGS_NOEXTERNALCALLS |
										   LINEADDRCAPFLAGS_SETCALLINGID);
		lpAddressCaps->dwAddressFeatures &= ~(LINEADDRFEATURE_PICKUPHELD |
			                               LINEADDRFEATURE_PICKUPGROUP |
										   LINEADDRFEATURE_PICKUPDIRECT |
										   LINEADDRFEATURE_PICKUPWAITING |
										   LINEADDRFEATURE_FORWARDFWD |
										   LINEADDRFEATURE_FORWARDDND);
		lpAddressCaps->dwCallFeatures &= ~(LINECALLFEATURE_SETTREATMENT |
			                               LINECALLFEATURE_SETQOS |
										   LINECALLFEATURE_SETCALLDATA);
		lpAddressCaps->dwCallInfoStates &= ~(LINECALLINFOSTATE_TREATMENT |
											LINECALLINFOSTATE_QOS |
											LINECALLINFOSTATE_CALLDATA);
		lpAddressCaps->dwDisconnectModes &= ~(LINEDISCONNECTMODE_NUMBERCHANGED |
			                                LINEDISCONNECTMODE_OUTOFORDER |
											LINEDISCONNECTMODE_TEMPFAILURE |
											LINEDISCONNECTMODE_QOSUNAVAIL |
											LINEDISCONNECTMODE_BLOCKED |
											LINEDISCONNECTMODE_DONOTDISTURB);
	}
    
	// Strip out unsupporting additions added to Win95.
	if (dwTSPIVersion < TAPIVER_14)
    {
    	lpAddressCaps->dwAddressStates &= ~LINEADDRESSSTATE_CAPSCHANGE;
		lpAddressCaps->dwCallFeatures &= ~LINECALLFEATURE_RELEASEUSERUSERINFO;
    	lpAddressCaps->dwDisconnectModes &= ~LINEDISCONNECTMODE_NODIALTONE;
		lpAddressCaps->dwForwardModes &= ~(LINEFORWARDMODE_UNKNOWN | LINEFORWARDMODE_UNAVAIL);
    }

    // Add the string if we have the space.
	AddDataBlock (lpAddressCaps, lpAddressCaps->dwAddressOffset, 
				  lpAddressCaps->dwAddressSize,
				  (LPCTSTR)strAddr);

    // Add the completion messages if we have the space
    if (lpAddressCaps->dwNumCompletionMessages > 0)
    {   
        // See if we have the space
        if (lpAddressCaps->dwTotalSize >= lpAddressCaps->dwUsedSize + 
			lpAddressCaps->dwCompletionMsgTextSize)
        {
			lpAddressCaps->dwCompletionMsgTextSize = 0L;
			int iTextEntryLen = (int)((lpAddressCaps->dwCompletionMsgTextEntrySize - sizeof(wchar_t)) / sizeof(wchar_t));
            for (int i = 0; i < m_arrCompletionMsgs.GetSize(); i++)
            {   
                CString strBuff = m_arrCompletionMsgs[i];
				if (strBuff.GetLength() < iTextEntryLen)
					strBuff += CString(_T(' '), iTextEntryLen - strBuff.GetLength());
				AddDataBlock (lpAddressCaps, lpAddressCaps->dwCompletionMsgTextOffset,
					lpAddressCaps->dwCompletionMsgTextSize, strBuff);
			}
        }
    }

	// Add the call treatment information if we have the space.
	if (dwTSPIVersion >= TAPIVER_20)
	{
		if (lpAddressCaps->dwNumCallTreatments > 0)
		{
			// Make sure our address caps pointer is DWORD-aligned.
#ifndef _X86_
			while ((lpAddressCaps->dwUsedSize % 4) != 0)
				lpAddressCaps->dwUsedSize++;
#endif
			if (lpAddressCaps->dwUsedSize > lpAddressCaps->dwTotalSize)
				lpAddressCaps->dwUsedSize = lpAddressCaps->dwTotalSize;
			if (lpAddressCaps->dwTotalSize >= lpAddressCaps->dwUsedSize + 
				(dwCallTextSize+lpAddressCaps->dwCallTreatmentListSize))
			{
				LPTSTR lpszBuff = (LPTSTR)lpAddressCaps + lpAddressCaps->dwUsedSize;
				lpAddressCaps->dwCallTreatmentListOffset = lpAddressCaps->dwUsedSize;
				lpAddressCaps->dwUsedSize += lpAddressCaps->dwCallTreatmentListSize;

				// Copy the LINECALLTREATMENTENTRY array information into place.  
				// Our string names get placed directly after the array information.
				for (pos = m_mapCallTreatment.GetStartPosition(); pos != NULL;)
				{
					DWORD dwKey;
					CString strValue;
					m_mapCallTreatment.GetNextAssoc (pos, dwKey, strValue);

					LINECALLTREATMENTENTRY lct;
					lct.dwCallTreatmentID = dwKey;
					lct.dwCallTreatmentNameSize = (strValue.GetLength()+1) * sizeof(TCHAR);
					lct.dwCallTreatmentNameOffset = lpAddressCaps->dwUsedSize;
				
					CopyBuffer (lpszBuff, (LPCTSTR)&lct, sizeof(LINECALLTREATMENTENTRY));
					lpszBuff += sizeof(LINECALLTREATMENTENTRY);
					lpAddressCaps->dwUsedSize += lct.dwCallTreatmentNameSize;
#ifndef _X86_
					while ((lpAddressCaps->dwUsedSize % 4) != 0)
						lpAddressCaps->dwUsedSize++;
#endif
					CopyBuffer ((LPTSTR)lpAddressCaps+lct.dwCallTreatmentNameOffset, 
								(LPCTSTR)strValue, lct.dwCallTreatmentNameSize);
				}
			}
		}

		// If we have some lineGetID supported device classes,
		// return the list of supported device classes.
		if (cbDeviceNameLen)
		{
			if (lpAddressCaps->dwTotalSize >= lpAddressCaps->dwUsedSize + cbDeviceNameLen)
			{
				AddDataBlock (lpAddressCaps, lpAddressCaps->dwDeviceClassesOffset,
					          lpAddressCaps->dwDeviceClassesSize, strDeviceNames);
				// Remove the '~' chars and change to NULLs.
				wchar_t* lpBuff = (wchar_t*)((LPBYTE)lpAddressCaps + lpAddressCaps->dwDeviceClassesOffset);
				ASSERT (lpBuff != NULL);
				do
				{
					if (*lpBuff == L'~')
						*lpBuff = L'\0';
					lpBuff++;

				} while (*lpBuff);
			}
		}
	}

    return FALSE;

}// CTSPIAddressInfo::GatherCapabilities

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GatherStatusInformation
//
// Gather all the status information for this address.
//
LONG CTSPIAddressInfo::GatherStatusInformation (LPLINEADDRESSSTATUS lpAddressStatus)
{
	CEnterCode sLock(this);  // Synch access to object

    m_AddressStatus.dwTotalSize = lpAddressStatus->dwTotalSize;
    m_AddressStatus.dwNeededSize = sizeof(LINEADDRESSSTATUS);

#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
    if (lpAddressStatus->dwTotalSize < m_AddressStatus.dwNeededSize)
	{
		ASSERT (FALSE);
        return LINEERR_STRUCTURETOOSMALL;
	}
#endif
    
    m_AddressStatus.dwUsedSize = m_AddressStatus.dwNeededSize;
    CopyBuffer (lpAddressStatus, &m_AddressStatus, m_AddressStatus.dwUsedSize);

    // If we have room for forwarding information, then include it.
    lpAddressStatus->dwForwardNumEntries = m_arrForwardInfo.GetSize();
    if (lpAddressStatus->dwForwardNumEntries > 0)
    {
		// Determine how much space is required for these forwarding entries.
        DWORD dwTotalSize = 0;
        for (int i = 0; i < (int) lpAddressStatus->dwForwardNumEntries; i++)
        {
            TSPIFORWARDINFO* pFwdInfo = (TSPIFORWARDINFO*) m_arrForwardInfo[i];
            dwTotalSize += pFwdInfo->dwTotalSize;
		}

		// Add in the total size of the FWDINFO information.
        lpAddressStatus->dwNeededSize += dwTotalSize;

		// If we have the room within our structure, add it to the end.
        if (lpAddressStatus->dwTotalSize >= (dwTotalSize + lpAddressStatus->dwUsedSize))
        {
            lpAddressStatus->dwForwardSize = dwTotalSize;
            lpAddressStatus->dwForwardOffset = lpAddressStatus->dwUsedSize;

            // Create a pointer to the area of memory where the forward information
			// is stored, and move our current block pointer to the end of ALL the
			// forwarding information blocks.  The address information will all be placed
			// directly following all the LINEFORWARD blocks.
			LPLINEFORWARD lpForward = (LPLINEFORWARD)((LPSTR)lpAddressStatus+lpAddressStatus->dwForwardOffset);
            lpAddressStatus->dwUsedSize += (sizeof(LINEFORWARD) * m_arrForwardInfo.GetSize());
            for (i = 0; i < m_arrForwardInfo.GetSize(); i++, lpForward++)
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
                            strFinalBuffer += _T("\r\n");
                        strFinalBuffer += GetSP()->GetDialableNumber(pDialInfo->strNumber);
                    }
                }
                
				// Add the caller information to the forwarding buffer at the end.
                if (!strFinalBuffer.IsEmpty())
					AddDataBlock(lpAddressStatus, lpForward->dwCallerAddressOffset,
							lpForward->dwCallerAddressSize, strFinalBuffer);

                // Copy the destination address information if available.
                for (j = 0; j < pInfo->arrDestAddress.GetSize(); j++)
                {
                    DIALINFO* pDialInfo = (DIALINFO*) pInfo->arrDestAddress[j];
                    if (!pDialInfo->strNumber.IsEmpty())
                    {
                        if (!strFinalBuffer.IsEmpty())   
                            strFinalBuffer += _T("\r\n");
                        strFinalBuffer += GetSP()->GetDialableNumber(pDialInfo->strNumber);
                    }
                }
				
				// Add the destination address to the forwarding buffer at the end.
                if (!strFinalBuffer.IsEmpty())
					AddDataBlock (lpAddressStatus, lpForward->dwDestAddressOffset,
							lpForward->dwDestAddressSize, strFinalBuffer);
            }
        }    
    }

    // If we have room for terminal entries, then include them.
    if (m_arrTerminals.GetSize() > 0)
    {
        if (lpAddressStatus->dwTotalSize >= lpAddressStatus->dwUsedSize + 
			(m_arrTerminals.GetSize() * sizeof(DWORD)))
        {
			for (int i = 0; i < m_arrTerminals.GetSize(); i++)
			{
				DWORD dwValue = m_arrTerminals[i];
				AddDataBlock (lpAddressStatus, lpAddressStatus->dwTerminalModesOffset,
					lpAddressStatus->dwTerminalModesSize, &dwValue, sizeof(DWORD));
			}
        }
    }

    return 0L;

}// CTSPIAddressInfo::GatherStatusInformation

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
	// Make sure we can perform this action now.
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
// CTSPIAddressInfo::SetStatusMessages
//
// Set the state notifications to tell TAPI about
//
void CTSPIAddressInfo::SetStatusMessages (DWORD dwStates)
{ 
	CEnterCode sLock(this);
    m_dwAddressStates = dwStates;

}// CTSPIAddressInfo::SetStatusMessages

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetNumRingsNoAnswer
//
// Set the number of rings before a call is considered a "no answer".
//
void CTSPIAddressInfo::SetNumRingsNoAnswer (DWORD dwNumRings)
{ 
	CEnterCode sLock(this);
    m_AddressStatus.dwNumRingsNoAnswer = dwNumRings; 
    OnAddressStateChange (LINEADDRESSSTATE_FORWARD);

}// CTSPIAddressInfo::SetNumRingsNoAnswer

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetCurrentRate
//
// Set the current data rate
//
void CTSPIAddressInfo::SetCurrentRate (DWORD dwRate)
{                                   
	CEnterCode sLock(this);
    m_dwCurrRate = dwRate;
    
}// CTSPIAddressInfo::SetCurrentRate

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetTerminal
//
// Redirect the flow of specifiec terminal events to a destination
// terminal for this specific call.
//
LONG CTSPIAddressInfo::SetTerminal (DRV_REQUESTID dwRequestID, 
									TSPILINESETTERMINAL* lpLine)
{
	// Make sure we can perform this action now.
	if ((GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_SETTERMINAL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Submit the request.
    if (AddAsynchRequest(REQUEST_SETTERMINAL, dwRequestID, lpLine) != NULL) 
		return dwRequestID;
	return LINEERR_OPERATIONFAILED;

}// CTSPIAddressInfo::SetTerminal

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetID
//
// Manage device-level requests for information based on a device id.
//
LONG CTSPIAddressInfo::GetID (CString& strDevClass, 
							LPVARSTRING lpDeviceID, HANDLE hTargetProcess)
{
	DEVICECLASSINFO* pDeviceClass = GetDeviceClass(strDevClass);
	if (pDeviceClass == NULL)
		return GetLineOwner()->GetID(strDevClass, lpDeviceID, hTargetProcess);
	return GetSP()->CopyDeviceClass (pDeviceClass, lpDeviceID, hTargetProcess);

}// CTSPIAddressInfo::GetID

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
	// Make sure we can perform this action now.
	if ((GetAddressStatus()->dwAddressFeatures & (LINEADDRFEATURE_PICKUP | 
			LINEADDRFEATURE_PICKUPHELD | LINEADDRFEATURE_PICKUPGROUP | 
			LINEADDRFEATURE_PICKUPDIRECT | LINEADDRFEATURE_PICKUPWAITING)) == 0)
		return LINEERR_OPERATIONUNAVAIL;

	// If we have no more available call appearances, then error out.             
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
// CTSPIAddressInfo::SetTerminalModes
//
// This is the function which will be called when a lineSetTerminal is
// completed by the derived service provider class.
// This stores or removes the specified terminal from the terminal modes 
// given, and then forces it to happen for any existing calls on the 
// address.
//
void CTSPIAddressInfo::SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal)
{
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
void CTSPIAddressInfo::OnTerminalCountChanged (BOOL fAdded, int iPos, DWORD dwMode)
{
	CEnterCode sLock(this);  // Synch access to object

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
    // If the call is unavailable because it is a conference, error it.
    if (lpTransfer->pCall->GetCallType() == CALLTYPE_CONFERENCE)
        return LINEERR_OPERATIONUNAVAIL;   

#if STRICT_CALLSTATES
    // Verify the call state allows this.
    if (lpTransfer->pCall->GetCallState() != LINECALLSTATE_CONNECTED)
        return LINEERR_INVALCALLSTATE;
#endif

    // Create the consultation call and associate it with this call appearance.
    DWORD dwCallParamFlags = (lpTransfer->lpCallParams != NULL) ? lpTransfer->lpCallParams->dwCallParamFlags : 0;
	lpTransfer->pConsult = lpTransfer->pCall->CreateConsultationCall(htConsultCall, dwCallParamFlags);

    // Submit the request.  The worker should look at the related call field
    // and transition the consultant call to the DIALTONE state.   
    if (lpTransfer->pCall->AddAsynchRequest(REQUEST_SETUPXFER, dwRequestID, lpTransfer))
    {
        *lphdConsultCall = (HDRVCALL) lpTransfer->pConsult;
        return (LONG) dwRequestID;
    }

    // It failed, kill the new call.
    RemoveCallAppearance (lpTransfer->pConsult);
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
	// Make sure we can perform this function now.
	if ((lpTransfer->pConsult->GetCallStatus()->dwCallFeatures & LINECALLFEATURE_COMPLETETRANSF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

#if STRICT_CALLSTATES
    // Verify the call state of the consultant call.  Note that the call may not
    // be the same as the one we created during SetupTransfer.  It is possible
    // to drop/deallocate the original call and release a held call in order to
    // transfer to another line.
    if (lpTransfer->pConsult->GetCallState() != LINECALLSTATE_CONNECTED &&
        lpTransfer->pConsult->GetCallState() != LINECALLSTATE_RINGBACK &&
        lpTransfer->pConsult->GetCallState() != LINECALLSTATE_BUSY &&
        lpTransfer->pConsult->GetCallState() != LINECALLSTATE_PROCEEDING)
        return LINEERR_INVALCALLSTATE;
#endif
                                
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
	// Make sure we can perform this action now.
	if ((GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_SETUPCONF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If we have an initial call, then it must be connected.
    if (lpConf->pCall != NULL)
    {   
        // If we are not supposed to have a call appearance during setup, then fail this
        // call.
        if (m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_SETUPCONFNULL)
            return LINEERR_INVALCALLHANDLE;
    
#if STRICT_CALLSTATES
        if (lpConf->pCall->GetCallState() != LINECALLSTATE_CONNECTED)
            return LINEERR_INVALCALLSTATE;
#endif
    }                               
    // Or if we don't have one, but we need one, fail it.       
    else if ((m_AddressCaps.dwAddrCapFlags & LINEADDRCAPFLAGS_SETUPCONFNULL) == 0)
        return LINEERR_INVALCALLHANDLE;
    
    // Verify the count we are getting against our maximum allowed in a conference.
    if (lpConf->dwPartyCount > m_AddressCaps.dwMaxNumConference)
        return LINEERR_CONFERENCEFULL;
    
    // Create the conference call we are going to master this with.
    lpConf->pConfCall = CreateConferenceCall (htConfCall);

    // Create the consultation call which will be associated with the
    // initial call -OR- be a new call altogether on the address.
    lpConf->pConsult = CreateCallAppearance (htConsultCall);
    ASSERT (lpConf->pConsult != NULL);
    
    // Associate the consultation call with the conference call
	lpConf->pConfCall->SetConsultationCall(lpConf->pConsult);

    // Attach the original call to the conference call.  With this attachment,
    // when the call enters the "CONFERENCED" state, the conference call will
    // automatically add it to the conferencing array.
    if (lpConf->pCall != NULL)
        lpConf->pCall->SetConferenceOwner(lpConf->pConfCall);
    
    // Submit the request via the conference call.
    if (lpConf->pConfCall->AddAsynchRequest (REQUEST_SETUPCONF, dwRequestID, lpConf))
    {
        *lphdConfCall = (HDRVCALL) lpConf->pConfCall;
        *lphdConsultCall = (HDRVCALL) lpConf->pConsult;
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
int CTSPIAddressInfo::AddCompletionMessage (LPCTSTR pszBuff)
{   
	CEnterCode sLock(this);  // Synch access to object

    int cbLen = (lstrlen(pszBuff)+1) * sizeof(wchar_t);
    if (m_AddressCaps.dwCompletionMsgTextEntrySize < (DWORD) cbLen)
        m_AddressCaps.dwCompletionMsgTextEntrySize = (DWORD) cbLen;
        
    m_AddressCaps.dwNumCompletionMessages++;                   
    m_AddressCaps.dwCompletionMsgTextSize = (m_AddressCaps.dwCompletionMsgTextEntrySize * m_AddressCaps.dwNumCompletionMessages);
        
    return m_arrCompletionMsgs.Add (pszBuff);

}// CTSPIAddressInfo::AddCompletionMessage

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
	CEnterCode sLock(this);  // Synch access to object

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
	// Make sure we can perform this action now.
	if ((GetAddressStatus()->dwAddressFeatures & (LINEADDRFEATURE_FORWARD|
			LINEADDRFEATURE_FORWARDFWD | LINEADDRFEATURE_FORWARDDND)) == 0)
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
void CTSPIAddressInfo::SetMediaControl (TSPIMEDIACONTROL* lpMediaControl)
{   
	// If this is the CURRENT media control object, ignore this set.
	if (lpMediaControl == m_lpMediaControl)
		return;

	// Remove our reference to the PREVIOUS media control structure (if any)
	if (m_lpMediaControl != NULL)
		m_lpMediaControl->DecUsage();

    m_lpMediaControl = lpMediaControl;    
	if (m_lpMediaControl != NULL)
		m_lpMediaControl->IncUsage();

    // Go through all calls and tell them about this new media monitoring.
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION pos = m_lstCalls.GetHeadPosition(); pos != NULL;)
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstCalls.GetNext(pos);
        pCall->SetMediaControl (lpMediaControl);
    }

}// CTSPIAddressInfo::SetMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::DeleteForwardingInfo
//
// Delete the information in our forwarding array.  This array
// holds the forwarding information reported to TAPI.
//
void CTSPIAddressInfo::DeleteForwardingInfo()
{   
	CEnterCode sLock(this);  // Synch access to object
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
void CTSPIAddressInfo::OnRequestComplete(CTSPIRequest* pReq, LONG lResult)
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
        			DTRACE(TRC_MIN, _T("Deleting invalid consultation call <0x%lx>\r\n"), (DWORD) pForward->pCall);
					RemoveCallAppearance(pForward->pCall);
				}
			}
        }
    }                                      

	// If this is a lineSetMediaControl event, then store the new MediaControl
	// information in the address (and all of it's calls)
	else if (wRequest == REQUEST_MEDIACONTROL)
	{
		if (lResult == 0 && pReq->GetCallInfo() == NULL)
			SetMediaControl((TSPIMEDIACONTROL*)pReq->GetDataPtr());
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
        			DTRACE(TRC_MIN, _T("Deleting invalid consultation call <0x%lx>\r\n"), (DWORD) pTrans->pConsult);
        			RemoveCallAppearance(pTrans->pConsult);               
        		}
        	}
    	}
    }   
    // If this is a COMPLETE transfer request, and it failed, remove the 
    // consultation call appearance created.
    else if (wRequest == REQUEST_COMPLETEXFER)
    {
   		TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
    	if (lResult != 0)
    	{
    		if (pTrans->pConf != NULL)
    		{
    			if ((pTrans->pConf->m_dwFlags & CTSPICallAppearance::InitNotify) == 0)
    			{
    				DTRACE(TRC_MIN, _T("Deleting invalid conference call <0x%lx>\r\n"), (DWORD) pTrans->pConf);
    				RemoveCallAppearance(pTrans->pConf);
    			}
    		}

			// Reset the related conference call of the other two calls.
			if (pTrans->pCall)
				pTrans->pCall->SetConferenceOwner(NULL);
			if (pTrans->pConsult)
				pTrans->pConsult->SetConferenceOwner(NULL);
    	}

		// Otherwise it was successful. If it transitioned into a conference,
		// remove the consultation call data from the call since both calls 
		// are now part of the conference
		else
		{
			// Part of a conference?
			if (pTrans->pConf != NULL && pTrans->pCall != NULL)
				pTrans->pCall->SetConsultationCall(NULL);
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
        			DTRACE(TRC_MIN, _T("Deleting invalid consultation call <0x%lx>\r\n"), (DWORD) pConf->pConsult);
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
    				DTRACE(TRC_MIN, _T("Deleting invalid consultation call <0x%lx>\r\n"), (DWORD) pCall);
    				RemoveCallAppearance(pCall);
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
int CTSPIAddressInfo::AddForwardEntry (DWORD dwForwardMode, LPCTSTR pszCaller, 
			LPCTSTR pszDestination, DWORD dwDestCountry)
{   
    int iPos = -1;
    if (dwForwardMode == 0)
	{
        DeleteForwardingInfo();
        OnAddressStateChange (LINEADDRESSSTATE_FORWARD);
	}
    else
    {
        TSPIFORWARDINFO* pForward = new TSPIFORWARDINFO;
        if (pForward != NULL)
        {
            pForward->dwForwardMode = dwForwardMode;
            pForward->dwDestCountry = dwDestCountry;
            
            if (pszCaller != NULL && *pszCaller != _T('\0'))
                GetSP()->CheckDialableNumber(GetLineOwner(), this, pszCaller, &pForward->arrCallerAddress, dwDestCountry);
            
            if (pszDestination != NULL && *pszDestination != _T('\0'))
                GetSP()->CheckDialableNumber(GetLineOwner(), this, pszDestination, &pForward->arrDestAddress, dwDestCountry);

			// Calculate the size used for this forwarding information object so
			// we don't have to do it everytime TAPI requests our configuration.
			// This size is what is needed in TAPI terms for the forwarding information.
			pForward->dwTotalSize = sizeof(LINEFORWARD);
            for (int j = 0; j < pForward->arrCallerAddress.GetSize(); j++)
            {
                DIALINFO* pDialInfo = (DIALINFO*) pForward->arrCallerAddress[j];
                if (!pDialInfo->strNumber.IsEmpty())
                {
					int iLen = (pDialInfo->strNumber.GetLength()+1) * sizeof(TCHAR);
					if (j > 0)
						iLen += sizeof(TCHAR);
					while ((iLen%4) != 0)
						iLen++;
					pForward->dwTotalSize += iLen;
                }
            }

            for (j = 0; j < pForward->arrDestAddress.GetSize(); j++)
            {
                DIALINFO* pDialInfo = (DIALINFO*) pForward->arrDestAddress[j];
                if (!pDialInfo->strNumber.IsEmpty())
                {
					int iLen = (pDialInfo->strNumber.GetLength()+1) * sizeof(TCHAR);
					if (j > 0)
						iLen += sizeof(TCHAR);
					while ((iLen%4) != 0)
						iLen++;
					pForward->dwTotalSize += iLen;
                }
            }                

			// Add the forwarding information to our array.
			CEnterCode sLock(this);  // Synch access to object
            iPos = m_arrForwardInfo.Add (pForward);
            OnAddressStateChange (LINEADDRESSSTATE_FORWARD);
        }            
    }           
    
    return iPos;

}// CTSPIAddressInfo::AddForwardEntry

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo:AddCallTreatment
//
// Add a new call treatment entry into our address.  This will be
// returned in the ADDRESSCAPS for this address.
//
void CTSPIAddressInfo::AddCallTreatment (DWORD dwCallTreatment, LPCTSTR pszName)
{
	// Add or alter the existing call treatment.
	m_mapCallTreatment[dwCallTreatment] = pszName;
	m_AddressCaps.dwNumCallTreatments = m_mapCallTreatment.GetCount();
	m_AddressCaps.dwCallTreatmentListSize = sizeof(LINECALLTREATMENTENTRY) * m_AddressCaps.dwNumCallTreatments;
	
}// CTSPIAddressInfo::AddCallTreatment

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::RemoveCallTreatment
//
// Remove a call treatment entry from our address.
//
void CTSPIAddressInfo::RemoveCallTreatment (DWORD dwCallTreatment)
{
	// Locate and remove the specified call treatment name.
	if (m_mapCallTreatment.RemoveKey (dwCallTreatment))
	{
		m_AddressCaps.dwNumCallTreatments = m_mapCallTreatment.GetCount();
		m_AddressCaps.dwCallTreatmentListSize = sizeof(LINECALLTREATMENTENTRY) * m_AddressCaps.dwNumCallTreatments;
		// Tell TAPI our address capabilities have changed.
		OnAddressCapabiltiesChanged();
	}

}// CTSPIAddressInfo::RemoveCallTreatment

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnAddressStateChange
//
// Send a TAPI event for this address.
//
void CTSPIAddressInfo::OnAddressStateChange (DWORD dwAddressState)
{
    if ((m_dwAddressStates & dwAddressState) == dwAddressState)
        GetLineOwner()->Send_TAPI_Event (NULL, LINE_ADDRESSSTATE, GetAddressID(), dwAddressState);

}// CTSPIAddressInfo::Send_TAPI_Event

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnAddressCapabiltiesChanged
//
// The address capabilities have changed (ADDRESSCAPS), tell TAPI 
// about it.
//
void CTSPIAddressInfo::OnAddressCapabiltiesChanged()
{
	OnAddressStateChange (LINEADDRESSSTATE_CAPSCHANGE);

}// CTSPIAddressInfo::OnAddressCapabiltiesChanged

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::OnCreateCall
//
// This method gets called whenever a new call appearance is created
// on this address.
//
void CTSPIAddressInfo::OnCreateCall (CTSPICallAppearance* /*pCall*/)
{ 
	CEnterCode sLock(this);
	if (m_AddressStatus.dwNumInUse == 0)
	{
		m_AddressStatus.dwNumInUse = 1;
		sLock.Unlock();
		OnAddressStateChange(LINEADDRESSSTATE_INUSEONE);
	}

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
// CTSPIAddressInfo::SetAddressFeatures
//
// This method sets the current features available on the address
// It does NOT invoke the OnAddressFeaturesChanged.
//
void CTSPIAddressInfo::SetAddressFeatures(DWORD dwFeatures)
{
	CEnterCode sLock (this);

	// Make sure the capabilities structure reflects this ability.
	if ((m_AddressCaps.dwAddressFeatures & dwFeatures) == 0)
	{
		// If you get this, then you need to update the object dwAddressFeatures
		// in the Init method of the address or in the line while adding it.
		DTRACE(TRC_MIN, _T("LINEADDRCAPS.dwAddressFeatures missing 0x%lx bit\r\n"), dwFeatures);
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

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::RecalcAddrFeatures
//
// Recalculate the address features associated with this address
//
void CTSPIAddressInfo::RecalcAddrFeatures()
{
	// Adjust our address features based on the call counts we now have.
	DWORD dwAddressFeatures = m_AddressCaps.dwAddressFeatures;
	if ((GetLineOwner()->GetLineDevStatus()->dwDevStatusFlags & 
			(LINEDEVSTATUSFLAGS_INSERVICE | LINEDEVSTATUSFLAGS_CONNECTED)) != 
			(LINEDEVSTATUSFLAGS_INSERVICE | LINEDEVSTATUSFLAGS_CONNECTED))
		dwAddressFeatures = 0;
	else if (dwAddressFeatures > 0)
	{
		// If we have no more terminals, then remove the TERMINAL bit.
		if (GetLineOwner()->GetTerminalCount() == 0)
			dwAddressFeatures &= ~LINEADDRFEATURE_SETTERMINAL;

		// If there are active calls, and we support conferencing, then show SetupConf as a feature
		if (m_dwConnectedCallCount == 0)
			dwAddressFeatures &= ~LINEADDRFEATURE_SETUPCONF;
		else
			dwAddressFeatures &= ~(LINEADDRFEATURE_FORWARD | 
				LINEADDRFEATURE_FORWARDFWD | LINEADDRFEATURE_FORWARDDND);

		// Determine if any calls are waiting (camped on) pending call completions.
		if (dwAddressFeatures & LINEADDRFEATURE_UNCOMPLETECALL)
		{
    		dwAddressFeatures &= ~LINEADDRFEATURE_UNCOMPLETECALL;

			// Walk through all the calls on the address.
			CEnterCode sLock(this);
			for (int i = 0; i < GetCallCount(); i++)
    		{
				CTSPICallAppearance* pCall = GetCallInfo(i);
    			if (GetLineOwner()->FindCallCompletionRequest(pCall))
    			{
        			dwAddressFeatures |= LINEADDRFEATURE_UNCOMPLETECALL;
    				break;
    			}
    		}
			sLock.Unlock();
		}

		// If we have don't the bandwidth for another active call, then remove all
		// the features which create a new call appearance.
		if (m_AddressStatus.dwNumActiveCalls >= m_AddressCaps.dwMaxNumActiveCalls)
   			dwAddressFeatures &= ~(LINEADDRFEATURE_MAKECALL | 
						LINEADDRFEATURE_PICKUP | 
						LINEADDRFEATURE_UNPARK | 
						LINEADDRFEATURE_PICKUPHELD | 
						LINEADDRFEATURE_PICKUPGROUP | 
						LINEADDRFEATURE_PICKUPDIRECT | 
						LINEADDRFEATURE_PICKUPWAITING);
	}

	// If our new address features are zero, recalc our call features.
	else
	{
		CEnterCode sLock(this);
		for (int i = 0; i < GetCallCount(); i++)
    	{
			CTSPICallAppearance* pCall = GetCallInfo(i);
			// Don't do it for IDLE calls since they might de-allocate in 
			// the middle of this loop and cause us to GPF.
			if (pCall->GetCallState() != LINECALLSTATE_IDLE)
				pCall->OnCallStatusChange(pCall->GetCallState(), 0, 0);
		}
		sLock.Unlock();
	}

	// Set our address features
	dwAddressFeatures &= m_AddressCaps.dwAddressFeatures;
	m_AddressStatus.dwAddressFeatures = OnAddressFeaturesChanged(dwAddressFeatures);

}// CTSPIAddressInfo::RecalcAddrFeatures

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::Dump
//
// Debug method to dump out the Address object
//
void CTSPIAddressInfo::Dump (CDumpContext& /*dc*/) const
{
	DTRACE(TRC_MIN, _T("Address ID:0x%lx [%s] %s\r\n"), m_dwAddressID, (LPCTSTR) m_strName, (LPCTSTR) m_strAddress);

}// CTSPIAddressInfo::Dump
#endif // _DEBUG

