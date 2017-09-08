/******************************************************************************/
//                                                                        
// PHONECONN.CPP - Source code for the CTSPIPhoneConnection object        
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code to manage the phone objects which are  
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

IMPLEMENT_DYNCREATE( CTSPIPhoneConnection, CTSPIConnection )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::CTSPIPhoneConnection
//
// Constructor for the phone connection device
//
CTSPIPhoneConnection::CTSPIPhoneConnection() :
	m_lpfnEventProc(NULL), m_htPhone(0), m_dwPhoneStates(0),
	m_dwButtonModes(0), m_dwButtonStates(0)
{
    FillBuffer (&m_PhoneCaps, 0, sizeof(PHONECAPS));
    FillBuffer (&m_PhoneStatus, 0, sizeof(PHONESTATUS));

}// CTSPIPhoneConnection::CTSPIPhoneConnection

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::~CTSPIPhoneConnection
//
// Destructor
//
CTSPIPhoneConnection::~CTSPIPhoneConnection()
{
    /* Do nothing */
    
}// CTSPIPhoneConnection::~CTSPIPhoneConnection

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::Init
//
// Initialize the phone connection object
//
void CTSPIPhoneConnection::Init(CTSPIDevice* pDevice, DWORD dwPhoneId, DWORD dwPos, DWORD /*dwItemData*/)
{
    CTSPIConnection::Init(pDevice, dwPhoneId);

    // Initialize the phone device capabilities.  The permanent phone id
    // is a combination of our device id plus the position of the phone itself
    // within our device array.  This uniquely identifies the phone to us
    // with a single DWORD.  The MSB of the loword is always one, this indicates
    // a phone vs. line.
    m_PhoneCaps.dwPermanentPhoneID = ((pDevice->GetProviderID() << 16) + (dwPos&0x7fff)) | 0x8000;

#ifdef _UNICODE
    m_PhoneCaps.dwStringFormat = STRINGFORMAT_UNICODE;
#else
    m_PhoneCaps.dwStringFormat = STRINGFORMAT_ASCII;
#endif
    
    // Add all the phone capabilities since we can notify TAPI about any of
    // these changing.  With some providers, they may not change, and that is O.K.
    m_PhoneCaps.dwPhoneStates = (PHONESTATE_OTHER | PHONESTATE_CONNECTED | 
                                 PHONESTATE_DISCONNECTED | PHONESTATE_DISPLAY | PHONESTATE_LAMP |
                                 PHONESTATE_RINGMODE | PHONESTATE_RINGVOLUME | PHONESTATE_HANDSETHOOKSWITCH |
                                 PHONESTATE_HANDSETGAIN | PHONESTATE_SPEAKERHOOKSWITCH | PHONESTATE_SPEAKERGAIN |
                                 PHONESTATE_SPEAKERVOLUME | PHONESTATE_HANDSETVOLUME | PHONESTATE_HEADSETHOOKSWITCH |
                                 PHONESTATE_HEADSETVOLUME | PHONESTATE_HEADSETGAIN | PHONESTATE_SUSPEND |
                                 PHONESTATE_RESUME | PHONESTATE_CAPSCHANGE);
    
    // If the device supports more than one ring mode, then change this during INIT through
    // the GetPhoneCaps() API.
    m_PhoneCaps.dwNumRingModes = 1;                                 
    
    m_PhoneStatus.dwStatusFlags = PHONESTATUSFLAGS_CONNECTED;
    m_PhoneStatus.dwRingMode = 0L;
    m_PhoneStatus.dwRingVolume = 0xffff;

	// Add in the phone features.  Others will be added as the various
	// AddXXX functions are invoked.
	m_PhoneCaps.dwPhoneFeatures = 0;
	if (CanHandleRequest(TSPI_PHONEGETRING))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETRING;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETRING;
	}
	if (CanHandleRequest(TSPI_PHONESETRING))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETRING;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETRING;
	}

	// Add in the "tapi/phone" device class.
	AddDeviceClass (_T("tapi/phone"), GetDeviceID());
	AddDeviceClass (_T("tapi/providerid"), GetDeviceInfo()->GetProviderID());

}// CTSPIPhoneConnection::Init

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetPermanentDeviceID
//
// Return a permanent device id for this phone identifying the provider
// and phone.
//
DWORD CTSPIPhoneConnection::GetPermanentDeviceID() const
{
    return m_PhoneCaps.dwPermanentPhoneID;

}// CTSPIPhoneConnection::GetPermanentDeviceID

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetupDisplay
//
// Setup the display device for this phone.  If there is no
// display, then don't call this function.
//
void CTSPIPhoneConnection::SetupDisplay (int iColumns, int iRows, char cLineFeed)
{                                     
	CEnterCode sLock(this);  // Synch access to object
    m_Display.Init (iColumns, iRows, cLineFeed);
    m_PhoneCaps.dwDisplayNumRows = (DWORD) iRows;
    m_PhoneCaps.dwDisplayNumColumns = (DWORD) iColumns;
	
	// Adjust our phone capabilities.
	if (CanHandleRequest(TSPI_PHONEGETDISPLAY))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETDISPLAY;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETDISPLAY;
	}
	if (CanHandleRequest(TSPI_PHONESETDISPLAY))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETDISPLAY;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETDISPLAY;
	}

}// CTSPIPhoneConnection::SetupDisplay

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddUploadBuffer
//
// This function adds an upload data buffer to the phone.
//
int CTSPIPhoneConnection::AddUploadBuffer (DWORD dwSize)
{
    // Add it to our buffer array.
	CEnterCode sLock(this);  // Synch access to object
    int iPos = m_arrUploadBuffers.Add (dwSize);
    
    // Add a buffer count to our list.
    m_PhoneCaps.dwNumGetData++;
    ASSERT (iPos+1 == (int) m_PhoneCaps.dwNumGetData);
	if (CanHandleRequest(TSPI_PHONEGETDATA))
	{
	    m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETDATA;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETDATA;
	}
    return iPos;
    
}// CTSPIPhoneConnection::AddUploadBuffer

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddDownloadBuffer
//
// This function adds a download data buffer to the phone.
//
int CTSPIPhoneConnection::AddDownloadBuffer (DWORD dwSize)
{
    // Add it to our buffer array.
	CEnterCode sLock(this);  // Synch access to object
    int iPos = m_arrDownloadBuffers.Add (dwSize);
    
    // Add a buffer count to our list.
    m_PhoneCaps.dwNumSetData++;
    ASSERT (iPos+1 == (int)m_PhoneCaps.dwNumSetData);
	if (CanHandleRequest(TSPI_PHONESETDATA))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETDATA;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETDATA;
	}    
    return iPos;
    
}// CTSPIPhoneConnection::AddDownloadBuffer

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddHookSwitchDevice
//
// Add a hookswitch device to our phone object.  This should be completed
// at INIT time (providerInit).    
//
// Pass in a (-1L) for volume/gain if volume/gain changes are not supported.
//
void CTSPIPhoneConnection::AddHookSwitchDevice (DWORD dwHookSwitchDev, 
					 DWORD dwAvailModes, DWORD dwCurrMode, 
					 DWORD dwVolume, DWORD dwGain,
					 DWORD dwSettableModes, DWORD dwMonitoredModes)
{   
    BOOL fSupportsVolumeChange = FALSE, fSupportsGainChange = FALSE;
                                             
	// Set our volume flags based on whether the volume was passed into 
	// the function or came in from the default parameters.
    if (dwVolume != (DWORD)-1L)
        fSupportsVolumeChange = TRUE;
    else
        dwVolume = 0xffff;

	// Same with the gain.
    if (dwGain != (DWORD)-1L)
        fSupportsGainChange = TRUE;
    else
        dwGain = 0xffff;            

	// Backward compatibility:
	if (dwSettableModes == -1L)
		dwSettableModes = dwAvailModes;
	if (dwMonitoredModes == -1L)
		dwMonitoredModes = dwAvailModes;

	// Synch access to object
	CEnterCode sLock(this);

    if (dwHookSwitchDev == PHONEHOOKSWITCHDEV_HANDSET)
    {
        m_PhoneCaps.dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HANDSET;
		if (CanHandleRequest(TSPI_PHONEGETHOOKSWITCH))
		{
			m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHHANDSET;
			m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHHANDSET;
		}		
		if (CanHandleRequest (TSPI_PHONESETHOOKSWITCH) && dwSettableModes)
		{
			m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETHOOKSWITCHHANDSET;
			m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETHOOKSWITCHHANDSET;
		}
        if (fSupportsVolumeChange)
		{
            m_PhoneCaps.dwVolumeFlags |= PHONEHOOKSWITCHDEV_HANDSET;
			if (CanHandleRequest(TSPI_PHONEGETVOLUME))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETVOLUMEHANDSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETVOLUMEHANDSET;
			}
			if (CanHandleRequest (TSPI_PHONESETVOLUME))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETVOLUMEHANDSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETVOLUMEHANDSET;
			}
		}

        if (fSupportsGainChange)
		{
            m_PhoneCaps.dwGainFlags |= PHONEHOOKSWITCHDEV_HANDSET;
			if (CanHandleRequest(TSPI_PHONEGETGAIN))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETGAINHANDSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETGAINHANDSET;
			}
			if (CanHandleRequest (TSPI_PHONESETGAIN))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETGAINHANDSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETGAINHANDSET;
			}
		}
            
        m_PhoneCaps.dwHandsetHookSwitchModes = dwAvailModes;
		m_PhoneCaps.dwSettableHandsetHookSwitchModes = dwSettableModes;
		m_PhoneCaps.dwMonitoredHandsetHookSwitchModes = dwMonitoredModes;

        m_PhoneStatus.dwHandsetHookSwitchMode = dwCurrMode;
        m_PhoneStatus.dwHandsetVolume = dwVolume;
        m_PhoneStatus.dwHandsetGain = dwGain;
    }
    else if (dwHookSwitchDev == PHONEHOOKSWITCHDEV_SPEAKER)
    {
        m_PhoneCaps.dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_SPEAKER;
		if (CanHandleRequest(TSPI_PHONEGETHOOKSWITCH))
		{
			m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHSPEAKER;
			m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHSPEAKER;
		}
		if (CanHandleRequest (TSPI_PHONESETHOOKSWITCH) && dwSettableModes)
		{
			m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETHOOKSWITCHSPEAKER;
			m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETHOOKSWITCHSPEAKER;
		}

        if (fSupportsVolumeChange)
		{
            m_PhoneCaps.dwVolumeFlags |= PHONEHOOKSWITCHDEV_SPEAKER;
			if (CanHandleRequest(TSPI_PHONEGETVOLUME))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETVOLUMESPEAKER;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETVOLUMESPEAKER;
			}
			if (CanHandleRequest (TSPI_PHONESETVOLUME))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETVOLUMESPEAKER;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETVOLUMESPEAKER;
			}
		}

        if (fSupportsGainChange)
		{
            m_PhoneCaps.dwGainFlags |= PHONEHOOKSWITCHDEV_SPEAKER;
			if (CanHandleRequest(TSPI_PHONEGETGAIN))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETGAINSPEAKER;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETGAINSPEAKER;
			}
			if (CanHandleRequest (TSPI_PHONESETGAIN))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETGAINSPEAKER;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETGAINSPEAKER;
			}
		}
            
        m_PhoneCaps.dwSpeakerHookSwitchModes = dwAvailModes;
        m_PhoneStatus.dwSpeakerHookSwitchMode = dwCurrMode;
		m_PhoneCaps.dwSettableSpeakerHookSwitchModes = dwSettableModes;
		m_PhoneCaps.dwMonitoredSpeakerHookSwitchModes = dwMonitoredModes;

        m_PhoneStatus.dwSpeakerVolume = dwVolume;
        m_PhoneStatus.dwSpeakerGain = dwGain;
    }
    else if (dwHookSwitchDev == PHONEHOOKSWITCHDEV_HEADSET)
    {
        m_PhoneCaps.dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HEADSET;
		if (CanHandleRequest(TSPI_PHONEGETHOOKSWITCH))
		{
			m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHHEADSET;
			m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETHOOKSWITCHHEADSET;
		}
		if (CanHandleRequest (TSPI_PHONESETHOOKSWITCH) && dwSettableModes)
		{
			m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETHOOKSWITCHHEADSET;
			m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETHOOKSWITCHHEADSET;
		}

        if (fSupportsVolumeChange)
		{
            m_PhoneCaps.dwVolumeFlags |= PHONEHOOKSWITCHDEV_HEADSET;
			if (CanHandleRequest(TSPI_PHONEGETVOLUME))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETVOLUMEHEADSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETVOLUMEHEADSET;
			}
			if (CanHandleRequest (TSPI_PHONESETVOLUME))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETVOLUMEHEADSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETVOLUMEHEADSET;
			}
		}

        if (fSupportsGainChange)
		{
            m_PhoneCaps.dwGainFlags |= PHONEHOOKSWITCHDEV_HEADSET;
			if (CanHandleRequest(TSPI_PHONEGETGAIN))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETGAINHEADSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETGAINHEADSET;
			}
			if (CanHandleRequest (TSPI_PHONESETGAIN))
			{
				m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETGAINHEADSET;
				m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETGAINHEADSET;
			}
		}
            
        m_PhoneCaps.dwHeadsetHookSwitchModes = dwAvailModes;
        m_PhoneStatus.dwHeadsetHookSwitchMode = dwCurrMode;
		m_PhoneCaps.dwSettableHeadsetHookSwitchModes = dwSettableModes;
		m_PhoneCaps.dwMonitoredHeadsetHookSwitchModes = dwMonitoredModes;

        m_PhoneStatus.dwHeadsetVolume = dwVolume;
        m_PhoneStatus.dwHeadsetGain = dwGain;
    }
