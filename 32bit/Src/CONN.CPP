/******************************************************************************/
//                                                                        
// CONN.CPP - Source file for the TSPIConnection class object.
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code for the base connection object upon    
// which the phone and line connections are derived.                      
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

IMPLEMENT_DYNAMIC( CTSPIConnection, CObject )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::~CTSPIConnection
//
// Destructor
//
CTSPIConnection::~CTSPIConnection()
{
	CEnterCode cLock(this);  // Synch access to object

    // Delete any pending requests
    for (POSITION pos = m_oblAsynchRequests.GetHeadPosition(); pos != NULL;)
    {
        CTSPIRequest* pReq = (CTSPIRequest*) m_oblAsynchRequests.GetNext(pos);
        delete pReq;
    }
    m_oblAsynchRequests.RemoveAll();

}// CTSPIConnection::~CTSPIConnection

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::Init
//
// Initialize the connection.  Each CTSPIConnection is owned by a
// specific device and has a positional identifier which is set by
// TAPI.
//
VOID CTSPIConnection::Init(CTSPIDevice* pDevice, DWORD dwDeviceId)
{
	ASSERT (pDevice != NULL);
    m_pDevice = pDevice;
    m_dwDeviceID = dwDeviceId;

}// CTSPIConnection::Init

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddAsynchRequest
//
// This method inserts a new asynchronous request into our request
// list, and if the connection has no pending requests, sends it to the 
// device.
//
CTSPIRequest* CTSPIConnection::AddAsynchRequest(CTSPIAddressInfo* pAddr, 
                        CTSPICallAppearance* pCall, WORD wReqId, 
                        DRV_REQUESTID dwReqId, LPCVOID lpBuff, DWORD dwSize)
{
    // Allocate the request object through our dynamic object
    // creation so that the derived class may have its own types.
    CTSPIRequest* pReq = (CTSPIRequest*) GetSP()->GetTSPIRequestObj()->CreateObject();
    if (pReq != NULL)
    {
        ASSERT(pReq->IsKindOf(RUNTIME_CLASS(CTSPIRequest)));
        pReq->Init(this, pAddr, pCall, wReqId, dwReqId, (LPVOID)lpBuff, dwSize);

#ifdef _DEBUG        
        DTRACE(TRC_MIN, _T("<0x%lx> created new asynch request: <0x%lx>\r\n"), (DWORD)this, (DWORD) pReq);
        pReq->Dump(afxDump);
#endif

		// If this is a DROP request, then move it to the front of the pack.
		int iPos = -1;
		if (pReq->GetCommand() == REQUEST_DROPCALL)
			iPos = 0;

		// Give the provider a chance to re-shuffle existing requests.
		if (OnNewRequest (pReq, &iPos) == FALSE)
		{
			DTRACE(TRC_MIN, _T("Request <0x%lx> canceled by OnNewRequest\r\n"), (DWORD)pReq);
			delete pReq;
			return NULL;
		}

		// Insert the request into our list.
		if (AddAsynchRequest(pReq, iPos))
			GetSP()->StartNextCommand(this);

    }
    return pReq;
   
}// CTSPIConnection::AddAsynchRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddAsynchRequest
//
// Add the listed asynch request to the list and return whether to
// immediately start the request.  This function is overridable for
// providers which need to examine the request be starting it.
//
BOOL CTSPIConnection::AddAsynchRequest (CTSPIRequest* pReq, int iPos)
{
	CEnterCode Key (this);  // Synch access to object

	// Validate the position.
	int iTotal = m_oblAsynchRequests.GetCount();
	if (iPos > iTotal)
		iPos = -1;

    // If the request is to be added to the FRONT.
	if (iPos == 0)
    {
		// Add to front, always START this request.
        m_oblAsynchRequests.AddHead(pReq);
		return TRUE;
    }

	// Otherwise, add to the end and start the request only if we don't 
	// have other active requests already running.  This is so we don't 
	// interrupt some response from the device which is associated with a
	// particular running request by invoking a new command.
	else if (iPos == -1)
	{
		m_oblAsynchRequests.AddTail(pReq);
		return (++iTotal == 1);
	}

	// Or somewhere between.
	else
	{
		POSITION pos = m_oblAsynchRequests.FindIndex(iPos);
		if (pos == NULL)
			pos = m_oblAsynchRequests.GetHeadPosition();
		m_oblAsynchRequests.InsertBefore(pos, pReq);
		return FALSE;
	}

}// CTSPIConnection::AddAsynchRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::CompleteRequest
//
// Complete a specific request and information TAPI through the 
// asynchronous completion callback that the request is finished.
// This function optionally removes the request (deletes it).
// It does NOT start a new request.
//
VOID CTSPIConnection::CompleteRequest (CTSPIRequest* pReq, LONG lResult, 
                                       BOOL fTellTapi, BOOL fRemoveRequest)
{
#ifdef _DEBUG
	if (!pReq->HaveSentResponse() && fTellTapi)
		DTRACE(TRC_MIN, _T("Completing  request <0x%lx %s>, rc=0x%lx, TellTapi=%d, RemoveRequest=%d\r\n"), 
				(DWORD)pReq, pReq->GetRequestTypeName(), lResult, fTellTapi, fRemoveRequest);
#endif

    // Remove the request from our list.
    if (fRemoveRequest)
		RemoveRequest(pReq, FALSE);

	// Tell TAPI the result of the operation.  We do this AFTER we have removed the
	// request since TAPISRV.EXE tends to reuse handles quickly.
    if (!pReq->HaveSentResponse() && fTellTapi)
	{
		ASSERT (GetDeviceInfo() != NULL);
        GetDeviceInfo()->OnAsynchRequestComplete(lResult, pReq);
	}

    // Unblock any threads waiting on this to complete.  This will cause our
    // thread to relinquish its time-slice in order to let the other threads
    // come alive and return our result code.
    // This will also cause the 'OnRequestComplete' method of the connection, 
    // address, and call to be invoked.
    pReq->Complete (lResult, fTellTapi);
             
    // Delete the request now.
    if (fRemoveRequest)
    {
        DTRACE(TRC_MIN, _T("Deleting request <0x%lx>\r\n"), (DWORD) pReq);
        delete pReq;             

		// Start the next request if there is one.
        if (GetRequestCount() > 0)
            GetSP()->StartNextCommand(this);
    }

}// CTSPIConnection::CompleteRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::CompleteCurrentRequest
//
// This method informs TAPI of the completion of an asynchronous
// task and optionally removes and starts the next task in the
// list.
//
BOOL CTSPIConnection::CompleteCurrentRequest(LONG lResult, BOOL fTellTapi, BOOL fRemoveRequest)
{
    // If the request list is not empty, then grab the head request and complete it.
    CTSPIRequest* pReq = GetCurrentRequest();
    if (pReq != NULL)
    {
        // Unblock any threads waiting on this request to complete.
        CompleteRequest (pReq, lResult, fTellTapi, fRemoveRequest);
        return TRUE;
    }
    return FALSE;

}// CTSPIConnection::CompleteCurrentRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::RemovePendingRequests
//
// Removes any pending requests for the specified connection.
//
VOID CTSPIConnection::RemovePendingRequests(CTSPICallAppearance* pCall, WORD wReqType, LONG lErrorCode, BOOL fOnlyUnstarted, CTSPIRequest* preqIgnore)
{
	// Don't allow other threads to mess with the request list 
	// while we are deleting requests.
	CEnterCode sLock(this);

	// Walk through all the requests in the list matching them up to the connection object
	// and the optional call appearance and request type.  We need to make multiple passes
	// since we are removing requests as we go - so we start over from the top each time.
	while (TRUE)
	{
		// Walk through the list and find the first request which matches the
		// requested criteria.
		CTSPIRequest* pReq = NULL;
		for (POSITION posCurr = m_oblAsynchRequests.GetHeadPosition(); posCurr != NULL; )
		{
			CTSPIRequest* pReqCk = (CTSPIRequest*) m_oblAsynchRequests.GetNext(posCurr);
			if (pReqCk != preqIgnore && // Ignore specific -- used for DropCall
				IsMatchingRequest (pReqCk, pCall, wReqType, fOnlyUnstarted))
			{
				pReq = pReqCk;
				break;
			}
		}

		// If we found no requests in the list, then exit.
		if (pReq == NULL)
			break;

		// Otherwise, delete this request.
#ifdef _DEBUG
		DTRACE(TRC_MIN, _T("Removing pending request <0x%lx>\r\n"), (DWORD) pReq);
		pReq->Dump(afxDump);
#endif
            
		// See if this request has started.  We assume that
		// the state will be changed.
		if (pReq->GetState() != STATE_INITIAL)
		{
			// The request has at least started, tell the service 
			// provider that we are CANCELING the request.  It
			// can do anything it needs to do on this call with the request.
			OnCancelRequest (pReq);
		}
    
		// Remove the request.
		RemoveRequest(pReq);

		// Tell TAPI that the request is canceled.
		if (!pReq->HaveSentResponse())
			GetDeviceInfo()->OnAsynchRequestComplete(lErrorCode, pReq);

		// Unblock anyone waiting on this request to finish.
		pReq->Complete (lErrorCode, TRUE);

		// Delete the request
		delete pReq;
    }
      
}// CTSPIConnection::RemovePendingRequests

