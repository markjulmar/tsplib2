/******************************************************************************/
//                                                                        
// LINE.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our line override code for our ATSP sample.
// 
// This service provider drives a Hayes compatible AT style modem.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
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
#include "resource.h"
#include "atsp.h"
#include "talkdrop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE (CATSPLine, CTSPILineConnection)

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::CATSPLine
//
// Constructor for the line device.
//
CATSPLine::CATSPLine() : m_evtClose (FALSE, TRUE)
{
	m_hdlgTalkDrop = NULL;
	m_hComm = INVALID_HANDLE_VALUE;

}// CATSPLine::CATSPLine

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::~CATSPLine
//
// Destructor for the line device.
//
CATSPLine::~CATSPLine()
{
	// If we have an OPEN comm port, then close it all down.
	if (m_hComm != INVALID_HANDLE_VALUE)
        CloseDevice();

}// CATSPLine::~CATSPLine

/////////////////////////////////////////////////////////////////////////////////////
// CATSPLine::Init
//
// Initialize the line object
//
VOID CATSPLine::Init(CTSPIDevice* pDev, DWORD dwLineDeviceID, DWORD dwPos, DWORD /*dwItemData*/)
{
	// Let the base class initialize first.
	CTSPILineConnection::Init (pDev, dwLineDeviceID, dwPos);

	// Turn on our internal timer, 1 second intervals
	pDev->SetIntervalTimer(1000);

    // Set the name associated with this line.  This is optional, it gives
    // the user a displayable name which is associated with the line.  Most
	// applications use this name in their UI.
	CString strName;
	strName.Format(GetString(IDS_LINENAME), dwPos);
    SetName (strName);

	// Read the registry settings for this line.
	ReadRegistrySettings();

    // Add our line device capabilities for this line.  We no longer have to override
    // lineGetDevCaps; simply set it into the structure and the base class will handle it.
    LPLINEDEVCAPS lpDevCaps = GetLineDevCaps();
    lpDevCaps->dwAnswerMode = LINEANSWERMODE_DROP;
    lpDevCaps->dwDevCapFlags |= (LINEDEVCAPFLAGS_CLOSEDROP | LINEDEVCAPFLAGS_DIALQUIET | LINEDEVCAPFLAGS_DIALDIALTONE);
    lpDevCaps->dwBearerModes |= LINEBEARERMODE_PASSTHROUGH;
        
    // Setup our DIALPARAMS
    lpDevCaps->DefaultDialParams.dwDialPause = DEF_DIALPAUSE;
    lpDevCaps->DefaultDialParams.dwDialSpeed = DEF_DIALSPEED;
    lpDevCaps->DefaultDialParams.dwDigitDuration = DEF_DURATION;
    lpDevCaps->DefaultDialParams.dwWaitForDialtone = DEF_DIALTONE;
    lpDevCaps->MinDialParams.dwDialPause = MIN_DIALPAUSE;
    lpDevCaps->MinDialParams.dwDialSpeed = MIN_DIALSPEED;
    lpDevCaps->MinDialParams.dwDigitDuration = MIN_DURATION;
    lpDevCaps->MinDialParams.dwWaitForDialtone = MIN_DIALTONE;
    lpDevCaps->MaxDialParams.dwDialPause = MAX_DIALPAUSE;
    lpDevCaps->MaxDialParams.dwDialSpeed = MAX_DIALSPEED;
    lpDevCaps->MaxDialParams.dwDigitDuration = MAX_DURATION;
    lpDevCaps->MaxDialParams.dwWaitForDialtone = MAX_DIALTONE;

}// CATSPLine::Init

////////////////////////////////////////////////////////////////////////////
// CATSPLine::ReadRegistrySettings
//
// Read our settings from the registry and optionally create the 
// address associated with them.
//
BOOL CATSPLine::ReadRegistrySettings()
{
	BOOL fAvailable = TRUE;

	// Read the COMM port from the registry.
	CString strCOMM = GetSP()->ReadProfileString(GetPermanentDeviceID(), GetString(IDS_COMMPORT));
	fAvailable = (strCOMM.IsEmpty() == FALSE);

	// Setup our device class information
	if (fAvailable)
	{
		// Setup the lineGetID for COMM and COMM/DATAMODEM for the line.
		// This will affect all addresses added to the line too.
		if (!_tcsnicmp((LPCTSTR)strCOMM, _T("COM"), 3))
			AddDeviceClass (_T("comm"),(DWORD)atoi(strCOMM.Mid(3)));
		AddDeviceClass (_T("comm/datamodem/portname"), (LPCSTR)strCOMM);
		SetModemHandle();
	}
	else
	{
		RemoveDeviceClass (_T("comm"));
		RemoveDeviceClass (_T("comm/datamodem"));
		RemoveDeviceClass (_T("comm/datamodem/portname"));
	}

    // Get the address we have setup for this line.
    CString strDialableAddr = GetSP()->ReadProfileString(GetPermanentDeviceID(), GetString(IDS_LINEADDR));
    if (strDialableAddr.IsEmpty())
        strDialableAddr = _T("Address0");
    
    // See if we have an address name present.
    CString strName = GetSP()->ReadProfileString (GetPermanentDeviceID(), GetString(IDS_ADDRNAME));

    // Add the address - it should be ADDRESSID zero.
	if (GetAddressCount() == 0)
		VERIFY (CreateAddress(strDialableAddr, strName, TRUE, TRUE,
							  (LINEMEDIAMODE_INTERACTIVEVOICE | LINEMEDIAMODE_DATAMODEM), 
							  LINEBEARERMODE_VOICE, MIN_DATARATE, MAX_DATARATE) == 0);

	// Otherwise simply adjust the current address settings.
	else
	{
		CTSPIAddressInfo* pAddress = GetAddress ((DWORD)0);
		ASSERT (pAddress != NULL);
		pAddress->SetName (strName);
		pAddress->SetDialableAddress(strDialableAddr);
	}

	return fAvailable;

}// CATSPLine::ReadRegistrySettings