#ifdef _DEBUG
    else
        // Unsupported hookswitch device!
        ASSERT (FALSE);
#endif
                           
}// CTSPIPhoneConnection::AddHookSwitchDevice

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddButton
//
// Add a new button to our button array
//
int CTSPIPhoneConnection::AddButton (DWORD dwFunction, DWORD dwMode, 
                                DWORD dwAvailLampModes, DWORD dwLampState, LPCTSTR lpszText)
{                                  
	CEnterCode sLock(this);  // Synch access to object
    m_PhoneCaps.dwNumButtonLamps++;

	// Adjust our phone features
	if (CanHandleRequest(TSPI_PHONEGETBUTTONINFO))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETBUTTONINFO;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETBUTTONINFO;
	}
	if (CanHandleRequest(TSPI_PHONEGETLAMP))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_GETLAMP;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_GETLAMP;
	}
	if (CanHandleRequest(TSPI_PHONESETBUTTONINFO))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETBUTTONINFO;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETBUTTONINFO;
	}
	if (CanHandleRequest(TSPI_PHONESETLAMP))
	{
		m_PhoneCaps.dwPhoneFeatures |= PHONEFEATURE_SETLAMP;
		m_PhoneStatus.dwPhoneFeatures |= PHONEFEATURE_SETLAMP;
	}
	return m_arrButtonInfo.Add (dwFunction, dwMode, dwAvailLampModes, dwLampState, lpszText);

}// CTSPIPhoneConnection::AddButton

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::Send_TAPI_Event
//
// Calls an event back into the TAPI DLL
//
void CTSPIPhoneConnection::Send_TAPI_Event(DWORD dwMsg, DWORD dwP1, DWORD dwP2, DWORD dwP3)
{
    ASSERT(m_lpfnEventProc != NULL);                                  
    
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
    ASSERT (dwMsg <= 26);
    DTRACE(TRC_MIN, _T("Send_TAPI_Event: <0x%lx> Phone=0x%lx, Msg=0x%lx (%s), P1=0x%lx, P2=0x%lx, P3=0x%lx\r\n"),
                    (DWORD)this, (DWORD)GetPhoneHandle(), dwMsg, (LPCTSTR)g_pszMsgs[dwMsg], 
                    dwP1, dwP2, dwP3);
#endif

    (*m_lpfnEventProc)(GetPhoneHandle(), dwMsg, dwP1, dwP2, dwP3);

}// CTSPIPhoneConnection::Send_TAPI_Event

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetButtonInfo
//
// This function returns information about the specified phone 
// button.
//
LONG CTSPIPhoneConnection::GetButtonInfo (DWORD dwButtonId, LPPHONEBUTTONINFO lpButtonInfo)
{   
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETBUTTONINFO) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // Get our button.
    const CPhoneButtonInfo* pButton = GetButtonInfo((int)dwButtonId);
    if (pButton == NULL)
        return PHONEERR_INVALBUTTONLAMPID;
        
    // Zero out the structure
    DWORD dwTotalSize = lpButtonInfo->dwTotalSize;
    FillBuffer (lpButtonInfo, 0, sizeof(PHONEBUTTONINFO));

    // Get the descriptive name if available.    
    CString strName = pButton->GetDescription();
    int cbSize = 0;
    if (!strName.IsEmpty())
        cbSize = (strName.GetLength()+1) * sizeof(TCHAR);

    // Fill in the PHONEBUTTONINFO structure.  Do NOT touch
    // the total size since it is what TAPI set.
    lpButtonInfo->dwButtonMode = pButton->GetButtonMode();
    lpButtonInfo->dwButtonFunction = pButton->GetFunction();
	lpButtonInfo->dwButtonState = pButton->GetButtonState();

    // Set the data length
    lpButtonInfo->dwTotalSize = dwTotalSize;
    lpButtonInfo->dwNeededSize = sizeof(PHONEBUTTONINFO) + cbSize;
    lpButtonInfo->dwUsedSize = sizeof(PHONEBUTTONINFO);

    // Set the text description string if available.
	AddDataBlock (lpButtonInfo, lpButtonInfo->dwButtonTextOffset,
				  lpButtonInfo->dwButtonTextSize, strName);
    return FALSE;
    
}// CTSPIPhoneConnection::GetButtonInfo   

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GatherCapabilities
//
// This function queries a specified phone device to determine its
// telephony capabilities
//
LONG CTSPIPhoneConnection::GatherCapabilities(DWORD dwTSPIVersion, DWORD /*dwExtVersion*/, LPPHONECAPS lpPhoneCaps)
{  
	// Adjust the TSPI version if this phone was opened at a lower TSPI version.
	if (dwTSPIVersion > GetNegotiatedVersion())
		dwTSPIVersion = GetNegotiatedVersion();

	// Synch access to object
	CEnterCode sLock(this);  

	// Grab all the pieces which are offset/sizes to add to the PHONECAPS
	// structure and determine the total size of the PHONECAPS buffer required
	// for ALL blocks of information.
    CString strPhoneName = GetName();
    CString strProviderInfo = GetSP()->GetProviderInfo();
    CString strPhoneInfo = GetConnInfo();
    int cbName=0, cbInfo=0, cbPhoneInfo=0;
    int cbButton = (int) (m_PhoneCaps.dwNumButtonLamps * sizeof(DWORD));
    int cbUpload = (int) (m_PhoneCaps.dwNumGetData * sizeof(DWORD));
    int cbDownload = (int) (m_PhoneCaps.dwNumSetData * sizeof(DWORD));

    if (!strPhoneName.IsEmpty())
        cbName = (strPhoneName.GetLength()+1) * sizeof(TCHAR);
    if (!strProviderInfo.IsEmpty())
        cbInfo = (strProviderInfo.GetLength()+1) * sizeof(TCHAR);
    if (!strPhoneInfo.IsEmpty())
        cbPhoneInfo = (strPhoneInfo.GetLength()+1) * sizeof(TCHAR);

	// Get the length of the device classes we support.
	CString strDeviceNames;
	int cbDeviceNameLen = 0;
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

    // Save off the sections that TAPI provides
    m_PhoneCaps.dwTotalSize = lpPhoneCaps->dwTotalSize;
    m_PhoneCaps.dwNeededSize = sizeof(PHONECAPS) + cbName + cbInfo + (cbButton*3) + 
							cbUpload + cbDownload + cbPhoneInfo + cbDeviceNameLen;

	// Determine what the minimum size required is based on the TSPI version
	// we negotiated to.
	DWORD dwReqSize = sizeof(PHONECAPS);
	if (dwTSPIVersion < TAPIVER_20)
		dwReqSize -= 9*sizeof(DWORD);

#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
    if (lpPhoneCaps->dwTotalSize < dwReqSize)
	{
		lpPhoneCaps->dwNeededSize = m_PhoneCaps.dwNeededSize;
		ASSERT (FALSE);
        return PHONEERR_STRUCTURETOOSMALL;
	}
#endif
    
    // Copy the phone capabilities over from our structure
    CopyBuffer (lpPhoneCaps, &m_PhoneCaps, dwReqSize);
	lpPhoneCaps->dwUsedSize = dwReqSize;
    
    // Now add the phone name if we have the room
	AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwPhoneNameOffset, 
			lpPhoneCaps->dwPhoneNameSize, strPhoneName);
    
    // Add the phone information if we have the room.
	AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwPhoneInfoOffset, 
			lpPhoneCaps->dwPhoneInfoSize, strPhoneInfo);
    
    // Add the Provider information if we have the room
	AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwProviderInfoOffset, 
			lpPhoneCaps->dwProviderInfoSize, strProviderInfo);

    // Fill in the button information - mode first
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbButton)
    {
        for (int i = 0; i < GetButtonCount(); i++)
        {
			DWORD dwButtonMode = GetButtonInfo(i)->GetButtonMode();
			AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwButtonModesOffset, 
					lpPhoneCaps->dwButtonModesSize, &dwButtonMode, sizeof(DWORD));
		}
	}
    
    // Now the functions
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbButton)
    {
        for (int i = 0; i < GetButtonCount(); i++)
        {
			DWORD dwButtonFunction = GetButtonInfo(i)->GetFunction();
			AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwButtonFunctionsOffset,
					lpPhoneCaps->dwButtonFunctionsSize, &dwButtonFunction, sizeof(DWORD));			
        }
    }
    
    // Add the lamps
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbButton)
    {
        for (int i = 0; i < GetButtonCount(); i++)
        {
			DWORD dwLampMode = GetButtonInfo(i)->GetAvailLampModes();
			AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwLampModesOffset,
					lpPhoneCaps->dwLampModesSize, &dwLampMode, sizeof(DWORD));			
	    }
	}

    // Add the download buffer sizes.
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbDownload)
    {
        for (int i = 0; i < m_arrDownloadBuffers.GetSize(); i++)
		{
			DWORD dwBuffer = m_arrDownloadBuffers[i];
			AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwSetDataOffset,
					lpPhoneCaps->dwSetDataSize, &dwBuffer, sizeof(DWORD));
		}
	}

    // Add the upload buffer sizes.
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbUpload)
    {
        for (int i = 0; i < m_arrUploadBuffers.GetSize(); i++)
		{
			DWORD dwBuffer = m_arrUploadBuffers[i];
			AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwGetDataOffset,
				lpPhoneCaps->dwGetDataSize, &dwBuffer, sizeof(DWORD));
		}
	}
    
	// If we negotiated to TAPI 2.0, then add the additional fields.
	if (dwTSPIVersion >= TAPIVER_20)
	{
		if (cbDeviceNameLen)
		{
			if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbDeviceNameLen)
			{
				AddDataBlock (lpPhoneCaps, lpPhoneCaps->dwDeviceClassesOffset,
					          lpPhoneCaps->dwDeviceClassesSize, strDeviceNames);

				// Remove the '~' chars and change to NULLs.
				wchar_t* lpBuff = (wchar_t*)((LPBYTE)lpPhoneCaps + lpPhoneCaps->dwDeviceClassesOffset);
				ASSERT(lpBuff != NULL);
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
   
}// CTSPIPhoneConnection::GatherCapabilities

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GatherStatus
// 
// This method gathers the PHONESTATUS values for TAPI
//
LONG CTSPIPhoneConnection::GatherStatus (LPPHONESTATUS lpPhoneStatus)
{                                                          
	CEnterCode sLock(this);  // Synch access to object

	// Get the version we negotiated this phone at
	DWORD dwTSPIVersion = GetNegotiatedVersion();

    // Save off the values which TAPI supplies.
    m_PhoneStatus.dwTotalSize = lpPhoneStatus->dwTotalSize;
    m_PhoneStatus.dwNumOwners = lpPhoneStatus->dwNumOwners;
    m_PhoneStatus.dwNumMonitors = lpPhoneStatus->dwNumMonitors;
    m_PhoneStatus.dwOwnerNameSize = lpPhoneStatus->dwOwnerNameSize;
    m_PhoneStatus.dwOwnerNameOffset = lpPhoneStatus->dwOwnerNameOffset;

	// If this is the first call, then set the phone features to be
	// what we determined during our INIT phase.
	if (m_PhoneStatus.dwPhoneFeatures == 0)
		m_PhoneStatus.dwPhoneFeatures = m_PhoneCaps.dwPhoneFeatures;

    // Now begin filling in our side.
	DWORD dwReqSize = sizeof (PHONESTATUS);
	if (dwTSPIVersion < TAPIVER_20)
		dwReqSize -= sizeof(DWORD);

    // Determine the space for the display.
    CString strDisplay = GetDisplayBuffer();
    int cbDisplay = 0;
    if (!strDisplay.IsEmpty())
        cbDisplay = (strDisplay.GetLength()+1) * sizeof(TCHAR);
    int cbButton = GetButtonCount() * sizeof(DWORD);

    // Fill in the required size.
    m_PhoneStatus.dwNeededSize = dwReqSize+cbDisplay+cbButton;

#ifdef _DEBUG
    // If we don't have enough space based on our negotiated version, return an error and tell
	// TAPI how much we need for the full structure to come down.  NOTE: This should never
	// happen - TAPI.DLL is supposed to verify that there is enough space in the structure
	// for the negotiated version and return an error.  We only check this in DEBUG builds
	// to insure that TAPI is doing its job.
    if (lpPhoneStatus->dwTotalSize < dwReqSize)
	{
		lpPhoneStatus->dwNeededSize = m_PhoneStatus.dwNeededSize;
		ASSERT (FALSE);
        return PHONEERR_STRUCTURETOOSMALL;
	}
#endif

    // Copy over the basic PHONESTATUS structure
    CopyBuffer (lpPhoneStatus, &m_PhoneStatus, dwReqSize);
	lpPhoneStatus->dwUsedSize = dwReqSize;
    
    // Copy the display information.
	if (!strDisplay.IsEmpty())
		AddDataBlock (lpPhoneStatus, lpPhoneStatus->dwDisplayOffset, 
				lpPhoneStatus->dwDisplaySize, (LPVOID)(LPCTSTR)strDisplay, cbDisplay);

    // Copy in the lamp mode information if enough space.
    if (cbButton && lpPhoneStatus->dwTotalSize >= lpPhoneStatus->dwUsedSize+cbButton)
    {
        for (int i = 0; i < GetButtonCount(); i++)
        {
			DWORD dwLampMode = GetButtonInfo(i)->GetLampMode();
			AddDataBlock (lpPhoneStatus, lpPhoneStatus->dwLampModesOffset,
				lpPhoneStatus->dwLampModesSize, &dwLampMode, sizeof(DWORD));
		}
	}
    
    return FALSE;
   
}// CTSPIPhoneConnection::GatherStatus

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetDisplay
//
// Retrieve the display for the phone device into a VARSTRING
// buffer.
//
LONG CTSPIPhoneConnection::GetDisplay (LPVARSTRING lpVarString)
{
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETDISPLAY) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    CString strDisplay = GetDisplayBuffer();

#ifdef _UNICODE
    lpVarString->dwStringFormat = STRINGFORMAT_UNICODE;
#else
    lpVarString->dwStringFormat = STRINGFORMAT_ASCII;
#endif
    lpVarString->dwNeededSize = sizeof(VARSTRING);
    lpVarString->dwUsedSize = sizeof(VARSTRING);
    lpVarString->dwStringSize = 0;
    lpVarString->dwStringOffset = 0;
    
    if (!strDisplay.IsEmpty())
		AddDataBlock (lpVarString, lpVarString->dwStringOffset, 
					  lpVarString->dwStringSize, (LPVOID)(LPCTSTR)strDisplay, 
					  (strDisplay.GetLength()+1) * sizeof(TCHAR));
            
    return FALSE;
    
}// CTSPIPhoneConnection::GetDisplay

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetGain
//
// Return the current gain value for one of our hookswitch devices.
//
LONG CTSPIPhoneConnection::GetGain (DWORD dwHookSwitchDevice, LPDWORD lpdwGain)
{       
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & (PHONEFEATURE_GETGAINHANDSET|
			PHONEFEATURE_GETGAINHEADSET|PHONEFEATURE_GETGAINSPEAKER)) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // If we don't support the hook switch device.                         
    if ((m_PhoneCaps.dwHookSwitchDevs & dwHookSwitchDevice) != dwHookSwitchDevice)
        return PHONEERR_INVALHOOKSWITCHDEV;
    
    // The hook switch must be only a single bit.
    switch (dwHookSwitchDevice)
    {
        case PHONEHOOKSWITCHDEV_HANDSET:
			if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETGAINHANDSET) == 0)
				return PHONEERR_OPERATIONUNAVAIL;
            *lpdwGain = m_PhoneStatus.dwHandsetGain;
            break;
        case PHONEHOOKSWITCHDEV_HEADSET:
			if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETGAINHEADSET) == 0)
				return PHONEERR_OPERATIONUNAVAIL;
            *lpdwGain = m_PhoneStatus.dwHeadsetGain;
            break;
        case PHONEHOOKSWITCHDEV_SPEAKER:
			if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETGAINSPEAKER) == 0)
				return PHONEERR_OPERATIONUNAVAIL;
            *lpdwGain = m_PhoneStatus.dwSpeakerGain;
            break;
        default:
            return PHONEERR_INVALHOOKSWITCHDEV;
    }       
    return FALSE;

}// CTSPIPhoneConnection::GetGain

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetVolume
//
// Return the volume level for a specified hook switch device.  If
// the hookswitch device is not supported, return an error.
//
LONG CTSPIPhoneConnection::GetVolume (DWORD dwHookSwitchDevice, LPDWORD lpdwVolume)
{                                  
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & (PHONEFEATURE_GETVOLUMEHANDSET|
			PHONEFEATURE_GETVOLUMEHEADSET|PHONEFEATURE_GETVOLUMESPEAKER)) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // If we don't support the hook switch device.                         
    if ((m_PhoneCaps.dwHookSwitchDevs & dwHookSwitchDevice) != dwHookSwitchDevice)
        return PHONEERR_INVALHOOKSWITCHDEV;
    
    // The hook switch must be only a single bit.
    switch (dwHookSwitchDevice)
    {
        case PHONEHOOKSWITCHDEV_HANDSET:
			if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETVOLUMEHANDSET) == 0)
				return PHONEERR_OPERATIONUNAVAIL;
            *lpdwVolume = m_PhoneStatus.dwHandsetVolume;
            break;
        case PHONEHOOKSWITCHDEV_HEADSET:
			if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETVOLUMEHANDSET) == 0)
				return PHONEERR_OPERATIONUNAVAIL;
            *lpdwVolume = m_PhoneStatus.dwHeadsetVolume;
            break;
        case PHONEHOOKSWITCHDEV_SPEAKER:
			if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETVOLUMEHANDSET) == 0)
				return PHONEERR_OPERATIONUNAVAIL;
            *lpdwVolume = m_PhoneStatus.dwSpeakerVolume;
            break;
        default:
            return PHONEERR_INVALHOOKSWITCHDEV;
    }       
    return FALSE;

}// CTSPIPhoneConnection::GetVolume

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetHookSwitch
//
// This method returns the current hook switch mode for all our
// hookswitch devices.  Each device which is offhook gets a bit
// set in the field representing handset/speaker/headset.
//
LONG CTSPIPhoneConnection::GetHookSwitch (LPDWORD lpdwHookSwitch)
{   
	if ((GetPhoneStatus()->dwPhoneFeatures & (PHONEFEATURE_GETHOOKSWITCHHANDSET|
			PHONEFEATURE_GETHOOKSWITCHSPEAKER|PHONEFEATURE_GETHOOKSWITCHHEADSET)) == 0)
		return PHONEERR_OPERATIONUNAVAIL;
	
    *lpdwHookSwitch = 0L; // All onhook.
    if (m_PhoneStatus.dwHandsetHookSwitchMode & 
            (PHONEHOOKSWITCHMODE_MICSPEAKER | PHONEHOOKSWITCHMODE_MIC | PHONEHOOKSWITCHMODE_SPEAKER))
        *lpdwHookSwitch |= PHONEHOOKSWITCHDEV_HANDSET;
    if (m_PhoneStatus.dwHeadsetHookSwitchMode & 
            (PHONEHOOKSWITCHMODE_MICSPEAKER | PHONEHOOKSWITCHMODE_MIC | PHONEHOOKSWITCHMODE_SPEAKER))
        *lpdwHookSwitch |= PHONEHOOKSWITCHDEV_HEADSET;
    if (m_PhoneStatus.dwSpeakerHookSwitchMode & 
            (PHONEHOOKSWITCHMODE_MICSPEAKER | PHONEHOOKSWITCHMODE_MIC | PHONEHOOKSWITCHMODE_SPEAKER))
        *lpdwHookSwitch |= PHONEHOOKSWITCHDEV_SPEAKER;
    return FALSE;            

}// CTSPIPhoneConnection::GetHookSwitch

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetLamp
//
// Return the current lamp mode of the specified lamp
//
LONG CTSPIPhoneConnection::GetLamp (DWORD dwButtonId, LPDWORD lpdwLampMode)
{
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETLAMP) == 0)
		return PHONEERR_OPERATIONUNAVAIL;
	
    // Make sure the button id is valid.
    const CPhoneButtonInfo* pButton = GetButtonInfo ((int) dwButtonId);
    if (pButton == NULL || pButton->GetLampMode() == PHONELAMPMODE_DUMMY)
        return PHONEERR_INVALBUTTONLAMPID;
        
    *lpdwLampMode = pButton->GetLampMode();    
    return FALSE;

}// CTSPIPhoneConnection::GetLamp

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetLampMode
//
// Return the current lamp mode of the specified lamp
//
DWORD CTSPIPhoneConnection::GetLampMode (int iButtonId)
{                                
    // Make sure the button id is valid.
    const CPhoneButtonInfo* pButton = GetButtonInfo (iButtonId);
    if (pButton == NULL || pButton->GetLampMode() == PHONELAMPMODE_DUMMY)
        return PHONELAMPMODE_DUMMY;
    return pButton->GetLampMode();

}// CTSPIPhoneConnection::GetLampMode

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetButtonInfo
//
// Sets the button information for a specified button
//
LONG CTSPIPhoneConnection::SetButtonInfo (DRV_REQUESTID dwRequestID, TSPISETBUTTONINFO* lpButtInfo)
{                                      
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_SETBUTTONINFO) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // Make sure the button id is valid.
    const CPhoneButtonInfo* pButton = GetButtonInfo ((int)lpButtInfo->dwButtonLampId);
    if (pButton == NULL || pButton->GetButtonMode() == PHONEBUTTONMODE_DUMMY)
        return PHONEERR_INVALBUTTONLAMPID;
        
    // Submit the request.    
    if (AddAsynchRequest (NULL, REQUEST_SETBUTTONINFO, dwRequestID, lpButtInfo))
        return (LONG) dwRequestID;   
        
    return PHONEERR_OPERATIONUNAVAIL;

}// CTSPIPhoneConnection::SetButtonInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetButtonInfo
//
// Set the button information internally and notify TAPI.  This should
// be called when the worker code completes a REQUEST_SETBUTTONINFO.
//
void CTSPIPhoneConnection::SetButtonInfo (int iButtonID, DWORD dwFunction, DWORD dwMode, LPCTSTR pszName)
{
	CEnterCode sLock(this);  // Synch access to object
    CPhoneButtonInfo* pButton = (CPhoneButtonInfo*) GetButtonInfo (iButtonID);
    if (pButton != NULL)
    {
        pButton->SetButtonInfo (dwFunction, dwMode, pszName);
        m_arrButtonInfo.SetDirtyFlag (TRUE);
    }

}// CTSPIPhoneConnection::SetButtonInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetLamp
//
// This method causes the specified lamp to be set on our phone device.
//
LONG CTSPIPhoneConnection::SetLamp (DRV_REQUESTID dwRequestID, TSPISETBUTTONINFO* lpButtInfo)
{                                
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_SETLAMP) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // Make sure the button id is valid.
    const CPhoneButtonInfo* pButton = GetButtonInfo ((int)lpButtInfo->dwButtonLampId);
    if (pButton == NULL || pButton->GetLampMode() == PHONELAMPMODE_DUMMY)
        return PHONEERR_INVALBUTTONLAMPID;

    // Submit the request.    
    if (AddAsynchRequest (NULL, REQUEST_SETLAMP, dwRequestID, lpButtInfo))
        return (LONG) dwRequestID;   
    return PHONEERR_OPERATIONFAILED;    

}// CTSPIPhoneConnection::SetLamp

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetLampState
//
// Set the lamp state for the specified lamp id (button id).  This
// should be called when the worker code has completed a REQUEST_SETLAMP.
// 
// This notifies TAPI of the lampstate change.
//
DWORD CTSPIPhoneConnection::SetLampState (int iButtonId, DWORD dwLampState)
{                                     
	CEnterCode sLock(this);  // Synch access to object
    CPhoneButtonInfo* pButton = (CPhoneButtonInfo*) GetButtonInfo (iButtonId);
    if (pButton != NULL)
    {
        DWORD dwCurrState = pButton->GetLampMode();
        pButton->SetLampMode (dwLampState);

		// See if this is the MSGWAIT lamp.  If so, record it in the line device capabilities.
		if (pButton->GetFunction() == PHONEBUTTONFUNCTION_MSGINDICATOR)
		{
			CTSPILineConnection* pLine = GetAssociatedLine();
			if (pLine != NULL)
			{
				DWORD dwFlags = pLine->GetLineDevStatus()->dwDevStatusFlags;
				if (dwLampState == PHONELAMPMODE_OFF)
					dwFlags &= ~LINEDEVSTATUSFLAGS_MSGWAIT;
				else
					dwFlags |= LINEDEVSTATUSFLAGS_MSGWAIT;
				pLine->SetDeviceStatusFlags(dwFlags);
			}
		}

        OnPhoneStatusChange (PHONESTATE_LAMP, (DWORD)iButtonId);
        return dwCurrState;
    }   
    
    return PHONELAMPMODE_DUMMY;

}// CTSPIPhoneConnection::SetLampState

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetButtonState
//
// Set the current state of the button for a specified button id.  This
// should be called when the worker code detects a button going up or down.
//
DWORD CTSPIPhoneConnection::SetButtonState (int iButtonId, DWORD dwButtonState)
{
	CEnterCode sLock(this);  // Synch access to object
    CPhoneButtonInfo* pButton = (CPhoneButtonInfo*) GetButtonInfo (iButtonId);
    if (pButton != NULL)
    {
        DWORD dwCurrState = pButton->GetButtonState();
        pButton->SetButtonState (dwButtonState);
        
        // Tell TAPI if it is valid for a button state message.
        if (pButton->GetButtonMode() != PHONEBUTTONMODE_DUMMY &&
            (dwButtonState == PHONEBUTTONSTATE_UP && dwCurrState == PHONEBUTTONSTATE_DOWN) ||
            (dwCurrState == PHONEBUTTONSTATE_UP && dwButtonState == PHONEBUTTONSTATE_DOWN))
            OnButtonStateChange (iButtonId, pButton->GetButtonMode(), dwButtonState);
        return dwCurrState;
    }   
    
    return PHONEBUTTONSTATE_UNAVAIL;

}// CTSPIPhoneDevice::SetButtonState
                                          
