/******************************************************************************/
//                                                                        
// LINECONN.CPP - Source code for the CTSPILineConnection object          
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the source to manage the line objects which are 
// held by the CTSPIDevice.                                               
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

IMPLEMENT_DYNCREATE( CTSPILineConnection, CTSPIConnection )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::CTSPILineConnection
//
// Constructor
//
CTSPILineConnection::CTSPILineConnection() : 
	m_dwLineMediaModes(0), m_dwLineStates(0), m_lpfnEventProc(NULL),
	m_htLine(0), m_dwConnectedCallCount(0)
{ 
    FillBuffer (&m_LineCaps, 0, sizeof(LINEDEVCAPS));
    FillBuffer (&m_LineStatus, 0, sizeof(LINEDEVSTATUS));

}// CTSPILineConnection::CTSPILineConnection

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::Init
//
// Initialize this line connection object
//
VOID CTSPILineConnection::Init(CTSPIDevice* pDevice, DWORD dwLineId, DWORD dwPos)
{
    CTSPIConnection::Init(pDevice, dwLineId);
    
    // Initialize the line device capabilities.  The permanent line id
    // is a combination of our device id plus the position of the line itself
    // within our device array.  This uniquely identifies the line to us
    // with a single DWORD.  The MSB of the loword is always zero - this
    // identifies it as a LINE device.
    m_LineCaps.dwPermanentLineID = (pDevice->GetProviderID() << 16) + (dwPos&0x00007fff);
    
    m_LineCaps.dwStringFormat = STRINGFORMAT_ASCII;
    m_LineCaps.dwAddressModes = LINEADDRESSMODE_ADDRESSID;
    m_LineCaps.dwBearerModes = LINEBEARERMODE_VOICE;
    m_LineCaps.dwLineStates = (LINEDEVSTATE_OTHER | LINEDEVSTATE_RINGING | LINEDEVSTATE_CONNECTED |
			LINEDEVSTATE_DISCONNECTED | LINEDEVSTATE_MSGWAITON | LINEDEVSTATE_MSGWAITOFF |
            LINEDEVSTATE_INSERVICE | LINEDEVSTATE_OUTOFSERVICE | LINEDEVSTATE_MAINTENANCE |
            LINEDEVSTATE_TERMINALS | LINEDEVSTATE_NUMCALLS | LINEDEVSTATE_NUMCOMPLETIONS | 
			LINEDEVSTATE_ROAMMODE | LINEDEVSTATE_BATTERY | LINEDEVSTATE_SIGNAL | 
			LINEDEVSTATE_LOCK);
                        
    // Always set the number of ACTIVE calls (i.e. connected) to one - derived
    // classes may override this if they can support more than one active call
    // at a time.  Reset the value after this function is invoked.
    m_LineCaps.dwMaxNumActiveCalls = 1;
                        
    // If we are running a v1.4 service provider, include some new messages.                               
    if (GetSP()->GetSupportedVersion() >= TAPIVER_14 &&
        GetSP()->GetSystemVersion() >= TAPIVER_14)
        m_LineCaps.dwLineStates |= (LINEDEVSTATE_COMPLCANCEL | LINEDEVSTATE_CAPSCHANGE | LINEDEVSTATE_CONFIGCHANGE);

    // This will be adjusted by each address added as the "SetAvailableMediaModes" API is called.
    m_LineCaps.dwMediaModes = 0;
    
    // This should be kept in synch with any PHONECAPS structures.
    m_LineCaps.dwRingModes = 1;

    // Now fill in the line device status
    m_LineStatus.dwNumActiveCalls = 0L;       // This will be modified as callstates change
    m_LineStatus.dwNumOnHoldCalls = 0L;       // This will be modified as callstates change
    m_LineStatus.dwNumOnHoldPendCalls = 0L;   // This will be modified as callstates change
    m_LineStatus.dwNumCallCompletions = 0L;   // This is filled out by the call appearance
    m_LineStatus.dwRingMode = 0;
    m_LineStatus.dwBatteryLevel = 0xffff;
    m_LineStatus.dwSignalLevel = 0xffff;
    m_LineStatus.dwRoamMode = LINEROAMMODE_UNAVAIL;
    m_LineStatus.dwDevStatusFlags = LINEDEVSTATUSFLAGS_CONNECTED | LINEDEVSTATUSFLAGS_INSERVICE;

    // Set the device capability flags and v1.4 line features
    if (CanHandleRequest(REQUEST_MAKECALL))
        m_LineCaps.dwLineFeatures |= LINEFEATURE_MAKECALL;
    
    if (CanHandleRequest(REQUEST_MEDIACONTROL))
    {
        m_LineCaps.dwDevCapFlags |= LINEDEVCAPFLAGS_MEDIACONTROL;
        m_LineCaps.dwLineFeatures |= LINEFEATURE_SETMEDIACONTROL;
        m_LineStatus.dwLineFeatures |= LINEFEATURE_SETMEDIACONTROL;
    }        

    if (CanHandleRequest(REQUEST_FORWARD))
    {
        m_LineCaps.dwLineFeatures |= LINEFEATURE_FORWARD;
        m_LineStatus.dwLineFeatures |= LINEFEATURE_FORWARD;
    }
    
    // Add terminal support - the LINEDEVSTATUS field will be updated when
    // the terminal is actually ADDED.
    if (CanHandleRequest(REQUEST_SETTERMINAL))
        m_LineCaps.dwLineFeatures |= LINEFEATURE_SETTERMINAL;

    // Derived class should fill in the remainder of the line capabilities.  We automatically
    // will fill in: (through various Addxxx functions).
    //
    // dwNumTerminals, MinDialParams, MaxDialParams, dwMediaModes, dwBearerModes, dwNumAddresses
    // dwLineNameSize, dwLineNameOffset, dwProviderInfoOffset, dwProviderInfoSize, dwSwitchInfoSize,
    // dwSwitchInfoOffset, dwMaxRate.
    //
    // All the other values should be filled in or left zero if not supported.  To fill them in
    // simply use "GetLineDevCaps()" and fill it in or derive a object from this and override Init.

}// CTSPILineConnection::Init

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::~CTSPILineConnection
//
// Destructor for the line connection object.  Remove all the address
// information structures allocated for this line.
//
CTSPILineConnection::~CTSPILineConnection()
{
    // Remove all the addresses
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        delete pAddr;   
    }
    m_arrAddresses.RemoveAll();

    // Remove all the terminal information
    for (i = 0; i < m_arrTerminalInfo.GetSize(); i++)
    {
        TERMINALINFO* lpInfo = (TERMINALINFO*) m_arrTerminalInfo[i];
        delete lpInfo;
    }
    m_arrTerminalInfo.RemoveAll();
    
    // Remove all the call completion information
    for (POSITION pos = m_lstCompletions.GetHeadPosition(); pos != NULL;)
    {
        TSPICOMPLETECALL* pCall = (TSPICOMPLETECALL*) m_lstCompletions.GetNext(pos);
        delete pCall;
    }
    m_lstCompletions.RemoveAll();

}// CTSPILineConnection::~CTSPILineConnection

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::CanHandleRequest
//
// Determine if our service provider is capable of handling the request.
// 
BOOL CTSPILineConnection::CanHandleRequest(WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest(this, wRequest, dwData);                

}// CTSPILineConnection::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetPermanentDeviceID
//
// Return a permanent device id for this line identifying the provider
// and line.
//
DWORD CTSPILineConnection::GetPermanentDeviceID() const
{
    return m_LineCaps.dwPermanentLineID;

}// CTSPILineConnection::GetPermanentDeviceID

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnCallStateChange
//
// A call has changed state on an address on this line.  Update our
// status.
//
VOID CTSPILineConnection::OnCallStateChange (CTSPIAddressInfo* /*pAddr*/, CTSPICallAppearance* /*pCall*/, 
                                             DWORD dwNewState, DWORD dwOldState)
{   
	// If the state has not changed, ignore.
	if (dwNewState == dwOldState)
		return;

    // Determine if the number of active calls has changed.
    BOOL fWasActive = CTSPICallAppearance::IsActiveCallState(dwOldState);
    BOOL fIsActive  = CTSPICallAppearance::IsActiveCallState(dwNewState);
    BOOL fSendNumCalls = FALSE;

    if (fWasActive == FALSE && fIsActive == TRUE)
    {       
        m_LineStatus.dwNumActiveCalls++;
        fSendNumCalls = TRUE;
    }
    else if (fWasActive == TRUE && fIsActive == FALSE)
    {
        m_LineStatus.dwNumActiveCalls--;
        if (m_LineStatus.dwNumActiveCalls & 0x80000000)
            m_LineStatus.dwNumActiveCalls = 0L;
        fSendNumCalls = TRUE;
    }       

    // Determine if the HOLD status has changed.        
    if (dwNewState == LINECALLSTATE_ONHOLD)
    {
        m_LineStatus.dwNumOnHoldCalls++;
        fSendNumCalls = TRUE;
    }
    else if (dwNewState == LINECALLSTATE_ONHOLDPENDTRANSFER || dwNewState == LINECALLSTATE_ONHOLDPENDCONF)
    {
        m_LineStatus.dwNumOnHoldPendCalls++;
        fSendNumCalls = TRUE;
    }

    if (dwOldState == LINECALLSTATE_ONHOLD)
    {
        m_LineStatus.dwNumOnHoldCalls--;
        if (m_LineStatus.dwNumOnHoldCalls & 0x80000000)
            m_LineStatus.dwNumOnHoldCalls = 0L;
        fSendNumCalls = TRUE;
    }

    else if (dwOldState == LINECALLSTATE_ONHOLDPENDTRANSFER || dwOldState == LINECALLSTATE_ONHOLDPENDCONF)
    {
        m_LineStatus.dwNumOnHoldPendCalls--;
        if (m_LineStatus.dwNumOnHoldPendCalls & 0x80000000)
            m_LineStatus.dwNumOnHoldPendCalls = 0L;
        fSendNumCalls = TRUE;
    }

    TRACE("Line 0x%lx Active=%ld, OnHold=%ld,  OnHoldPend=%ld\r\n", GetPermanentDeviceID(),
		m_LineStatus.dwNumActiveCalls, m_LineStatus.dwNumOnHoldCalls, m_LineStatus.dwNumOnHoldPendCalls);            

    if (fSendNumCalls)
        OnLineStatusChange (LINEDEVSTATE_NUMCALLS);

}// CTSPILineConnection::OnCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnLineStatusChange
//
// This method is called when any of the values in our line device
// status record are changed.  It is called internally by the library
// and should also be called by the derived class if the LINDEVSTATUS
// structure is modified directly.
//
VOID CTSPILineConnection::OnLineStatusChange (DWORD dwState, DWORD dwP2, DWORD dwP3)
{
    if ((m_dwLineStates & dwState) || dwState == LINEDEVSTATE_REINIT)
        Send_TAPI_Event (NULL, LINE_LINEDEVSTATE, dwState, dwP2, dwP3);

}// CTSPILineConnection::OnLineStatusChange

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::Open
//
// Open the line device
//
LONG CTSPILineConnection::Open (
HTAPILINE htLine,                // TAPI opaque handle to use for line
LINEEVENT lpfnEventProc,         // Event procedure address for notifications
DWORD dwTSPIVersion)             // Version of TSPI to synchronize to
{
    // If we are already open, return allocated.
    if (GetLineHandle())
        return LINEERR_ALLOCATED;

    // Save off the event procedure for this line and the TAPI
    // opaque line handle which represents this line to the application.
    m_lpfnEventProc = lpfnEventProc;
    m_htLine = htLine;
    m_dwNegotiatedVersion = dwTSPIVersion;

    TRACE("Opening line %lx, TAPI handle=%lx, SP handle=%lx\r\n", GetDeviceID(), GetLineHandle(), (DWORD)this);

    // Tell our device to perform an open for this connection.
    if (!OpenDevice())
	{
		m_lpfnEventProc = NULL;
		m_htLine = (HTAPILINE)0;
		m_dwNegotiatedVersion = 0;
		return LINEERR_RESOURCEUNAVAIL;
	}
    return FALSE;

}// CTSPILineConnection::Open

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::Close
//
// Close the line connection and destroy any call appearances that
// are active.
//
// The htLine handle is invalid after this completes.
//
LONG CTSPILineConnection::Close()
{                       
    if (GetLineHandle())
    {
        TRACE("Closing line %lx, TAPI handle=%lx, SP handle=%lx\r\n", GetDeviceID(), GetLineHandle(), (DWORD)this);
        
        // Make sure any pending call close operations complete.
        WaitForAllRequests (NULL, REQUEST_DROPCALLONCLOSE);
        WaitForAllRequests (NULL, REQUEST_DROPCALL);
        WaitForAllRequests (NULL, REQUEST_DROPNOOWNER);
        
        // Remove any other pending requests for this connection.
        RemovePendingRequests();

        // Decrement our total line open count.
        OnLineStatusChange (LINEDEVSTATE_CLOSE);
        
        // Reset our event and line proc.
        m_lpfnEventProc = NULL;
        m_htLine = (HTAPILINE)0;
        m_dwLineStates = 0L;
		m_dwNegotiatedVersion = GetSP()->GetSupportedVersion();
		m_dwConnectedCallCount = 0;
        
        // Tell our device to close.
        CloseDevice();
        return FALSE;
    }   
    
    return LINEERR_OPERATIONFAILED;

}// CTSPILineConnection::Close

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::Send_TAPI_Event
//
// Calls an event back into the TAPI DLL.  It is assumed that the
// validity of the event has been verified before calling this 
// function.
//
VOID CTSPILineConnection::Send_TAPI_Event(
CTSPICallAppearance* pCall, // Call appearance to send event for
DWORD dwMsg,                // Message to send (LINExxx)
DWORD dwP1,                 // Parameter 1 (depends on above message)
DWORD dwP2,                 // Parameter 2 (depends on above message)
DWORD dwP3)                 // Parameter 3 (depends on above message)
{                       
	// If we don't have a handle for this line, ignore the event.
	if (GetLineHandle() == 0)
		return;

    // We cannot send events for lines which haven't been initialized.
    // This happens if the line hasn't been opened yet.
    if (m_lpfnEventProc != NULL)
    {
        HTAPILINE htLine = GetLineHandle();
        HTAPICALL htCall = (HTAPICALL) 0;

        // If a call appearance was supplied, get the call handle
        // from it.
        if (pCall)
            htCall = pCall->GetCallHandle();
        
        // Strip out any notifications which are not in TAPI 1.3
        BOOL fSend = TRUE;
        if (m_dwNegotiatedVersion < TAPIVER_14)
        {
            if (dwMsg == LINE_LINEDEVSTATE)
            {
                if (dwP1 == LINEDEVSTATE_CAPSCHANGE || dwP1 == LINEDEVSTATE_CONFIGCHANGE ||
                    dwP1 == LINEDEVSTATE_COMPLCANCEL)
                { 
                    fSend = FALSE;
                }
            }
            else if (dwMsg == PHONE_STATE)
            {
                 if (dwP1 == PHONESTATE_CAPSCHANGE)
                    fSend = FALSE;
            } 
            else if (dwMsg == LINE_ADDRESSSTATE)
            {
                if (dwP1 == LINEADDRESSSTATE_CAPSCHANGE)
                    fSend = FALSE;
            }
        }

#ifdef _DEBUG
        static const char * g_pszMsgs[] = {
                {"Line_AddressState"},               // 0
                {"Line_CallInfo"},                   // 1
                {"Line_CallState"},                  // 2
                {"Line_Close"},                      // 3
                {"Line_DevSpecific"},                // 4
                {"Line_DevSpecificFeature"},         // 5
                {"Line_GatherDigits"},               // 6
                {"Line_Generate"},                   // 7
                {"Line_LineDevState"},               // 8
                {"Line_MonitorDigits"},              // 9
                {"Line_MonitorMedia"},               // 10
                {"Line_MonitorTone"},                // 11
                {"Line_Reply"},                      // 12
                {"Line_Request"},                    // 13
                {"Phone_Button"},                    // 14
                {"Phone_Close"},                     // 15
                {"Phone_DevSpecific"},               // 16
                {"Phone_Reply"},                     // 17
                {"Phone_State"},                     // 18
                {"Line_Create"},                     // 19
                {"Phone_Create"}                     // 20
            };                
    
        // Send the notification to TAPI.
        TRACE ("Send_TAPI_Event: <0x%lx> Line=0x%lx, Call=0x%lx, Msg=0x%lx (%s), P1=0x%lx, P2=0x%lx, P3=0x%lx\r\n",
                    (DWORD)this, (DWORD)htLine, (DWORD)htCall, 
                    dwMsg, (dwMsg <= 20) ? (LPCSTR)g_pszMsgs[dwMsg] : "", dwP1, dwP2, dwP3);
#endif

        if (fSend)                    
            (*m_lpfnEventProc)(htLine, htCall, dwMsg, dwP1, dwP2, dwP3);
#ifdef _DEBUG           
        else
            TRACE("  **MSG NOT SENT - TAPI VERSION < 1.4**\r\n");
#endif          
    }

}// CTSPILineConnection::Send_TAPI_Event

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::CreateAddress
//
// Create a new address on this line.
//
DWORD CTSPILineConnection::CreateAddress (LPCSTR lpszDialableAddr, LPCSTR lpszAddrName, 
                                          BOOL fInput, BOOL fOutput, DWORD dwAvailMediaModes,
                                          DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate,
                                          LPLINEDIALPARAMS lpDial, DWORD dwMaxNumActiveCalls,
                                          DWORD dwMaxNumOnHoldCalls, DWORD dwMaxNumOnHoldPendCalls,
                                          DWORD dwMaxNumConference, DWORD dwMaxNumTransConf)
                                          
