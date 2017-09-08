/******************************************************************************/
//                                                                        
// DEVICE.CPP - Source code for the TSPIDevice class                      
//                                             
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code for managing the device class which    
// encapsulates a TAPI SP device (managing lines and phones).             
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

IMPLEMENT_DYNCREATE( CTSPIDevice, CTSPIBaseObject )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// IntervalTimerThread
//
// Runs the interval timer for this device
//
UINT _IntervalTimerThread(LPVOID pParam)
{
    CTSPIDevice* pDevice = (CTSPIDevice*)pParam;
    pDevice->OnTimer();
	return 0;

}// IntervalTimerThread

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::Init
//
// This initializer allocates all the connection information for
// each phone/line available and sets up all the device information
// for this device id.
//
VOID CTSPIDevice::Init(
DWORD dwProviderId,              // Permanent provider Id
DWORD dwBaseLineId,              // Base line entry
DWORD dwBasePhoneId,             // Base phone entry
DWORD dwNumLines,                // Total # of lines
DWORD dwNumPhones,               // Total # of phones
HPROVIDER hProvider,             // Provider Handle
ASYNC_COMPLETION lpfnCompletion) // Asynch completion Id.
{
    // Validate the parameters
    ASSERT(dwProviderId < 65535);
    ASSERT(dwNumLines < 65535 && dwNumPhones < 65535);
    
    // Store all the information about this device.
    m_dwProviderId = dwProviderId;
    m_lpfnCompletionProc = lpfnCompletion;
	m_hProvider = hProvider;

    DTRACE(TRC_MIN, _T("Device %lx, Base Line=%lx, Count=%lx, Base Phone=%lx, Count=%lx\r\n"),
            dwProviderId, dwBaseLineId, dwNumLines, dwBasePhoneId, dwNumPhones);
    DTRACE(TRC_MIN, _T("Completion Callback function = %08lx\r\n"), (LONG)lpfnCompletion);

    // Now create a TSPIConnection structure for each available line
    // on this device.  Make sure to use the CRuntimeClass as appropriate
    // so the derived class will be created properly.
    for (int i = 0; i < (int)dwNumLines; i++)
    {
        CTSPILineConnection* pConn = (CTSPILineConnection*) GetSP()->GetTSPILineObj()->CreateObject();
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
        int iPos = AddLineConnectionInfo (pConn);
        DTRACE(TRC_MIN, _T("Adding Line #%d (id %lx) to device list\r\n"), i, dwBaseLineId+i);
        pConn->Init(this, dwBaseLineId+i, iPos, 0);
    }

    // Now create a TSPIConnection structure for each available phone
    // on this device.
    for (i = 0; i < (int)dwNumPhones; i++)
    {
        CTSPIPhoneConnection* pConn = (CTSPIPhoneConnection*) GetSP()->GetTSPIPhoneObj()->CreateObject();
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
        int iPos = AddPhoneConnectionInfo(pConn);
        DTRACE(TRC_MIN, _T("Adding Phone #%d (id %lx) to device list\r\n"), i, dwBasePhoneId+i);
        pConn->Init(this, dwBasePhoneId+i, iPos, 0);
    }

    // Verify that we did it ok.
    ASSERT (GetLineCount() == (int) dwNumLines);
    ASSERT (GetPhoneCount() == (int) dwNumPhones);

}// CTSPIDevice::Init

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::~CTSPIDevice
//
// This method destroys all the connection information structures
//
CTSPIDevice::~CTSPIDevice()
{
	// Shutdown the interval timer.
	SetIntervalTimer(0);

    // Destroy all the line connection objects
	CEnterCode cLock(this);  // Synch access to object
    for (int i = 0; i < m_arrayLines.GetSize(); i++)
    {
        CTSPILineConnection* pConn = (CTSPILineConnection*) m_arrayLines[i];
        delete pConn;
    }
    m_arrayLines.RemoveAll();

    // Destroy all the phone connection objects
    for (i = 0; i < m_arrayPhones.GetSize(); i++)
    {
        CTSPIPhoneConnection* pConn = (CTSPIPhoneConnection*) m_arrayPhones[i];
        delete pConn;
    }
    m_arrayPhones.RemoveAll();

}// CTSPIDevice::~CTSPIDevice

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::SetIntervalTimer
//
// Sets the current interval timer timeout.  If the timer is set
// to zero the thread associated with the timer is destroyed.
//
void CTSPIDevice::SetIntervalTimer (DWORD dwTimeout)
{
	// Is it the same as before?
	if (m_dwIntervalTimeout == dwTimeout)
		return;

	// Is it now zero?  Delete any existing interval timer.
	if (dwTimeout == 0)
	{
		if (m_pIntervalTimer != NULL)
		{
			m_evtDeviceShutdown.SetEvent();
			DTRACE(TRC_MIN, _T("Shutting down provider interval timer\r\n"));
			if (WaitForSingleObject (m_pIntervalTimer->m_hThread, 5000) == WAIT_TIMEOUT)
			{
				TerminateThread (m_pIntervalTimer->m_hThread, 0);
				delete m_pIntervalTimer;
			}
			m_pIntervalTimer = NULL;
		}
	}

	// Otherwise reset the timeout
	else
	{
		// Set the new timeout
		m_dwIntervalTimeout = dwTimeout;

		// Create our objects if necessary
		if (m_pIntervalTimer == NULL)
		{
			DTRACE(TRC_MIN, _T("Starting provider interval timer\r\n"));
			m_pIntervalTimer = AfxBeginThread((AFX_THREADPROC)_IntervalTimerThread, (LPVOID)this);
			ASSERT(m_pIntervalTimer != NULL);
		}
	}

}// CTSPIDevice::SetIntervalTimer

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::OnAsynchRequestComplete
//
// An asynchronous request has completed and TAPI needs to be 
// notified.
//
VOID CTSPIDevice::OnAsynchRequestComplete(LONG lResult, CTSPIRequest* pRequest)
{
    ASSERT(pRequest != NULL);

    // Get the asynchronous request id.  If it is non-zero, inform TAPI
    // about the request being completed.  If it is zero, this is an internally
    // generated request which has no cooresponding result to TAPI (we already
    // said it completed ok).
    DRV_REQUESTID dwReqId = pRequest->GetAsynchRequestId();
    if (dwReqId != (DRV_REQUESTID)0L)
    {
		if (m_lpfnCompletionProc != NULL)
			(*m_lpfnCompletionProc)(dwReqId, lResult);
#ifdef _DEBUG
		else
			ASSERT(m_lpfnCompletionProc != NULL); 
		DTRACE(TRC_MIN, _T("OnAsynchRequestComplete: <Obj:0x%lx> ReqID:0x%lx, Result:0x%lx\r\n"), 
				(DWORD)pRequest, dwReqId, lResult);
#endif
    }

}// CTSPIDevice::OnAsynchRequestComplete

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetLineConnectionInfo
//
// Return a line connection object based on a position
//
CTSPILineConnection* CTSPIDevice::GetLineConnectionInfo(int nIndex) const
{
	// Note: we don't bother to lock the object here since lines
	// are considered a static entity which only go up -- a line
	// object is NEVER removed from the array until provider shutdown,
	// even if the line itself is removed from TAPI.
    if (nIndex >= 0 && nIndex < GetLineCount())
        return (CTSPILineConnection*) m_arrayLines[nIndex];
    return NULL;

}// CTSPIDevice::GetLineConnectionInfo

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::FindLineConnectionByDeviceID
//
// Find a line connection based on a device id.
//
CTSPILineConnection* CTSPIDevice::FindLineConnectionByDeviceID(DWORD dwDeviceId) const
{
    int iCount = (int) GetLineCount();
    for (int i = 0; i < iCount; i++)
    {
        CTSPILineConnection* pLine = GetLineConnectionInfo (i);
        if (pLine && pLine->GetDeviceID() == dwDeviceId)
            return pLine;
    }
    return NULL;

}// CTSPIDevice::FindLineConnectionByDeviceID

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetPhoneConnectionInfo
//
// Return a line connection object based on a position.
//
CTSPIPhoneConnection* CTSPIDevice::GetPhoneConnectionInfo(int nIndex) const
{
	// Note: we don't bother to lock the object here since phones
	// are considered a static entity which only go up -- a phone
	// object is NEVER removed from the array until provider shutdown,
	// even if the phone itself is removed from TAPI.
    if (nIndex >= 0 && nIndex < GetPhoneCount())
        return (CTSPIPhoneConnection*) m_arrayPhones[nIndex];
    return NULL;

}// CTSPIDevice::GetPhoneConnectionInfo

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::FindPhoneConnectionByDeviceID
//
// Find a line connection based on a device id.
//
CTSPIPhoneConnection* CTSPIDevice::FindPhoneConnectionByDeviceID(DWORD dwDeviceId) const
{
    int iCount = (int) GetPhoneCount();
    for (int i = 0; i < iCount; i++)
    {
        CTSPIPhoneConnection* pPhone = GetPhoneConnectionInfo (i);
        if (pPhone && pPhone->GetDeviceID() == dwDeviceId)
            return pPhone;
    }
    return NULL;

}// CTSPIDevice::FindPhoneConnectionByDeviceID

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::CreateLine
//
// Create a new line device object and signal TAPI to give us a new
// line device id.
//
int CTSPIDevice::CreateLine(DWORD dwItemData /*=0*/)
{
    HINSTANCE hInst = AfxGetInstanceHandle();
    ASSERT (hInst != NULL);
    LINEEVENT lpfn = GetSP()->GetLineCreateProc();
    if (lpfn == NULL || GetProcAddress(hInst, "TSPI_providerCreateLineDevice") == NULL)
    {
        // If we are here, we probably don't have "TSPI_providerEnumDevices" exported, or the
        // version of TAPI isn't sufficient to handle it.
        DTRACE(TRC_MIN, _T("WARNING: Attempted to dynamically create line without TAPI support\r\n"));
        ASSERT (FALSE);
        return -1;
    }

    CTSPILineConnection* pConn = (CTSPILineConnection*) GetSP()->GetTSPILineObj()->CreateObject();
    ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    int iPos = AddLineConnectionInfo (pConn);
    pConn->Init(this, (DWORD)pConn, iPos, dwItemData);

    (*lpfn)(NULL, NULL, LINE_CREATE, (DWORD)GetProviderHandle(), (DWORD)pConn, 0L);

    DTRACE(TRC_MIN, _T("Dynamically created line object <0x%lx>\r\n"), (DWORD) pConn);

    return iPos;

}// CTSPIDevice::CreateLine

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::RemoveLine
//
// Remove a line device from our device array.
//
VOID CTSPIDevice::RemoveLine (CTSPILineConnection* pLine)
{
    LINEEVENT lpfn = GetSP()->GetLineCreateProc();
    if (lpfn == NULL)
    {
        // If we are here, we probably don't have "TSPI_providerEnumDevices" exported, or the
        // version of TAPI isn't sufficient to handle it.
        DTRACE(TRC_MIN, _T("WARNING: Attempted to dynamically remove line without TAPI support\r\n"));
        ASSERT (FALSE);
        return;
    }

	// The derived service provider will be responsible for
	// renumbering the line connection array when providerShutdown
	// is called.
	(*lpfn)(NULL, NULL, LINE_REMOVE, pLine->GetDeviceID(), 0L, 0L);

	// Mark the line as REMOVED.  Once it is closed it will be marked
	// as DELETED and all further references to it will be returned
	// in the TSPI_xxx layer.
	pLine->m_dwFlags |= CTSPIConnection::_IsRemoved;

}// CTSPIDevice::RemoveLine

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::CreatePhone
//
// Create a new phone device object and signal TAPI to give us a new
// phone device id.
//
int CTSPIDevice::CreatePhone(DWORD dwItemData /*=0*/)
{
    HINSTANCE hInst = AfxGetInstanceHandle();
    ASSERT (hInst != NULL);
    PHONEEVENT lpfn = GetSP()->GetPhoneCreateProc();
    if (lpfn == NULL || GetProcAddress(hInst, "TSPI_providerCreatePhoneDevice") == NULL)
    {
        // If we are here, we probably don't have "TSPI_providerEnumDevices" exported, or the
        // version of TAPI isn't sufficient to handle it.
        DTRACE(TRC_MIN, _T("WARNING: Attempted to dynamically create phone without TAPI support\r\n"));
        ASSERT (FALSE);
        return -1;
    }

    CTSPIPhoneConnection* pConn = (CTSPIPhoneConnection*) GetSP()->GetTSPIPhoneObj()->CreateObject();
    ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
    int iPos = AddPhoneConnectionInfo (pConn);
    pConn->Init(this, (DWORD)pConn, iPos, dwItemData);

    (*lpfn)(NULL, PHONE_CREATE, (DWORD)GetProviderHandle(), (DWORD)pConn, 0L);

    DTRACE(TRC_MIN, _T("Dynamically created phone object <0x%lx>\r\n"), (DWORD) pConn);

    return iPos;

}// CTSPIDevice::CreatePhone

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::RemovePhone
//
// Remove a phone device from our provider.
//
VOID CTSPIDevice::RemovePhone (CTSPIPhoneConnection* pPhone)
{
    PHONEEVENT lpfn = GetSP()->GetPhoneCreateProc();
    if (lpfn == NULL)
    {
        // If we are here, we probably don't have "TSPI_providerEnumDevices" exported, or the
        // version of TAPI isn't sufficient to handle it.
        DTRACE(TRC_MIN, _T("WARNING: Attempted to dynamically remove phone without TAPI support\r\n"));
        ASSERT (FALSE);
		return;
    }

	// The derived service provider will be responsible for
	// renumbering the phone connection array when providerShutdown
	// is called.
	(*lpfn)(NULL, PHONE_REMOVE, pPhone->GetDeviceID(), 0L, 0L);

	// Mark the phone as REMOVED.  Once it is closed it will be marked
	// as DELETED and all further references to it will be returned
	// in the TSPI_xxx layer.
	pPhone->m_dwFlags |= CTSPIConnection::_IsRemoved;

}// CTSPIDevice::RemovePhone

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::OnTimer
//
// This is the loop that the device interval timer thread uses to periodically
// show activity on the line/phone devices.  It is only activated if the 
// derived service provider uses the "SetIntervalTimer" API of this device
// object and sets the timeout to a non-zero value.
//
// It then wakes up every "m_dwIntervalTimeout" msecs and calls the OnTimer()
// methods of the line and phone devices.
//
void CTSPIDevice::OnTimer()
{
    for (;;)
    {
        // Wait for either our object to signal (meaning our provider
        // is being shutdown), or for our timeout value to elapse.
        LONG lResult = WaitForSingleObject (m_evtDeviceShutdown, m_dwIntervalTimeout);
        if (lResult == WAIT_OBJECT_0 || lResult == WAIT_ABANDONED)
            break;

		// Lock the device object while we access the line/phone array
		int iLineCount = GetLineCount(), iPhoneCount = GetPhoneCount();
		try
		{
			for (int i = 0; i < iLineCount; i++)
			{
				CTSPILineConnection* pLine = GetLineConnectionInfo(i);
				if (pLine && !pLine->HasBeenDeleted()) 
					pLine->OnTimer();
			}

			for (i = 0; i < iPhoneCount; i++)
			{
				CTSPIPhoneConnection* pPhone = GetPhoneConnectionInfo(i);
				if (pPhone && !pPhone->HasBeenDeleted()) 
					pPhone->OnTimer();
			}
		}
		catch (...)
		{
		}
    }

}// CTSPIDevice::OnTimer

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::ReceieveData
//
// This is the first stop in directing a block of received data to the
// proper connection.  If the connection id identifies a particular
// connection (i.e. the LOWORD of the CONN id is a line/phone index+1
// the shoot it directly to that connection.  Otherwise, cycle through
// all the connections and see if any take it.
//
VOID CTSPIDevice::ReceiveData (DWORD dwConnID, DWORD dwData, const LPVOID lpBuff, DWORD dwSize)
{                            
    WORD wIndex = (WORD) (dwConnID & 0xffff);
    if (wIndex != 0)
    {
        BOOL fIsPhone = ((wIndex & 0x8000) > 0);
        wIndex &= (0x7fff);
        wIndex--;
        
        if (fIsPhone)
        {
            ASSERT (wIndex < GetPhoneCount());
            CTSPIPhoneConnection* pPhone = GetPhoneConnectionInfo(wIndex);
            pPhone->ReceiveData (dwData, lpBuff, dwSize);
        }
        else
        {
            ASSERT (wIndex < GetLineCount());
            CTSPILineConnection* pLine = GetLineConnectionInfo(wIndex);
            pLine->ReceiveData (dwData, lpBuff, dwSize);
        }
    }
    else
    {   
        // These better match up if no line/phone is specified!
        ASSERT (dwConnID == GetPermanentDeviceID());           
        
        // Spin through all line and phone devices and hand them the data.
        // If the connection returns TRUE, then stop.
		CEnterCode sLock(this);
        for (int i = 0; i < m_arrayLines.GetSize(); i++)
        {
            CTSPILineConnection* pLine = (CTSPILineConnection*) m_arrayLines[i];
            if (pLine->ReceiveData (dwData, lpBuff, dwSize) && lpBuff != NULL)
                return;
        }
        for (i = 0; i < m_arrayPhones.GetSize(); i++)
        {
            CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) m_arrayPhones[i];
            if (pPhone->ReceiveData (dwData, lpBuff, dwSize) && lpBuff != NULL)
                return;
        }
    }        
        
}// CTSPIDevice::ReceieveData

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GenericDialogData
//
// Dialog Data for UI constructions in TAPI 2.0
//
LONG CTSPIDevice::GenericDialogData (LPVOID /*lpParam*/, DWORD /*dwSize*/)
{
	return FALSE;

}// CTSPIDevice::GenericDialogData

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::OpenDevice
//
// This method is called when lineOpen or phoneOpen is called.
//
BOOL CTSPIDevice::OpenDevice (CTSPIConnection* pConn)
{
    // Default behavior is to pass onto the service provider class. This way,
    // the only class that REQUIRES to be overriden is the service provider to
    // remain compatibile with v1.20 of the library.  Another way to implement
    // this is to override this method, Close(), and SendData() and
    // do all the device-specific stuff there.
    return GetSP()->OpenDevice (pConn);

}// CTSPIDevice::Open

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::CloseDevice
//
// This method is called when the last line/phone connection object is
// closed.  It will not be called if multiple lines/phones are open on this
// device.
//
BOOL CTSPIDevice::CloseDevice (CTSPIConnection* pConn)
{
    // Default behavior is to pass onto the service provider class. This way,
    // the only class that REQUIRES to be overriden is the service provider to
    // remain compatibile with v1.x of the library.  Another way to implement
    // this is to override this method, Close(), and SendData() and
    // do all the device-specific stuff there.
    return GetSP()->CloseDevice (pConn);

}// CTSPIDevice::CloseDevice

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::SendData
//
// Send data out the device.  This is called whenever data needs to 
// go out to this specific device.
//
BOOL CTSPIDevice::SendData (CTSPIConnection* pConn, LPCVOID lpBuff, DWORD dwSize)
{
    // Default behavior is to pass onto the service provider class. This way,
    // the only class that REQUIRES to be overriden is the service provider to
    // remain compatibile with v1.x of the library.  Another way to implement
    // this is to override this method, Open(), and Close() and
    // do all the device-specific stuff there.
    return GetSP()->SendData (pConn, lpBuff, dwSize);

}// CTSPIDevice::SendData

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::OnCancelRequest
//
// This is called by the connection object when a request is being 
// canceled due to the line/phone being closed.
//
VOID CTSPIDevice::OnCancelRequest (CTSPIRequest* pRequest)
{
    // Default behavior is to pass onto the service provider class. This way,
    // the only class that REQUIRES to be overriden is the service provider to
    // remain compatibile with v1.x of the library.
	GetSP()->OnCancelRequest(pRequest);	

}// CTSPIDevice::CancelRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::OnNewRequest
//
// A new request is being added to our connection object.  The derived
// provider may override this function or CTSPIConnection::OnNewRequest or
// the CServiceProvider::OnNewRequest function to catch these and perform 
// some function BEFORE the request has officially been added.
//
// If FALSE is returned, the request will be canceled.
//
BOOL CTSPIDevice::OnNewRequest (CTSPIConnection* pConn, CTSPIRequest* pReq, int* piPos)
{
	return GetSP()->OnNewRequest (pConn, pReq, piPos);

}// CTSPIDevice::OnNewRequest