///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetGain
//
// This method is called by the CServiceProvider class in response to
// a phoneSetGain function.  It calls the worker thread to do the
// H/W setting.
//
LONG CTSPIPhoneConnection::SetGain (DRV_REQUESTID dwRequestId, TSPIHOOKSWITCHPARAM* pHSParam)
{                               
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & (PHONEFEATURE_SETGAINHANDSET|
			PHONEFEATURE_SETGAINSPEAKER|PHONEFEATURE_SETGAINHEADSET)) == 0)
		return PHONEERR_OPERATIONUNAVAIL;
	
    // If we don't support the hook switch device.                         
    if ((m_PhoneCaps.dwHookSwitchDevs & pHSParam->dwHookSwitchDevice) != pHSParam->dwHookSwitchDevice)
        return PHONEERR_INVALHOOKSWITCHDEV;

    // Submit the request.
    if (AddAsynchRequest (NULL, REQUEST_SETHOOKSWITCHGAIN, dwRequestId, pHSParam))
        return (LONG) dwRequestId;
    return PHONEERR_OPERATIONFAILED;    

}// CTSPIPhoneConnection::SetGain

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetVolume
//
// This method is called by the CServiceProvider class in response to
// a phoneSetVolume function.  It calls the worker thread to do the
// H/W setting.
//
LONG CTSPIPhoneConnection::SetVolume (DRV_REQUESTID dwRequestId, TSPIHOOKSWITCHPARAM* pHSParam)
{                                
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & (PHONEFEATURE_SETVOLUMEHANDSET|
			PHONEFEATURE_SETVOLUMESPEAKER|PHONEFEATURE_SETVOLUMEHEADSET)) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // If we don't support the hook switch device.                         
    if ((m_PhoneCaps.dwHookSwitchDevs & pHSParam->dwHookSwitchDevice) != pHSParam->dwHookSwitchDevice)
        return PHONEERR_INVALHOOKSWITCHDEV;

    // Submit the request.
    if (AddAsynchRequest (NULL, REQUEST_SETHOOKSWITCHVOL, dwRequestId, pHSParam))
        return (LONG) dwRequestId;
    return PHONEERR_OPERATIONFAILED;    

}// CTSPIPhoneConnection::SetVolume

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetHookSwitch
//
// This method is called by the CServiceProvider class in response to
// a phoneSetHookSwitch function.  It calls the worker thread to do the
// H/W setting.
//
LONG CTSPIPhoneConnection::SetHookSwitch (DRV_REQUESTID dwRequestId, TSPIHOOKSWITCHPARAM* pHSParam)
{                                
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & (PHONEFEATURE_SETHOOKSWITCHHANDSET|
			PHONEFEATURE_SETHOOKSWITCHSPEAKER|PHONEFEATURE_SETHOOKSWITCHHEADSET)) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // If we don't support one of the hook switch devices.                         
    if ((m_PhoneCaps.dwHookSwitchDevs & pHSParam->dwHookSwitchDevice) != pHSParam->dwHookSwitchDevice)
        return PHONEERR_INVALHOOKSWITCHDEV;

    // Submit the request.
    if (AddAsynchRequest (NULL, REQUEST_SETHOOKSWITCH, dwRequestId, pHSParam))
        return (LONG) dwRequestId;
    return PHONEERR_OPERATIONFAILED;    

}// CTSPIPhoneConnection::SetHookSwitch

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetRing
//
// This method is called in response to a phoneSetRing request.  It
// passes control to the worker thread.
//
LONG CTSPIPhoneConnection::SetRing (DRV_REQUESTID dwRequestID, TSPIRINGPATTERN* pRingPattern)
{                               
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_SETRING) == 0)
		return PHONEERR_OPERATIONUNAVAIL;
	
    // Verify the ring mode.
    if (pRingPattern->dwRingMode >= m_PhoneCaps.dwNumRingModes)
        return PHONEERR_INVALRINGMODE;
        
    // Submit the request
    if (AddAsynchRequest (NULL, REQUEST_SETRING, dwRequestID, pRingPattern))
        return (LONG) dwRequestID;
    return PHONEERR_OPERATIONFAILED;

}// CTSPIPhoneConnection::SetRing

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetDisplay
//
// This method is called in response to a phoneSetDisplay command.  It
// submits an asynch request to the worker code.
//
LONG CTSPIPhoneConnection::SetDisplay (DRV_REQUESTID dwRequestID, TSPIPHONESETDISPLAY* lpDisplay)
{                                  
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_SETDISPLAY) == 0)
		return PHONEERR_OPERATIONUNAVAIL;
	
    // Verify the row/col field.
    if (lpDisplay->dwRow > m_PhoneCaps.dwDisplayNumRows ||
        lpDisplay->dwColumn > m_PhoneCaps.dwDisplayNumColumns)
        return PHONEERR_INVALPARAM;
        
    // Submit the request
    if (AddAsynchRequest (NULL, REQUEST_SETDISPLAY, dwRequestID, lpDisplay))
        return (LONG) dwRequestID;
    return PHONEERR_OPERATIONFAILED;

}// CTSPIPhoneConnection::SetDisplay

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetData
//
// Downloads the information in the specified buffer to the phone buffer.
//
LONG CTSPIPhoneConnection::SetData (DRV_REQUESTID dwRequestID, TSPIPHONEDATA* pPhoneData)
{                                
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_SETDATA) == 0)
		return PHONEERR_OPERATIONUNAVAIL;

    // Verify the buffer id.
    if (pPhoneData->dwDataID > m_PhoneCaps.dwNumSetData)
        return PHONEERR_INVALDATAID;
    
    // Submit the request
    if (AddAsynchRequest (NULL, REQUEST_SETPHONEDATA, dwRequestID, pPhoneData))    
        return (LONG) dwRequestID;
    return PHONEERR_OPERATIONFAILED;

}// CTSPIPhoneConnection::SetData

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetData
//
// Return the data from an uploadable buffer on a phone.
//
LONG CTSPIPhoneConnection::GetData (TSPIPHONEDATA* pPhoneData)
{                               
	// Validate the request
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETDATA) == 0)
		return PHONEERR_OPERATIONUNAVAIL;
	
    // Verify the buffer id.
    if (pPhoneData->dwDataID > m_PhoneCaps.dwNumGetData)
        return PHONEERR_INVALDATAID;

    // Pass the request to the worker code.  This request is handled specially in that
    // it is NOT asynchronous.  Therefore, WE must delete the phone data on an error
    // since when we delete the request it will be deleted as well.
    CTSPIRequest* pReq = AddAsynchRequest (NULL, REQUEST_GETPHONEDATA, 0, pPhoneData);
    if (pReq == NULL)        
    {
        delete pPhoneData;
        return PHONEERR_OPERATIONFAILED;
    }        
        
    // Wait for the request to complete.
    if (WaitForRequest(0, pReq) == -1L) // Timeout?
    {
        RemoveRequest(pReq);
        delete pReq;
        return PHONEERR_OPERATIONFAILED;
    }
    return FALSE;

}// CTSPIPhoneConnection::GetData

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetStatusMessages
//
// Enables an application to monitor the specified phone device 
// for selected status events.
//
LONG CTSPIPhoneConnection::SetStatusMessages(DWORD dwPhoneStates, DWORD dwButtonModes, DWORD dwButtonStates)
{   
    // Validate the button modes/states.  If dwButtonModes is zero, then ignore both
    // fields.
    if (dwButtonModes != 0)
    {
        // Otherwise, button states CANNOT be zero
        if (dwButtonStates == 0)
            return PHONEERR_INVALBUTTONSTATE;
        
        // Set button modes/states
    	m_dwButtonModes = dwButtonModes;
    	m_dwButtonStates = dwButtonStates;  
    }
    
    // Set the phone states to monitor for.
    m_dwPhoneStates = dwPhoneStates;
    return FALSE;

}// CTSPIPhoneConnection::SetStatusMessages