{
    CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) GetSP()->GetTSPIAddressObj()->CreateObject();
    ASSERT (pAddr->IsKindOf(RUNTIME_CLASS(CTSPIAddressInfo)));

    // Add it to our array.
    int iPos = m_arrAddresses.Add (pAddr);
    
    // Update our line status features if they are now different.
    if (fOutput && CanHandleRequest(REQUEST_MAKECALL))
        m_LineStatus.dwLineFeatures |= LINEFEATURE_MAKECALL;

    // Update our line device capabilities with mediamode, bearermode info.
    m_LineCaps.dwBearerModes |= dwBearerMode;
    m_LineCaps.dwMediaModes |= dwAvailMediaModes;
    
    // Update the MAXRATE information.  This field is used in two fashions:
    // If the bearermode includes DATA, then this field indicates the top bit rate
    // of the digital channel.
	//
    // Otherwise, if it doesn't include data, but has VOICE, and the mediamode includes
    // DATAMODEM, then this should be set to the highest synchronous DCE bit rate
    // supported (excluding compression).
    if (dwMaxRate > m_LineCaps.dwMaxRate)
        m_LineCaps.dwMaxRate = dwMaxRate;
        
    // If we got a dial parameters list, modify our min/max dial parameters.
    if (lpDial)
    {
        if (m_LineCaps.MinDialParams.dwDialPause > lpDial->dwDialPause)
            m_LineCaps.MinDialParams.dwDialPause = lpDial->dwDialPause;
        if (m_LineCaps.MinDialParams.dwDialSpeed > lpDial->dwDialSpeed)
            m_LineCaps.MinDialParams.dwDialSpeed = lpDial->dwDialSpeed;
        if (m_LineCaps.MinDialParams.dwDigitDuration > lpDial->dwDigitDuration)
            m_LineCaps.MinDialParams.dwDigitDuration = lpDial->dwDigitDuration;
        if (m_LineCaps.MinDialParams.dwWaitForDialtone > lpDial->dwWaitForDialtone)
            m_LineCaps.MinDialParams.dwWaitForDialtone = lpDial->dwWaitForDialtone;
        if (m_LineCaps.MaxDialParams.dwDialPause < lpDial->dwDialPause)
            m_LineCaps.MaxDialParams.dwDialPause = lpDial->dwDialPause;
        if (m_LineCaps.MaxDialParams.dwDialSpeed < lpDial->dwDialSpeed)
            m_LineCaps.MaxDialParams.dwDialSpeed = lpDial->dwDialSpeed;
        if (m_LineCaps.MaxDialParams.dwDigitDuration < lpDial->dwDigitDuration)
            m_LineCaps.MaxDialParams.dwDigitDuration = lpDial->dwDigitDuration;
        if (m_LineCaps.MaxDialParams.dwWaitForDialtone < lpDial->dwWaitForDialtone)
            m_LineCaps.MaxDialParams.dwWaitForDialtone = lpDial->dwWaitForDialtone;
    }

    // Init the address
    pAddr->Init (this, (DWORD) iPos, lpszDialableAddr, lpszAddrName, fInput, fOutput,
                 dwAvailMediaModes, dwBearerMode, dwMinRate, dwMaxRate,
                 dwMaxNumActiveCalls, dwMaxNumOnHoldCalls, dwMaxNumOnHoldPendCalls,
                 dwMaxNumConference, dwMaxNumTransConf);

	// If the name of the address was NULL, then create a NEW name for the address.
	if (lpszAddrName == NULL)
	{
		CString strName;
		strName.Format("Address%ld", m_LineCaps.dwNumAddresses);
		pAddr->SetName(strName);
	}

    // Increment the number of addresses.
    m_LineCaps.dwNumAddresses++;

    // And return the position
    return (DWORD) iPos;

}// CTSPILineConnection::CreateAddress

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddress
//
// Return an address based on an address ID.
//
CTSPIAddressInfo* CTSPILineConnection::GetAddress (int iAddressID) const
{
    return GetAddress((DWORD)iAddressID);

}// CTSPILineConnection::GetAddress

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddress
//
// Return an address based on an address ID.
//
CTSPIAddressInfo* CTSPILineConnection::GetAddress (DWORD dwAddressID) const
{
    if (dwAddressID < (DWORD) m_arrAddresses.GetSize())
        return (CTSPIAddressInfo*) m_arrAddresses[(int)dwAddressID];
    return NULL;

}// CTSPILineConnection::GetAddress
                        