///////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::IsMatchingRequest
//
// Determine if the request matches a set of criteria.  This is called
// to delete the request.  DO NOT CALL THIS FOR ANY OTHER PURPOSE -
// SPECIAL CHECKS ARE DONE FOR EMBEDDED CALL APPEARANCES IN REQUEST
// PARAMETERS!
//
BOOL CTSPIConnection::IsMatchingRequest (CTSPIRequest* pReq, CTSPICallAppearance* pCall, WORD wRequest, BOOL fOnlyUnstarted)
{   
    // First do the simple matching.
    if ((pCall && pReq->GetCallInfo() == pCall) &&
        (wRequest == REQUEST_ALL || wRequest == pReq->GetCommand()) &&
		(fOnlyUnstarted == FALSE || pReq->GetState() == STATE_UNINITIALIZED))
        return TRUE;        
    
    // Now see if this call is the target of a command in a parameter
    // block.  The only two requests where this happens are transfer
    // and conference events.  In both these cases, the events need
    // to be removed or modified to reflect that the call is no longer
    // available.
    if (pCall != NULL)
    {
        if (wRequest == REQUEST_ALL || wRequest == pReq->GetCommand())
        {
            // If the SETUPXFER hasn't completed, then the consultant call
            // is not yet valid.  This call will be transitioned to IDLE
            // by the dropping of the REAL call, and the request would be deleted
            // from the above test since the REQUEST block is always inserted
            // on the REAL call appearance.  If this is the consultant call
            // that is wanting to delete the request, then we need to 
            // simply set the consultant call to NULL in the setupXfer block,
            // it will automatically be detached from the call appearance by
            // the CTSPICallAppearance::Drop method.            
            if (pReq->GetCommand() == REQUEST_SETUPXFER)
            {   
                TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
                if (pTrans->pConsult == pCall)
                    pTrans->pConsult = NULL;
            }
            
            // Otherwise, if this is a CompleteTransfer request, then
            // this call could be either the consultant call or the
            // created conference call.  The consultant call SHOULD be
            // valid and connected to the REAL call appearance.  The
            // conference is still not been attached - it is simply an
            // object at this point.
            //
            // If this is the consultant call requesting deletion, then
            // transition the conference call to idle (if available) and
            // return this block for deletion.
            //
            // Else if this is the conference call requesting deletion, 
            // then simply remove the conference pointer in the request
            // block and allow the call to proceed (as a straight transfer
            // to the consultant call).
            else if (pReq->GetCommand() == REQUEST_COMPLETEXFER)
            {
                TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
                if (pTrans->pConsult == pCall)
                {
                    if (pTrans->pConf != NULL)
                        pTrans->pConf->SetCallState(LINECALLSTATE_IDLE);
                    return TRUE;
                }
                else if (pTrans->pConf == pCall)
                    pTrans->pConf = NULL;
            }
            
            // If this is a SetupConference command, then the request is 
            // always inserted on the CONFERENCE call handle, so we need to
            // check for the consultation and REAL call appearances.  If
            // this is the real call asking for the deletion, then the conference
            // call is considered invalid (since it hasn't been completed).
            else if (pReq->GetCommand() == REQUEST_SETUPCONF)
            {
                TSPICONFERENCE* pConf = (TSPICONFERENCE*) pReq->GetDataPtr();
                // If it is the REAL call being dropped, then the
                // entire request becomes invalid.  Idle the conference
                // and consultant call and delete this request.
                if (pConf->pCall == pCall)
                {
                    pConf->pConfCall->SetCallState(LINECALLSTATE_IDLE);
                    pConf->pConsult->SetCallState(LINECALLSTATE_IDLE);
                    return TRUE;   
                }
                
                // If it is the consultant call, then simply remove it
                // from the call structure, but allow the conference to
                // still be setup - the request can be invalidated later 
                // by the service provider (when it processes this) if that
                // type of conference is not allowed.
                if (pConf->pConsult == pCall)
                    pConf->pConsult = NULL;
            }
            
            // Else if this is an ADD TO CONFERENCE request, then we 
            // only have a consultant call to add to the conference.  If
            // it gets dropped, then delete the request
            else if (pReq->GetCommand() == REQUEST_ADDCONF)
            {
                TSPICONFERENCE* pConf = (TSPICONFERENCE*) pReq->GetDataPtr();
                if (pConf->pConsult == pCall)
                    return TRUE;
            }        
            
            // Else if this is a REMOVE FROM CONFERENCE and the call
            // is dropped, we will see it as the callstate changes,
            // so remove this request.
            else if (pReq->GetCommand() == REQUEST_REMOVEFROMCONF)
            {
                TSPICONFERENCE* pConf = (TSPICONFERENCE*) pReq->GetDataPtr();
                if (pConf->pCall == pCall)
                    return TRUE;
            }
        }            
    }            
    
    // Request didn't match.
    return FALSE;
            
}// CTSPIConnection::IsMatchingRequest

