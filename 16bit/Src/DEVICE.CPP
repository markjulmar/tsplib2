/******************************************************************************/
//                                                                        
// DEVICE.CPP - Source code for the TSPIDevice class                      
//                                             
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code for managing the device class which    
// encapsulates a TAPI SP device (managing lines and phones).             
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

IMPLEMENT_DYNCREATE( CTSPIDevice, CObject )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::CTSPIDevice
//
// Constructor
//
CTSPIDevice::CTSPIDevice()
{
    m_dwProviderId = (DWORD) -1;
    m_lpfnCompletionProc = NULL;

}// CTSPIDevice::CTSPIDevice

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
ASYNC_COMPLETION lpfnCompletion) // Asynch completion Id.
{
    // Validate the parameters
    ASSERT(dwProviderId < 65535);
    ASSERT(dwNumLines < 65535 && dwNumPhones < 65535);
    
    // Store all the information about this device.
    m_dwProviderId = dwProviderId;
    m_lpfnCompletionProc = lpfnCompletion;

    TRACE("Device %lx, Base Line=%lx, Count=%lx, Base Phone=%lx, Count=%lx\r\n",
            dwProviderId, dwBaseLineId, dwNumLines, dwBasePhoneId, dwNumPhones);
    TRACE("Completion Callback function = %08lx\r\n", (LONG)lpfnCompletion);

    // Now create a TSPIConnection structure for each available line
    // on this device.  Make sure to use the CRuntimeClass as appropriate
    // so the derived class will be created properly.
    for (int i = 0; i < (int)dwNumLines; i++)
    {
        CTSPILineConnection* pConn = (CTSPILineConnection*) GetSP()->GetTSPILineObj()->CreateObject();
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
        AddLineConnectionInfo (pConn);
        pConn->Init(this, (DWORD)i + dwBaseLineId, (DWORD)i+1);
        TRACE("Adding Line #%d (id %lx) to device list\r\n", i, pConn->GetDeviceID());
    }

    // Now create a TSPIConnection structure for each available phone
    // on this device.
    for (i = 0; i < (int)dwNumPhones; i++)
    {
        CTSPIPhoneConnection* pConn = (CTSPIPhoneConnection*) GetSP()->GetTSPIPhoneObj()->CreateObject();
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
        AddPhoneConnectionInfo(pConn);
        pConn->Init(this, (DWORD)i + dwBasePhoneId, (DWORD)i+1);
        TRACE("Adding Phone #%d (id %lx) to device list\r\n", i, pConn->GetDeviceID());
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
    // Destroy all the line connection objects
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
// CTSPIDevice::GetPermanentDeviceID
//
// Return a permanent device id which identifies this device but
// no line or phone.
//
DWORD CTSPIDevice::GetPermanentDeviceID() const
{
    return ((m_dwProviderId << 16) & 0xffff0000);

}// CTSPILineConnection::GetPermanentDeviceID

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
        ASSERT(m_lpfnCompletionProc != NULL); 
        (*m_lpfnCompletionProc)(dwReqId, lResult);
    }

}// CTSPIDevice::OnAsynchRequestComplete

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetLineCount
//
// Return the count of lines available on this device.
//
int CTSPIDevice::GetLineCount() const
{
    return m_arrayLines.GetSize();

}// CTSPIDevice::GetLineCount

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetPhoneCount
//
// Return the count of phones available on this device.
//
int CTSPIDevice::GetPhoneCount() const
{
    return m_arrayPhones.GetSize();

}// CTSPIDevice::GetPhoneCount

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetLineConnectionInfo
//
// Return a line connection object based on a position
//
CTSPILineConnection* CTSPIDevice::GetLineConnectionInfo(int nIndex) const
{
    if (nIndex < GetLineCount())
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
    int iCount = GetLineCount();
    for (int i = 0; i < iCount; i++)
    {
        CTSPILineConnection* pLine = GetLineConnectionInfo (i);
        if (pLine && pLine->GetDeviceID() == dwDeviceId)
            return pLine;
    }
    return NULL;

}// CTSPIDevice::FindLineConnectionByDeviceID

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::AddLineConnectionInfo
//
// Add a new line connection object to our device list.
//
WORD CTSPIDevice::AddLineConnectionInfo(CTSPILineConnection* pConn)
{
    return (WORD) m_arrayLines.Add((CObject*) pConn);

}// CTSPIDevice::AddLineConnectionInfo

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetPhoneConnectionInfo
//
// Return a line connection object based on a position.
//
CTSPIPhoneConnection* CTSPIDevice::GetPhoneConnectionInfo(int nIndex) const
{
    if (nIndex < GetPhoneCount())
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
    int iCount = GetPhoneCount();
    for (int i = 0; i < iCount; i++)
    {
        CTSPIPhoneConnection* pPhone = GetPhoneConnectionInfo (i);
        if (pPhone && pPhone->GetDeviceID() == dwDeviceId)
            return pPhone;
    }
    return NULL;

}// CTSPIDevice::FindPhoneConnectionByDeviceID

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::AddPhoneConnectionInfo
//
// Add a new line connection object to our device list.
//
WORD CTSPIDevice::AddPhoneConnectionInfo(CTSPIPhoneConnection* pConn)
{
    return (WORD) m_arrayPhones.Add((CObject*) pConn);

}// CTSPIDevice::AddPhoneConnectionInfo

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetProviderId
//
// The provider id is assigned by the TAPI.DLL when a service provider
// is installed through TSPI_providerInstall.  It allows the TSP to 
// create a unique section in the "telephon.ini" file using "PROVIDERx"
// as its section ('x' is the passed provider id).  It generally starts
// with zero (the first one) and gets incremented as each new TSP is
// installed.
//
DWORD CTSPIDevice::GetProviderID() const
{
    return m_dwProviderId;

}// CTSPIDevice::GetProviderId

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::CreateLine
//
// Create a new line device object and signal TAPI to give us a new
// line device id.
//
int CTSPIDevice::CreateLine()
{
    LINEEVENT lpfn = GetSP()->GetLineCreateProc();
    if (lpfn == NULL)
    {
        // If we are here, we probably don't have "TSPI_providerEnumDevices" exported, or the
        // version of TAPI isn't sufficient to handle it.
        TRACE("WARNING: Attempted to dynamically create line without TAPI support\r\n");
        ASSERT (FALSE);
        return -1;
    }

    CTSPILineConnection* pConn = (CTSPILineConnection*) GetSP()->GetTSPILineObj()->CreateObject();
    ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    pConn->Init(this, (DWORD)pConn, GetLineCount()+1);
    int iPos = AddLineConnectionInfo (pConn);

    (*lpfn)(NULL, NULL, LINE_CREATE, (DWORD)pConn, 0L, 0L);

    TRACE ("Dynamically created line object <0x%lx>\r\n", (DWORD) pConn);

    return iPos;

}// CTSPIDevice::CreateLine

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::CreatePhone
//
// Create a new phone device object and signal TAPI to give us a new
// phone device id.
//
int CTSPIDevice::CreatePhone()
{
    PHONEEVENT lpfn = GetSP()->GetPhoneCreateProc();
    if (lpfn == NULL)
    {
        // If we are here, we probably don't have "TSPI_providerEnumDevices" exported, or the
        // version of TAPI isn't sufficient to handle it.
        TRACE("WARNING: Attempted to dynamically create phone without TAPI support\r\n");
        ASSERT (FALSE);
        return -1;
    }

    CTSPIPhoneConnection* pConn = (CTSPIPhoneConnection*) GetSP()->GetTSPIPhoneObj()->CreateObject();
    ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
    pConn->Init(this, (DWORD)pConn, GetPhoneCount()+1);
    int iPos = AddPhoneConnectionInfo (pConn);

    (*lpfn)(NULL, PHONE_CREATE, (DWORD)pConn, 0L, 0L);

    TRACE ("Dynamically created phone object <0x%lx>\r\n", (DWORD) pConn);

    return iPos;

}// CTSPIDevice::CreatePhone

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::OnTimer
//
// This function is called during "DistributeIntervalTimer" in order
// to allow the device to do some cleanup or process data which needs to
// be polled.
//
VOID CTSPIDevice::OnTimer()
{
    // Pass it to all our lines to give each line a chance to
    // do some work.  This is used in digit gathering at the call 
    // level.
    for (int i = 0; i < m_arrayLines.GetSize(); i++)
    {
        CTSPILineConnection* pLine = (CTSPILineConnection*) m_arrayLines[i];
        pLine->OnTimer();
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
            ASSERT (wIndex < (WORD) GetPhoneCount());
            CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) m_arrayPhones[wIndex];
            pPhone->ReceiveData (dwData, lpBuff, dwSize);
        }
        else
        {
            ASSERT (wIndex < (WORD) GetLineCount());
            CTSPILineConnection* pLine = (CTSPILineConnection*) m_arrayLines[wIndex];
            pLine->ReceiveData (dwData, lpBuff, dwSize);
        }
    }
    else
    {   
        // These better match up if no line/phone is specified!
        ASSERT (dwConnID == GetPermanentDeviceID());           
        
        // Spin through all line and phone devices and hand them the data.
        // If the connection returns TRUE, then stop.
        BOOL fProcessed = FALSE;
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
    // remain compatibile with v1.20 of the library.  Another way to implement
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
    // remain compatibile with v1.20 of the library.  Another way to implement
    // this is to override this method, Open(), and Close() and
    // do all the device-specific stuff there.
    return GetSP()->SendData (pConn, lpBuff, dwSize);

}// CTSPIConnection::SendData

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

}// CTSPIDevice::OnCancelRequest

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