///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddress
//
// Locate an address based on a dialable address.
//
CTSPIAddressInfo* CTSPILineConnection::GetAddress (LPCSTR lpszAddress) const
{                                  
    CTSPIAddressInfo* pAddr = NULL;
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pTestAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        if (!stricmp (pTestAddr->GetDialableAddress(), lpszAddress))
        {
            pAddr = pTestAddr;
            break;            
        }
    }
    return pAddr;

}// CTSPILineConnection::GetAddress
                        
///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddressCount
//
// Return the number of addresses available on this line.
//
DWORD CTSPILineConnection::GetAddressCount() const
{
    return (DWORD) m_arrAddresses.GetSize();

}// CTSPILineConnection::GetAddressCount

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GatherCapabilities
//
// Gather the line device capabilities for this list.
//
LONG CTSPILineConnection::GatherCapabilities (DWORD dwTSPIVersion, DWORD /*dwExtVer*/, LPLINEDEVCAPS lpLineCaps)
{   
    CString strLineName = GetName();
    CString strProviderInfo = GetSP()->GetProviderInfo();
    CString strSwitchInfo = GetConnInfo();
    int cbName=0, cbInfo=0, cbSwitchInfo=0;
    
    if (!strLineName.IsEmpty())
        cbName = strLineName.GetLength()+1;
    if (!strProviderInfo.IsEmpty())
        cbInfo = strProviderInfo.GetLength()+1;
    if (!strSwitchInfo.IsEmpty())
        cbSwitchInfo = strSwitchInfo.GetLength()+1;
    
    // Check the available size to make sure we have enough room.   
    DWORD dwTotalSize = lpLineCaps->dwTotalSize;
    DWORD dwReqSize   = sizeof(LINEDEVCAPS)-sizeof(DWORD);
    if (dwTSPIVersion >= TAPIVER_14)
        dwReqSize = sizeof(LINEDEVCAPS);

#ifdef _DEBUG
    // TAPI is supposed to verify the structure size for us.  Check it in DEBUG
    // builds just to be certain, but count on it in the retail version.
    if (dwReqSize > dwTotalSize)
    {
        lpLineCaps->dwNeededSize = sizeof(LINEDEVCAPS) + cbName + cbInfo;
        return LINEERR_STRUCTURETOOSMALL;
    }
#endif

    CopyBuffer (lpLineCaps, &m_LineCaps, dwReqSize);
    lpLineCaps->dwTotalSize = dwTotalSize;
    lpLineCaps->dwUsedSize   = dwReqSize;
    lpLineCaps->dwNeededSize = sizeof(LINEDEVCAPS) + cbName + cbInfo + cbSwitchInfo;
    
    // Remove the additional capabilities if we are not at the proper
    // TSPI version.  These caps are in our static structure.
    if (dwTSPIVersion < TAPIVER_14)    
    {
        lpLineCaps->dwLineStates &= ~(LINEDEVSTATE_COMPLCANCEL | LINEDEVSTATE_CAPSCHANGE | LINEDEVSTATE_CONFIGCHANGE);
        lpLineCaps->dwMediaModes &= ~(LINEMEDIAMODE_VOICEVIEW);
    }        
    
    // If we have enough room for the provider information, then add
    // it to the end.
    if (lpLineCaps->dwTotalSize >= lpLineCaps->dwUsedSize + cbInfo)
    {
        CopyBuffer((LPSTR)lpLineCaps + lpLineCaps->dwUsedSize, strProviderInfo, cbInfo);
        lpLineCaps->dwProviderInfoSize = cbInfo;
        lpLineCaps->dwProviderInfoOffset = lpLineCaps->dwUsedSize;
        lpLineCaps->dwUsedSize += cbInfo;
    }

    // If we have enough room for the line name, then add it.
    if (lpLineCaps->dwTotalSize >= lpLineCaps->dwUsedSize + cbName)
    {
        CopyBuffer((LPSTR)lpLineCaps + lpLineCaps->dwUsedSize, strLineName, cbName);
        lpLineCaps->dwLineNameSize = cbName;
        lpLineCaps->dwLineNameOffset = lpLineCaps->dwUsedSize;
        lpLineCaps->dwUsedSize += cbName;
    }
    
    // If we have enough room for the switch information, then add it.
    if (lpLineCaps->dwTotalSize >= lpLineCaps->dwUsedSize + cbSwitchInfo)
    {
        CopyBuffer((LPSTR)lpLineCaps + lpLineCaps->dwUsedSize, strSwitchInfo, cbSwitchInfo);
        lpLineCaps->dwSwitchInfoSize = cbSwitchInfo;
        lpLineCaps->dwSwitchInfoOffset = lpLineCaps->dwUsedSize;
        lpLineCaps->dwUsedSize += cbSwitchInfo;
    }
    
    // Handle the line terminal capabilities    
    lpLineCaps->dwTerminalCapsSize = (m_LineCaps.dwNumTerminals * sizeof(LINETERMCAPS));
    lpLineCaps->dwNeededSize += lpLineCaps->dwTerminalCapsSize;

    int cbCount = 0;
    if (dwTotalSize >= lpLineCaps->dwNeededSize)
    {
        LPLINETERMCAPS lpCaps = (LPLINETERMCAPS)((LPSTR)lpLineCaps+lpLineCaps->dwUsedSize);
        lpLineCaps->dwTerminalCapsOffset = lpLineCaps->dwUsedSize;
        lpLineCaps->dwUsedSize += lpLineCaps->dwTerminalCapsSize;
        
        for (int i = 0; i < m_arrTerminalInfo.GetSize(); i++)
        {
            TERMINALINFO* lpTermInfo = (TERMINALINFO*) m_arrTerminalInfo[i];
            cbCount += lpTermInfo->strName.GetLength()+1;
            CopyBuffer (lpCaps, &lpTermInfo->Capabilities, sizeof(LINETERMCAPS));
            lpCaps++;
        }
    }

    // Add the terminal name information if we have the space.
    lpLineCaps->dwNeededSize += cbCount;
    lpLineCaps->dwTerminalTextSize = cbCount;
    if (dwTotalSize >= lpLineCaps->dwNeededSize)
    {
        LPSTR lpBuff = (LPSTR)lpLineCaps+lpLineCaps->dwUsedSize;
        lpLineCaps->dwTerminalTextOffset = lpLineCaps->dwUsedSize;
        lpLineCaps->dwUsedSize += cbCount;

        for (int i = 0; i < m_arrTerminalInfo.GetSize(); i++)
        {
            TERMINALINFO* lpTermInfo = (TERMINALINFO*) m_arrTerminalInfo[i];
            CopyBuffer (lpBuff, (LPCSTR)lpTermInfo->strName, lpTermInfo->strName.GetLength());
            lpBuff += lpTermInfo->strName.GetLength();
            *lpBuff++ = '\0';
        }
    }

    return FALSE;

}// CTSPILineConnection::GatherCapabilities

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GatherStatus
//
// Return the status of this line connection.
//
LONG CTSPILineConnection::GatherStatus (LPLINEDEVSTATUS lpLineDevStatus)
{
    DWORD dwTotalSize = lpLineDevStatus->dwTotalSize;
    lpLineDevStatus->dwNeededSize = sizeof(LINEDEVSTATUS);
    
#ifdef _DEBUG
    // TAPI is supposed to verify the structure size for us.  Check it in DEBUG
    // builds just to be certain, but count on it in the retail version.
    if (dwTotalSize < lpLineDevStatus->dwNeededSize)
        return LINEERR_STRUCTURETOOSMALL;
#endif
    
    // Copy our structure onto the user structure
    CopyBuffer (lpLineDevStatus, &m_LineStatus, sizeof(LINEDEVSTATUS));
    lpLineDevStatus->dwUsedSize = sizeof(LINEDEVSTATUS);

    // Now fill in the additional fields.
    lpLineDevStatus->dwTotalSize = dwTotalSize;
    lpLineDevStatus->dwNumCallCompletions = m_lstCompletions.GetCount();
	m_LineStatus.dwNumOpens = lpLineDevStatus->dwNumOpens;
	m_LineStatus.dwOpenMediaModes = lpLineDevStatus->dwOpenMediaModes;
                        
    // Fill in the terminal information if we have space.
    if (m_arrTerminalInfo.GetSize() > 0)
    {
        DWORD dwReqSize = m_arrTerminalInfo.GetSize() * sizeof(DWORD);
        lpLineDevStatus->dwNeededSize += dwReqSize;

        if (lpLineDevStatus->dwTotalSize >= (dwReqSize + lpLineDevStatus->dwUsedSize))
        {
            lpLineDevStatus->dwTerminalModesOffset = lpLineDevStatus->dwUsedSize;
            lpLineDevStatus->dwUsedSize += dwReqSize;
            lpLineDevStatus->dwTerminalModesSize = dwReqSize;
            LPDWORD lpDW = (LPDWORD)((LPSTR)lpLineDevStatus+lpLineDevStatus->dwTerminalModesOffset);
            for (int i = 0; i < m_arrTerminalInfo.GetSize(); i++)
            {
                TERMINALINFO* lpTermInfo = (TERMINALINFO*) m_arrTerminalInfo[i];
                *lpDW++ = lpTermInfo->dwMode;
            }                
        }
    }
    else
    {
        lpLineDevStatus->dwTerminalModesSize =
        lpLineDevStatus->dwTerminalModesOffset = 0L;
    }
    return FALSE;

}// CTSPILineConnection::GatherStatus