///////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::FindRequest
//
// Locate a request packet based on a call appearance and command type.
//
CTSPIRequest* CTSPIConnection::FindRequest(CTSPICallAppearance* pCall, WORD wReqType)
{
    // Walk through all the requests and see if any match our connection, call, request criteria.
	CEnterCode sLock(this);  // Synch access to object
    for (POSITION posCurr = m_oblAsynchRequests.GetHeadPosition(); posCurr != NULL; )
    {
        CTSPIRequest* pReq = (CTSPIRequest*) m_oblAsynchRequests.GetNext(posCurr);

        // If this request doesn't match what we are searching for, then skip it.
        if ((pCall && pReq->GetCallInfo() != pCall) ||
            (wReqType != REQUEST_ALL && wReqType != pReq->GetCommand()))
            continue; 

		// Otherwise, reset the UNINITIALIZED state.
		if (pReq->GetState() == STATE_UNINITIALIZED)
			pReq->SetState(STATE_INITIAL);
        return pReq;            
    }
    return NULL;

}// CTSPIConnection::FindRequest

///////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::RemoveRequest
//
// Remove a specific request from our request list and possibly delete it.
//
BOOL CTSPIConnection::RemoveRequest(CTSPIRequest* pReq, BOOL fDelete/*=FALSE*/)
{
	CEnterCode sLock(this);  // Synch access to object
    POSITION pos = m_oblAsynchRequests.Find((CObject*)pReq);
    if (pos != NULL)
        m_oblAsynchRequests.RemoveAt(pos);                                    
	if (fDelete)
		delete pReq;                                         
    return (pos != NULL);

}// CTSPIConnection::RemoveRequest