////////////////////////////////////////////////////////////////////////////
// CATSPLine::GetDevConfig
//
// Overriden to supply configuration information about our modem.
//
LONG CATSPLine::GetDevConfig(CString& strDeviceClass, LPVARSTRING lpDeviceConfig)
{
	if (strDeviceClass == _T("comm/datamodem"))
	{
		// We fill it in with junk.. some MODEM applications expect this
		// structure and won't talk to us unless we provide this function.
		lpDeviceConfig->dwUsedSize = lpDeviceConfig->dwNeededSize = sizeof(VARSTRING)+1;
		lpDeviceConfig->dwStringFormat = STRINGFORMAT_BINARY;
		lpDeviceConfig->dwStringOffset = sizeof(VARSTRING);
		lpDeviceConfig->dwStringSize = 1;
		return FALSE;
	}
	return LINEERR_INVALDEVICECLASS;

}// CATSPLine::GetDevConfig

////////////////////////////////////////////////////////////////////////////
// CATSPLine::SetDevConfig
//
// Overriden to supply configuration information about our modem.
//
LONG CATSPLine::SetDevConfig(CString& strDeviceClass, LPVOID const /*lpDevConfig*/, DWORD /*dwSize*/)
{
	if (strDeviceClass == _T("comm/datamodem"))
		return FALSE;
	return LINEERR_INVALDEVICECLASS;

}// CATSPLine::SetDevConfig

////////////////////////////////////////////////////////////////////////////
// CATSPLine::OnTimer
//
// This message is called by our timer thread in the class library
// at the provider interval timer rate.
//
VOID CATSPLine::OnTimer()
{               
    ReceiveData ((DWORD)MODEM_INTERVAL_TIMER);

}// CATSPLine::OnTimer

////////////////////////////////////////////////////////////////////////////
// CATSPLine::ReceiveData
//
// This method is called in three conditions:
//
// 1. A string was received from our input thread.
// 2. An interval timer was received by our OnTimer() method.
// 3. A new command was inserted into our request queue with no 
//    pending requests
//
BOOL CATSPLine::ReceiveData (DWORD dwData, const LPVOID /*lpBuff*/, DWORD /*dwSize*/)
{
    BYTE bReply = (BYTE) dwData;

	// If this is a RING event, then start a new call.
	if (bReply == MODEM_RING)
		processRingEvent();
	else
	{
		// Grab the current request from our request list.
		CTSPIRequest* pReq = GetCurrentRequest();
		if (pReq != NULL)
		{
			// Determine which command is being executed.  We only support a
			// minimum service provider, which has very few request types.
			switch (pReq->GetCommand())
			{
				case REQUEST_ACCEPT:
					CompleteCurrentRequest(0);
					break;

				case REQUEST_ANSWER:
					processAnswer(pReq, bReply);
					break;

				case REQUEST_MAKECALL:      
					processMakeCall(pReq, bReply); 
					break;
            
				case REQUEST_DIAL:          
					processDial(pReq, bReply); 
					break;
        
				case REQUEST_SETCALLPARAMS:
					processSetCallParams(pReq, bReply);
					break;
            
				case REQUEST_DROPCALL:      
					processDropCall(pReq, bReply); 
					break;
            
				default:                    
					ASSERT(FALSE); 
					break;
			}
		}

		// Look for OFFERING calls with no more RING events
		else if (bReply == MODEM_INTERVAL_TIMER)
		{
			// If we haven't seen activity from the modem in 8 seconds, and
			// we have an offering call, then kill it.
			if (m_dwLastActivity + 8000L < GetTickCount())
			{
				CTSPICallAppearance* pCall = GetAddress((DWORD)0L)->FindCallByState(LINECALLSTATE_OFFERING);
				if (pCall)
					pCall->SetCallState(LINECALLSTATE_IDLE);
			}
		}
	}

    // The request is directed directly here since we have a dedicated
	// thread servicing our input.
    return TRUE;

}// CATSPLine::ReceiveData