/////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::FindAvailableAddress
//
// Locate an address which is available for an outgoing call.
//
CTSPIAddressInfo* CTSPILineConnection::FindAvailableAddress (LPLINECALLPARAMS const lpCallParams,
															 DWORD dwFeature) const
{
    // Walk through all our addresses and look to see if they can support
    // the type of call desired.
    for (int x = 0; x < (int) GetAddressCount(); x++)
    {                                                                                           
        CTSPIAddressInfo* pAddr = GetAddress((DWORD)x);
        
		// If the call appearance can have another active call.. 
		LINEADDRESSSTATUS* pAS = pAddr->GetAddressStatus();
		LINEADDRESSCAPS*   pAC = pAddr->GetAddressCaps();
        if (pAS->dwNumActiveCalls < pAC->dwMaxNumActiveCalls)
        {
			// If the feature is available
			if (dwFeature > 0 && (pAS->dwAddressFeatures & dwFeature) != dwFeature)
				continue;

            // And can support the type of call required..
            if (lpCallParams && pAddr->CanSupportCall(lpCallParams) == FALSE)
                continue;

			// This address fits the bill.
            return pAddr;
        }
    }
    return NULL;

}// CTSPILineConnection::FindAvailableAddress        

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::CanSupportCall
//
// Return TRUE/FALSE whether this line can support the type of
// call specified.
//
LONG CTSPILineConnection::CanSupportCall (LPLINECALLPARAMS const lpCallParams) const
{                                    
    if ((lpCallParams->dwBearerMode & m_LineCaps.dwBearerModes) != lpCallParams->dwBearerMode)
        return LINEERR_INVALBEARERMODE;
        
    if (m_LineCaps.dwMaxRate > 0 && lpCallParams->dwMaxRate > m_LineCaps.dwMaxRate)
        return LINEERR_INVALRATE;
        
    if ((lpCallParams->dwMediaMode & m_LineCaps.dwMediaModes) != lpCallParams->dwMediaMode)
        return LINEERR_INVALMEDIAMODE;

    // If a specific address is identified, then run it through that address to
    // insure that the other fields are ok, otherwise, check them all.
    if (lpCallParams->dwAddressMode == LINEADDRESSMODE_ADDRESSID)
    {
        CTSPIAddressInfo* pAddr = GetAddress(lpCallParams->dwAddressID);
        if (pAddr != NULL)
            return pAddr->CanSupportCall (lpCallParams);
        return LINEERR_INVALADDRESSID;
    }
    else
    {   
        // Attempt to pass it to an address with the specified dialable address.
        if (lpCallParams->dwAddressMode == LINEADDRESSMODE_DIALABLEADDR &&
            lpCallParams->dwOrigAddressSize > 0)
        {                              
            LPSTR lpBuff = (LPSTR)lpCallParams + lpCallParams->dwOrigAddressOffset;
            CString strAddress (lpBuff, (int)lpCallParams->dwOrigAddressSize);
            CTSPIAddressInfo* pAddr = GetAddress(strAddress);
            if (pAddr != NULL)
                return pAddr->CanSupportCall (lpCallParams);
        }
        
        // Search through ALL our addresses and see if any can support this call.
        for (int x = 0; x < (int) GetAddressCount(); x++)
        {
            CTSPIAddressInfo* pAddr = GetAddress((DWORD)x);
            if (pAddr && pAddr->CanSupportCall(lpCallParams) == FALSE)
                return FALSE;
        }
    }
    
    return LINEERR_INVALCALLPARAMS;
            
}// CTSPILineConnection::CanSupportCall

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::MakeCall
//
// Make a new call on our line connection.  Allocate a call appearance
// from an available address on the line.
//
LONG CTSPILineConnection::MakeCall (DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall,
                                    TSPIMAKECALL* lpMakeCall)
{                         
	// Check to see if we can perform this function now.
	if ((GetLineDevStatus()->dwLineFeatures & LINEFEATURE_MAKECALL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // If the total number of active calls has already been reached, then we
    // cannot make a call until one of the existing calls goes away or moves to
    // an inactive (i.e. holding) state.
	if (m_LineCaps.dwMaxNumActiveCalls <= m_dwConnectedCallCount)
        return LINEERR_RESOURCEUNAVAIL;

	// Or, if we still have the bandwidth for one more call, but have a pending MAKECALL
	// request in our queue..
	else if (m_LineCaps.dwMaxNumActiveCalls-1 == m_dwConnectedCallCount && 
			 FindRequest(NULL, REQUEST_MAKECALL) != NULL)
		return LINEERR_RESOURCEUNAVAIL;

    // Create a call appearance on a known address.
    CTSPICallAppearance* pCall = NULL;

    // If the user passes a specific call appearance in the 
    // call parameters, use it.
    if (lpMakeCall->lpCallParams)
    {
        // If they specified a specific address ID, then find the address on this
        // line and create the call appearance on the address.
        if (lpMakeCall->lpCallParams->dwAddressMode == LINEADDRESSMODE_ADDRESSID)
        {
            CTSPIAddressInfo* pAddr = GetAddress(lpMakeCall->lpCallParams->dwAddressID);
            if (pAddr == NULL)
            {
                TRACE("lineMakeCall: invalid address id <%ld>\r\n", lpMakeCall->lpCallParams->dwAddressID);
                return LINEERR_INVALADDRESSID;
            }
                        
			// If the address cannot current make a call then fail.
			if ((pAddr->GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_MAKECALL) == 0)
				return LINEERR_OPERATIONUNAVAIL;
                        
            // If the address has exceeded the active call appearance count,
            // then return an error.
            if (pAddr->GetAddressCaps()->dwMaxNumActiveCalls <= 
                pAddr->GetAddressStatus()->dwNumActiveCalls)
                return LINEERR_CALLUNAVAIL;
            
            // Create the call appearance on this address.
            pCall = pAddr->CreateCallAppearance(htCall);
        }

        // Otherwise, if they specified a dialable address, then walk through all
        // our addresses and find the matching address.
        else if (lpMakeCall->lpCallParams->dwAddressMode == LINEADDRESSMODE_DIALABLEADDR)
        {                       
            if (lpMakeCall->lpCallParams->dwOrigAddressSize > 0 && 
                lpMakeCall->lpCallParams->dwOrigAddressOffset > 0)
            {
                LPCSTR lpszAddress = (LPCSTR)lpMakeCall->lpCallParams + lpMakeCall->lpCallParams->dwOrigAddressSize;
                for (int x = 0; x < (int) GetAddressCount(); x++)
                {
                    CTSPIAddressInfo* pAddr = GetAddress((DWORD)x);
                    if (stricmp(pAddr->GetDialableAddress(), lpszAddress) == 0)
                    {                 
						// If the address cannot current make a call then fail.
						if ((pAddr->GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_MAKECALL) == 0)
							return LINEERR_OPERATIONUNAVAIL;
                    
                        // If the address has exceeded the active call appearance count,
                        // then return an error.
            			if (pAddr->GetAddressCaps()->dwMaxNumActiveCalls <= pAddr->GetAddressStatus()->dwNumActiveCalls)
							return LINEERR_CALLUNAVAIL;
							
						// Otherwise create the call.							
                        pCall = pAddr->CreateCallAppearance(htCall);
                        break;
                    }
                }
            }
        }
      
        // If we didn't find any addresses matching our criteria, 
        // return an error.
        if (pCall == NULL)
        {
            TRACE("lineMakeCall: address explicitly specified does not exist\r\n");
            return (lpMakeCall->lpCallParams->dwAddressMode == LINEADDRESSMODE_DIALABLEADDR) ?
                    LINEERR_INVALADDRESS : LINEERR_INVALADDRESSID;                
        }
    }

    // If they did not specify which call appearance to use by address, then locate one
    // which matches the specifications they desire from our service provider.
    CTSPIAddressInfo* pAddr = NULL;
    if (pCall == NULL)
    {
        pAddr = FindAvailableAddress (lpMakeCall->lpCallParams);
        if (pAddr != NULL)
            pCall = pAddr->CreateCallAppearance(htCall);
    }
   
    // If there are no more call appearances, exit.
    if (pCall == NULL)
    {
        TRACE("lineMakeCall: no address available for outgoing call!\r\n");
        return LINEERR_CALLUNAVAIL;
    }

    // Return the call appearance handle.
    *lphdCall = (HDRVCALL) pCall;

    // Otherwise, tell the call appearance to make a call.
    LONG lResult = pCall->MakeCall (dwRequestID, lpMakeCall);
    if ((DRV_REQUESTID)lResult != dwRequestID)
    {
    	if (pAddr != NULL)
    		pAddr->RemoveCallAppearance(pCall);
    	*lphdCall = NULL;
    }    
    return lResult;

}// CTSPILineConnection::MakeCall

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetLineHandle
//
// Return the line handle for this line device.
//
HTAPILINE CTSPILineConnection::GetLineHandle() const
{
    return m_htLine;

}// CTSPILineConnection::GetLineHandle

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetLineDevCaps
//
// Return the line device capabilities.  Keep in mind that the capabilities
// are not complete here - the optional fields are not filled out until
// an actual call to GetCapabilities is made, and they are not stored in
// our structure (only the callers).
//
LPLINEDEVCAPS CTSPILineConnection::GetLineDevCaps()
{
    return &m_LineCaps;

}// CTSPILineConnection::GetLineDevCaps

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetLineDevStatus
//
// Return the line device status.  Keep in mind that the status information
// are not complete here - the optional fields are not filled out until
// an actual call to GatherStatus is made, and they are not stored in
// our structure (only the callers).
//
LPLINEDEVSTATUS CTSPILineConnection::GetLineDevStatus()
{
    return &m_LineStatus;

}// CTSPILineConnection::GetLineDevStatus;

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddressID
//
// This method returns the address ID associated with this line
// in the specified format.
//
LONG CTSPILineConnection::GetAddressID(
LPDWORD lpdwAddressId,                 // DWORD for return address ID
DWORD dwAddressMode,                   // Address mode in lpszAddress
LPCSTR lpszAddress,                    // Address of the specified line
DWORD dwSize)                          // Size of the above string/buffer
{
    // We don't support anything but the dialable address
    if (dwAddressMode != LINEADDRESSMODE_DIALABLEADDR)
        return LINEERR_INVALADDRESSMODE;

    // Make sure the size field is filled out ok.
    if (dwSize == 0)
        dwSize = lstrlen (lpszAddress);

    CString strAddress (lpszAddress, (int)dwSize);

    // Walk through all the addresses on this line and see if the
    // address passed matches up.
    CTSPIAddressInfo* pFinal = NULL;
    for (int i = 0; i < (int) GetAddressCount(); i++)
    {
        CTSPIAddressInfo* pAddr = GetAddress((DWORD)i);
        if (pAddr && !strAddress.CompareNoCase(pAddr->GetDialableAddress()))
        {
            pFinal = pAddr;
            break;
        }
    }

    // Never found it? return error.
    if (pFinal == (CTSPIAddressInfo*) NULL)
        return LINEERR_INVALADDRESS;

    // Otherwise set the returned address id to the call appearance
    // address id.
    *lpdwAddressId = pFinal->GetAddressID();
    
    return FALSE;

}// CTSPILineConnection::GetAddressID

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetTerminal
//
// Redirect the flow of specifiec terminal events to a destination
// terminal for all calls on this line.
//
LONG CTSPILineConnection::SetTerminal (DRV_REQUESTID dwRequestID, 
									   TSPILINESETTERMINAL* lpLine)
{                       
	if ((GetLineDevStatus()->dwLineFeatures & LINEFEATURE_SETTERMINAL) == 0)

    // Submit the request.
    if (AddAsynchRequest(NULL, REQUEST_SETTERMINAL, dwRequestID, lpLine) != NULL)
        return dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPILineConnection::SetTerminal

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetTerminalCount
//
// Return the count of terminals on this line.
//
int CTSPILineConnection::GetTerminalCount() const
{
    return m_arrTerminalInfo.GetSize();
    
}// CTSPILineConnection::GetTerminalCount

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetTerminalInformation
//
// Return the current terminal modes for the specified terminal
// identifier.
//
DWORD CTSPILineConnection::GetTerminalInformation (int iTerminalID) const
{                                              
    if (iTerminalID >= 0 && iTerminalID < GetTerminalCount())
    {
        TERMINALINFO* lpTermInfo = (TERMINALINFO*) m_arrTerminalInfo[iTerminalID];
        if (lpTermInfo)
            return lpTermInfo->dwMode;
    }       
    return 0L;
    
}// CTSPILineConnection::GetTerminalInformation

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::AddTerminal
//
// Add a new terminal to the line connection.
//
int CTSPILineConnection::AddTerminal (LPCSTR lpszName, LINETERMCAPS& Caps, DWORD dwModes)
{
    TERMINALINFO* lpTermInfo = new TERMINALINFO;
    lpTermInfo->strName = lpszName;
    CopyBuffer (&lpTermInfo->Capabilities, &Caps, sizeof(LINETERMCAPS));
    lpTermInfo->dwMode = dwModes;

    // Add it to our array.
    int iPos = m_arrTerminalInfo.Add (lpTermInfo);

    // Tell all our address about the new terminal count
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        pAddr->OnTerminalCountChanged(TRUE, iPos, dwModes);
    }

    // Set the new terminal count
    m_LineCaps.dwNumTerminals = (DWORD) m_arrTerminalInfo.GetSize();
    SetLineFeatures (OnLineFeaturesChanged(m_LineStatus.dwLineFeatures | LINEFEATURE_SETTERMINAL));

    // Tell TAPI our terminal information has changed.
    OnLineStatusChange (LINEDEVSTATE_TERMINALS);
    return iPos;

}// CTSPILineConnection::AddTerminal

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::RemoveTerminal
//
// Remove a terminal from our terminal list.  This will cause our
// terminal numbers to be re-ordered!
//
VOID CTSPILineConnection::RemoveTerminal (int iTerminalId)
{
    if (iTerminalId < m_arrTerminalInfo.GetSize())
    {
        TERMINALINFO* lpInfo = (TERMINALINFO*) m_arrTerminalInfo[iTerminalId];
        ASSERT (lpInfo != NULL);

        // Remove it and delete the object
        m_arrTerminalInfo.RemoveAt (iTerminalId);
        delete lpInfo;

        // Tell all our address about the new terminal count
        for (int i = 0; i < m_arrAddresses.GetSize(); i++)
        {
            CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
            pAddr->OnTerminalCountChanged(FALSE, iTerminalId, 0L);
        }

        // Set the new terminal count
        m_LineCaps.dwNumTerminals = (DWORD) m_arrTerminalInfo.GetSize();

        // Tell TAPI our terminal information has changed.
        OnLineStatusChange (LINEDEVSTATE_TERMINALS);
    }

}// CTSPILineConnection::RemoveTerminal

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetTerminalModes
//
// This is the function which should be called when a lineSetTerminal is
// completed by the derived service provider class.
// This stores or removes the specified terminal from the terminal modes 
// given, and then forces it to happen for any existing calls on the 
// line.
//
VOID CTSPILineConnection::SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal)
{
    if (iTerminalID < m_arrTerminalInfo.GetSize())
    {
        TERMINALINFO* lpInfo = (TERMINALINFO*) m_arrTerminalInfo[iTerminalID];
        ASSERT (lpInfo != NULL);

        // Either add the bits or mask them off based on what we are told
        // by TAPI.
        if (fRouteToTerminal)
            lpInfo->dwMode |= dwTerminalModes;
        else
            lpInfo->dwMode &= ~dwTerminalModes;
        
        // Notify TAPI about our device state changes
        OnLineStatusChange (LINEDEVSTATE_TERMINALS);
        
        // Force all the addresses to update the terminal list
        for (int i = 0; i < m_arrAddresses.GetSize(); i++)
        {
            CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
            pAddr->SetTerminalModes (iTerminalID, dwTerminalModes, fRouteToTerminal);
        }            
    }

}// CTSPILineConnection::SetTerminalModes

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetRingMode
//
// Set the ring mode for this line.
//
VOID CTSPILineConnection::SetRingMode (DWORD dwRingMode)
{
	if (m_LineStatus.dwRingMode != dwRingMode)
	{
		m_LineStatus.dwRingMode = dwRingMode;
		OnLineStatusChange (LINEDEVSTATE_OTHER);
	}

}// CTSPILineConnection::SetRingMode

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetBatteryLevel
//
// Set the battery level for this line connection
//
VOID CTSPILineConnection::SetBatteryLevel (DWORD dwBattery)
{
    if (dwBattery > 0xffff)
        dwBattery = 0xffff;

	if (m_LineStatus.dwBatteryLevel != dwBattery)
	{
		m_LineStatus.dwBatteryLevel = dwBattery;
		OnLineStatusChange (LINEDEVSTATE_BATTERY);
	}

}// CTSPILineConnection::SetBatteryLevel

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetSignalLevel
//
// Set the signal level for the line connection
//
VOID CTSPILineConnection::SetSignalLevel (DWORD dwSignal)
{
    if (dwSignal > 0xffff)
        dwSignal = 0xffff;

	if (m_LineStatus.dwSignalLevel != dwSignal)
	{
		m_LineStatus.dwSignalLevel = dwSignal;
		OnLineStatusChange (LINEDEVSTATE_SIGNAL);
	}

}// CTSPILineConnection::SetSignalLevel

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetRoamMode
//
// Set the roaming mode for the line
//
VOID CTSPILineConnection::SetRoamMode (DWORD dwRoamMode)
{
	if (m_LineStatus.dwRoamMode != dwRoamMode)
	{
		m_LineStatus.dwRoamMode = dwRoamMode;
		OnLineStatusChange (LINEDEVSTATE_ROAMMODE);
	}

}// CTSPILineConnection::SetRoamMode

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetLineFeatures
//
// Used by the user to sets the current line features
//
VOID CTSPILineConnection::SetLineFeatures (DWORD dwFeatures)
{   
	// Make sure the capabilities structure reflects this ability.
	if (m_LineCaps.dwLineFeatures & dwFeatures == 0)
	{
		TRACE("LINEDEVCAPS.dwLineFeatures missing 0x%lx bit\r\n", dwFeatures);
		m_LineCaps.dwLineFeatures |= dwFeatures;	
		OnLineCapabiltiesChanged();
	}

	// Update it only if it has changed.
	if (m_LineStatus.dwLineFeatures != dwFeatures)
	{
		m_LineStatus.dwLineFeatures = dwFeatures;
		OnLineStatusChange (LINEDEVSTATE_OTHER);
	}

}// CTSPILineConnection::SetLineFeatures

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnLineCapabilitiesChanged
//
// This function is called when the line capabilties change in the lifetime
// of the provider.
//
VOID CTSPILineConnection::OnLineCapabiltiesChanged()
{
	OnLineStatusChange (LINEDEVSTATE_CAPSCHANGE);

}// CTSPILineConnection::OnLineCapabiltiesChanged

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnLineFeaturesChanged
//
// Hook to allow a derived line to adjust the line features.
//
DWORD CTSPILineConnection::OnLineFeaturesChanged(DWORD dwLineFeatures)
{
	return dwLineFeatures;

}// CTSPILineConnection::OnLineFeaturesChanged

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetDeviceStatusFlags
//
// Sets the device status flags in the LINEDEVSTATUS structure
//
VOID CTSPILineConnection::SetDeviceStatusFlags (DWORD dwStatus)
{   
    DWORD dwOldStatus = m_LineStatus.dwDevStatusFlags;
    m_LineStatus.dwDevStatusFlags = dwStatus;         

    // Send TAPI the appropriate notifications.
    if ((dwOldStatus & LINEDEVSTATUSFLAGS_CONNECTED) &&
        (dwStatus & LINEDEVSTATUSFLAGS_CONNECTED) == 0)
        OnLineStatusChange (LINEDEVSTATE_DISCONNECTED);
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_CONNECTED) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_CONNECTED))
        OnLineStatusChange (LINEDEVSTATE_CONNECTED);
                                         
    if ((dwOldStatus & LINEDEVSTATUSFLAGS_MSGWAIT) &&
        (dwStatus & LINEDEVSTATUSFLAGS_MSGWAIT) == 0)
        OnLineStatusChange (LINEDEVSTATE_MSGWAITOFF);
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_MSGWAIT) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_MSGWAIT))
        OnLineStatusChange (LINEDEVSTATE_MSGWAITON);

    if ((dwOldStatus & LINEDEVSTATUSFLAGS_INSERVICE) &&
        (dwStatus & LINEDEVSTATUSFLAGS_INSERVICE) == 0)
        OnLineStatusChange (LINEDEVSTATE_OUTOFSERVICE);
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_INSERVICE) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_INSERVICE))
        OnLineStatusChange (LINEDEVSTATE_OUTOFSERVICE);

    if ((dwOldStatus & LINEDEVSTATUSFLAGS_LOCKED) &&
        (dwStatus & LINEDEVSTATUSFLAGS_LOCKED) == 0)
        OnLineStatusChange (LINEDEVSTATE_LOCK);
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_LOCKED) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_LOCKED))
        OnLineStatusChange (LINEDEVSTATE_LOCK);

}// CTSPILineConnection::SetDeviceStatusFlags

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::Forward
//
// Forward a specific or all addresses on this line to a destination
// address.
//
LONG CTSPILineConnection::Forward (
DRV_REQUESTID dwRequestId,             // Asynchronous request id
CTSPIAddressInfo* pAddr,               // Address to forward to (NULL=all)
TSPILINEFORWARD* lpForwardInfo,        // Local Forward information
HTAPICALL htConsultCall,               // New TAPI call handle if necessary
LPHDRVCALL lphdConsultCall)            // Our return call handle if needed
{                               
    // Pass it directly onto the address specified, or onto all our addresses.
    LONG lResult = 0;
    if (pAddr != NULL)
    {
        lResult = pAddr->CanForward (lpForwardInfo, 0);
        if (!ReportError(lResult))
            lResult = pAddr->Forward (dwRequestId, lpForwardInfo, htConsultCall, lphdConsultCall);
    }            
    else
    {                                       
    	// Check to see if we can perform this function now.
    	if ((GetLineDevStatus()->dwLineFeatures & LINEFEATURE_FORWARD) == 0)
    		return LINEERR_OPERATIONUNAVAIL;
    
        int iCount = m_arrAddresses.GetSize();

        // Run through all the addresses and see if they can forward given the 
        // forwarding instructions.  This function should NOT insert a request!
        for (int i = 0; i < iCount; i++)
        {
            CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
            lResult = pAddr->CanForward (lpForwardInfo, iCount);
            if (ReportError(lResult))
                break;
        }
        
        // Add an asynch request for the line connection with no address information
        // associated with it.  When it completes, we will route the completion to all
        // the addresses on the line.
        if (!ReportError(lResult))
        {
            *lphdConsultCall = NULL;
            if (AddAsynchRequest (NULL, REQUEST_FORWARD, dwRequestId, lpForwardInfo))
                lResult = (LONG) dwRequestId;
            else
                lResult = LINEERR_OPERATIONFAILED;
        }   
    }
    return lResult; 

}// CTSPILineConnection::Forward

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnTimer
//
// This is invoked by our periodic timer
//
VOID CTSPILineConnection::OnTimer()
{                               
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        pAddr->OnTimer();
    }
    
    CTSPIConnection::OnTimer();
    
}// CTSPILineConnection::OnTimer

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetDefaultMediaDetection
//
// This sets our current set of media mode detection being done
// on this line.  The new modes should be tested for on any offering calls.
//
LONG CTSPILineConnection::SetDefaultMediaDetection (DWORD dwMediaModes)
{                                                
    // Validate the media modes 
    if ((dwMediaModes & m_LineCaps.dwMediaModes) != dwMediaModes)
        return LINEERR_INVALMEDIAMODE;

    m_dwLineMediaModes = dwMediaModes;          
    return FALSE;

}// CTSPILineConnection::SetDefaultMediaDetection

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetDefaultMediaDetection
//
// Return the current set of media modes being detected by the SP.
// This is used when a new call is created to setup the initial monitoring
// of media modes.
//
DWORD CTSPILineConnection::GetDefaultMediaDetection() const
{                                                
    return m_dwLineMediaModes;
    
}// CTSPILineConnection::GetDefaultMediaDetection

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetStatusMessages
//
// This operation enables the TAPI DLL to specify which notification 
// messages the Service Provider should generate for events related to 
// status changes for the specified line or any of its addresses.
// 
LONG CTSPILineConnection::SetStatusMessages(DWORD dwLineStates, DWORD dwAddressStates)
{
    m_dwLineStates = dwLineStates;
    
    // Tell all the addresses which states to send.
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        pAddr->SetStatusMessages (dwAddressStates);
    }
    return FALSE;
    
}// CTSPILineConnection::SetStatusMessages

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::ConditionalMediaDetection
//
// This method is invoked by TAPI.DLL when the application requests a
// line open using the LINEMAPPER.  This method will check the 
// requested media modes and return an acknowledgement based on whether 
// we can monitor all the requested modes.
// 
LONG CTSPILineConnection::ConditionalMediaDetection(DWORD dwMediaModes, LPLINECALLPARAMS const lpCallParams)
{   
    // We MUST have call parameters (TAPI should always pass these).
    if (lpCallParams == NULL)
        return LINEERR_INVALCALLPARAMS;
    
    // Copy the call params into our own private buffer so we may alter them.
    LPLINECALLPARAMS lpMyCallParams = (LPLINECALLPARAMS) AllocMem (lpCallParams->dwTotalSize);
    if (lpMyCallParams == NULL)
        return LINEERR_NOMEM;
    CopyBuffer(lpMyCallParams, lpCallParams, lpCallParams->dwTotalSize);
    
    // Allow searching for ANY address.
    if (lpMyCallParams->dwAddressMode == LINEADDRESSMODE_ADDRESSID)
        lpMyCallParams->dwAddressMode = 0;        
    lpMyCallParams->dwMediaMode = dwMediaModes;
        
    // Verify the call parameters for the line/address given.
    LONG lResult = GetSP()->ProcessCallParameters(this, lpMyCallParams);
    FreeMem (lpMyCallParams);
    
    if (ReportError(lResult))
        return lResult;
    return FALSE;
    
}// CTSPILineConnection::ConditionalMediaDetection

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::ValidateMediaControlList
//
// This method is called by the lineSetMediaControl to validate that
// the media control parameters are ok for this line device.
//
LONG CTSPILineConnection::ValidateMediaControlList(TSPIMEDIACONTROL* lpMediaControl) const
{
    // If media control is not available, then exit.
    if ((m_LineCaps.dwDevCapFlags & LINEDEVCAPFLAGS_MEDIACONTROL) == 0)                                
        return LINEERR_OPERATIONUNAVAIL;
                                
    // Validate the media control elements.
    if (lpMediaControl->arrDigits.GetSize() > (int) m_LineCaps.dwMedCtlDigitMaxListSize)
        return LINEERR_INVALDIGITLIST;
        
    for (int i = 0; i< lpMediaControl->arrDigits.GetSize(); i++)
    {
        LPLINEMEDIACONTROLDIGIT lpDigit = (LPLINEMEDIACONTROLDIGIT) lpMediaControl->arrDigits[i];
        if ((lpDigit->dwDigitModes & m_LineCaps.dwMonitorDigitModes) != lpDigit->dwDigitModes)
            return LINEERR_INVALDIGITLIST;

        char cDigit = LOBYTE(LOWORD(lpDigit->dwDigit));        
        if (lpDigit->dwDigitModes & (LINEDIGITMODE_DTMF | LINEDIGITMODE_DTMFEND))
        {
            if (strchr ("0123456789ABCD*#", cDigit) == NULL)
                return LINEERR_INVALDIGITLIST;
        }
        else if (lpDigit->dwDigitModes & LINEDIGITMODE_PULSE)
        {
            if (strchr ("0123456789", cDigit) == NULL)
                return LINEERR_INVALDIGITLIST;
        }
    }

    if (lpMediaControl->arrMedia.GetSize() > (int) m_LineCaps.dwMedCtlMediaMaxListSize)
        return LINEERR_INVALMEDIALIST;
        
    for (i = 0; i <lpMediaControl->arrMedia.GetSize(); i++)
    {
        LPLINEMEDIACONTROLMEDIA lpMedia = (LPLINEMEDIACONTROLMEDIA) lpMediaControl->arrMedia[i];
        if ((lpMedia->dwMediaModes & m_LineCaps.dwMediaModes) != lpMedia->dwMediaModes)
            return LINEERR_INVALMEDIALIST;
    }
    
    if (lpMediaControl->arrTones.GetSize() > (int) m_LineCaps.dwMedCtlToneMaxListSize)
        return LINEERR_INVALTONELIST;
        
    for (i = 0; i < lpMediaControl->arrTones.GetSize(); i++)
    {
        LPLINEMEDIACONTROLTONE lpTone = (LPLINEMEDIACONTROLTONE) lpMediaControl->arrTones[i];
        int iFreqCount = 0;
        if (lpTone->dwFrequency1 > 0)
            iFreqCount++;
        if (lpTone->dwFrequency2 > 0)
            iFreqCount++;
        if (lpTone->dwFrequency3 > 0)
            iFreqCount++;
        if (iFreqCount > (int) m_LineCaps.dwMonitorToneMaxNumFreq)
            return LINEERR_INVALTONELIST;
    }
    
    if (lpMediaControl->arrCallStates.GetSize() > (int) m_LineCaps.dwMedCtlCallStateMaxListSize)
        return LINEERR_INVALCALLSTATELIST;

    // Alls ok.
    return FALSE;
    
}// CTSPILineConnection::ValidateMediaControlList

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetMediaControl
//
// This function enables and disables control actions on the media stream 
// associated with this line and all addresses/calls present here. Media control 
// actions can be triggered by the detection of specified digits, media modes, 
// custom tones, and call states.  The new specified media controls replace 
// all the ones that were in effect for this line, address, or call prior 
// to this request.
//
LONG CTSPILineConnection::SetMediaControl (TSPIMEDIACONTROL* lpMediaControl)
{   
    // We don't need to store this at the LINE level - since addresses are
    // static, we can simply pass it through them.
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        pAddr->SetMediaControl (lpMediaControl);
    }
    return FALSE;    

}// CTSPILineConnection::SetMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnRingDetected
//
// This method should be called by the service provider worker code
// when an incoming ring is detected on a line.
//
VOID CTSPILineConnection::OnRingDetected (DWORD dwRingMode, BOOL fStop)
{                                                                      
    // If our ring count array is empty, then grab the total number of
    // ring modes supported and add an entry for each.
    if (m_arrRingCounts.GetSize() < (int) m_LineCaps.dwRingModes)
    {
        for (int i = 0; i < (int)m_LineCaps.dwRingModes; i++)
            m_arrRingCounts.Add(0);
        ASSERT (dwRingMode < (DWORD) m_arrRingCounts.GetSize());
    }

    // Grab the current ring count.
    UINT uiRingCount = m_arrRingCounts[(int)dwRingMode];
    if (fStop)
        uiRingCount = 0;
    else
        uiRingCount++;
    m_arrRingCounts.SetAtGrow((int)dwRingMode, uiRingCount);

    // Notify TAPI about the ring.
    OnLineStatusChange (LINEDEVSTATE_RINGING, dwRingMode, (DWORD) uiRingCount);

}// CTSPILineConnection::OnRingDetected

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnRequestComplete
//
// This virtual method is called when an outstanding request on this
// line device has completed.  The return code indicates the success
// or failure of the request.  Note that the request will filter to
// the address and caller where available.
//
VOID CTSPILineConnection::OnRequestComplete(CTSPIRequest* pReq, LONG lResult)
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
            if (pTermStruct->pLine != NULL)
                SetTerminalModes ((int)pTermStruct->dwTerminalID, pTermStruct->dwTerminalModes, pTermStruct->bEnable);
        }
    } 

    // If this is a forwarding request, and there is no address information (ie: forward ALL
    // addresses on the line), then route the request complete to each address so it will 
    // store the forwarding information.
    else if (wRequest == REQUEST_FORWARD)
    {   
        if (pReq->GetAddressInfo() == NULL)
        {   
            TSPILINEFORWARD* pForward = (TSPILINEFORWARD*) pReq->GetDataPtr();
            for (int i = 0; i < m_arrAddresses.GetSize(); i++)
            {
                CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
                pAddr->OnRequestComplete (pReq, lResult);
            }
        }            
    }
    
    // If this is a COMPLETION request then, if it was successful, mark the completion
    // request and data filled in by the service provider.
    else if (wRequest == REQUEST_COMPLETECALL)
    {   
        if (lResult == 0)
        {   
            // Copy the request over to a new "storable" request.
            TSPICOMPLETECALL* pCall = (TSPICOMPLETECALL*) pReq->GetDataPtr();
            TSPICOMPLETECALL* pComplete = new TSPICOMPLETECALL(pCall);
            
            // If the request isn't for a CAMP, then reset the call handle since
            // it should transition to IDLE and be deallocated.  Otherwise, leave
            // it in-place so that the address object can find it for a call.
            if (pCall->dwCompletionMode != LINECALLCOMPLMODE_CAMPON)
                pComplete->pCall = NULL;
            
            m_lstCompletions.AddTail (pComplete);
            OnLineStatusChange (LINEDEVSTATE_NUMCOMPLETIONS);
        }
    }
    
    // If this is an UNCOMPLETE call request which completed successfully, then
    // remove the request from the list.
    else if (wRequest == REQUEST_UNCOMPLETECALL)
    {
        if (lResult == 0)                                     
        {
            TSPICOMPLETECALL* pComplete = (TSPICOMPLETECALL*) pReq->GetDataPtr();
            RemoveCallCompletionRequest (pComplete->dwCompletionID, FALSE);
        }            
    }

}// CTSPILineConnection::OnRequestComplete 

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::FindCallCompletionRequest
//
// Return the TSPICOMPLETECALL ptr associated with a completion ID.
//
TSPICOMPLETECALL* CTSPILineConnection::FindCallCompletionRequest (DWORD dwSwitchInfo, const char * pszSwitchInfo)
{       
    for (POSITION pos = m_lstCompletions.GetHeadPosition(); pos != NULL;)
    {
        TSPICOMPLETECALL* pCall = (TSPICOMPLETECALL*) m_lstCompletions.GetNext(pos);
        if (pCall->dwSwitchInfo == dwSwitchInfo &&
            (pszSwitchInfo == NULL || pCall->strSwitchInfo.CompareNoCase(pszSwitchInfo) == 0))
            return pCall;
    } 
    return NULL;

}// CTSPILineConnection::FindCallCompletionRequest

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::FindCallCompletionRequest
//
// Locate a call completion request for a specific call appearance.
// Note that this will only locate call completions for CAMPed requests.
//
TSPICOMPLETECALL* CTSPILineConnection::FindCallCompletionRequest(CTSPICallAppearance* pCall)
{
    for (POSITION pos = m_lstCompletions.GetHeadPosition(); pos != NULL;)
    {
        TSPICOMPLETECALL* pComplete = (TSPICOMPLETECALL*) m_lstCompletions.GetNext(pos);
        if (pComplete->pCall == pCall)
            return pComplete;
    } 
    return NULL;

}// CTSPILineConnection::FindCallCompletionRequest

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::FindCallCompletionRequest
//
// Locate a call completion request based on a completion ID passed
// back to TAPI.
//
TSPICOMPLETECALL* CTSPILineConnection::FindCallCompletionRequest(DWORD dwCompletionID)
{
    for (POSITION pos = m_lstCompletions.GetHeadPosition(); pos != NULL;)
    {
        TSPICOMPLETECALL* pComplete = (TSPICOMPLETECALL*) m_lstCompletions.GetNext(pos);
        if (pComplete->dwCompletionID == dwCompletionID)
            return pComplete;
    } 
    return NULL;

}// CTSPILineConnection::FindCallCompletionRequest

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::RemoveCallCompletionRequest
//
// Remove an existing call completion request from our array.  This
// is called by the "OnRequestComplete"
//
VOID CTSPILineConnection::RemoveCallCompletionRequest(DWORD dwCompletionID, BOOL fNotifyTAPI)
{                                
    // Locate the completion ID and delete it from our list.                   
    TSPICOMPLETECALL* pComplete = NULL;
    for (POSITION pos = m_lstCompletions.GetHeadPosition(); pos != NULL;)
    {                                 
        POSITION posLast = pos;
        pComplete = (TSPICOMPLETECALL*) m_lstCompletions.GetNext(pos);
        if (pComplete->dwCompletionID == dwCompletionID)
        {
            m_lstCompletions.RemoveAt(posLast);
            break;
        }
    } 
    
    // If we didn't find any completion request, then ignore.    
    if (pComplete == NULL)
        return;
    
    // If we are to notify TAPI, then do so.  This should only happen 
    // when the completion is canceled by the derived class (i.e. the hardware
    // canceled the request.
    if (fNotifyTAPI && GetSP()->GetSupportedVersion() >= TAPIVER_14 &&
        GetSP()->GetSystemVersion() >= TAPIVER_14 &&
        GetNegotiatedVersion() >= TAPIVER_14)
    {
        OnLineStatusChange (LINEDEVSTATE_COMPLCANCEL, dwCompletionID);
    }            

    OnLineStatusChange (LINEDEVSTATE_NUMCOMPLETIONS);
    delete pComplete;
    
}// CTSPILineConnection::RemoveCallCompletionRequest

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::UncompleteCall
//
// Cancel a call completion request from TAPI.
//
LONG CTSPILineConnection::UncompleteCall (DRV_REQUESTID dwRequestID, DWORD dwCompletionID)
{
    // Make sure the completion ID is valid.  
    TSPICOMPLETECALL* pNewCall = FindCallCompletionRequest (dwCompletionID);
    if (pNewCall == NULL)
        return LINEERR_INVALCOMPLETIONID;

    // Submit the request to the worker code to uncode the actual line device.
    if (AddAsynchRequest(NULL, REQUEST_UNCOMPLETECALL, dwRequestID, pNewCall))
        return (LONG) dwRequestID;
    
    return LINEERR_OPERATIONFAILED;
        
}// CTSPILineConnection::UncompleteCall

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetID
//
// Manage device-level requests for information based on a device id.
//
LONG CTSPILineConnection::GetID (CString& strDevClass, LPVARSTRING lpDeviceID)
{
    // Base class supports default "tapi/line" which returns our device id.
    if (strDevClass == "tapi/line")
    {
        lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
        if (lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize)
        {
            lpDeviceID->dwUsedSize     = lpDeviceID->dwNeededSize;
            lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
            lpDeviceID->dwStringSize   = sizeof(DWORD);
            lpDeviceID->dwStringOffset = sizeof(VARSTRING);
            LPDWORD lpDW = (LPDWORD)((LONG)lpDeviceID + sizeof(VARSTRING));
            *lpDW = GetDeviceID();
        }
        return FALSE;
    }

    // Base class supports "tapi/phone" if we have a phone device at
    // the same array position as the line passed.
    else if (strDevClass == "tapi/phone")
    {
        CTSPIDevice* pDev = GetDeviceInfo();
        CTSPIPhoneConnection* pPhone = NULL;
        for (int iPos = 0; iPos < (int) pDev->GetLineCount(); iPos++)
        {
            if (pDev->GetLineConnectionInfo(iPos) == this)        
            {
                pPhone = pDev->GetPhoneConnectionInfo(iPos);        
                break;
            }
        }
    
        if (pPhone)
        {
            lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
            if (lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize)
            {
                lpDeviceID->dwUsedSize     = lpDeviceID->dwNeededSize;
                lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
                lpDeviceID->dwStringSize   = sizeof(DWORD);
                lpDeviceID->dwStringOffset = sizeof(VARSTRING);
                LPDWORD lpDW = (LPDWORD)((LONG)lpDeviceID + sizeof(VARSTRING));
                *lpDW = pPhone->GetDeviceID();
                return FALSE;
            }
        }            
        return FALSE;
    }

    // Additional support must be provided by derived class
    return LINEERR_INVALDEVICECLASS;
    
}// CTSPILineConnection::GetID

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetDevConfig
//
// Return the device configuration for this line and device class.
//
LONG CTSPILineConnection::GetDevConfig(CString& /*strDeviceClass*/, LPVARSTRING /*lpDeviceConfig*/)
{   
    // Derived class needs to supply the data structures for this based on
    // what may be configured.                                 
    return LINEERR_OPERATIONUNAVAIL;

}// CTSPILineConnection::GetDevConfig

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetDevConfig
//
// Sets the device configuration for this line and device class.
//
LONG CTSPILineConnection::SetDevConfig(CString& /*strDeviceClass*/, 
                                       LPVOID const /*lpDevConfig*/, DWORD /*dwSize*/)
{   
    // Derived class needs to supply the data structures for this based on
    // what may be configured.                                 
    return LINEERR_OPERATIONUNAVAIL;

}// CTSPILineConnection::SetDevConfig

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::DevSpecificFeature
//
// Invoke a device-specific feature on this line device.
//
LONG CTSPILineConnection::DevSpecificFeature(DWORD /*dwFeature*/, DRV_REQUESTID /*dwRequestId*/,
                                             LPVOID /*lpParams*/, DWORD /*dwSize*/)
{                                          
    // Derived class must manage device-specific features.
    return LINEERR_OPERATIONUNAVAIL;
    
}// CTSPILineConnection::DevSpecificFeature

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetIcon
//
// This function retrieves a service line device-specific icon for display
// in user-interface dialogs.
//
LONG CTSPILineConnection::GetIcon (CString& /*strDevClass*/, LPHICON /*lphIcon*/)
{
    // Return not available, TAPI will supply a default icon.
    return LINEERR_OPERATIONUNAVAIL;
    
}// CTSPILineConnection::GetIcon

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::ConfigDialog
//
// Process a configuration dialog for the line device.  This is called
// by both lineConfigDialog and lineConfigDialogEdit.
//
LONG CTSPILineConnection::ConfigDialog (CWnd* /*pwndOwner*/, CString& /*strDeviceClass*/,
                        LPVOID const /*lpDeviceConfigIn*/, DWORD /*dwSize*/, 
                        LPVARSTRING /*lpDeviceConfigOut*/)
{
    // Derived class must manage configuration features.
    return LINEERR_OPERATIONUNAVAIL;

}// CTSPILineConnection::ConfigDialog

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnCallDeleted
//
// A call is being deleted from some address.
//
VOID CTSPILineConnection::OnCallDeleted(CTSPICallAppearance* /*pCall*/)
{
	/* Do nothing */
	
}// CTSPILineConnection::OnCallDeleted

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::IsConferenceAvailable
//
// This determines whether there is a conference call active on the
// same line/address as the call passed.
//
BOOL CTSPILineConnection::IsConferenceAvailable(CTSPICallAppearance* pCall)
{   
    // Check on the same address.                                        
    CTSPIAddressInfo* pAddr = pCall->GetAddressInfo();
    for (int iCall = 0; iCall < pAddr->GetCallCount(); iCall++)
    {
        CTSPICallAppearance* pThisCall = pAddr->GetCallInfo(iCall);
        if (pThisCall != NULL && pThisCall != pCall && pThisCall->GetCallType() == CALLTYPE_CONFERENCE)
            return TRUE;
    }

    // None found there, check on the same line if cross-address conferencing is
    // available.
    if (m_LineCaps.dwDevCapFlags & LINEDEVCAPFLAGS_CROSSADDRCONF)
    {
        for (int iAddr = 0; iAddr < (int) GetAddressCount(); iAddr++)
        {
            CTSPIAddressInfo* pThisAddr = GetAddress(iAddr);
            if (pThisAddr != pAddr)
            {
                for (iCall = 0; iCall < (int) pThisAddr->GetCallCount(); iCall++)
                {
                    CTSPICallAppearance* pThisCall = pThisAddr->GetCallInfo(iCall);
                    if (pThisCall != NULL && pThisCall->GetCallType() == CALLTYPE_CONFERENCE)
                        return TRUE;
                }
            }                                       
        }
    }
    
    // No conference found.
    return FALSE;
    
}// CTSPILineConnection::IsConferenceAvailable

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::IsTransferConsultAvailable
//
// This determines whether there is a consultant call or some other
// call which we could transfer to right now.
//
BOOL CTSPILineConnection::IsTransferConsultAvailable(CTSPICallAppearance* pCall)
{   
    // See if we have an attached consultation call which can be used for
    // transfer.  This would have been created through the SetupTransfer API.
    CTSPICallAppearance* pThisCall = pCall->GetAttachedCall();
    if (pThisCall != NULL)
    {
        DWORD dwCallState = pThisCall->GetCallState();
        if (dwCallState != LINECALLSTATE_CONNECTED && dwCallState != LINECALLSTATE_RINGBACK &&
            dwCallState != LINECALLSTATE_BUSY && dwCallState != LINECALLSTATE_PROCEEDING)
            return FALSE;
        return TRUE;            
    }               
    
    // If the address supports creation of a new consultation call, then search 
    // our address for calls in the appropriate state.  This may or may not have been
    // created with the SetupTransfer API.
    CTSPIAddressInfo* pAddr = pCall->GetAddressInfo();
    if ((pAddr->GetAddressCaps()->dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERMAKE) == 0)
        return FALSE;

    for (int i = 0; i < (int) pAddr->GetAddressStatus()->dwNumInUse; i++)
    {
        CTSPICallAppearance* pThisCall = pAddr->GetCallInfo(i);
        if (pThisCall != pCall)
        {
            DWORD dwCallState = pThisCall->GetCallState();
            if (dwCallState == LINECALLSTATE_CONNECTED && dwCallState == LINECALLSTATE_RINGBACK &&
                dwCallState == LINECALLSTATE_BUSY && dwCallState == LINECALLSTATE_PROCEEDING)
                return TRUE;
        }                   
    }            
    
    return FALSE;
        
}// CTSPILineConnection::IsTransferConsultAvailable

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::FindCallByState
//
// Run through all our addresses and look for a call in the specified
// state.
//
CTSPICallAppearance* CTSPILineConnection::FindCallByState(DWORD dwState) const
{                
    for (int j = 0; j < (int) GetAddressCount(); j++)
    {
        CTSPIAddressInfo* pAddr = GetAddress((DWORD)j);
        for (int i = 0; i < (int) pAddr->GetAddressStatus()->dwNumInUse; i++)
        {
            CTSPICallAppearance* pThisCall = pAddr->GetCallInfo(i);
            if (dwState & pThisCall->GetCallState())
                return pThisCall;
        }            
    }       
    return NULL;

}// CTSPILineConnection::FindCallByState