/////////////////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::Open
//
// This method opens the phone device, returning the service provider's 
// opaque handle for the device and retaining the TAPI opaque handle.
// CTSPIPhoneConnection::Open 
//
LONG CTSPIPhoneConnection::Open (
HTAPIPHONE htPhone,                 // TAPI opaque phone handle
PHONEEVENT lpfnEventProc,           // PHONEEVENT callback   
DWORD dwTSPIVersion)                // Version expected
{   
    // We should not have an existing handle.
    if (GetPhoneHandle() != NULL)
		return PHONEERR_ALLOCATED;

    // Save off the event procedure for this phone and the TAPI
    // opaque phone handle which represents this device to the application.
    m_htPhone = htPhone;
    m_lpfnEventProc = lpfnEventProc;
    m_dwNegotiatedVersion = dwTSPIVersion;

    DTRACE(TRC_MIN, _T("Opening phone 0x%lX, TAPI handle=0x%lX, SP handle=0x%lX\r\n"), GetDeviceID(), m_htPhone, (DWORD)this);
    
    // Tell our device to perform an open for this connection.
    if (!OpenDevice())
	{
		m_htPhone = 0;
		m_lpfnEventProc = NULL;
		m_dwNegotiatedVersion = 0;
		return LINEERR_RESOURCEUNAVAIL;
	}

    return FALSE;
    
}// CTSPIPhoneConnection::Open

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::Close
//
// Close the phone connection object and reset the phone handle
//
LONG CTSPIPhoneConnection::Close()
{       
    if (GetPhoneHandle())
    {
        DTRACE(TRC_MIN, _T("Closing phone 0x%lX, TAPI handle=0x%lX, SP handle=0x%lX\r\n"), GetDeviceID(), GetPhoneHandle(), (DWORD)this);

        // Kill any pending requests.
        RemovePendingRequests();
        
        // Reset the event procedure and phone handle.
        m_lpfnEventProc = NULL;
        m_htPhone = 0;
        m_dwPhoneStates = 0L;
        m_dwButtonModes = 0L;
        m_dwButtonStates = 0L;
		m_dwNegotiatedVersion = GetSP()->GetSupportedVersion();
        
        // Close the device
        CloseDevice();

		// If the phone has been removed, then mark it as DELETED now
		// so we will refuse any further traffic on this line.
		if (GetFlags() & _IsRemoved)
			m_dwFlags |= _IsDeleted;
		
        return FALSE;
    }
    return PHONEERR_OPERATIONFAILED;

}// CTSPIPhoneConnection::Close

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetStatusFlags
//
// This method sets the "dwStatusFlags" field of the PHONESTATUS
// record.  TAPI is notified of the appropriate things.
// 
// The previous status flags are returned.
//
DWORD CTSPIPhoneConnection::SetStatusFlags (DWORD dwStatus)
{                                       
    DWORD dwOldStatus = m_PhoneStatus.dwStatusFlags;
    m_PhoneStatus.dwStatusFlags = dwStatus;

    // Send TAPI notifications
    if (((dwStatus & PHONESTATUSFLAGS_CONNECTED) != 0) &&
         (dwOldStatus & PHONESTATUSFLAGS_CONNECTED) == 0)
        OnPhoneStatusChange (PHONESTATE_CONNECTED);
    else if (((dwStatus & PHONESTATUSFLAGS_CONNECTED) == 0) &&        
        (dwOldStatus & PHONESTATUSFLAGS_CONNECTED) != 0)
        OnPhoneStatusChange (PHONESTATE_DISCONNECTED);
        
    if (((dwStatus & PHONESTATUSFLAGS_SUSPENDED) != 0) &&
        (dwOldStatus & PHONESTATUSFLAGS_SUSPENDED) == 0)
        OnPhoneStatusChange (PHONESTATE_SUSPEND);
    else if (((dwStatus & PHONESTATUSFLAGS_SUSPENDED) == 0) &&
        (dwOldStatus & PHONESTATUSFLAGS_SUSPENDED) != 0)
        OnPhoneStatusChange (PHONESTATE_RESUME);

    return dwOldStatus;

}// CTSPIPhoneConnection::SetStatusFlags

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetHookSwitch
//
// Change the hookswitch state of a hookswitch device(s).
//
void CTSPIPhoneConnection::SetHookSwitch (DWORD dwHookSwitchDev, DWORD dwMode)
{                                      
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_HANDSET)
    {
        m_PhoneStatus.dwHandsetHookSwitchMode = dwMode;
        OnPhoneStatusChange (PHONESTATE_HANDSETHOOKSWITCH, dwMode);
    }
    
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_SPEAKER)
    {   
        m_PhoneStatus.dwSpeakerHookSwitchMode = dwMode;
        OnPhoneStatusChange (PHONESTATE_SPEAKERHOOKSWITCH, dwMode);
    }
    
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_HEADSET)
    {                                                      
        m_PhoneStatus.dwHeadsetHookSwitchMode = dwMode;
        OnPhoneStatusChange (PHONESTATE_HEADSETHOOKSWITCH, dwMode);
    }

}// CTSPIPhoneConnection::SetHookSwitch

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetVolume
//
// Set the volume of a hookswitch device(s)
//
void CTSPIPhoneConnection::SetVolume (DWORD dwHookSwitchDev, DWORD dwVolume)
{                                      
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_HANDSET)
    {
        m_PhoneStatus.dwHandsetVolume = dwVolume;
        OnPhoneStatusChange (PHONESTATE_HANDSETVOLUME);
    }
    
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_SPEAKER)
    {   
        m_PhoneStatus.dwSpeakerVolume = dwVolume;
        OnPhoneStatusChange (PHONESTATE_SPEAKERVOLUME);
    }
    
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_HEADSET)
    {                                                      
        m_PhoneStatus.dwHeadsetVolume = dwVolume;
        OnPhoneStatusChange (PHONESTATE_HEADSETVOLUME);
    }

}// CTSPIPhoneConnection::SetVolume

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetGain
//
// Set the gain of a hookswitch device(s)
//
void CTSPIPhoneConnection::SetGain (DWORD dwHookSwitchDev, DWORD dwGain)
{                                      
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_HANDSET)
    {
        m_PhoneStatus.dwHandsetGain = dwGain;
        OnPhoneStatusChange (PHONESTATE_HANDSETGAIN);
    }
    
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_SPEAKER)
    {   
        m_PhoneStatus.dwSpeakerGain = dwGain;
        OnPhoneStatusChange (PHONESTATE_SPEAKERGAIN);
    }
    
    if (dwHookSwitchDev & PHONEHOOKSWITCHDEV_HEADSET)
    {                                                      
        m_PhoneStatus.dwHeadsetGain = dwGain;
        OnPhoneStatusChange (PHONESTATE_HEADSETGAIN);
    }

}// CTSPIPhoneConnection::SetGain

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::OnRequestComplete
//
// This virtual method is called when an outstanding request on this
// phone device has completed.  The return code indicates the success
// or failure of the request.  
//
void CTSPIPhoneConnection::OnRequestComplete(CTSPIRequest* pReq, LONG lResult)
{                                         
	CEnterCode sLock(this);  // Synch access to object
    if (lResult == 0)
    {
        // If this is a SETBUTTON request, then set the information back into
        // our object.
        if (pReq->GetCommand() == REQUEST_SETBUTTONINFO)
        {
            TSPISETBUTTONINFO* pInfo = (TSPISETBUTTONINFO*) pReq->GetDataPtr();
            SetButtonInfo((int)pInfo->dwButtonLampId, pInfo->dwFunction, pInfo->dwMode,
                          pInfo->strText);
        }                                              
        
        // If this is a SETLAMP request, then set the information to our PHONESTATUS
        // record.
        else if (pReq->GetCommand() == REQUEST_SETLAMP)
        {
            TSPISETBUTTONINFO* pInfo = (TSPISETBUTTONINFO*) pReq->GetDataPtr();
            SetLampState ((int)pInfo->dwButtonLampId, pInfo->dwMode);
        }
        
        // If this is a SETRING request, then set the new ring pattern and volume
        // into our status record.
        else if (pReq->GetCommand() == REQUEST_SETRING)
        {
            TSPIRINGPATTERN* pInfo = (TSPIRINGPATTERN*) pReq->GetDataPtr();
            SetRingMode (pInfo->dwRingMode);
            SetRingVolume (pInfo->dwVolume);
        }

        // If this is a HOOKSWITCH request, then set the state of the hookswitch
        // into our status record.
        else if (pReq->GetCommand() == REQUEST_SETHOOKSWITCH)
        {
            TSPIHOOKSWITCHPARAM* pInfo = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
            SetHookSwitch (pInfo->dwHookSwitchDevice, pInfo->dwParam);
        }
        
        // If this is a hookswitch GAIN request, then set the gain value into
        // our status record.
        else if (pReq->GetCommand() == REQUEST_SETHOOKSWITCHGAIN)
        {
            TSPIHOOKSWITCHPARAM* pInfo = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
            SetGain (pInfo->dwHookSwitchDevice, pInfo->dwParam);
        }
        
        // If this is a hookswitch VOLUME request, then set the new volume into
        // our status record.
        else if (pReq->GetCommand() == REQUEST_SETHOOKSWITCHVOL)
        {
            TSPIHOOKSWITCHPARAM* pInfo = (TSPIHOOKSWITCHPARAM*) pReq->GetDataPtr();
            SetVolume (pInfo->dwHookSwitchDevice, pInfo->dwParam);
        }
    }

}// CTSPIPhoneConnection::OnRequestComplete

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetDisplay
//
// Set the display to a known string.
//
void CTSPIPhoneConnection::SetDisplay (LPCTSTR pszBuff)
{
	CEnterCode sLock(this);  // Synch access to object
    for (int iRow = 0; iRow < m_Display.GetDisplaySize().cy; iRow++)
        for (int iCol = 0 ; iCol < m_Display.GetDisplaySize().cx; iCol++)
            m_Display.SetCharacterAtPosition (iCol, iRow, *pszBuff++);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);
            
}// CTSPIPhoneConnection::SetDisplay

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetID
//
// This method returns device information about the phone and its
// associated resource handles.
//
LONG CTSPIPhoneConnection::GetID (CString& strDevClass, LPVARSTRING lpDeviceID,
								  HANDLE hTargetProcess)
{   
	DEVICECLASSINFO* pDeviceClass = GetDeviceClass(strDevClass);
	if (pDeviceClass != NULL)
		return GetSP()->CopyDeviceClass (pDeviceClass, lpDeviceID, hTargetProcess);
    return PHONEERR_INVALDEVICECLASS;
    
}// CTSPIPhoneConnection::GetID

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::OnPhoneStatusChange
//
// This is called when anything in our PHONESTATUS record changes.
//
void CTSPIPhoneConnection::OnPhoneStatusChange(DWORD dwState, DWORD dwParam)
{                                                                         
    if ((m_dwPhoneStates & dwState) || dwState == PHONESTATE_REINIT)
        Send_TAPI_Event (PHONE_STATE, dwState, dwParam);

}// CTSPIPhoneConnection::OnPhoneStatusChange

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::OnButtonStateChange
//
// This method is invoked when a button changes state (UP/DOWN)
//
void CTSPIPhoneConnection::OnButtonStateChange (DWORD dwButtonID, DWORD dwMode, DWORD dwState)
{                                            
    if ((m_dwButtonModes & dwMode) && (m_dwButtonStates & dwState))
        Send_TAPI_Event (PHONE_BUTTON, dwButtonID, dwMode, dwState);

}// CTSPIPhoneConnection::OnButtonStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetRingMode
//
// Set the ring mode in the PHONESTATUS record and notify TAPI.  This
// should only be called by the worker code when the ring mode really
// changes on the device.
//
void CTSPIPhoneConnection::SetRingMode (DWORD dwRingMode)
{   
    // The ringmode should have already been verified.
	CEnterCode sLock(this);  // Synch access to object
    m_PhoneStatus.dwRingMode = dwRingMode;
    OnPhoneStatusChange (PHONESTATE_RINGMODE, dwRingMode);

}// CTSPIPhoneConnection::SetRingMode

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetRingVolume
//
// Set the ring volume in the PHONESTATUS record and notify TAPI.  This
// should only be called by the worker code when the ring volume really
// changes on the device.
//
void CTSPIPhoneConnection::SetRingVolume (DWORD dwRingVolume)
{                                      
    // The ring volume should have already been verified.
    m_PhoneStatus.dwRingVolume = dwRingVolume;
    OnPhoneStatusChange (PHONESTATE_RINGVOLUME);

}// CTSPIPhoneConnection::SetRingVolume

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::DevSpecificFeature
//
// Invoke a device-specific feature on this phone device.
//
LONG CTSPIPhoneConnection::DevSpecificFeature(DRV_REQUESTID /*dwRequestId*/,
                                             LPVOID /*lpParams*/, DWORD /*dwSize*/)
{                                          
    // Derived class must manage device-specific features.
    return LINEERR_OPERATIONUNAVAIL;
    
}// CTSPIPhoneConnection::DevSpecificFeature

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetIcon
//
// This function retrieves a service phone device-specific icon for display
// in user-interface dialogs.
//
LONG CTSPIPhoneConnection::GetIcon (CString& /*strDevClass*/, LPHICON /*lphIcon*/)
{
    // Return not available, TAPI will supply a default icon.
    return LINEERR_OPERATIONUNAVAIL;
    
}// CTSPIPhoneConnection::GetIcon

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GenericDialogData
//
// This method is called when a dialog which sent in a PHONE
// device ID called our UI callback.
//
LONG CTSPIPhoneConnection::GenericDialogData (LPVOID /*lpParam*/, DWORD /*dwSize*/)
{
	return FALSE;

}// CTSPIPhoneConnection::GenericDialogData