////////////////////////////////////////////////////////////////////////////
// CATSPLine::BuildInitString
//
// Build the INIT string to send to our modem device.
//
CString CATSPLine::BuildInitString(TSPIMAKECALL* lpMakeCall, BOOL fUseSecs)
{                      
    LPLINEDEVCAPS lpDevCaps = GetLineDevCaps();
    CString strInit = GetSP()->ReadProfileString (GetPermanentDeviceID(), GetString(IDS_INITSTR));
    
    // Get the alterable values, use the defaults - replace with the passed call
    // parameters if they are there.
    DWORD dwDigitDuration = lpDevCaps->DefaultDialParams.dwDigitDuration;
    DWORD dwDialPause = lpDevCaps->DefaultDialParams.dwDialPause;
    DWORD dwWaitForDialtone = lpDevCaps->DefaultDialParams.dwWaitForDialtone;
    
    if (lpMakeCall->lpCallParams && 
        !IsBadReadPtr (lpMakeCall->lpCallParams, sizeof(LINECALLPARAMS)))
    {
        dwDigitDuration = lpMakeCall->lpCallParams->DialParams.dwDigitDuration;
        dwDialPause = lpMakeCall->lpCallParams->DialParams.dwDialPause;
        dwWaitForDialtone = lpMakeCall->lpCallParams->DialParams.dwWaitForDialtone;
    }

    // If the modem uses SECONDS for dialtone and pauses, then convert it.
	if (fUseSecs)
	{
		dwDialPause /= 1000;
		dwWaitForDialtone /= 1000;
	}

    // Build our string
	CString strBuff;
	strBuff.Format (GetString(IDS_MODEMINIT), dwDigitDuration, dwDialPause, 
					dwWaitForDialtone, (LPCTSTR)strInit);
    return strBuff;

}// CATSPLine::BuildInitString    

////////////////////////////////////////////////////////////////////////////
// CATSPLine::BuildDialString
//
// Build the required DIAL string for this provider.  This is an 
// extemely basic version of a dial string builder.  
// Pulse/Tone is ignored in the dial string.
//
CString CATSPLine::BuildDialString (CTSPICallAppearance* pCall, 
                                    DIALINFO* pDialInfo, DWORD /*dwCountryCode*/)
{   
	// Get the PULSE/TONE setting
	CString strBuff = GetSP()->ReadProfileString (GetPermanentDeviceID(), GetString(IDS_PULSE));
    int fPulse = (strBuff[0] == '1');

    // Get the NUMBERS in our dialable address, allow the pause, dialtone,
    // hookswitch and wait to be filtered through.
    CString strAddr = GetSP()->GetDialableNumber (pDialInfo->strNumber, _T(",!W@"));
	
    // If we don't have a number to dial to, then simply go offhook.
    if (!strAddr.IsEmpty())
    {                                  
		strBuff.Format(GetString(IDS_DIAL), (fPulse) ? _T('P') : _T('T'), 
						(LPCTSTR)strAddr);
        // If the media type is interactive voice, then add a semicolon to
        // the end of the dialstring to tell the modem to return to command mode.
        if (pCall->GetCallInfo()->dwMediaMode == LINEMEDIAMODE_INTERACTIVEVOICE)
            strBuff += _T(";\r");
        else
            strBuff += _T("\r");
    }
	else
		strBuff = GetString(IDS_OFFHOOK);
        
    return strBuff;    

}// CATSPLine::BuildDialString

////////////////////////////////////////////////////////////////////////////
// CATSPLine::CreateTalkDropDialog
//
// Create a talk drop dialog instance on the specified requester
// application.
//
VOID CATSPLine::CreateTalkDropDialog(CTSPIRequest* pRequest)
{
	CTSPICallAppearance* pCall = pRequest->GetCallInfo();
	if (pCall && pCall->GetCallInfo()->dwMediaMode & LINEMEDIAMODE_INTERACTIVEVOICE)
		m_hdlgTalkDrop = CreateUIDialog (pRequest->GetAsynchRequestId(), NULL, 0L);

}// CATSPLine::CreateTalkDropDialog

////////////////////////////////////////////////////////////////////////////
// CATSPLine::ShowTalkDropDialog
//
// Show (display) the talk/drop dialog.  This will cause the Talk/Drop
// dialog to show itself.
//
VOID CATSPLine::ShowTalkDropDialog()
{   
	if (m_hdlgTalkDrop)
	{
		DWORD dwCommand = TALKDROP_SHOW;
		SendDialogInstanceData (m_hdlgTalkDrop, &dwCommand, sizeof(DWORD));
	}

}// CATSPLine::ShowTalkDropDialog

////////////////////////////////////////////////////////////////////////////
// CATSPLine::DismissTalkDropDialog
//
// Destroy the talk/drop dialog.
//
VOID CATSPLine::DismissTalkDropDialog()
{   
	if (m_hdlgTalkDrop)
	{
		DWORD dwCommand = TALKDROP_DESTROY;
		SendDialogInstanceData (m_hdlgTalkDrop, &dwCommand, sizeof(DWORD));
		m_hdlgTalkDrop = NULL;
	}
	
}// CATSPLine::DismissTalkDropDialog