//////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnCallFeaturesChanged
//
// This method gets called whenever a call changes its currently
// available features in the CALLINFO structure.
//
DWORD CTSPILineConnection::OnCallFeaturesChanged (CTSPICallAppearance* pCall, DWORD dwFeatures)
{ 
	return GetSP()->CheckCallFeatures (pCall, dwFeatures);

}// CTSPILineConnection::OnCallFeaturesChanged

//////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnAddressFeaturesChanged
//
// This method gets called whenever an address changes its currently
// available features in the ADDRESSSTATUS structure.
//
DWORD CTSPILineConnection::OnAddressFeaturesChanged (CTSPIAddressInfo* /*pAddr*/, DWORD dwFeatures)
{ 
	return dwFeatures;

}// CTSPILineConnection::OnAddressFeaturesChanged

//////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnConnectedCallCountChange
//
// This method gets called whenever any address on this line changes
// the total count of connected calls.  This impacts the bandwidth of the
// service provider (i.e. whether MAKECALL and such may be called).
//
VOID CTSPILineConnection::OnConnectedCallCountChange(CTSPIAddressInfo* /*pInfo*/, int iDelta)
{
	// Get a new count.
	m_dwConnectedCallCount += iDelta;
	TRACE("OnConnectedCallCountChange: Delta=%d, New Count=%ld\r\n", iDelta, m_dwConnectedCallCount);

	// Now adjust our LINE features based on the total counts.
	DWORD dwLineFeatures = m_LineStatus.dwLineFeatures;
	if (m_dwConnectedCallCount < m_LineCaps.dwMaxNumActiveCalls)
		dwLineFeatures |= LINEFEATURE_MAKECALL;
	else
		dwLineFeatures &= ~LINEFEATURE_MAKECALL;
	SetLineFeatures (OnLineFeaturesChanged(dwLineFeatures));

}// CTSPILineConnection::OnConnectedCallCountChange

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnMediaConfigChanged
//
// This method may be used by the service provider to notify TAPI that
// the media configuration has changed.
//
void CTSPILineConnection::OnMediaConfigChanged()
{
	// Tell TAPI the configuration has changed.
	Send_TAPI_Event (NULL, LINE_LINEDEVSTATE, LINEDEVSTATE_CONFIGCHANGE);

}// CTSPILineConnection::OnMediaConfigChanged

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::ForceClose
//
// This method may be used by the service provider to forcibly close the
// line object.
//
void CTSPILineConnection::ForceClose()
{
	Send_TAPI_Event (NULL, LINE_CLOSE);

}// CTSPILineConnection::ForceClose
