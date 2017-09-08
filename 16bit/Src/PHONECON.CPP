/******************************************************************************/
//                                                                        
// PHONECONN.CPP - Source code for the CTSPIPhoneConnection object        
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code to manage the phone objects which are  
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
VOID CTSPIPhoneConnection::Init(CTSPIDevice* pDevice, DWORD dwPhoneId, DWORD dwPos)
{
    CTSPIConnection::Init(pDevice, dwPhoneId);

    // Initialize the phone device capabilities.  The permanent phone id
    // is a combination of our device id plus the position of the phone itself
    // within our device array.  This uniquely identifies the phone to us
    // with a single DWORD.  The MSB of the loword is always one, this indicates
    // a phone vs. line.
    m_PhoneCaps.dwPermanentPhoneID = ((pDevice->GetProviderID() << 16) + (dwPos&0x7fff)) | 0x8000;
    m_PhoneCaps.dwStringFormat = STRINGFORMAT_ASCII;  
    
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
VOID CTSPIPhoneConnection::SetupDisplay (int iColumns, int iRows, char cLineFeed)
{                                     
    m_Display.Init (iColumns, iRows, cLineFeed);
    m_PhoneCaps.dwDisplayNumRows = (DWORD) iRows;
    m_PhoneCaps.dwDisplayNumColumns = (DWORD) iColumns;

}// CTSPIPhoneConnection::SetupDisplay

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddUploadBuffer
//
// This function adds an upload data buffer to the phone.
//
int CTSPIPhoneConnection::AddUploadBuffer (DWORD dwSize)
{
    // Add it to our buffer array.
    int iPos = m_arrUploadBuffers.Add (dwSize);
    
    // Add a buffer count to our list.
    m_PhoneCaps.dwNumGetData++;
    ASSERT (iPos+1 == (int) m_PhoneCaps.dwNumGetData);
    
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
    int iPos = m_arrDownloadBuffers.Add (dwSize);
    
    // Add a buffer count to our list.
    m_PhoneCaps.dwNumSetData++;
    ASSERT (iPos+1 == (int)m_PhoneCaps.dwNumSetData);
    
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
VOID CTSPIPhoneConnection::AddHookSwitchDevice (DWORD dwHookSwitchDev, DWORD dwAvailModes, 
                                                DWORD dwCurrMode, DWORD dwVolume, DWORD dwGain)
{   
    BOOL fSupportsVolumeChange = FALSE, fSupportsGainChange = FALSE;
                                             
    if (dwVolume != (DWORD)-1L)
        fSupportsVolumeChange = TRUE;
    else
        dwVolume = 0xffff;

    if (dwGain != (DWORD)-1L)
        fSupportsGainChange = TRUE;
    else
        dwGain = 0xffff;            

    if (dwHookSwitchDev == PHONEHOOKSWITCHDEV_HANDSET)
    {
        m_PhoneCaps.dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HANDSET;
        
        if (fSupportsVolumeChange)
            m_PhoneCaps.dwVolumeFlags |= PHONEHOOKSWITCHDEV_HANDSET;
        if (fSupportsGainChange)
            m_PhoneCaps.dwGainFlags |= PHONEHOOKSWITCHDEV_HANDSET;
            
        m_PhoneCaps.dwHandsetHookSwitchModes = dwAvailModes;
        m_PhoneStatus.dwHandsetHookSwitchMode = dwCurrMode;
        m_PhoneStatus.dwHandsetVolume = dwVolume;
        m_PhoneStatus.dwHandsetGain = dwGain;
    }
    else if (dwHookSwitchDev == PHONEHOOKSWITCHDEV_SPEAKER)
    {
        m_PhoneCaps.dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_SPEAKER;

        if (fSupportsVolumeChange)
            m_PhoneCaps.dwVolumeFlags |= PHONEHOOKSWITCHDEV_SPEAKER;
        if (fSupportsGainChange)
            m_PhoneCaps.dwGainFlags |= PHONEHOOKSWITCHDEV_SPEAKER;
            
        m_PhoneCaps.dwSpeakerHookSwitchModes = dwAvailModes;
        m_PhoneStatus.dwSpeakerHookSwitchMode = dwCurrMode;
        m_PhoneStatus.dwSpeakerVolume = dwVolume;
        m_PhoneStatus.dwSpeakerGain = dwGain;
    }
    else if (dwHookSwitchDev == PHONEHOOKSWITCHDEV_HEADSET)
    {
        m_PhoneCaps.dwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HEADSET;

        if (fSupportsVolumeChange)
            m_PhoneCaps.dwVolumeFlags |= PHONEHOOKSWITCHDEV_HEADSET;
        if (fSupportsGainChange)
            m_PhoneCaps.dwGainFlags |= PHONEHOOKSWITCHDEV_HEADSET;
            
        m_PhoneCaps.dwHeadsetHookSwitchModes = dwAvailModes;
        m_PhoneStatus.dwHeadsetHookSwitchMode = dwCurrMode;
        m_PhoneStatus.dwHeadsetVolume = dwVolume;
        m_PhoneStatus.dwHeadsetGain = dwGain;
    }
    else
    {
        // Unsupported hookswitch device!
        ASSERT (FALSE);
    }                           
                           
}// CTSPIPhoneConnection::AddHookSwitchDevice

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddButton
//
// Add a new button to our button array
//
int CTSPIPhoneConnection::AddButton (DWORD dwFunction, DWORD dwMode, 
                                DWORD dwAvailLampModes, DWORD dwLampState, LPCSTR lpszText)
{                                  
    m_PhoneCaps.dwNumButtonLamps++;
    return m_arrButtonInfo.Add (dwFunction, dwMode, dwAvailLampModes, dwLampState, lpszText);

}// CTSPIPhoneConnection::AddButton

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::Send_TAPI_Event
//
// Calls an event back into the TAPI DLL
//
VOID CTSPIPhoneConnection::Send_TAPI_Event(DWORD dwMsg, DWORD dwP1, DWORD dwP2, DWORD dwP3)
{
    ASSERT(m_lpfnEventProc != NULL);                                  
    
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
    ASSERT (dwMsg <= 20);
    TRACE ("Send_TAPI_Event: <0x%lx> Phone=0x%lx, Msg=0x%lx (%s), P1=0x%lx, P2=0x%lx, P3=0x%lx\r\n",
                    (DWORD)this, (DWORD)GetPhoneHandle(), dwMsg, (LPCSTR)g_pszMsgs[dwMsg], 
                    dwP1, dwP2, dwP3);
#endif

    (*m_lpfnEventProc)(GetPhoneHandle(), dwMsg, dwP1, dwP2, dwP3);

}// CTSPIPhoneConnection::Send_TAPI_Event

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetPhoneHandle
//
// Return the TAPI phone handle associated with this object
//
HTAPIPHONE CTSPIPhoneConnection::GetPhoneHandle() const
{                                       
    return m_htPhone;

}// CTSPIPhoneConnection::GetPhoneHandle

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::CanHandleRequest
//
// Determine if our service provider is capable of handling the request.
// 
BOOL CTSPIPhoneConnection::CanHandleRequest(WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest(this, wRequest, dwData);                

}// CTSPIPhoneConnection::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::OnPhoneStatusChange
//
// This is called when anything in our PHONESTATUS record changes.
//
VOID CTSPIPhoneConnection::OnPhoneStatusChange(DWORD dwState, DWORD dwParam)
{                                                                         
    if ((m_dwPhoneStates & dwState) || dwState == PHONESTATE_REINIT)
        Send_TAPI_Event (PHONE_STATE, dwState, dwParam);

}// CTSPIPhoneConnection::OnPhoneStatusChange

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::OnButtonStateChange
//
// This method is invoked when a button changes state (UP/DOWN)
//
VOID CTSPIPhoneConnection::OnButtonStateChange (DWORD dwButtonID, DWORD dwMode, DWORD dwState)
{                                            
    if ((m_dwButtonModes & dwMode) && (m_dwButtonStates & dwState))
        Send_TAPI_Event (PHONE_BUTTON, dwButtonID, dwMode, dwState);

}// CTSPIPhoneConnection::OnButtonStateChange

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetButtonCount
//
// Returns the number of button/lamp pairs in our array
//
int CTSPIPhoneConnection::GetButtonCount() const
{                      
    ASSERT (m_PhoneCaps.dwNumButtonLamps == (DWORD) m_arrButtonInfo.GetSize());
    return m_arrButtonInfo.GetSize();

}// CTSPIPhoneConnection::GetButtonCount

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetButtonInfo
//
// Return the button object for the specified button.
//
const CPhoneButtonInfo* CTSPIPhoneConnection::GetButtonInfo(int iButtonID) const
{   
    if (m_arrButtonInfo.IsValidIndex (iButtonID))
        return m_arrButtonInfo.GetAt (iButtonID);
    return NULL;

}// CTSPIPhoneConnection::GetButtonInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetButtonInfo
//
// This function returns information about the specified phone 
// button.
//
LONG CTSPIPhoneConnection::GetButtonInfo (DWORD dwButtonId, LPPHONEBUTTONINFO lpButtonInfo)
{   
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
        cbSize = strName.GetLength()+1;

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
    if (cbSize && lpButtonInfo->dwTotalSize >= lpButtonInfo->dwNeededSize)
    {
        lpButtonInfo->dwButtonTextSize = cbSize;
        lpButtonInfo->dwButtonTextOffset  = lpButtonInfo->dwUsedSize;
        lpButtonInfo->dwUsedSize += cbSize;
        CopyBuffer ((LPSTR)lpButtonInfo+lpButtonInfo->dwButtonTextOffset, strName, cbSize);
    }
    return FALSE;
    
}// CTSPIPhoneConnection::GetButtonInfo   

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetPhoneCaps
//
// Return a pointer to the phone capabilities
//
LPPHONECAPS CTSPIPhoneConnection::GetPhoneCaps()
{
    return &m_PhoneCaps;
    
}// CTSPIPhoneConnection::GetPhoneCaps

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetPhoneStatus
//
// Return a pointer to the phone status
//
LPPHONESTATUS CTSPIPhoneConnection::GetPhoneStatus()
{                                       
    return &m_PhoneStatus;

}// CTSPIPhoneConnection::GetPhoneStatus

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GatherCapabilities
//
// This function queries a specified phone device to determine its
// telephony capabilities
//
LONG CTSPIPhoneConnection::GatherCapabilities(DWORD /*dwTSPIVersion*/, DWORD /*dwExtVersion*/, LPPHONECAPS lpPhoneCaps)
{  
    CString strPhoneName = GetName();
    CString strProviderInfo = GetSP()->GetProviderInfo();
    CString strPhoneInfo = GetConnInfo();
    int cbName=0, cbInfo=0, cbPhoneInfo=0;
    int cbButton = GetButtonCount() * sizeof(DWORD);
    int cbUpload = (int) (m_PhoneCaps.dwNumGetData * sizeof(DWORD));
    int cbDownload = (int) (m_PhoneCaps.dwNumSetData * sizeof(DWORD));

    if (!strPhoneName.IsEmpty())
        cbName = strPhoneName.GetLength()+1;
    if (!strProviderInfo.IsEmpty())
        cbInfo = strProviderInfo.GetLength()+1;
    if (!strPhoneInfo.IsEmpty())
        cbPhoneInfo = strPhoneInfo.GetLength()+1;

    // Save off the sections that TAPI provides
    m_PhoneCaps.dwTotalSize = lpPhoneCaps->dwTotalSize;       
    m_PhoneCaps.dwUsedSize = sizeof(PHONECAPS);
    m_PhoneCaps.dwNeededSize = 
    lpPhoneCaps->dwNeededSize = sizeof(PHONECAPS) + cbName + cbInfo + (cbButton*3) + cbUpload + cbDownload + cbPhoneInfo;

#ifdef _DEBUG
    // TAPI is supposed to verify the structure size for us.  Check it in DEBUG
    // builds just to be certain, but count on it in the retail version.
    if (lpPhoneCaps->dwTotalSize < m_PhoneCaps.dwUsedSize)
        return PHONEERR_STRUCTURETOOSMALL;
#endif
    
    // Copy the phone capabilities over from our structure
    CopyBuffer (lpPhoneCaps, &m_PhoneCaps, m_PhoneCaps.dwUsedSize);
    
    // Now add the phone name if we have the room
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbName)
    {
        lpPhoneCaps->dwPhoneNameSize = cbName;
        lpPhoneCaps->dwPhoneNameOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbName;
        CopyBuffer ((LPSTR)lpPhoneCaps + lpPhoneCaps->dwPhoneNameOffset, (LPCSTR)strPhoneName, cbName);
    }
    
    // Add the phone information if we have the room.
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbPhoneInfo)
    {
        lpPhoneCaps->dwPhoneInfoSize = cbPhoneInfo;
        lpPhoneCaps->dwPhoneInfoOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbPhoneInfo;
        CopyBuffer ((LPSTR)lpPhoneCaps+lpPhoneCaps->dwPhoneInfoOffset, (LPCSTR)strPhoneInfo, cbPhoneInfo);
    }
    
    // Add the Provider information if we have the room
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbInfo)
    {
        lpPhoneCaps->dwProviderInfoSize = cbInfo;
        lpPhoneCaps->dwProviderInfoOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbInfo;
        CopyBuffer ((LPSTR)lpPhoneCaps + lpPhoneCaps->dwProviderInfoOffset, (LPCSTR)strProviderInfo, cbInfo);
    }

    // Fill in the button information - mode first
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbButton)
    {
        LPDWORD lpDW = (LPDWORD)((LPSTR)lpPhoneCaps + lpPhoneCaps->dwUsedSize);
        lpPhoneCaps->dwButtonModesSize = cbButton;
        lpPhoneCaps->dwButtonModesOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbButton;
        
        for (int i = 0; i < GetButtonCount(); i++)
        {
            const CPhoneButtonInfo* pButton = GetButtonInfo(i);
            ASSERT(pButton != NULL);
            *lpDW++ = pButton->GetButtonMode();
        }
    }
    
    // Now the functions
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbButton)
    {
        LPDWORD lpDW = (LPDWORD)((LPSTR)lpPhoneCaps + lpPhoneCaps->dwUsedSize);
        lpPhoneCaps->dwButtonFunctionsSize = cbButton;
        lpPhoneCaps->dwButtonFunctionsOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbButton;
        
        for (int i = 0; i < GetButtonCount(); i++)
        {
            const CPhoneButtonInfo* pButton = GetButtonInfo(i);
            ASSERT(pButton != NULL);
            *lpDW++ = pButton->GetFunction();
        }
    }
    
    // Add the lamps
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbButton)
    {
        LPDWORD lpDW = (LPDWORD)((LPSTR)lpPhoneCaps + lpPhoneCaps->dwUsedSize);
        lpPhoneCaps->dwLampModesSize = cbButton;
        lpPhoneCaps->dwLampModesOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbButton;
        
        for (int i = 0; i < GetButtonCount(); i++)
        {
            const CPhoneButtonInfo* pButton = GetButtonInfo(i);
            ASSERT(pButton != NULL);
            *lpDW++ = pButton->GetAvailLampModes();
        }
    }
    
    // Add the download buffer sizes.
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbDownload)
    {
        LPDWORD lpDW = (LPDWORD)((LPSTR)lpPhoneCaps + lpPhoneCaps->dwUsedSize);
        lpPhoneCaps->dwSetDataSize = cbDownload;
        lpPhoneCaps->dwSetDataOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbDownload;
        for (int i = 0; i < m_arrDownloadBuffers.GetSize(); i++)
            *lpDW++ = m_arrDownloadBuffers[i];
    }

    // Add the upload buffer sizes.
    if (lpPhoneCaps->dwTotalSize >= lpPhoneCaps->dwUsedSize + cbUpload)
    {
        LPDWORD lpDW = (LPDWORD)((LPSTR)lpPhoneCaps + lpPhoneCaps->dwUsedSize);
        lpPhoneCaps->dwSetDataSize = cbUpload;
        lpPhoneCaps->dwSetDataOffset = lpPhoneCaps->dwUsedSize;
        lpPhoneCaps->dwUsedSize += cbUpload;
        for (int i = 0; i < m_arrUploadBuffers.GetSize(); i++)
            *lpDW++ = m_arrUploadBuffers[i];
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
    // Save off the values which TAPI supplies.
    m_PhoneStatus.dwTotalSize = lpPhoneStatus->dwTotalSize;
    m_PhoneStatus.dwNumOwners = lpPhoneStatus->dwNumOwners;
    m_PhoneStatus.dwNumMonitors = lpPhoneStatus->dwNumMonitors;
    m_PhoneStatus.dwOwnerNameSize = lpPhoneStatus->dwOwnerNameSize;
    m_PhoneStatus.dwOwnerNameOffset = lpPhoneStatus->dwOwnerNameOffset;

    // Now begin filling in our side.
    m_PhoneStatus.dwUsedSize = lpPhoneStatus->dwUsedSize = sizeof (PHONESTATUS);
    
    // Determine the space for the display.
    CString strDisplay = GetDisplayBuffer();
    int cbDisplay = 0;
    if (!strDisplay.IsEmpty())
        cbDisplay = strDisplay.GetLength()+1;
    int cbButton = GetButtonCount() * sizeof(DWORD);

    // Fill in the required size.
    m_PhoneStatus.dwNeededSize = lpPhoneStatus->dwNeededSize = m_PhoneStatus.dwUsedSize+cbDisplay+cbButton;
    if (lpPhoneStatus->dwTotalSize < lpPhoneStatus->dwUsedSize)
        return PHONEERR_STRUCTURETOOSMALL;
        
    // Copy over the basic PHONESTATUS structure
    CopyBuffer (lpPhoneStatus, &m_PhoneStatus, sizeof (PHONESTATUS));
    
    // Copy the display information.
    if (cbDisplay && lpPhoneStatus->dwTotalSize >= lpPhoneStatus->dwUsedSize+cbDisplay)
    {
        lpPhoneStatus->dwDisplaySize = cbDisplay;
        lpPhoneStatus->dwDisplayOffset = lpPhoneStatus->dwUsedSize;
        lpPhoneStatus->dwUsedSize += cbDisplay;
        CopyBuffer ((LPSTR)lpPhoneStatus+lpPhoneStatus->dwDisplayOffset, (LPCSTR)strDisplay, cbDisplay);
    }
    
    // Copy in the lamp mode information if enough space.
    if (cbButton && lpPhoneStatus->dwTotalSize >= lpPhoneStatus->dwUsedSize+cbButton)
    {
        lpPhoneStatus->dwLampModesSize = cbButton;
        lpPhoneStatus->dwLampModesOffset = lpPhoneStatus->dwUsedSize;
        lpPhoneStatus->dwUsedSize += cbButton;
        LPDWORD lpDW = (LPDWORD)((LPSTR)lpPhoneStatus + lpPhoneStatus->dwLampModesOffset);
        for (int i = 0; i < GetButtonCount(); i++)
        {
            const CPhoneButtonInfo* pButton = GetButtonInfo(i);
            *lpDW++ = pButton->GetLampMode();
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
    CString strDisplay = GetDisplayBuffer();

    lpVarString->dwStringFormat = STRINGFORMAT_ASCII;
    lpVarString->dwNeededSize = sizeof(VARSTRING);
    lpVarString->dwUsedSize = sizeof(VARSTRING);
    
    if (!strDisplay.IsEmpty())
    {
        int cbSize = strDisplay.GetLength()+1;
        lpVarString->dwNeededSize += cbSize;
        if (lpVarString->dwTotalSize >= lpVarString->dwNeededSize)
        {
            lpVarString->dwStringSize = cbSize;
            lpVarString->dwStringOffset = lpVarString->dwUsedSize;
            lpVarString->dwUsedSize += cbSize;
            CopyBuffer ((LPSTR)lpVarString + lpVarString->dwStringOffset, (LPCSTR)strDisplay, cbSize);
        }
    }
    else
    {
        lpVarString->dwStringSize = 0;
        lpVarString->dwStringOffset = 0;
    }
            
    return FALSE;
    
}// CTSPIPhoneConnection::GetDisplay

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetGain
//
// Return the current gain value for one of our hookswitch devices.
//
LONG CTSPIPhoneConnection::GetGain (DWORD dwHookSwitchDevice, LPDWORD lpdwGain)
{       
    // If we don't support the hook switch device.                         
    if ((m_PhoneCaps.dwHookSwitchDevs & dwHookSwitchDevice) != dwHookSwitchDevice)
        return PHONEERR_INVALHOOKSWITCHDEV;
    
    // The hook switch must be only a single bit.
    switch (dwHookSwitchDevice)
    {
        case PHONEHOOKSWITCHDEV_HANDSET:
            *lpdwGain = m_PhoneStatus.dwHandsetGain;
            break;
        case PHONEHOOKSWITCHDEV_HEADSET:
            *lpdwGain = m_PhoneStatus.dwHeadsetGain;
            break;
        case PHONEHOOKSWITCHDEV_SPEAKER:
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
    // If we don't support the hook switch device.                         
    if ((m_PhoneCaps.dwHookSwitchDevs & dwHookSwitchDevice) != dwHookSwitchDevice)
        return PHONEERR_INVALHOOKSWITCHDEV;
    
    // The hook switch must be only a single bit.
    switch (dwHookSwitchDevice)
    {
        case PHONEHOOKSWITCHDEV_HANDSET:
            *lpdwVolume = m_PhoneStatus.dwHandsetVolume;
            break;
        case PHONEHOOKSWITCHDEV_HEADSET:
            *lpdwVolume = m_PhoneStatus.dwHeadsetVolume;
            break;
        case PHONEHOOKSWITCHDEV_SPEAKER:
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
VOID CTSPIPhoneConnection::SetButtonInfo (int iButtonID, DWORD dwFunction, DWORD dwMode, LPCSTR pszName)
{
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
    CPhoneButtonInfo* pButton = (CPhoneButtonInfo*) GetButtonInfo (iButtonId);
    if (pButton != NULL)
    {
        DWORD dwCurrState = pButton->GetLampMode();
        pButton->SetLampMode (dwLampState);
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
// CTSPIPhoneConnection::GetRing
//
// Return the ring value for the phone device.
//
LONG CTSPIPhoneConnection::GetRing (LPDWORD lpdwRingMode, LPDWORD lpdwVolume) 
{                                
    *lpdwRingMode = m_PhoneStatus.dwRingMode;
    *lpdwVolume = m_PhoneStatus.dwRingVolume;
    return FALSE;

}// CTSPIPhoneConnection::GetRing

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
    if (GetPhoneHandle())
    	return PHONEERR_ALLOCATED;

    // Save off the event procedure for this phone and the TAPI
    // opaque phone handle which represents this device to the application.
    m_htPhone = htPhone;
    m_lpfnEventProc = lpfnEventProc;
    m_dwNegotiatedVersion = dwTSPIVersion;

    TRACE("Opening phone 0x%lX, TAPI handle=0x%lX, SP handle=0x%lX\r\n", GetDeviceID(), m_htPhone, (DWORD)this);
    
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
        TRACE("Closing phone 0x%lX, TAPI handle=0x%lX, SP handle=0x%lX\r\n", GetDeviceID(), GetPhoneHandle(), (DWORD)this);

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
// CTSPIPhoneConnection::GetDisplayBuffer
//
// Return the current display buffer as a string.  Each row is
// spaced out to the full size for our display.
//
CString CTSPIPhoneConnection::GetDisplayBuffer() const
{                                         
    return m_Display.GetTextBuffer();

}// CTSPIPhoneConnection::GetDisplayBuffer

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetCursorPos
//
// Return the cursor position of the display.
//
CPoint CTSPIPhoneConnection::GetCursorPos() const
{
    return m_Display.GetCursorPosition();

}// CTSPIPhoneConnection::GetCursorPos

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddDisplayChar
//
// Add a single character to our display at the specified position.
//
VOID CTSPIPhoneConnection::AddDisplayChar (char cChar)
{                                       
    m_Display.AddCharacter (cChar);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::AddDisplayChar

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetDisplayChar
//
// Set a character at a specified position in the buffer to a vlue.
//
VOID CTSPIPhoneConnection::SetDisplayChar (int iColumn, int iRow, char cChar)
{                                       
    m_Display.SetCharacterAtPosition (iColumn, iRow, cChar);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::SetDisplayChar

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::ResetDisplay
//
// Reset the display (cursor, buffer)
//
VOID CTSPIPhoneConnection::ResetDisplay()
{                                       
    m_Display.Reset();
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::ResetDisplay()

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetDisplayCursorPos
//
// Set the current cursor position for the display.
//
VOID CTSPIPhoneConnection::SetDisplayCursorPos (int iColumn, int iRow)
{                                            
    m_Display.SetCursorPosition (iColumn, iRow);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::SetDisplayCursorPos

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::ClearDisplayLine
//
// Clear a single display line
//
VOID CTSPIPhoneConnection::ClearDisplayLine (int iRow)
{                                         
    m_Display.ClearRow (iRow);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::ClearDisplayLine

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddDisplayString 
//
// Add a string to the display.
//
VOID CTSPIPhoneConnection::AddDisplayString (LPCSTR lpszText)
{                                          
    m_Display.AddString (lpszText);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);
    
}// CTSPIPhoneConnection::AddDisplayString 

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetRingMode
//
// Set the ring mode in the PHONESTATUS record and notify TAPI.  This
// should only be called by the worker code when the ring mode really
// changes on the device.
//
VOID CTSPIPhoneConnection::SetRingMode (DWORD dwRingMode)
{   
    // The ringmode should have already been verified.
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
VOID CTSPIPhoneConnection::SetRingVolume (DWORD dwRingVolume)
{                                      
    // The ring volume should have already been verified.
    m_PhoneStatus.dwRingVolume = dwRingVolume;
    OnPhoneStatusChange (PHONESTATE_RINGVOLUME);

}// CTSPIPhoneConnection::SetRingVolume

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetHookSwitch
//
// Change the hookswitch state of a hookswitch device(s).
//
VOID CTSPIPhoneConnection::SetHookSwitch (DWORD dwHookSwitchDev, DWORD dwMode)
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
VOID CTSPIPhoneConnection::SetVolume (DWORD dwHookSwitchDev, DWORD dwVolume)
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
VOID CTSPIPhoneConnection::SetGain (DWORD dwHookSwitchDev, DWORD dwGain)
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
VOID CTSPIPhoneConnection::OnRequestComplete(CTSPIRequest* pReq, LONG lResult)
{                                         
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
VOID CTSPIPhoneConnection::SetDisplay (const char * pszBuff)
{
    for (int iRow = 0; iRow < m_Display.GetDisplaySize().cy; iRow++)
        for (int iCol = 0 ; iCol < m_Display.GetDisplaySize().cx; iCol++)
            m_Display.SetCharacterAtPosition (iCol, iRow, *pszBuff++);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);
            
}// CTSPIPhoneConnection::SetDisplay

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
// CTSPIPhoneConnection::ConfigDialog
//
// Process a configuration dialog for the phone device.  This is called
// by phoneConfigDialog.
//
LONG CTSPIPhoneConnection::ConfigDialog (CWnd* /*pwndOwner*/, CString& /*strDeviceClass*/)
{
    // Derived class must manage configuration features.
    return LINEERR_OPERATIONUNAVAIL;

}// CTSPIPhoneConnection::ConfigDialog

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
// CTSPIPhoneConnection::GetID
//
// This method returns device information about the phone and its
// associated resource handles.
//
LONG CTSPIPhoneConnection::GetID (CString& strDevClass, LPVARSTRING lpDeviceID)
{   
    strDevClass.MakeLower();
    if (strDevClass == "tapi/phone" || strDevClass.IsEmpty())
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
    // Assume that the line/phones have a one-to-one coorelation.  If this
    // is not the case, override and replace the resulting value.
    else if (strDevClass == "tapi/line")
    {               
        CTSPIDevice* pDev = GetDeviceInfo();
        CTSPILineConnection* pLine = NULL;
        for (int iPos = 0; iPos < (int) pDev->GetLineCount(); iPos++)
        {
            if (pDev->GetPhoneConnectionInfo(iPos) == this)        
            {
                pLine = pDev->GetLineConnectionInfo(iPos);      
                break;
            }
        }
    
        if (pLine != NULL)
        {
            lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
            if (lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize)
            {
                lpDeviceID->dwUsedSize     = lpDeviceID->dwNeededSize;
                lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
                lpDeviceID->dwStringSize   = sizeof(DWORD);
                lpDeviceID->dwStringOffset = sizeof(VARSTRING);
                LPDWORD lpDW = (LPDWORD)((LONG)lpDeviceID + sizeof(VARSTRING));
                *lpDW = pLine->GetDeviceID();
            }   
            return FALSE;
        }
    }
    
    // Additional support must be provided by derived class
    return PHONEERR_INVALDEVICECLASS;
    
}// CTSPIPhoneConnection::GetID

////////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::OnPhoneCapabilitiesChanged
//
// This function is called when the phone capabilties change in the lifetime
// of the provider.
//
VOID CTSPIPhoneConnection::OnPhoneCapabiltiesChanged()
{
	OnPhoneStatusChange (PHONESTATE_CAPSCHANGE);

}// CTSPIPhoneConnection::OnPhoneCapabiltiesChanged

////////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::ForceClose
//
// This method may be used by the service provider to forcibly close the
// phone object.
//
void CTSPIPhoneConnection::ForceClose()
{
	Send_TAPI_Event (PHONE_CLOSE);

}// CTSPIPhoneConnection::ForceClose
