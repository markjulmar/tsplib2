/******************************************************************************/
//                                                                        
// CONFCALL.CPP - Source code for the CTSPIConferenceCall object.
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code for managing the conference call appearance       
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

IMPLEMENT_DYNCREATE( CTSPIConferenceCall, CTSPICallAppearance )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::~CTSPIConferenceCall
// 
// Conference call destructor
//
CTSPIConferenceCall::~CTSPIConferenceCall()
{
    // DO NOT DELETE THE CONFERENCE LIST - WE ONLY WANT THE LINKS
    // DELETED WHICH THE CObArray WILL DO AUTOMATICALLY.

}// CTSPIConferenceCall::~CTSPIConferenceCall

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::PrepareAddToConference
//
// This method asks the address owner to create a new consultation
// call appearance and submits a Prepare request.
//
LONG CTSPIConferenceCall::PrepareAddToConference(DRV_REQUESTID dwRequestID, HTAPICALL htConsultCall, 
                                                 LPHDRVCALL lphdConsultCall, TSPICONFERENCE* lpConf)
{   
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_PREPAREADDCONF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Get our address owner.  All new call appearances will be created on it.
    CTSPIAddressInfo* pAddr = GetAddressOwner();

    // If this is not a conference call, fail it.
    if (GetCallType() != CALLTYPE_CONFERENCE)
        return LINEERR_INVALCONFCALLHANDLE;
    
    // Get the total count of people in the conference right now.
    int iCurrCount = m_arrConference.GetSize();
    
    // If we have exceeded our conference count, then fail this request.
    if (pAddr->GetAddressCaps()->dwMaxNumConference <= (DWORD) iCurrCount+1)
        return LINEERR_CONFERENCEFULL;
    
    // Otherwise, create a consultation call.
	lpConf->pConsult = CreateConsultationCall(htConsultCall);
    if (lpConf->pConsult == NULL)
        return LINEERR_CALLUNAVAIL;
    
    // And add the request                                  
    if (AddAsynchRequest(REQUEST_PREPAREADDCONF, dwRequestID, lpConf) != NULL)
    {
        *lphdConsultCall = (HDRVCALL) lpConf->pConsult;
        return (LONG) dwRequestID;          
    }
    
    pAddr->RemoveCallAppearance(lpConf->pConsult);
    return LINEERR_OPERATIONFAILED;

}// CTSPIConferenceCall::PrepareAddToConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::AddToConference
//
// Add the passed call to our conference list.
//
LONG CTSPIConferenceCall::AddToConference (DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall, TSPICONFERENCE* lpConf)
{
	if ((pCall->GetCallStatus()->dwCallFeatures & LINECALLFEATURE_ADDTOCONF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If this isn't a consultation call we created, then make sure a new 
    // call (non-consultation) can be created by the address for addition to
    // a conference.
    if (pCall->GetCallType() != CALLTYPE_CONSULTANT)
    {
        // Make sure this call isn't another conference call!
        if (pCall->GetCallType() == CALLTYPE_CONFERENCE)
            return LINEERR_INVALCONSULTCALLHANDLE;
        
        // Verify that we can add ANY type of call since this isn't a consultant
        // call created by PrepareAddToConf. or SetupConf.
        if ((GetAddressOwner()->GetAddressCaps()->dwAddrCapFlags & 
                LINEADDRCAPFLAGS_CONFERENCEMAKE) == 0)
            return LINEERR_INVALCONSULTCALLHANDLE;
            
        // If the call being added is not on the same address, then verify that
        // cross-address conferences are supported.
        if (pCall->GetAddressOwner() != GetAddressOwner())
        {
            if ((GetLineOwner()->GetLineDevCaps()->dwDevCapFlags & 
                    LINEDEVCAPFLAGS_CROSSADDRCONF) == 0)
                return LINEERR_INVALCONSULTCALLHANDLE;
        }    
    }

#if STRICT_CALLSTATES
    // Make sure the call is in the proper state to be added to our
    // conference.  
    // Added PROCEEDING and RINGBACK to the available call state (TAPI Bakeoff fix).
    if ((pCall->GetCallState() & 
				(LINECALLSTATE_CONNECTED |
				 LINECALLSTATE_ONHOLD |
				 LINECALLSTATE_PROCEEDING |
				 LINECALLSTATE_RINGBACK)) == 0)
        return LINEERR_INVALCALLSTATE;
#endif
    
    // Make sure our conference call is ready to receive a new 
    // member - it must be in the onHoldPendConf state or the
    // onHold state if our address caps say we can do that.
    if (GetCallState() == LINECALLSTATE_ONHOLDPENDCONF ||
        (GetCallState() == LINECALLSTATE_ONHOLD &&
         (GetAddressOwner()->GetAddressCaps()->dwAddrCapFlags & LINEADDRCAPFLAGS_CONFERENCEHELD)))
        ;
    else
        return LINEERR_INVALCALLSTATE;

    // Attach the call appearance to the conference so we can locate the
    // original conference call later.  We store it into the related call
    // field.
    pCall->SetConferenceOwner(lpConf->pConfCall);

    // Pass the request to our service provider worker code.
    if (AddAsynchRequest(REQUEST_ADDCONF, dwRequestID, lpConf) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPIConferenceCall::AddToConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::AddToConference
//
// This method forcibly adds a call to a conference.  It should only
// be used to build a conference by hand (i.e. directed by an attached 
// handset or phone).
//
void CTSPIConferenceCall::AddToConference(CTSPICallAppearance* pCall)
{   
	// Add the call to the conference.
    if (!IsCallInConference(pCall) && pCall != this)
	{
		// Attach the call to the conference
		pCall->SetConferenceOwner(this);
		if (pCall->GetCallState() == LINECALLSTATE_CONFERENCED)
			m_arrConference.Add (pCall);
	}

	// If this is the consultation call for the conference, change the type
	// of call and remove the attachment.
    if (GetAttachedCall() == pCall)
	{
		SetConsultationCall(NULL);
		pCall->SetCallType(CALLTYPE_NORMAL);
		pCall->OnCallStatusChange(m_CallStatus.dwCallState, m_CallStatus.dwCallStateMode, m_CallInfo.dwMediaMode);
	}

}// CTSPIConferenceCall::AddToConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::RemoveFromConference
//
// Remove the specified call from our conference list.
//
LONG CTSPIConferenceCall::RemoveFromConference(DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall, TSPICONFERENCE* lpConf)
{
	if ((pCall->GetCallStatus()->dwCallFeatures & LINECALLFEATURE_REMOVEFROMCONF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Check the call state of this call and make sure it is conferenced in.
    if (pCall->GetCallState() != LINECALLSTATE_CONFERENCED)
        return LINEERR_INVALCALLSTATE;
    
    // Verify that this call may be removed.
    if (!CanRemoveFromConference(pCall))
        return LINEERR_OPERATIONUNAVAIL;
    
    // Looks ok, add the request.    
    if (AddAsynchRequest (REQUEST_REMOVEFROMCONF, dwRequestID, lpConf) != NULL)
        return (LONG) dwRequestID;

    return LINEERR_OPERATIONFAILED;

}// CTSPIConferenceCall::RemoveFromConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::CanRemoveFromConference
//
// Return TRUE/FALSE indicating whether the call may be removed
// from the conference.
//
BOOL CTSPIConferenceCall::CanRemoveFromConference(CTSPICallAppearance* pCall) const
{
    // Validate that the call being removed is in the conference.
    if (!IsCallInConference (pCall))
        return FALSE;

    // If our address doesn't allow removal of conference parties, then error out.
    DWORD dwFlags = GetAddressOwner()->GetAddressCaps()->dwRemoveFromConfCaps;
    if (dwFlags == LINEREMOVEFROMCONF_NONE)
        return FALSE;

    // If only the last party can be removed, make sure this is the last entry in the 
    // conference list.
    if (dwFlags & LINEREMOVEFROMCONF_LAST)
    {
		CEnterCode cLock (this);  // Synch access to object
        int iLastIndex = m_arrConference.GetUpperBound();
        CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[iLastIndex];
        if (pTest != pCall)
            return FALSE;
    }
    
    // Looks like we can remove it.
    return TRUE;
    
}// CTSPIConferenceCall::CanRemoveFromConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::IsCallInConference
//
// Determine whether the specified call appearance is in our conference
// list or not.
//
BOOL CTSPIConferenceCall::IsCallInConference(CTSPICallAppearance* pCall) const
{
	CEnterCode sLock(this);  // Synch access to object
    for (int i = 0; i < m_arrConference.GetSize(); i++)
    {
        CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
        if (pTest == pCall)
            return TRUE;
    }
    return FALSE;

}// CTSPIConferenceCall::IsCallInConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::RemoveConferenceCall
//
// Remove the conference call specified from our array.  Callstate
// of the removed call is not changed.  Conference is idle'd if this
// is the last call.
//
VOID CTSPIConferenceCall::RemoveConferenceCall(CTSPICallAppearance* pCall, BOOL fForceBreakdown/*=TRUE*/)
{   
    // Locate the call appearance and remove it.
	CEnterCode sLock(this);  // Synch access to object
    for (int i = 0; i < m_arrConference.GetSize(); i++)
    {
        CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
        if (pTest == pCall)
        {
            m_arrConference.RemoveAt(i);
            pCall->SetConferenceOwner(NULL);
            
            // If we have no more calls in our array, then transition 
            // the conference to IDLE.
            if (m_arrConference.GetSize() == 0 && fForceBreakdown)
                SetCallState(LINECALLSTATE_IDLE);
			else if (m_arrConference.GetSize() == 1 && fForceBreakdown)
			{
				pCall = GetConferenceCall(0);
				RemoveConferenceCall(pCall);
				if (pCall->GetCallState() == LINECALLSTATE_CONFERENCED)
				{
					// If the remove state is not IDLE, change the call state to the given
					// state in the ADDRESSCAPS structure.
					if (GetAddressOwner()->GetAddressCaps()->dwRemoveFromConfState != LINECALLSTATE_IDLE)
						pCall->SetCallState(GetAddressOwner()->GetAddressCaps()->dwRemoveFromConfState);
					else
						pCall->SetCallState(LINECALLSTATE_CONNECTED);
				}
			}
            break;
        }
    }               

}// CTSPIConferenceCall::RemoveConferenceCall

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::OnCallStatusChange
//
// This verifies that we have torn down the conference call
// before our conference call handle goes idle.  The TAPI dynamic link
// library maintains the conference calls in a linked list which will
// not correctly get deallocated if we don't idle all the calls in 
// the conference before the conference call.
//
// Generally, all the calls should already be changed or removed as
// a result of this.
//
VOID CTSPIConferenceCall::OnCallStatusChange (DWORD dwState, DWORD dwMode, DWORD dwMediaMode)
{             
	int i;

	// Should NEVER move to conferenced - this is for parties IN the conference only!
	ASSERT (dwState != LINECALLSTATE_CONFERENCED);

	// Pass to base class
	CTSPICallAppearance::OnCallStatusChange(dwState, dwMode, dwMediaMode);

	// Remove the features which should never be available in a conference call.
	m_CallStatus.dwCallFeatures &= ~(LINECALLFEATURE_ACCEPT |
			LINECALLFEATURE_ANSWER |
			LINECALLFEATURE_BLINDTRANSFER |
			LINECALLFEATURE_COMPLETECALL |
			LINECALLFEATURE_COMPLETETRANSF |
			LINECALLFEATURE_DIAL |
			LINECALLFEATURE_GATHERDIGITS |
			LINECALLFEATURE_GENERATEDIGITS |
			LINECALLFEATURE_GENERATETONE |
			LINECALLFEATURE_MONITORDIGITS |
			LINECALLFEATURE_MONITORMEDIA |
			LINECALLFEATURE_MONITORTONES |
			LINECALLFEATURE_PARK |
			LINECALLFEATURE_REDIRECT |
			LINECALLFEATURE_REMOVEFROMCONF |
			LINECALLFEATURE_SECURECALL |
			LINECALLFEATURE_SENDUSERUSER |
			LINECALLFEATURE_SETCALLPARAMS |
			LINECALLFEATURE_SETMEDIACONTROL |
			LINECALLFEATURE_SETTERMINAL |
			LINECALLFEATURE_SETUPCONF |
			LINECALLFEATURE_SETUPTRANSFER |
			LINECALLFEATURE_RELEASEUSERUSERINFO |
			LINECALLFEATURE_SETTREATMENT |
			LINECALLFEATURE_SETQOS);

    // The last behavior of a REQUEST_DROPxxx on a conference call should be to idle
    // the conference call - this causes the conference to disconnect from all the calls.
    if (dwState == LINECALLSTATE_IDLE)
    {   
        // Remove our back-pointers from the call appearances since we are about to
        // become non-existant.
		CEnterCode sLock(this);  // Synch access to object
        for (i = 0; i < m_arrConference.GetSize(); i++)
        {
            CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
			ASSERT (pTest != NULL);
			pTest->SetConferenceOwner(NULL);
        }       
    }

    // Otherwise, we are making some other transition, spin through all the connected
    // conference calls and force them to update their capabilities.                 
    // This handles the situation where attached calls change state BEFORE the conference
    // does and make sure that the capabilities stay updated properly.
    else
    {                                                                 
		CEnterCode sLock(this);  // Synch access to object
        for (i = 0; i < m_arrConference.GetSize(); i++)
        {
            CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
			ASSERT (pTest != NULL);
            pTest->OnCallStatusChange(pTest->GetCallState(), 0, 0);
        }       
    }

    // NOTE: Depending on the switch behavior, all calls might be dropped when
    // the conference is dropped - in which case, the LINEADDRESSCAPS 
    // "LINEADDRCAPFLAGS_CONFDROP" should be specified.  In other cases, the original
    // call is transitioned back to CONNECTED - the library cannot really do much 
    // in this area, so the best way to process this is a little code in the 
    // REQUEST_DROPxxx handler which looks at the call type for a CALLTYPE_CONFERENCE
    // and does whatever necessary to all the calls before really dropping the
    // conference.  Remember that Drop() is asynch, and therefore will return *before*
    // it is actually finished - therefore use the WaitForCompletion method to wait
    // for it if you have to physically drop the connection.

}// CTSPIConferenceCall::OnCallStatusChange

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::OnRelatedCallStateChange
//
// This method is called by all calls which are part of our confernece
// as their state changes.  We use it to insert and delete calls from
// our conference.
//
VOID CTSPIConferenceCall::OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD /*dwOldState*/)
{
	CEnterCode sLock(this);  // Synch access to object

    // If the call is now conferenced in, add it to our list.
    if (dwState == LINECALLSTATE_CONFERENCED)
    {    
		// Add the call to the conference.
		if (!IsCallInConference(pCall))
			m_arrConference.Add (pCall);

		// Mark the conference owner if it isn't set yet.
		pCall->SetConferenceOwner(this);

		// If this is the consultation call for the conference, change the type
		// of call and remove the attachment.
		if (GetAttachedCall() == pCall)
		{
			SetConsultationCall(NULL);
			pCall->SetCallType(CALLTYPE_NORMAL);
		}

		// Force the call to re-evaluate it's call features so it gets the
		// lineRemoveFromConference bit set.
		pCall->OnCallStatusChange(dwState, m_CallStatus.dwCallStateMode, m_CallInfo.dwMediaMode);
    }

    // Otherwise if it has transitioned to IDLE then remove it from the conference
    else if (dwState == LINECALLSTATE_IDLE)
    {   
        // If this was a consultant call created by PrepareConf or SetupConf, then
        // it is not part of our conference, and now never will be.
        if (GetAttachedCall() == pCall)
        {   
            // Give any derived class an opportunity to do something about the call being
            // destroyed - i.e. some switches actually drop back to a dialtone, which would
            // require a new call be created.  Some switch back to a two party call when only
            // one member is still in the conference.  The default is to do nothing.
			OnConsultantCallIdle(GetConsultationCall());

			// Detach it from the conference
			SetConsultationCall(NULL);
        }
        else
        {   
            // Locate the call appearance which went IDLE and remove it from our conference
            // array.  If the total number of conferenced calls hits zero, then IDLE this
            // call.
            RemoveConferenceCall(pCall);
        }
    }

}// CTSPIConferenceCall::OnRelatedCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::OnRequestComplete
//
// This method is called by the request object when an outstanding
// request completes.
//
VOID CTSPIConferenceCall::OnRequestComplete (CTSPIRequest* pReq, LONG lResult)
{   
    WORD wCommand = pReq->GetCommand();
    
    // If this is a request to remove a call from the conference,
	// go ahead and do it if the service provider didn't already.
    if (wCommand == REQUEST_REMOVEFROMCONF)
    {
		if (lResult == 0)
		{
			TSPICONFERENCE* pConf = (TSPICONFERENCE*) pReq->GetDataPtr();
			if (pConf->pCall && pConf->pCall->GetCallState() != LINECALLSTATE_IDLE)
				RemoveConferenceCall(pConf->pCall);
		}
	}

}// CTSPIConferenceCall::OnRequestComplete