////////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetPhoneFeatures
//
// Used by the user to sets the current phone features
//
void CTSPIPhoneConnection::SetPhoneFeatures (DWORD dwFeatures)
{   
	CEnterCode sLock (this);

	// Make sure the capabilities structure reflects this ability.
	if ((m_PhoneCaps.dwPhoneFeatures & dwFeatures) == 0)
	{
		DTRACE(TRC_MIN, _T("PHONECAPS.dwPhoneFeatures missing 0x%lx bit\r\n"), dwFeatures);
		m_PhoneCaps.dwPhoneFeatures |= dwFeatures;	
		OnPhoneCapabiltiesChanged();
	}

	// Update it only if it has changed.
	if (m_PhoneStatus.dwPhoneFeatures != dwFeatures)
	{
		m_PhoneStatus.dwPhoneFeatures = dwFeatures;
		OnPhoneStatusChange (PHONESTATE_OTHER);
	}

}// CTSPIPhoneConnection::SetPhoneFeatures

////////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::OnPhoneCapabilitiesChanged
//
// This function is called when the phone capabilties change in the lifetime
// of the provider.
//
void CTSPIPhoneConnection::OnPhoneCapabiltiesChanged()
{
	OnPhoneStatusChange (PHONESTATE_CAPSCHANGE);

}// CTSPIPhoneConnection::OnPhoneCapabiltiesChanged

