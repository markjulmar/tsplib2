/******************************************************************************/
//                                                                        
// LINECONN.CPP - Source code for the CTSPILineConnection object          
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the source to manage the line objects which are 
// held by the CTSPIDevice.                                               
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
	m_dwLineMediaModes(0), m_dwLineStates(0),
	m_lpfnEventProc(NULL), m_htLine(0), m_dwConnectedCallCount(0)
{ 
    FillBuffer (&m_LineCaps, 0, sizeof(LINEDEVCAPS));
    FillBuffer (&m_LineStatus, 0, sizeof(LINEDEVSTATUS));

}// CTSPILineConnection::CTSPILineConnection

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::Init
//
// Initialize this line connection object
//
void CTSPILineConnection::Init(CTSPIDevice* pDevice, DWORD dwLineId, DWORD dwPos, DWORD /*dwItemData*/)
{
    CTSPIConnection::Init(pDevice, dwLineId);
    
    // Initialize the line device capabilities.  The permanent line id
    // is a combination of our device id plus the position of the line itself
    // within our device array.  This uniquely identifies the line to us
    // with a single DWORD.  The MSB of the loword is always zero - this
    // identifies it as a LINE device.
    m_LineCaps.dwPermanentLineID = (pDevice->GetProviderID() << 16) + (dwPos&0x00007fff);
    
#ifdef _UNICODE
    m_LineCaps.dwStringFormat = STRINGFORMAT_UNICODE;
#else
    m_LineCaps.dwStringFormat = STRINGFORMAT_ASCII;
#endif

    m_LineCaps.dwAddressModes = LINEADDRESSMODE_ADDRESSID;
    m_LineCaps.dwBearerModes = LINEBEARERMODE_VOICE;
    m_LineCaps.dwLineStates = (LINEDEVSTATE_OTHER | LINEDEVSTATE_RINGING | LINEDEVSTATE_CONNECTED |
                               LINEDEVSTATE_DISCONNECTED | LINEDEVSTATE_MSGWAITON | LINEDEVSTATE_MSGWAITOFF |
                               LINEDEVSTATE_INSERVICE | LINEDEVSTATE_OUTOFSERVICE | LINEDEVSTATE_MAINTENANCE |
                               LINEDEVSTATE_TERMINALS |
                               LINEDEVSTATE_NUMCALLS | LINEDEVSTATE_NUMCOMPLETIONS | LINEDEVSTATE_ROAMMODE |
                               LINEDEVSTATE_BATTERY | LINEDEVSTATE_SIGNAL | LINEDEVSTATE_LOCK |
							LINEDEVSTATE_COMPLCANCEL |		// TAPI v1.4
							LINEDEVSTATE_CAPSCHANGE |		// TAPI v1.4
							LINEDEVSTATE_CONFIGCHANGE |		// TAPI v1.4
							LINEDEVSTATE_TRANSLATECHANGE |  // TAPI v1.4
							LINEDEVSTATE_REMOVED);          // TAPI v1.4

    // This will be adjusted by each address added as the "SetAvailableMediaModes" API is called.
    m_LineCaps.dwMediaModes = 0;

    // Always set the number of ACTIVE calls (i.e. connected) to one - derived
    // classes may override this if they can support more than one active call
    // at a time.  Reset the value after this function.
    m_LineCaps.dwMaxNumActiveCalls = 1;
    
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
    if (CanHandleRequest(TSPI_LINEMAKECALL))
        m_LineCaps.dwLineFeatures |= LINEFEATURE_MAKECALL;
    
    if (CanHandleRequest(TSPI_LINESETMEDIACONTROL))
    {
        m_LineCaps.dwDevCapFlags |= LINEDEVCAPFLAGS_MEDIACONTROL;
        m_LineCaps.dwLineFeatures |= LINEFEATURE_SETMEDIACONTROL;
        m_LineStatus.dwLineFeatures |= LINEFEATURE_SETMEDIACONTROL;
	}        

	if (CanHandleRequest(TSPI_LINEDEVSPECIFIC))
	{
		m_LineCaps.dwLineFeatures |= LINEFEATURE_DEVSPECIFIC;
		m_LineStatus.dwLineFeatures |= LINEFEATURE_DEVSPECIFIC;
	}

	if (CanHandleRequest(TSPI_LINEDEVSPECIFICFEATURE))
	{
		m_LineCaps.dwLineFeatures |= LINEFEATURE_DEVSPECIFICFEAT;
		m_LineStatus.dwLineFeatures |= LINEFEATURE_DEVSPECIFICFEAT;
	}

    if (CanHandleRequest(TSPI_LINEFORWARD))
    {
        m_LineCaps.dwLineFeatures |= LINEFEATURE_FORWARD;
        m_LineStatus.dwLineFeatures |= LINEFEATURE_FORWARD;
    }

	// Added for TAPI 2.0
    if (CanHandleRequest(TSPI_LINESETLINEDEVSTATUS))
	{
		m_LineCaps.dwLineFeatures |= LINEFEATURE_SETDEVSTATUS;
		m_LineStatus.dwLineFeatures |= LINEFEATURE_SETDEVSTATUS;
	}

    // Add terminal support - the LINEDEVSTATUS field will be updated when
    // the terminal is actually ADDED.
    if (CanHandleRequest(TSPI_LINESETTERMINAL))
        m_LineCaps.dwLineFeatures |= LINEFEATURE_SETTERMINAL;

	// Add in the "tapi/line" device class.
	AddDeviceClass (_T("tapi/line"), GetDeviceID());
	AddDeviceClass (_T("tapi/providerid"), GetDeviceInfo()->GetProviderID());

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
	CEnterCode cLock(this);  // Synch access to object
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

	// Delete any UI dialogs still running.
	for (i = 0; i < m_arrUIDialogs.GetSize(); i++)
	{
		LINEUIDIALOG* pLineDlg = (LINEUIDIALOG*) m_arrUIDialogs[i];
		if (pLineDlg != NULL)
		{
			// Force it to close
			SendDialogInstanceData (pLineDlg->htDlgInstance);
			delete pLineDlg;
		}
	}
	m_arrUIDialogs.RemoveAll();

}// CTSPILineConnection::~CTSPILineConnection

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
// CTSPILineConnection::OnPreCallStateChange
//
// A call is about to change state on an address on this line.
//
void CTSPILineConnection::OnPreCallStateChange(CTSPIAddressInfo* /*pAddr*/, CTSPICallAppearance* /*pCall*/, DWORD dwNewState, DWORD dwOldState)
{
	// If the state has not changed, ignore.
	if (dwNewState == dwOldState)
		return;

	// Make sure only this thread is updating the status.
	CEnterCode Key(this);  // Synch access to object

    // Determine if the number of active calls has changed.
    BOOL fWasActive = CTSPICallAppearance::IsActiveCallState(dwOldState);
    BOOL fIsActive  = CTSPICallAppearance::IsActiveCallState(dwNewState);

    if (fWasActive == FALSE && fIsActive == TRUE)
    {       
        m_LineStatus.dwNumActiveCalls++;
		m_dwFlags |= NotifyNumCalls;
    }
    else if (fWasActive == TRUE && fIsActive == FALSE)
    {
        if (--m_LineStatus.dwNumActiveCalls & 0x80000000)
            m_LineStatus.dwNumActiveCalls = 0L;
		m_dwFlags |= NotifyNumCalls;
    }       

    // Determine if the HOLD status has changed.        
    if (dwNewState == LINECALLSTATE_ONHOLD)
    {
        m_LineStatus.dwNumOnHoldCalls++;
		m_dwFlags |= NotifyNumCalls;
    }
    else if (dwNewState == LINECALLSTATE_ONHOLDPENDTRANSFER || dwNewState == LINECALLSTATE_ONHOLDPENDCONF)
    {
        m_LineStatus.dwNumOnHoldPendCalls++;
		m_dwFlags |= NotifyNumCalls;
    }

    if (dwOldState == LINECALLSTATE_ONHOLD)
    {
        if (--m_LineStatus.dwNumOnHoldCalls & 0x80000000)
            m_LineStatus.dwNumOnHoldCalls = 0L;
		m_dwFlags |= NotifyNumCalls;
    }

    else if (dwOldState == LINECALLSTATE_ONHOLDPENDTRANSFER || dwOldState == LINECALLSTATE_ONHOLDPENDCONF)
    {
        if (--m_LineStatus.dwNumOnHoldPendCalls & 0x80000000)
            m_LineStatus.dwNumOnHoldPendCalls = 0L;
		m_dwFlags |= NotifyNumCalls;
    }

	Key.Unlock();

    TRACE(_T("Line 0x%lx Active=%ld, OnHold=%ld,  OnHoldPend=%ld\n"), GetPermanentDeviceID(),
		m_LineStatus.dwNumActiveCalls, m_LineStatus.dwNumOnHoldCalls, m_LineStatus.dwNumOnHoldPendCalls);            

}// CTSPILineConnection::OnPreCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnCallStateChange
//
// A call has changed state on an address on this line.  Update our
// status.
//
void CTSPILineConnection::OnCallStateChange (CTSPIAddressInfo* /*pAddr*/, CTSPICallAppearance* /*pCall*/, 
                                             DWORD /*dwNewState*/, DWORD /*dwOldState*/)
{   
    if (m_dwFlags & NotifyNumCalls)
	{
        OnLineStatusChange (LINEDEVSTATE_NUMCALLS);
		m_dwFlags &= ~NotifyNumCalls;
	}

	// Recalc our line features based on the new call states.
	RecalcLineFeatures();

}// CTSPILineConnection::OnCallStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnLineStatusChange
//
// This method is called when any of the values in our line device
// status record are changed.  It is called internally by the library
// and should also be called by the derived class if the LINDEVSTATUS
// structure is modified directly.
//
void CTSPILineConnection::OnLineStatusChange (DWORD dwState, DWORD dwP2, DWORD dwP3)
{
	if (GetLineHandle() && ((m_dwLineStates & dwState) || dwState == LINEDEVSTATE_REINIT))
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

    DTRACE(TRC_MIN, _T("Opening line %lx, TAPI handle=%lx, SP handle=%lx\r\n"), GetDeviceID(), GetLineHandle(), (DWORD)this);

    // Tell our device to perform an open for this connection.
    if (!OpenDevice())
	{
		m_lpfnEventProc = NULL;
		m_htLine = 0;
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
    if (GetLineHandle() != 0)
    {
        TRACE(_T("Closing line %lx, TAPI handle=%lx, SP handle=%lx\r\n"), GetDeviceID(), GetLineHandle(), (DWORD)this);
        
		// If the line capabilities specify that we DROP all existing calls
		// on the line when it is closed, then spin through each of the 
		// calls active on this line and issue a drop request.
		if ((GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_CLOSEDROP) ==
				LINEDEVCAPFLAGS_CLOSEDROP)
		{
			for (int iAddress = 0; iAddress < (int) GetAddressCount(); iAddress++)
			{
				CTSPIAddressInfo* pAddress = GetAddress(iAddress);
				for (int iCalls = 0; iCalls < pAddress->GetCallCount(); iCalls++)
				{
					CTSPICallAppearance* pCall = pAddress->GetCallInfo(iCalls);
					pCall->Drop();
				}
			}
		}
        
        // Make sure any pending call close operations complete.
        WaitForAllRequests (NULL, REQUEST_DROPCALL);
        
        // Remove any other pending requests for this connection.
        RemovePendingRequests();

        // Decrement our total line open count.
        OnLineStatusChange (LINEDEVSTATE_CLOSE);
        
        // Reset our event and line proc.
        m_lpfnEventProc = NULL;
        m_htLine = 0;
        m_dwLineStates = 0L;
		m_dwNegotiatedVersion = GetSP()->GetSupportedVersion();
		m_dwConnectedCallCount = 0;
        
        // Tell our device to close.
        CloseDevice();

		// If the line has been removed, then mark it as DELETED now
		// so we will refuse any further traffic on this line.
		if (GetFlags() & _IsRemoved)
			m_dwFlags |= _IsDeleted;

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
void CTSPILineConnection::Send_TAPI_Event(
CTSPICallAppearance* pCall, // Call appearance to send event for
DWORD dwMsg,                // Message to send (LINExxx)
DWORD dwP1,                 // Parameter 1 (depends on above message)
DWORD dwP2,                 // Parameter 2 (depends on above message)
DWORD dwP3)                 // Parameter 3 (depends on above message)
{
    // We cannot send events for lines which haven't been initialized.
    // This happens if the line hasn't been opened yet.
    if (m_lpfnEventProc != NULL)
    {
        HTAPILINE htLine = GetLineHandle();
		if (dwMsg == LINE_CREATEDIALOGINSTANCE)
		{
			htLine = (HTAPILINE) GetDeviceInfo()->GetProviderHandle();
#ifdef _DEBUG
			if (htLine == NULL)
			{
				// This happens because TSPI_providerEnumDevices
				// is not exported.  It must be exported in TAPI 2.0
				ASSERT (FALSE);
				DTRACE(TRC_MIN, _T("Error: TSPI_providerEnumDevices needs to be exported!\r\n"));
				return;
			}
#endif
		}
		else if (dwMsg == LINE_SENDDIALOGINSTANCEDATA)
		{
			htLine = (HTAPILINE) dwP3;
			dwP3 = 0L;
		}

        HTAPICALL htCall = (HTAPICALL) 0;

        // If a call appearance was supplied, get the call handle
        // from it.
        if (pCall)
            htCall = pCall->GetCallHandle();
        
#ifdef _DEBUG
        static LPCTSTR g_pszMsgs[] = {
                {_T("Line_AddressState")},               // 0
                {_T("Line_CallInfo")},                   // 1
                {_T("Line_CallState")},                  // 2
                {_T("Line_Close")},                      // 3
                {_T("Line_DevSpecific")},                // 4
                {_T("Line_DevSpecificFeature")},         // 5
                {_T("Line_GatherDigits")},               // 6
                {_T("Line_Generate")},                   // 7
                {_T("Line_LineDevState")},               // 8
                {_T("Line_MonitorDigits")},              // 9
                {_T("Line_MonitorMedia")},               // 10
                {_T("Line_MonitorTone")},                // 11
                {_T("Line_Reply")},                      // 12
                {_T("Line_Request")},                    // 13
                {_T("Phone_Button")},                    // 14
                {_T("Phone_Close")},                     // 15
                {_T("Phone_DevSpecific")},               // 16
                {_T("Phone_Reply")},                     // 17
                {_T("Phone_State")},                     // 18
                {_T("Line_Create")},                     // 19
                {_T("Phone_Create")},                    // 20
				{_T("Line_AgentSpecific")},				 // 21
				{_T("Line_AgentStatus")},				 // 22
				{_T("Line_AppNewCall")},				 // 23
				{_T("Line_ProxyRequest")},				 // 24
				{_T("Line_Remove")},					 // 25
				{_T("Phone_Remove")}					 // 26
            };        
        static LPCTSTR g_pszTSPIMsgs[] = {
				{_T("Line_NewCall")},					 // 0
				{_T("Line_CallDevSpecific")},            // 1
				{_T("Line_CallDevSpecificFeature")},     // 2
				{_T("Line_CreateDialogInstance")},       // 3
				{_T("Line_SendDialogInstanceData")}      // 4
			};

		LPCTSTR pszMsg = _T("???");			
		if (dwMsg <= 26)
			pszMsg = g_pszMsgs[dwMsg];
		else if (dwMsg >= TSPI_MESSAGE_BASE && dwMsg < TSPI_MESSAGE_BASE+5)
			pszMsg = g_pszTSPIMsgs[dwMsg-TSPI_MESSAGE_BASE];
    
        // Send the notification to TAPI.
        DTRACE (TRC_MIN, _T("Send_TAPI_Event: <0x%lx> Line=0x%lx, Call=0x%lx, Msg=0x%lx (%s), P1=0x%lx, P2=0x%lx, P3=0x%lx\r\n"),
                    (DWORD)this, (DWORD)htLine, (DWORD)htCall, 
                    dwMsg, pszMsg, dwP1, dwP2, dwP3);
#endif

		(*m_lpfnEventProc)(htLine, htCall, dwMsg, dwP1, dwP2, dwP3);
    }

}// CTSPILineConnection::Send_TAPI_Event

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::CreateAddress
//
// Create a new address on this line.
//
DWORD CTSPILineConnection::CreateAddress (LPCTSTR lpszDialableAddr, LPCTSTR lpszAddrName, 
                                          BOOL fInput, BOOL fOutput, DWORD dwAvailMediaModes,
                                          DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate,
                                          LPLINEDIALPARAMS lpDial, DWORD dwMaxNumActiveCalls,
                                          DWORD dwMaxNumOnHoldCalls, DWORD dwMaxNumOnHoldPendCalls,
                                          DWORD dwMaxNumConference, DWORD dwMaxNumTransConf)
                                          
{
    CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) GetSP()->GetTSPIAddressObj()->CreateObject();
    ASSERT (pAddr->IsKindOf(RUNTIME_CLASS(CTSPIAddressInfo)));

    // Add it to our array.
	CEnterCode sLock(this);  // Synch access to object
    int iPos = m_arrAddresses.Add (pAddr);
    
    // Update our line status features if they are now different.
    if (fOutput && CanHandleRequest(TSPI_LINEMAKECALL))
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
		strName.Format(_T("Address%ld"), m_LineCaps.dwNumAddresses);
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
// Locate an address based on a dialable address.
//
CTSPIAddressInfo* CTSPILineConnection::GetAddress (LPCTSTR lpszAddress) const
{                                  
	// Note: we don't bother to lock the object here since addresses
	// are considered a static entity. If your implementation adds/removes
	// addresses dynamically (outside of Init), then it MUST lock the object
	// before calling this API!
    CTSPIAddressInfo* pAddr = NULL;
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pTestAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        if (!_tcsicmp (pTestAddr->GetDialableAddress(), lpszAddress))
        {
            pAddr = pTestAddr;
            break;            
        }
    }
    return pAddr;

}// CTSPILineConnection::GetAddress
                        
///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GatherCapabilities
//
// Gather the line device capabilities for this list.
//
LONG CTSPILineConnection::GatherCapabilities (DWORD dwTSPIVersion, DWORD /*dwExtVer*/, LPLINEDEVCAPS lpLineCaps)
{   
	CEnterCode sLock(this);  // Synch access to object

	// Adjust the TSPI version if this line was opened at a lower TSPI version.
	if (dwTSPIVersion > GetNegotiatedVersion())
		dwTSPIVersion = GetNegotiatedVersion();

	// Determine the full size required for our line capabilities
    CString strLineName = GetName(), strProviderInfo = GetSP()->GetProviderInfo(), strSwitchInfo = GetConnInfo();
    int cbName=0, cbInfo=0, cbSwitchInfo=0, cbDeviceNameLen=0;
    
    if (!strLineName.IsEmpty())
        cbName = (strLineName.GetLength()+1) * sizeof(TCHAR);
    if (!strProviderInfo.IsEmpty())
        cbInfo = (strProviderInfo.GetLength()+1) * sizeof(TCHAR);
    if (!strSwitchInfo.IsEmpty())
        cbSwitchInfo = (strSwitchInfo.GetLength()+1) * sizeof(TCHAR);

	// Get the length of the device classes we support.
	CString strDeviceNames;
	for (int i = 0; i < m_arrDeviceClass.GetSize(); i++)
	{
		DEVICECLASSINFO* pDevClass = (DEVICECLASSINFO*) m_arrDeviceClass[i];
		strDeviceNames += pDevClass->strName + _T('~');
	}
	if (!strDeviceNames.IsEmpty())
	{
		strDeviceNames += _T('~');
		cbDeviceNameLen = (strDeviceNames.GetLength()+1) * sizeof(TCHAR);
	}

	// Copy over the static structure of the specified size based on the TSPI version requested
	// from TAPI.  This is determined by the calling application's API level.  This means that
	// we can have older TAPI complient applications calling us which don't expect the extra
	// information in our LINEDEVCAPS record.
	DWORD dwReqSize = sizeof(LINEDEVCAPS);
	if (dwTSPIVersion < TAPIVER_20)
		dwReqSize -= sizeof(DWORD) * 3;
	if (dwTSPIVersion < TAPIVER_14)
		dwReqSize -= sizeof(DWORD);

    // Check the available size to make sure we have enough room.   
    m_LineCaps.dwTotalSize = lpLineCaps->dwTotalSize;
	m_LineCaps.dwUsedSize = dwReqSize;
	m_LineCaps.dwNeededSize = dwReqSize + cbName + cbInfo + cbSwitchInfo + 
					m_LineCaps.dwTerminalCapsSize + m_LineCaps.dwTerminalTextSize +
					cbDeviceNameLen;

#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
    if (dwReqSize > lpLineCaps->dwTotalSize)
    {
		ASSERT (FALSE);
		lpLineCaps->dwNeededSize = m_LineCaps.dwNeededSize;
        return LINEERR_STRUCTURETOOSMALL;
    }
#endif

	// Copy over only what is allowed for the negotiated version of TAPI.
    CopyBuffer (lpLineCaps, &m_LineCaps, dwReqSize);
  
    // Remove the additional capabilities if we are not at the proper TSPI version.  
	if (dwTSPIVersion < TAPIVER_20)
	{
		lpLineCaps->dwBearerModes &= ~LINEBEARERMODE_RESTRICTEDDATA;
		lpLineCaps->dwLineFeatures &= ~(LINEFEATURE_SETDEVSTATUS | LINEFEATURE_FORWARDFWD | LINEFEATURE_FORWARDDND);      
	}

    if (dwTSPIVersion < TAPIVER_14)    
    {
		lpLineCaps->dwBearerModes &= ~LINEBEARERMODE_PASSTHROUGH;
        lpLineCaps->dwMediaModes &= ~LINEMEDIAMODE_VOICEVIEW;
		lpLineCaps->dwLineStates &= ~(LINEDEVSTATE_CAPSCHANGE |
				LINEDEVSTATE_CONFIGCHANGE |
				LINEDEVSTATE_TRANSLATECHANGE |
				LINEDEVSTATE_COMPLCANCEL |
				LINEDEVSTATE_REMOVED);
	}        
    
    // If we have enough room for the provider information, then add it to the end.
	if (!strProviderInfo.IsEmpty())
		AddDataBlock (lpLineCaps, lpLineCaps->dwProviderInfoOffset, lpLineCaps->dwProviderInfoSize,
					  (LPCTSTR)strProviderInfo);

    // If we have enough room for the line name, then add it.
	if (!strLineName.IsEmpty())
		AddDataBlock (lpLineCaps, lpLineCaps->dwLineNameOffset, lpLineCaps->dwLineNameSize,
					  (LPCTSTR)strLineName);
    
    // If we have enough room for the switch information, then add it.
	if (!strSwitchInfo.IsEmpty())
		AddDataBlock (lpLineCaps, lpLineCaps->dwSwitchInfoOffset, lpLineCaps->dwSwitchInfoSize,
					  (LPCTSTR)strSwitchInfo);
	
	// Handle the line terminal capabilities.
	lpLineCaps->dwTerminalCapsSize = 0L;
	for (i = 0; i < m_arrTerminalInfo.GetSize(); i++)
	{
		TERMINALINFO* lpTermInfo = (TERMINALINFO*) m_arrTerminalInfo[i];
		AddDataBlock (lpLineCaps, lpLineCaps->dwTerminalCapsOffset, 
			lpLineCaps->dwTerminalCapsSize, &lpTermInfo->Capabilities,
			sizeof(LINETERMCAPS));
	}

    // Add the terminal name information if we have the space.
	int iTermNameLen = (int)((lpLineCaps->dwTerminalTextEntrySize-sizeof(TCHAR)) / sizeof(TCHAR));
	lpLineCaps->dwTerminalTextSize = 0L;
    for (i = 0; i < m_arrTerminalInfo.GetSize(); i++)
    {
        TERMINALINFO* lpTermInfo = (TERMINALINFO*) m_arrTerminalInfo[i];
		CString strName = lpTermInfo->strName;
		if (strName.GetLength() < iTermNameLen)
			strName += CString(' ', iTermNameLen - strName.GetLength());
		AddDataBlock (lpLineCaps, lpLineCaps->dwTerminalTextOffset,
			lpLineCaps->dwTerminalTextSize, strName);
	}

	// If we have some lineGetID supported device classes,
	// return the list of supported device classes.
	if (dwTSPIVersion >= TAPIVER_20)
	{
		if (cbDeviceNameLen)
		{
			if (AddDataBlock (lpLineCaps, lpLineCaps->dwDeviceClassesOffset,
					      lpLineCaps->dwDeviceClassesSize, strDeviceNames))
			{
				// Remove the '~' chars and change to NULLs.
				wchar_t* lpBuff = (wchar_t*)((LPBYTE)lpLineCaps + lpLineCaps->dwDeviceClassesOffset);
				while (*lpBuff)
				{
					if (*lpBuff == L'~')
						*lpBuff = L'\0';
					lpBuff++;
				}
			}
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
	// Get the version the line opened negotiation to.
	DWORD dwTSPIVersion = GetNegotiatedVersion();

	// Synch access to object
	CEnterCode sLock(this);

	// Run through the addresses and see what media modes are available on outgoing lines.
	m_LineStatus.dwAvailableMediaModes = 0L;
	for (int x = 0; x < (int) GetAddressCount(); x++)
	{
		// If the call appearance can have another active call.. grab its available media modes
		CTSPIAddressInfo* pAddr = GetAddress((DWORD)x);
		if (pAddr->CanMakeCalls() &&
			pAddr->GetAddressStatus()->dwNumActiveCalls < pAddr->GetAddressCaps()->dwMaxNumActiveCalls)
		{
			m_LineStatus.dwAvailableMediaModes |= pAddr->GetAddressCaps()->dwAvailableMediaModes;
		}
	}

	// Move over the pre-filled fields.
	m_LineStatus.dwTotalSize = lpLineDevStatus->dwTotalSize;
	m_LineStatus.dwNeededSize = sizeof(LINEDEVSTATUS) + (m_arrTerminalInfo.GetSize() * sizeof(DWORD));
	m_LineStatus.dwNumOpens = lpLineDevStatus->dwNumOpens;
	m_LineStatus.dwOpenMediaModes = lpLineDevStatus->dwOpenMediaModes;

	// Determine the required size of our TAPI structure based on the caller and the version
	// the line originally negotiated at.
	if (dwTSPIVersion < TAPIVER_20)
		m_LineStatus.dwNeededSize -= sizeof(DWORD)*3;
	else
	{
		// Copy over the information which TAPI will supply.
		m_LineStatus.dwAppInfoSize = lpLineDevStatus->dwAppInfoSize;
		m_LineStatus.dwAppInfoOffset = lpLineDevStatus->dwAppInfoOffset;
	}

#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
    if (lpLineDevStatus->dwTotalSize < m_LineStatus.dwNeededSize)
	{
		ASSERT (FALSE);
		lpLineDevStatus->dwNeededSize = m_LineStatus.dwNeededSize;
        return LINEERR_STRUCTURETOOSMALL;
	}
#endif
    
    // Copy our structure onto the user structure
    CopyBuffer (lpLineDevStatus, &m_LineStatus, m_LineStatus.dwNeededSize);
    lpLineDevStatus->dwUsedSize = m_LineStatus.dwNeededSize;

    // Now fill in the additional fields.
	lpLineDevStatus->dwNumCallCompletions = m_lstCompletions.GetCount();
	                        
    // Fill in the terminal information if we have space.
    if (m_arrTerminalInfo.GetSize() > 0)
    {
        DWORD dwReqSize = m_arrTerminalInfo.GetSize() * sizeof(DWORD);
        if (lpLineDevStatus->dwTotalSize >= (dwReqSize + lpLineDevStatus->dwUsedSize))
        {
            for (int i = 0; i < m_arrTerminalInfo.GetSize(); i++)
            {
                TERMINALINFO* lpTermInfo = (TERMINALINFO*) m_arrTerminalInfo[i];
				AddDataBlock (lpLineDevStatus, lpLineDevStatus->dwTerminalModesOffset,
					          lpLineDevStatus->dwTerminalModesSize, &lpTermInfo->dwMode,
							  sizeof(DWORD));
            }                
        }
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

		// If no feature is specified, check the MAX number of
		// calls available.
		if (dwFeature == 0 &&
			(pAS->dwNumActiveCalls >= pAC->dwMaxNumActiveCalls))
			continue;
		
		// If the feature(s) are available
		if (dwFeature > 0 && (pAS->dwAddressFeatures & dwFeature) != dwFeature)
			continue;

        // And can support the type of call required..
        if (lpCallParams && pAddr->CanSupportCall(lpCallParams) != 0)
            continue;

		// This address fits the bill.
        return pAddr;
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
            LPTSTR lpBuff = (LPTSTR)lpCallParams + lpCallParams->dwOrigAddressOffset;
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
	// Make sure we can do this function now.
	if ((GetLineDevStatus()->dwLineFeatures & LINEFEATURE_MAKECALL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

	// Lock the line connection until we add the request to the list.
	// This is done specifically for the MAKECALL since it creates new call appearances
	// and there is a window of opportunity if multiple threads are attempting to make 
	// calls simultaneously where both will be submitted as asychronous requests.  To
	// stop this, we lock the object for updates until it is submitted.
	CEnterCode sLock (this, TRUE);

	// If we still have the bandwidth for one more call, but have a pending MAKECALL
	// request in our queue..
	if (m_LineCaps.dwMaxNumActiveCalls-1 == m_dwConnectedCallCount && 
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
                DTRACE(TRC_MIN, _T("lineMakeCall: invalid address id <%ld>\r\n"), lpMakeCall->lpCallParams->dwAddressID);
                return LINEERR_INVALADDRESSID;
            }
            
            // If the address currently doesn't support placing a 
			// call, then fail this attempt.
            if ((pAddr->GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_MAKECALL) == 0)
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
                LPCTSTR lpszAddress = (LPCTSTR)lpMakeCall->lpCallParams + lpMakeCall->lpCallParams->dwOrigAddressSize;
                for (int x = 0; x < (int) GetAddressCount(); x++)
                {
                    CTSPIAddressInfo* pAddr = GetAddress((DWORD)x);
                    if (_tcsicmp(pAddr->GetDialableAddress(), lpszAddress) == 0)
                    {
            			// If the address currently doesn't support placing a 
						// call, then fail this attempt.
            			if ((pAddr->GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_MAKECALL) == 0)
							return LINEERR_CALLUNAVAIL;
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
            DTRACE(TRC_MIN, _T("lineMakeCall: address explicitly specified does not exist\r\n"));
            return (lpMakeCall->lpCallParams->dwAddressMode == LINEADDRESSMODE_DIALABLEADDR) ?
                    LINEERR_INVALADDRESS : LINEERR_INVALADDRESSID;                
        }
    }

    // If they did not specify which call appearance to use by address, then locate one
    // which matches the specifications they desire from our service provider.
	CTSPIAddressInfo* pAddr = NULL;
    if (pCall == NULL)
    {
        pAddr = FindAvailableAddress (lpMakeCall->lpCallParams, LINEADDRFEATURE_MAKECALL);
        if (pAddr != NULL)
            pCall = pAddr->CreateCallAppearance(htCall);
    }
   
    // If there are no more call appearances, exit.
    if (pCall == NULL)
    {
        DTRACE(TRC_MIN, _T("lineMakeCall: no address available for outgoing call!\r\n"));
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
// CTSPILineConnection::GetAddressID
//
// This method returns the address ID associated with this line
// in the specified format.
//
LONG CTSPILineConnection::GetAddressID(
LPDWORD lpdwAddressId,                 // DWORD for return address ID
DWORD dwAddressMode,                   // Address mode in lpszAddress
LPCTSTR lpszAddress,                   // Address of the specified line
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
// CTSPILineConnection::GetTerminalInformation
//
// Return the current terminal modes for the specified terminal
// identifier.
//
DWORD CTSPILineConnection::GetTerminalInformation (int iTerminalID) const
{                                              
	if (iTerminalID >= 0 && iTerminalID < GetTerminalCount())
	{
		CEnterCode sLock(this);  // Synch access to object
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
int CTSPILineConnection::AddTerminal (LPCTSTR lpszName, LINETERMCAPS& Caps, DWORD dwModes)
{
	CEnterCode sLock(this);  // Synch access to object
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
    m_LineCaps.dwTerminalCapsSize = (m_LineCaps.dwNumTerminals * sizeof(LINETERMCAPS));

	// Set our new text entry size for the terminal if it exceeds the total size of
	// the biggest terminal name already in place.
	DWORD dwTextLen = (DWORD) ((lpTermInfo->strName.GetLength()+1) * sizeof(TCHAR));
	if (m_LineCaps.dwTerminalTextEntrySize < dwTextLen)
		m_LineCaps.dwTerminalTextEntrySize = dwTextLen;
	m_LineCaps.dwTerminalTextSize = (m_LineCaps.dwTerminalTextEntrySize * m_LineCaps.dwNumTerminals);
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
void CTSPILineConnection::RemoveTerminal (int iTerminalId)
{
	CEnterCode sLock(this);  // Synch access to object
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
        m_LineCaps.dwNumTerminals = (DWORD) GetTerminalCount();
		m_LineCaps.dwTerminalCapsSize = (m_LineCaps.dwNumTerminals * sizeof(LINETERMCAPS));
		m_LineCaps.dwTerminalTextSize = m_LineCaps.dwTerminalTextEntrySize * m_LineCaps.dwNumTerminals;

		if (m_LineCaps.dwNumTerminals == 0)
		    m_LineStatus.dwLineFeatures &= ~LINEFEATURE_SETTERMINAL;

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
void CTSPILineConnection::SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal)
{
	CEnterCode sLock(this);  // Synch access to object
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
// CTSPILineConnection::SetLineFeatures
//
// Used by the user to sets the current line features
//
void CTSPILineConnection::SetLineFeatures (DWORD dwFeatures)
{   
	CEnterCode sLock (this);

	// Make sure the capabilities structure reflects this ability.
	if ((m_LineCaps.dwLineFeatures & dwFeatures) == 0)
	{
		DTRACE(TRC_MIN, _T("LINEDEVCAPS.dwLineFeatures missing 0x%lx bit\r\n"), dwFeatures);
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
void CTSPILineConnection::OnLineCapabiltiesChanged()
{
	// Verify that we haven't REMOVED capabilities from the line
	// features.  If so, remove them from the status as well.
	m_LineStatus.dwLineFeatures &= m_LineCaps.dwLineFeatures;
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
void CTSPILineConnection::SetDeviceStatusFlags (DWORD dwStatus)
{   
	CEnterCode sLock (this);
    DWORD dwOldStatus = m_LineStatus.dwDevStatusFlags;
	DWORD dwNotify = 0;
    m_LineStatus.dwDevStatusFlags = dwStatus;         
	
    // Send TAPI the appropriate notifications.
    if ((dwOldStatus & LINEDEVSTATUSFLAGS_CONNECTED) &&
        (dwStatus & LINEDEVSTATUSFLAGS_CONNECTED) == 0)
        dwNotify |= LINEDEVSTATE_DISCONNECTED;
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_CONNECTED) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_CONNECTED))
        dwNotify |= LINEDEVSTATE_CONNECTED;
                                         
    if ((dwOldStatus & LINEDEVSTATUSFLAGS_MSGWAIT) &&
        (dwStatus & LINEDEVSTATUSFLAGS_MSGWAIT) == 0)
        dwNotify |= LINEDEVSTATE_MSGWAITOFF;
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_MSGWAIT) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_MSGWAIT))
        dwNotify |= LINEDEVSTATE_MSGWAITON;

    if ((dwOldStatus & LINEDEVSTATUSFLAGS_INSERVICE) &&
        (dwStatus & LINEDEVSTATUSFLAGS_INSERVICE) == 0)
        dwNotify |= LINEDEVSTATE_OUTOFSERVICE;
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_INSERVICE) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_INSERVICE))
        dwNotify |= LINEDEVSTATE_INSERVICE;

    if ((dwOldStatus & LINEDEVSTATUSFLAGS_LOCKED) &&
        (dwStatus & LINEDEVSTATUSFLAGS_LOCKED) == 0)
        dwNotify |= LINEDEVSTATE_LOCK;
    else if ((dwOldStatus & LINEDEVSTATUSFLAGS_LOCKED) == 0 &&    
        (dwStatus & LINEDEVSTATUSFLAGS_LOCKED))
        dwNotify |= LINEDEVSTATE_LOCK;

	// Inform TAPI about the changes in our device status.
	OnLineStatusChange (dwNotify);

	// Force the line to recalc its feature set.
	RecalcLineFeatures();

	// Force each of the address objects to recalc their feature set.
	for (int i = 0; i < (int) GetAddressCount(); i++)
		GetAddress(i)->RecalcAddrFeatures();

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
		// Make sure we can do this function now.
		if ((GetLineDevStatus()->dwLineFeatures & (LINEFEATURE_FORWARD|
				LINEFEATURE_FORWARDFWD|LINEFEATURE_FORWARDDND)) == 0)
			return LINEERR_OPERATIONUNAVAIL;

		CEnterCode Key (this);  // Synch access to object
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

		Key.Unlock();

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
void CTSPILineConnection::OnTimer()
{
	/* Do nothing */
    
}// CTSPILineConnection::OnTimer

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
    FreeMem ((LPTSTR)lpMyCallParams);
    
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
void CTSPILineConnection::SetMediaControl (TSPIMEDIACONTROL* lpMediaControl)
{   
    // We don't need to store this at the LINE level - since addresses are
    // static, we can simply pass it through them.
    for (int i = 0; i < m_arrAddresses.GetSize(); i++)
    {
        CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
        pAddr->SetMediaControl (lpMediaControl);
    }

}// CTSPILineConnection::SetMediaControl

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnRingDetected
//
// This method should be called by the service provider worker code
// when an incoming ring is detected on a line.
//
void CTSPILineConnection::OnRingDetected (DWORD dwRingMode, BOOL fFirstRing /*=FALSE*/)
{                                                                      
    // If our ring count array is empty, then grab the total number of
    // ring modes supported and add an entry for each.
    if (m_arrRingCounts.GetSize() < (int) m_LineCaps.dwRingModes)
    {
        for (int i = 0; i < (int)m_LineCaps.dwRingModes; i++)
            m_arrRingCounts.Add(0);
    }

	// Ring mode must be 1 to LineDevCaps.dwRingModes per TAPI spec. (2.23)
	ASSERT(dwRingMode > 0 && dwRingMode <= (DWORD) m_arrRingCounts.GetSize());

    // Grab the current ring count.
    UINT uiRingCount = (fFirstRing) ? 1 : m_arrRingCounts[dwRingMode-1]+1;
    m_arrRingCounts.SetAtGrow(dwRingMode-1, uiRingCount);

    // Notify TAPI about the ring.
    OnLineStatusChange (LINEDEVSTATE_RINGING, dwRingMode, (DWORD)uiRingCount);

}// CTSPILineConnection::OnRingDetected

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnRequestComplete
//
// This virtual method is called when an outstanding request on this
// line device has completed.  The return code indicates the success
// or failure of the request.  Note that the request will filter to
// the address and caller where available.
//
void CTSPILineConnection::OnRequestComplete(CTSPIRequest* pReq, LONG lResult)
{                                         
    WORD wRequest = pReq->GetCommand();
	CEnterCode Key (this, FALSE);
    
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
			Key.Lock();
            for (int i = 0; i < m_arrAddresses.GetSize(); i++)
            {
                CTSPIAddressInfo* pAddr = (CTSPIAddressInfo*) m_arrAddresses[i];
                pAddr->OnRequestComplete (pReq, lResult);
            }
			Key.Unlock();
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
            
			Key.Lock();
            m_lstCompletions.AddTail (pComplete);
			Key.Unlock();

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

	// If this is a lineSetMediaControl event, then store the new MediaControl
	// information in the line (and all of it's addresses).
	else if (wRequest == REQUEST_MEDIACONTROL)
	{
		if (lResult == 0 && pReq->GetAddressInfo() == NULL && pReq->GetCallInfo() == NULL)
			SetMediaControl((TSPIMEDIACONTROL*)pReq->GetDataPtr());
	}

}// CTSPILineConnection::OnRequestComplete 

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::FindCallCompletionRequest
//
// Return the TSPICOMPLETECALL ptr associated with a completion ID.
//
TSPICOMPLETECALL* CTSPILineConnection::FindCallCompletionRequest (DWORD dwSwitchInfo, LPCTSTR pszSwitchInfo)
{       
	CEnterCode sLock(this);  // Synch access to object
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
	CEnterCode sLock(this);  // Synch access to object
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
	CEnterCode sLock(this);  // Synch access to object
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
void CTSPILineConnection::RemoveCallCompletionRequest(DWORD dwCompletionID, BOOL fNotifyTAPI)
{                                
    // Locate the completion ID and delete it from our list.                   
	CEnterCode sLock(this);  // Synch access to object
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

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnCallDeleted
//
// A call is being deleted from some address our line owns.
//
void CTSPILineConnection::OnCallDeleted(CTSPICallAppearance* /*pCall*/)
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
    CTSPIAddressInfo* pAddr = pCall->GetAddressOwner();
	CEnterCode KeyAddr (pAddr);  // Synch access to object
    for (int iCall = 0; iCall < pAddr->GetCallCount(); iCall++)
    {
        CTSPICallAppearance* pThisCall = pAddr->GetCallInfo(iCall);
        if (pThisCall != NULL && pThisCall != pCall && pThisCall->GetCallType() == CALLTYPE_CONFERENCE &&
			(pThisCall->GetCallStatus()->dwCallState & (LINECALLSTATE_IDLE | LINECALLSTATE_DISCONNECTED)) == 0)
            return TRUE;
    }
    KeyAddr.Unlock();

    // None found there, check on the same line if cross-address conferencing is
    // available.
    if (m_LineCaps.dwDevCapFlags & LINEDEVCAPFLAGS_CROSSADDRCONF)
    {
		CEnterCode sLock(this);  // Synch access to object
        for (int iAddr = 0; iAddr < GetAddressCount(); iAddr++)
        {
            CTSPIAddressInfo* pThisAddr = GetAddress(iAddr);
            if (pThisAddr != pAddr)
            {
				CEnterCode KeyAddr (pThisAddr);
                for (iCall = 0; iCall < (int) pThisAddr->GetCallCount(); iCall++)
                {
                    CTSPICallAppearance* pThisCall = pThisAddr->GetCallInfo(iCall);
                    if (pThisCall != NULL && pThisCall->GetCallType() == CALLTYPE_CONFERENCE &&
						(pThisCall->GetCallStatus()->dwCallState & (LINECALLSTATE_IDLE | LINECALLSTATE_DISCONNECTED)) == 0)
                        return TRUE;
                }
				KeyAddr.Unlock();
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
		return ((pThisCall->GetCallState() & 
					(LINECALLSTATE_CONNECTED | 
					 LINECALLSTATE_RINGBACK |
					 LINECALLSTATE_BUSY |
					 LINECALLSTATE_PROCEEDING)) != 0);
    }               

	// Walk through all the addresses checking for calls on addresses which 
	// support creation of TRANSFER consultation calls and are in the proper
	// state.
    for (int iAddr = 0; iAddr < GetAddressCount(); iAddr++)
	{
		CTSPIAddressInfo* pAddr = GetAddress(iAddr);
		if ((pAddr->GetAddressCaps()->dwAddrCapFlags & LINEADDRCAPFLAGS_TRANSFERMAKE) != 0)
		{
			// Synch access to object - don't allow system lockup just for this.
			CEnterCode KeyAddr(pAddr, FALSE);
			if (KeyAddr.Lock(100))
			{
				for (int iCall = 0; iCall < pAddr->GetCallCount(); iCall++)
				{
					CTSPICallAppearance* pThisCall = pAddr->GetCallInfo(iCall);
					if (pThisCall != pCall && 
						((pThisCall->GetCallState() & 
								(LINECALLSTATE_CONNECTED | 
								 LINECALLSTATE_RINGBACK |
								 LINECALLSTATE_BUSY |
								 LINECALLSTATE_PROCEEDING)) != 0))
						return TRUE;
				}            
				KeyAddr.Unlock();
			}
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
	CEnterCode sLock(this);  // Synch access to object
    for (int j = 0; j < GetAddressCount(); j++)
    {
        CTSPIAddressInfo* pAddr = GetAddress(j);
        for (int i = 0; i < pAddr->GetCallCount(); i++)
        {
            CTSPICallAppearance* pThisCall = pAddr->GetCallInfo(i);
            if (dwState & pThisCall->GetCallState())
                return pThisCall;
        }            
    }       
    return NULL;

}// CTSPILineConnection::FindCallByState

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::CreateUIDialog
//
// Create a new dialog on a user thread.
//
HTAPIDIALOGINSTANCE CTSPILineConnection::CreateUIDialog (
DRV_REQUESTID dwRequestID,				// Request ID to work with
LPVOID lpParams,						// Parameter block
DWORD dwSize,							// Size of param block
LPCTSTR lpszUIDLLName/*=NULL*/)			// Different UI dll?
{
	// If no UI dll was passed, use our own DLL as the UI dll.
	CString strDLLName;
	if (lpszUIDLLName == NULL)
	{
		GetSystemDirectory (strDLLName.GetBuffer(_MAX_PATH+1), _MAX_PATH);
		strDLLName.ReleaseBuffer();
		if (strDLLName.Right(1) != _T('\\'))
			strDLLName += _T('\\');
		strDLLName += AfxGetAppName();
		lpszUIDLLName = (LPCTSTR) strDLLName;
	}

	// Allocate a new UI structure
	LINEUIDIALOG* pLineDlg = new LINEUIDIALOG;

	// Ask TAPI for a UI dialog event.
	TUISPICREATEDIALOGINSTANCEPARAMS Params;
	Params.dwRequestID = dwRequestID;
	Params.hdDlgInst = (HDRVDIALOGINSTANCE) pLineDlg;
	Params.htDlgInst = NULL;
#ifndef UNICODE
	LPWSTR lpszUIBuff = NULL;
	int iSize = MultiByteToWideChar(CP_ACP, 0, lpszUIDLLName, -1, NULL, 0) * sizeof(WCHAR);
	if (iSize > 0)
	{
		lpszUIBuff = (LPWSTR) AllocMem (iSize);
		MultiByteToWideChar (CP_ACP, 0, lpszUIDLLName, -1, lpszUIBuff, iSize);
	}
	Params.lpszUIDLLName = lpszUIBuff;
#else
	Params.lpszUIDLLName = (LPCWSTR) lpszUIDLLName;
#endif
	Params.lpParams = lpParams;
	Params.dwSize = dwSize;

	Send_TAPI_Event (NULL, LINE_CREATEDIALOGINSTANCE, (DWORD)&Params, 0L, 0L);

#ifndef UNICODE
	if (lpszUIBuff)
		FreeMem (lpszUIBuff);
#endif

	if (Params.htDlgInst == NULL)
	{
		DTRACE(TRC_MIN, _T("Failed to create UI dialog for request ID 0x%lx\r\n"), dwRequestID);
		delete pLineDlg;
	}
	else
	{
		DTRACE(TRC_MIN, _T("New UI dialog created TAPI=0x%lx, SP=0x%lx\r\n"), (DWORD) Params.htDlgInst, (DWORD) Params.hdDlgInst);
		pLineDlg->pLineOwner = this;
		pLineDlg->dwRequestID = dwRequestID;
		pLineDlg->htDlgInstance = Params.htDlgInst;
		
		CEnterCode sLock(this);  // Synch access to object
		m_arrUIDialogs.Add(pLineDlg);
	}

	return Params.htDlgInst;

}// CTSPILineConnection::CreateUIDialog

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetUIDialog
//
// Retrieve a UI dialog instance data structure based on a TAPI
// handle.
//
LINEUIDIALOG* CTSPILineConnection::GetUIDialog (HTAPIDIALOGINSTANCE htDlgInst)
{
	CEnterCode sLock(this);  // Synch access to object
	for (int i = 0; i < m_arrUIDialogs.GetSize(); i++)
	{
		LINEUIDIALOG* pLineDlg = (LINEUIDIALOG*) m_arrUIDialogs[i];
		if (pLineDlg != NULL && pLineDlg->htDlgInstance == htDlgInst)
			return pLineDlg;
	}
	return NULL;

}// CTSPILineConnection::GetUIDialog

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::FreeDialogInstance
//
// Destroy a user-interface dialog for this line connection.  In general,
// don't call this function from user-level code.
//
LONG CTSPILineConnection::FreeDialogInstance(HTAPIDIALOGINSTANCE htDlgInst)
{
	CEnterCode sLock(this);  // Synch access to object
	for (int i = 0; i < m_arrUIDialogs.GetSize(); i++)
	{
		LINEUIDIALOG* pLineDlg = (LINEUIDIALOG*) m_arrUIDialogs[i];
		if (pLineDlg != NULL && pLineDlg->htDlgInstance == htDlgInst)
		{
			// Remove it from our array.
			m_arrUIDialogs.RemoveAt(i);
			delete pLineDlg;
			break;
		}
	}

	return FALSE;

}// CTSPILineConnection::FreeDialogInstance

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetLineDevStatus
//
// Change the line device status according to what TAPI
// has requested.  Derived provider should use the "SetDeviceStatusFlags"
// to actually set the flags.
//
LONG CTSPILineConnection::SetLineDevStatus (DRV_REQUESTID dwRequestID,
						DWORD dwStatusToChange, BOOL fSet)
{
	// Make sure we can do this function now.
	if ((GetLineDevStatus()->dwLineFeatures & LINEFEATURE_SETDEVSTATUS) == 0)
		return LINEERR_OPERATIONUNAVAIL;

	// If this isn't one of the supported status bits, error out.
	if ((dwStatusToChange & m_LineCaps.dwSettableDevStatus) == 0)
		return LINEERR_INVALPARAM;

	// Simply submit the request and let the derived provider set the appropriate bits.
    if (AddAsynchRequest(NULL, REQUEST_SETDEVSTATUS, dwRequestID, (LPVOID)dwStatusToChange, (DWORD)fSet))
        return (LONG) dwRequestID;
    
    return LINEERR_OPERATIONFAILED;
 
}// CTSPILineConnection::SetLineDevStatus

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetTerminal
//
// Redirect the flow of specifiec terminal events to a destination
// terminal for all calls on this line.
//
LONG CTSPILineConnection::SetTerminal (DRV_REQUESTID dwRequestID, 
									   TSPILINESETTERMINAL* lpLine)
{
	// Make sure we can do this function now.
	if ((GetLineDevStatus()->dwLineFeatures & LINEFEATURE_SETTERMINAL) == 0)
		return LINEERR_OPERATIONUNAVAIL;

    // Submit the request.
    if (AddAsynchRequest(NULL, REQUEST_SETTERMINAL, dwRequestID, lpLine) != NULL)
        return dwRequestID;
    return LINEERR_OPERATIONFAILED;

}// CTSPILineConnection::SetTerminal

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SetRingMode
//
// Set the ring mode for this line.
//
void CTSPILineConnection::SetRingMode (DWORD dwRingMode)
{
	CEnterCode sLock(this);  // Synch access to object
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
void CTSPILineConnection::SetBatteryLevel (DWORD dwBattery)
{
    if (dwBattery > 0xffff)
        dwBattery = 0xffff;
	CEnterCode sLock(this);  // Synch access to object
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
void CTSPILineConnection::SetSignalLevel (DWORD dwSignal)
{
    if (dwSignal > 0xffff)
        dwSignal = 0xffff;
	CEnterCode sLock(this);  // Synch access to object
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
void CTSPILineConnection::SetRoamMode (DWORD dwRoamMode)
{
	CEnterCode sLock(this);  // Synch access to object
	if (m_LineStatus.dwRoamMode != dwRoamMode)
	{
		m_LineStatus.dwRoamMode = dwRoamMode;
		OnLineStatusChange (LINEDEVSTATE_ROAMMODE);
	}

}// CTSPILineConnection::SetRoamMode

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

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetID
//
// Manage device-level requests for information based on a device id.
//
LONG CTSPILineConnection::GetID (CString& strDevClass, LPVARSTRING lpDeviceID,
								HANDLE hTargetProcess)
{
	DEVICECLASSINFO* pDeviceClass = GetDeviceClass(strDevClass);
	if (pDeviceClass != NULL)
		return GetSP()->CopyDeviceClass (pDeviceClass, lpDeviceID, hTargetProcess);
	return LINEERR_INVALDEVICECLASS;
    
}// CTSPILineConnection::GetID

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetDevConfig
//
// Return the device configuration for this line and device class.
//
LONG CTSPILineConnection::GetDevConfig(CString& /*strDeviceClass*/, 
												  LPVARSTRING /*lpDeviceConfig*/)
{   
    // Derived class needs to supply the data structures for this based on what may be configured.                                 
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
// CTSPILineConnection::GenericDialogData
//
// This method is called when a dialog which sent in a LINE
// device ID called our UI callback.
//
LONG CTSPILineConnection::GenericDialogData (LINEUIDIALOG* /*pLineDlg*/, LPVOID /*lpParam*/, 
											 DWORD /*dwSize*/)
{
	return FALSE;

}// CTSPILineConnection::GenericDialogData

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
void CTSPILineConnection::OnConnectedCallCountChange(CTSPIAddressInfo* /*pInfo*/, int iDelta)
{
	CEnterCode sLock (this);
	
	// Get a new count.
	m_dwConnectedCallCount += iDelta;
	DTRACE(TRC_MIN, _T("OnConnectedCallCountChange: Delta=%d, New Count=%ld\r\n"), iDelta, m_dwConnectedCallCount);

	// Now adjust our LINE features based on the total counts.
	RecalcLineFeatures();

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

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::OnMediaControl
//
// This method is called when a media control event was activated
// due to a media monitoring event being caught on this call.
//
void CTSPILineConnection::OnMediaControl (CTSPICallAppearance* /*pCall*/, DWORD /*dwMediaControl*/)
{                                      
	// User must override this or the CTSPICallAppearance::OnMediaControl
	// and perform action on the media event.
	ASSERT (FALSE);

}// CTSPILineConnection::OnMediaControl

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::RecalcLineFeatures
//
// This is called when the line device status changes to recalc our
// feature set based on the status and address/call information
//
void CTSPILineConnection::RecalcLineFeatures()
{
	DWORD dwFeatures = m_LineStatus.dwLineFeatures;
    DWORD dwStatus = m_LineStatus.dwDevStatusFlags;

	// If we are NOT in-service now, we have NO line features.
	if ((dwStatus & 
			(LINEDEVSTATUSFLAGS_INSERVICE | LINEDEVSTATUSFLAGS_CONNECTED)) != 
			(LINEDEVSTATUSFLAGS_INSERVICE | LINEDEVSTATUSFLAGS_CONNECTED))
		dwFeatures = 0;
	else
	{
		dwFeatures = m_LineCaps.dwLineFeatures & 
			(LINEFEATURE_DEVSPECIFIC |
			 LINEFEATURE_DEVSPECIFICFEAT |
			 LINEFEATURE_FORWARD |
			 LINEFEATURE_MAKECALL |
			 LINEFEATURE_SETMEDIACONTROL |
			 LINEFEATURE_SETDEVSTATUS |
			 LINEFEATURE_FORWARDFWD |
			 LINEFEATURE_FORWARDDND);

		// If we have the bandwidth for a call, 
		if (m_dwConnectedCallCount < m_LineCaps.dwMaxNumActiveCalls)
			dwFeatures |= LINEFEATURE_MAKECALL;
		else
			dwFeatures &= ~LINEFEATURE_MAKECALL;

		// If we have terminals, allow SETTERMINAL.
		if (GetTerminalCount() > 0)
			dwFeatures |= (m_LineCaps.dwLineFeatures & LINEFEATURE_SETTERMINAL);
	}

	dwFeatures &= m_LineCaps.dwLineFeatures;
    SetLineFeatures (OnLineFeaturesChanged(dwFeatures));

}// CTSPILineConnection::RecalcLineFeatures