///////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetCurrentRequest
//
// Return the head request in our asynch request list.  Override
// this function if you want to skip certain requests.
//
CTSPIRequest* CTSPIConnection::GetCurrentRequest() const
{
	CEnterCode sLock(this);  // Synch access to object
    CTSPIRequest* pReq = (m_oblAsynchRequests.IsEmpty()) ? NULL : (CTSPIRequest*) m_oblAsynchRequests.GetHead();
	if (pReq && pReq->GetState() == STATE_UNINITIALIZED)
		pReq->SetState(STATE_INITIAL);
	return pReq;

}// CTSPIConnection::GetCurrentRequest

///////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetRequest
//
// Get a specific request based on a position.
//
CTSPIRequest* CTSPIConnection::GetRequest(int iPos) const
{
	CEnterCode sLock(this);  // Synch access to object
    if (iPos < GetRequestCount())
    {
        for (POSITION pos = m_oblAsynchRequests.GetHeadPosition(); pos != NULL;)
        {
            CTSPIRequest* pReq = (CTSPIRequest*) m_oblAsynchRequests.GetNext(pos);
            if (iPos == 0)
			{
				if (pReq && pReq->GetState() == STATE_UNINITIALIZED)
					pReq->SetState(STATE_INITIAL);
                return pReq;
			}
            iPos--;
        }
    }
    return NULL;

}// CTSPIConnection::GetRequest

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::WaitForRequest
//
// Halts the current thread and waits for the current request entry
// to complete or timeout.  
//
// WARNING: After completion of this call, the request most likely
// will be deleted!
//
LONG CTSPIConnection::WaitForRequest(DWORD dwTimeout, CTSPIRequest* pReq)
{
    // Use the standard timeout if none is supplied.
    if (dwTimeout == 0)
        dwTimeout = (DWORD) GetSP()->GetTimeout();

    ASSERT (pReq != NULL);
    ASSERT (pReq->IsKindOf (RUNTIME_CLASS(CTSPIRequest)));

    return pReq->WaitForCompletion (dwTimeout);

}// CTSPIConnection::WaitForRequest

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::WaitForAllRequests
//
// Wait for pending requests to complete on the specifid line/call
// of the specifid type.
//
VOID CTSPIConnection::WaitForAllRequests(CTSPICallAppearance* pCall, WORD wRequest)
{           
    BOOL fDone = FALSE;
    while (!fDone)
    {
		CEnterCode Key (this);  // Synch access to object
        for (POSITION posCurr = m_oblAsynchRequests.GetHeadPosition(); posCurr != NULL; )
        {
            CTSPIRequest* pReq = (CTSPIRequest*) m_oblAsynchRequests.GetNext(posCurr);
            if ((pCall == NULL || pReq->GetCallInfo() == pCall) &&
                (wRequest == REQUEST_ALL || pReq->GetCommand() == wRequest))
            {
				Key.Unlock();
                pReq->WaitForCompletion();
                posCurr = (POSITION)1L;
                break;
            }
        }                
        
        if (posCurr == NULL)
            fDone = TRUE;          
    }

}// CTSPIConnection::WaitForAllRequests

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddDeviceClass
//
// Add a STRING data object to our device class list
//
int CTSPIConnection::AddDeviceClass (LPCTSTR pszClass, LPCTSTR lpszBuff, DWORD dwType)
{
	if (dwType == -1L)
	{
		if (IsLineDevice())
			dwType = ((CTSPIPhoneConnection*)this)->m_PhoneCaps.dwStringFormat;
		else
			dwType = ((CTSPILineConnection*)this)->m_LineCaps.dwStringFormat;
	}

	return AddDeviceClass (pszClass, dwType, (LPVOID)lpszBuff, (lstrlen(lpszBuff)+1) * sizeof(TCHAR));

}// CTSPIConnection::AddDeviceClass

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::OnNewRequest
//
// A new request is being added to our connection object.  The derived
// provider may override this function or CTSPIDevice::OnNewRequest or
// the CServiceProvider::OnNewRequest function to catch these and perform 
// some function BEFORE the request has officially been added.
//
// If FALSE is returned, the request will be canceled.
//
BOOL CTSPIConnection::OnNewRequest (CTSPIRequest* pReq, int* piPos)
{
	return GetDeviceInfo()->OnNewRequest (this, pReq, piPos);

}// CTSPIConnection::OnNewRequest

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::ReceiveData
//
// Data has been received by the device.  This method should be called
// by whatever is driving the input from the device.  In the 16-bit
// library this will always be the 'DeviceInput' function in SPDLL.
// In the 32-bit library, this would be the 'ReceiveData' function of
// the device object or some other input thread.
//
BOOL CTSPIConnection::ReceiveData (DWORD dwData, const LPVOID lpBuff, DWORD dwSize)
{
    // Tell the service provider about the input.
    return GetSP()->ProcessData (this, dwData, lpBuff, dwSize);

}// CTSPIConnection::ReceiveData

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::OpenDevice
//
// This method is called when lineOpen or phoneOpen is called.
//
BOOL CTSPIConnection::OpenDevice ()
{
    // Default behavior is to pass onto the device class. This allows for 
	// a single device object to control access for ALL connections (i.e.
	// a modem style device).
    return GetDeviceInfo()->OpenDevice (this);

}// CTSPIConnection::Open

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::CloseDevice
//
// This method is called when the last line/phone connection object is
// closed.  It will not be called if multiple lines/phones are open on this
// device.
//
BOOL CTSPIConnection::CloseDevice ()
{
    // Default behavior is to pass onto the device class. This allows for 
	// a single device object to control access for ALL connections (i.e.
	// a modem style device).
    return GetDeviceInfo()->CloseDevice (this);

}// CTSPIConnection::Close

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::SendData
//
// Send data out the device.  This is called whenever data needs to 
// go out to this specific device.
//
BOOL CTSPIConnection::SendData (LPCVOID lpBuff, DWORD dwSize)
{
    // Default behavior is to pass onto the device class. This allows for 
	// a single device object to control access for ALL connections (i.e.
	// a modem style device).
    return GetDeviceInfo()->SendData (this, lpBuff, dwSize);

}// CTSPIConnection::SendData

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::OnTimer
//
// This is invoked by our periodic timer called from the device manager
// object (m_pDevice).
//
VOID CTSPIConnection::OnTimer()
{                               
    // Default behavior is to pass onto the service provider class. This way,
    // the only class that REQUIRES to be overriden is the service provider to
    // remain compatibile with v1.20 of the library.  Another way to implement
    // this is to override this method, OpenDevice(), and CloseDevice() and 
    // SendData() and do all the device-specific stuff there.
    GetSP()->OnTimer(this);

}// CTSPIConnection::OnTimer

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::OnCancelRequest
//
// This is called when a request is cancled on this line/phone.
//
void CTSPIConnection::OnCancelRequest (CTSPIRequest* pReq)
{
	GetDeviceInfo()->OnCancelRequest(pReq);

}// CTSPIConnection::OnCancelRequest

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::Dump
//
// Diagnostic dump facility
//
void CTSPIConnection::Dump(CDumpContext& /*de*/) const
{
	DTRACE(TRC_STRUCT, _T("%s 0x%lx, ID:%d [%s]\r\n"), (IsLineDevice()) ? _T("Line") : _T("Phone"),
		GetDeviceID(), GetName());

}// CTSPIConnection::Dump
#endif // _DEBUG
