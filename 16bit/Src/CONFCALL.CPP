/******************************************************************************/
//                                                                        
// CONFCALL.CPP - Source code for the CTSPIConferenceCall object.
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code for managing the conference call appearance       
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

IMPLEMENT_DYNCREATE( CTSPIConferenceCall, CTSPICallAppearance )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::CTSPIConferenceCall
//
// Conference call constructor
//
CTSPIConferenceCall::CTSPIConferenceCall()
{
    m_iCallType = CALLTYPE_CONFERENCE;

}// CTSPIConferenceCall::CTSPIConferenceCall

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
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_PREPAREADDCONF) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Get our address owner.  All new call appearances will be created on it.
    CTSPIAddressInfo* pAddr = GetAddressInfo();

    // If this is not a conference call, fail it.
    if (GetCallType() != CALLTYPE_CONFERENCE)
        return LINEERR_INVALCONFCALLHANDLE;
    
    // Get the total count of people in the conference right now.
    int iCurrCount = m_arrConference.GetSize();
    
    // If we have exceeded our conference count, then fail this request.
    if (pAddr->GetAddressCaps()->dwMaxNumConference <= (DWORD) iCurrCount+1)
        return LINEERR_CONFERENCEFULL;
    
    // Otherwise, create a consultation call.
    CTSPICallAppearance* pCall = pAddr->CreateCallAppearance(htConsultCall);
    if (pCall == NULL)
        return LINEERR_CALLUNAVAIL;
    
    // Mark it as a consultation call.    
    pCall->SetCallType (CALLTYPE_CONSULTANT);
    lpConf->pConsult = pCall;
    
    // Attach it to the conference in case the conference ends and the derived
    // class needs to locate any consultant calls created.  Note: this implies that
    // there may be only one consultant call per conference active at any given point.
    AttachCall(pCall);
    pCall->AttachCall(this);
    
    // And add the request                                  
    if (AddAsynchRequest(REQUEST_PREPAREADDCONF, dwRequestID, lpConf) != NULL)
    {
        *lphdConsultCall = (HDRVCALL) pCall;
        return (LONG) dwRequestID;          
    }
    
    pAddr->RemoveCallAppearance (pCall);
    return LINEERR_OPERATIONFAILED;

}// CTSPIConferenceCall::PrepareAddToConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::AddToConference
//
// Add the passed call to our conference list.
//
LONG CTSPIConferenceCall::AddToConference (DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall, TSPICONFERENCE* lpConf)
{                               
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_ADDTOCONF) == 0)
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
        if ((GetAddressInfo()->GetAddressCaps()->dwAddrCapFlags & 
                LINEADDRCAPFLAGS_CONFERENCEMAKE) == 0)
            return LINEERR_INVALCONSULTCALLHANDLE;
            
        // If the call being added is not on the same address, then verify that
        // cross-address conferences are supported.
        if (pCall->GetAddressInfo() != GetAddressInfo())
        {
            if ((GetLineConnectionInfo()->GetLineDevCaps()->dwDevCapFlags & 
                    LINEDEVCAPFLAGS_CROSSADDRCONF) == 0)
                return LINEERR_INVALCONSULTCALLHANDLE;
        }    
    }

    // Make sure the call is in the proper state to be added to our
    // conference.  
    // Added PROCEEDING and RINGBACK to the available call state (TAPI Bakeoff fix).
    if (pCall->GetCallState() != LINECALLSTATE_CONNECTED &&
        pCall->GetCallState() != LINECALLSTATE_ONHOLD &&
        pCall->GetCallState() != LINECALLSTATE_PROCEEDING &&
        pCall->GetCallState() != LINECALLSTATE_RINGBACK)
        return LINEERR_INVALCALLSTATE;
    
    // Make sure our conference call is ready to receive a new 
    // member - it must be in the onHoldPendConf state or the
    // onHold state if our address caps say we can do that.
    if (GetCallState() == LINECALLSTATE_ONHOLDPENDCONF ||
        (GetCallState() == LINECALLSTATE_ONHOLD &&
         (GetAddressInfo()->GetAddressCaps()->dwAddrCapFlags & LINEADDRCAPFLAGS_CONFERENCEHELD)))
        ;
    else
        return LINEERR_INVALCALLSTATE;

    // Attach the call appearance to the conference so we can locate the
    // original conference call later.  We store it into the related call
    // field.
    pCall->SetConferenceOwner(lpConf->pConfCall);

    // If the call is a consultation call which was ATTACHED to the conference for
    // tracking purposes, then detach it so we are not connected TWICE.
    if (GetAttachedCall() == pCall)
    {
        DetachCall();
        pCall->DetachCall();
    }

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
    if (!IsCallInConference (pCall))
	{
		// Attach the call to the conference
		pCall->SetConferenceOwner(this);
		if (pCall->GetCallState() == LINECALLSTATE_CONFERENCED)
			m_arrConference.Add (pCall);
	}

}// CTSPIConferenceCall::AddToConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::RemoveFromConference
//
// Remove the specified call from our conference list.
//
LONG CTSPIConferenceCall::RemoveFromConference(DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall, TSPICONFERENCE* lpConf)
{   
	// If the function cannot be performed..
	if ((GetCallStatus()->dwCallFeatures & LINECALLFEATURE_REMOVEFROMCONF) == 0)
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
    DWORD dwFlags = GetAddressInfo()->GetAddressCaps()->dwRemoveFromConfCaps;
    if (dwFlags == LINEREMOVEFROMCONF_NONE)
        return FALSE;

    // If only the last party can be removed, make sure this is the last entry in the 
    // conference list.
    if (dwFlags & LINEREMOVEFROMCONF_LAST)
    {
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
    for (int i = 0; i < m_arrConference.GetSize(); i++)
    {
        CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
        if (pTest == pCall)
            return TRUE;
    }
    return FALSE;

}// CTSPIConferenceCall::IsCallInConference

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::GetConferenceCount
//
// Return the count of call appearances in our conference.
//
int CTSPIConferenceCall::GetConferenceCount() const
{                                          
    return m_arrConference.GetSize();

}// CTSPIConferenceCall::GetConferenceCount

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::GetConferenceCall
//
// Return the call at the index specified
//
CTSPICallAppearance* CTSPIConferenceCall::GetConferenceCall(int iPos)
{   
    if (iPos >= 0 && iPos < GetConferenceCount())                                      
        return (CTSPICallAppearance*) m_arrConference.GetAt(iPos);
    return NULL;        

}// CTSPIConferenceCall::GetConferenceCall

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::RemoveConferenceCall
//
// Remove the conference call specified from our array.  Callstate
// of the removed call is not changed.  Conference is idle'd if this
// is the last call.
//
VOID CTSPIConferenceCall::RemoveConferenceCall(CTSPICallAppearance* pCall)
{   
    // Locate the call appearance and remove it.
    for (int i = 0; i < m_arrConference.GetSize(); i++)
    {
        CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
        if (pTest == pCall)
        {
            m_arrConference.RemoveAt(i);
            pCall->SetConferenceOwner(NULL);
            
            // If we have no more calls in our array, then transition 
            // the conference to IDLE.
            if (m_arrConference.GetSize() == 0)
                SetCallState(LINECALLSTATE_IDLE);
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
VOID CTSPIConferenceCall::OnCallStatusChange (DWORD dwState, DWORD /*dwMode*/, DWORD /*dwMediaMode*/)
{            
	int i;
	
    // The last behavior of a REQUEST_DROPxxx on a conference call should be to idle
    // the conference call - this causes the conference to disconnect from all the
    // calls.
    if (dwState == LINECALLSTATE_IDLE)
    {   
        // Remove our back-pointers from the call appearances since we are about to
        // become non-existant.
        for (i = 0; i < m_arrConference.GetSize(); i++)
        {
            CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
            ASSERT(pTest != NULL);
            pTest->SetConferenceOwner(NULL);
        }       
    }
    
    // Otherwise, we are making some other transition, spin through all the connected
    // conference calls and force them to update their capabilities.                 
    // This handles the situation where attached calls change state BEFORE the conference
    // does and make sure that the capabilities stay updated properly.
    else
    {                                                                 
        for (i = 0; i < m_arrConference.GetSize(); i++)
        {
            CTSPICallAppearance* pTest = (CTSPICallAppearance*) m_arrConference[i];
            ASSERT(pTest != NULL);
            pTest->OnCallStatusChange(pTest->GetCallState(), 0, 0);
        }       
    }

	// Let the default capabilities be adjusted on this call based on its
	// new state.
	CTSPICallAppearance::OnCallStatusChange(dwState, 0, 0);
    
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
// CTSPIConferenceCall::OnConsultantCallIdle
//
// This member is called when the consultant call attached to this
// conference went IDLE, leaving us in a ONHOLDPENDCONF state.
//
// This should be overriden to provide for specific conferencing specifics
// such as providing a new consulting call (in the DIALTONE state), moving
// back into the conference, or moving back to a two-party call.
//
VOID CTSPIConferenceCall::OnConsultantCallIdle()
{
    /* Do nothing */
    
}// CTSPIConferenceCall::OnConsultantCallIdle

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::OnRelatedCallStateChange
//
// This method is called by all calls which are part of our confernece
// as their state changes.  We use it to insert and delete calls from
// our conference.
//
VOID CTSPIConferenceCall::OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD /*dwOldState*/)
{
    // If the call is now conferenced in, add it to our list.
    if (dwState == LINECALLSTATE_CONFERENCED)
    {    
        if (!IsCallInConference(pCall))
            m_arrConference.Add (pCall);
    }
    // Otherwise if it has transitioned to IDLE or to the state specified for
    // end of conference calls, then remove it.
    else if (dwState == LINECALLSTATE_IDLE ||
             dwState == GetAddressInfo()->GetAddressCaps()->dwRemoveFromConfState)
    {   
        // If this was a consultant call created by PrepareConf or SetupConf, then
        // it is not part of our conference, and now never will be.
        if (GetAttachedCall() == pCall)
        {   
            // Detach it from us.
            pCall->DetachCall();
            DetachCall();
            
            // Give any derived class an opprotunity to do something about the call being
            // destroyed - i.e. some switches actually drop back to a dialtone, which would
            // require a new call be created.  Some switch back to a two party call when only
            // one member is still in the conference.  Our default is to do nothing.
            if (GetCallState() == LINECALLSTATE_ONHOLDPENDCONF)
                OnConsultantCallIdle();
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
