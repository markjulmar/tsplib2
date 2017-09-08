/******************************************************************************/
//                                                                        
// SPPHONE.CPP - Service Provider phoneXXX source code                             
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the methods which are phone specific in the 
// "CServiceProvider" class which is the main CWinApp derived class 
// in the SPLIB C++ library.      
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
#define new DEBUG_NEW
#endif

// We don't use all parameters since some functions are not completely
// implemented, so turn off the warning.
#pragma warning(disable:4100)

/////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneNegotiateTSPIVersion
//
// This method is called when TAPI wishes to negotiate available
// versions with us for any phone device installed.  The low and high
// version numbers passed are ones which the installed TAPI.DLL supports,
// and we are expected to return the highest value which we can support
// so TAPI knows what type of calls to make to us.
//
LONG CServiceProvider::phoneNegotiateTSPIVersion(
DWORD dwDeviceId,             // Valid phone device
DWORD dwLowVersion,           // Lowest TAPI version known
DWORD dwHiVersion,            // Highest TAPI version known
LPDWORD lpdwTSPVersion)       // Return version
{
	// Validate the parameter
	if (lpdwTSPVersion == NULL || IsBadWritePtr(lpdwTSPVersion, sizeof(DWORD)))
		return PHONEERR_INVALPARAM;

    // This should ALWAYS be a specific phone initialization request.
    if (GetConnInfoFromPhoneDeviceID(dwDeviceId) == NULL)
        return PHONEERR_BADDEVICEID;

    // Ok, device Id looks ok, do the version negotiation.
    *lpdwTSPVersion = GetSupportedVersion();
    if (dwLowVersion > *lpdwTSPVersion) // The app is too new for us
        return PHONEERR_INCOMPATIBLEAPIVERSION;

    // If the version supported is LESS than what we support,
    // then drop to the version it allows.  The library can handle
    // down to TAPI 1.3
    if (dwHiVersion < *lpdwTSPVersion)
    {
        if (dwHiVersion < TAPIVER_13)
            return LINEERR_INCOMPATIBLEAPIVERSION;
        *lpdwTSPVersion = dwHiVersion;
    }

    // Everything looked Ok.
    return FALSE;

}// CServiceProvider::phoneNegotiateTSPIVersion

/////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneNegotiateExtVersion
//
// This function returns the highest extension version number the
// service provider is willing to operate under for this device,
// given the range of possible extension values.
//
LONG CServiceProvider::phoneNegotiateExtVersion(
DWORD dwDeviceId,             // Valid phone device
DWORD dwTSPIVersion,          // TSPI version negotiated
DWORD dwLowVersion,           // Lowest TAPI version known
DWORD dwHiVersion,            // Highest TAPI version known
LPDWORD lpdwVersion)          // Return version
{
	// Validate the parameter
	if (lpdwVersion == NULL || IsBadWritePtr(lpdwVersion, sizeof(DWORD)))
		return PHONEERR_INVALPARAM;

    *lpdwVersion = 0;

    // This should ALWAYS be a specific phone initialization request.
    if (GetConnInfoFromPhoneDeviceID(dwDeviceId) == NULL)
    {
        DTRACE(TRC_MIN, _T("Phone ext negotiation failure, device Id out of range\r\n"));
        return PHONEERR_BADDEVICEID;
    }
    
    // We don't natively support EXT versions.  The derived class should
    // override this function and determine whether it can support functions
    // in this range.
    return PHONEERR_OPERATIONUNAVAIL; 

}// CServiceProvider::phoneNegotiateExtVersion

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneOpen
//
// This method opens the phone device whose device ID is given,
// returning the service provider's opaque handle for the device and
// retaining the TAPI opaque handle.
// 
LONG CServiceProvider::phoneOpen(
CTSPIPhoneConnection * pPhone,      // Connection
HTAPIPHONE htPhone,                 // TAPI opaque phone handle
LPHDRVPHONE lphdPhone,              // Storage area for SP handle
DWORD dwTSPIVersion,                // Version expected
PHONEEVENT lpfnEventProc)           // PHONEEVENT callback   
{
    // Store our service provider line handle to return.  This
    // is the handle which TAPI will use henceforth to talk to us about
    // this particular phone device.
    LONG lResult = pPhone->Open(htPhone, lpfnEventProc, dwTSPIVersion);
    if (ReportError (lResult) == FALSE)
        *lphdPhone = (HDRVPHONE) pPhone;

    return lResult;

}// CServiceProvider::phoneOpen

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneClose
//
// This method closes the specified open phone device after completing
// or aborting all outstanding asynchronous requests on the device.
//
LONG CServiceProvider::phoneClose(CTSPIPhoneConnection* pPhone)
{
    return pPhone->Close();

}// CServiceProvider::phoneClose

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetButtonInfo
//
// This function returns information about the specified phone 
// button.
//
LONG CServiceProvider::phoneGetButtonInfo(
CTSPIPhoneConnection* pPhone,    // Connection
DWORD dwButtonId,                // Specifies the button on the phone
LPPHONEBUTTONINFO lpButtonInfo)  // Button information return buffer   
{
    return pPhone->GetButtonInfo (dwButtonId, lpButtonInfo);
    
}// CServiceProvider::phoneGetButtonInfo

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetDevCaps
// 
// This function queries a specified phone device to determine its
// telephony capabilities
//
LONG CServiceProvider::phoneGetDevCaps(
CTSPIPhoneConnection* pPhone,    // Connection
DWORD dwTSPIVersion,             // TSPI version
DWORD dwExtVersion,              // Extensions version
LPPHONECAPS lpPhoneCaps)         // Phone capabilities
{
    return pPhone->GatherCapabilities (dwTSPIVersion, dwExtVersion, lpPhoneCaps);

}// CServiceProvider::phoneGetDevCaps

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetDisplay
//
// This function returns the current contents of the specified phone
// display.
//
LONG CServiceProvider::phoneGetDisplay(
CTSPIPhoneConnection* pPhone,       // Connection
LPVARSTRING lpString)               // Buffer to store display into
{                                   
    return pPhone->GetDisplay (lpString);

}// CServiceProvider::phoneGetDisplay

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetExtensionID
//
// This function retrieves the extension ID that the service provider
// supports for the indicated device.
//
LONG CServiceProvider::phoneGetExtensionID(
CTSPIPhoneConnection* pConn,        // Connection
DWORD dwTSPIVersion,                // Interface version #
LPPHONEEXTENSIONID lpExtensionId)   // Extension structure
{
    // Base class offers no extension support - mark it as
    // all zeros for the derived class to support.
    lpExtensionId->dwExtensionID0 = 0;
    lpExtensionId->dwExtensionID1 = 0;
    lpExtensionId->dwExtensionID2 = 0;
    lpExtensionId->dwExtensionID3 = 0;

    return FALSE;

}// CServiceProvider::phoneGetExtensionID

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetGain
//
// This method returns the gain setting of the microphone of the
// specified phone's hookswitch device.
//
LONG CServiceProvider::phoneGetGain(
CTSPIPhoneConnection* pPhone,       // Connection
DWORD dwHookSwitchDev,              // Which hookswitchd device?
LPDWORD lpdwGain)                   // Return gain buffer   
{
    return pPhone->GetGain (dwHookSwitchDev, lpdwGain);

}// CServiceProvider::GetGain

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetHookSwitch
//
// This function retrieves the current hook switch setting of the
// specified open phone device
//
LONG CServiceProvider::phoneGetHookSwitch(
CTSPIPhoneConnection* pPhone,    // Connection
LPDWORD lpdwHookSwitchDevs)      // Buffer for hookswitch states
{
    return pPhone->GetHookSwitch (lpdwHookSwitchDevs);
    
}// CServiceProvider::GetHookSwitch

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetID
//
// This function retrieves the device id of the specified open phone
// handle (or some other media handle if available).
//
LONG CServiceProvider::phoneGetID(
CTSPIPhoneConnection* pPhone,    // Connection
CString& strDevClass,            // Device class
LPVARSTRING lpDeviceID,          // Area for storage
HANDLE hTargetProcess)           // Process to duplicate handle for.
{                              
    return pPhone->GetID(strDevClass, lpDeviceID, hTargetProcess);

}// CServiceProvider::phoneGetID

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetLamp
//
// This function returns the current lamp mode of the specified
// lamp.
//
LONG CServiceProvider::phoneGetLamp(
CTSPIPhoneConnection* pPhone,    // Connection
DWORD dwButtonLampId,            // Lamp indicator id
LPDWORD lpdwLampMode)            // Return buffer for mode
{
    return pPhone->GetLamp (dwButtonLampId, lpdwLampMode);

}// CServiceProvider::phoneGetLamp

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetStatus
//
// This function queries the specified open phone device for its
// overall status.
//
LONG CServiceProvider::phoneGetStatus(
CTSPIPhoneConnection* pPhone,
LPPHONESTATUS lpPhoneStatus)
{
    return pPhone->GatherStatus (lpPhoneStatus);
   
}// CServiceProvider::phoneGetStatus

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetVolume
//
// This function returns the volume setting of the phone device.
//
LONG CServiceProvider::phoneGetVolume(
CTSPIPhoneConnection* pPhone,    // Connection
DWORD dwHookSwitchDev,           // Which hookswitch device?
LPDWORD lpdwVolume)              // Return volume
{
    return pPhone->GetVolume (dwHookSwitchDev, lpdwVolume);

}// CServiceProvider::phoneGetVolume

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetButtonInfo
//
// This function sets information about the specified button on the
// phone device.
//
LONG CServiceProvider::phoneSetButtonInfo(
CTSPIPhoneConnection* pPhone,    // Connection
DRV_REQUESTID dwRequestId,       // Asynch request id
DWORD dwButtonId,                // Button/Lamp id
LPPHONEBUTTONINFO const lpPhoneInfo) // Phone info
{
    // Allocate a TSPISETBUTTONINFO structure
    TSPISETBUTTONINFO* pButtonInfo = new TSPISETBUTTONINFO;
    if (pButtonInfo == NULL)
        return PHONEERR_NOMEM;

    pButtonInfo->dwButtonLampId = dwButtonId;
    pButtonInfo->dwFunction = lpPhoneInfo->dwButtonFunction;
    pButtonInfo->dwMode = lpPhoneInfo->dwButtonMode;
    if (lpPhoneInfo->dwButtonTextSize > 0 && 
        lpPhoneInfo->dwButtonTextOffset > 0)
    {
        LPCTSTR lpszBuff = (LPCTSTR)lpPhoneInfo + lpPhoneInfo->dwButtonTextOffset;
        pButtonInfo->strText = lpszBuff;
    }
    
    // Pass the request to the phone object
    LONG lResult = pPhone->SetButtonInfo (dwRequestId, pButtonInfo);
    if (ReportError(lResult))
        delete pButtonInfo;
    return lResult;

}// CServiceProvider::phoneSetButtonInfo

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetGain
//
// This function sets the gain of the microphone of the specified hook
// switch device.
//
LONG CServiceProvider::phoneSetGain(
CTSPIPhoneConnection* pPhone,    // Connection
DRV_REQUESTID dwRequestId,       // RequestId
DWORD dwHookSwitchDev,           // Hook switch device
DWORD dwGain)                    // Gain to set it to
{
    // Allocate a hookswitch param structure                        
    TSPIHOOKSWITCHPARAM* pHSParam = new TSPIHOOKSWITCHPARAM;
    if (pHSParam == NULL)
        return PHONEERR_NOMEM;
        
    pHSParam->dwHookSwitchDevice = dwHookSwitchDev;
    pHSParam->dwParam = dwGain;
        
    // Pass the request to the phone object
    LONG lResult = pPhone->SetGain (dwRequestId, pHSParam);
    if (ReportError(lResult))
        delete pHSParam;
    return lResult;

}// CServiceProvider::phoneSetGain

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetVolume
// 
// This function either sets the volume of the speaker or the 
// specified hookswitch device on the phone
//
LONG CServiceProvider::phoneSetVolume(
CTSPIPhoneConnection* pPhone,    // Connection
DRV_REQUESTID dwRequestId,       // Request id
DWORD dwHookSwitchDev,           // Hookswitch device to work with
DWORD dwVolume)                  // Volume for device
{
	// Allocate a hookswitch param structure                        
    TSPIHOOKSWITCHPARAM* pHSParam = new TSPIHOOKSWITCHPARAM;
    if (pHSParam == NULL)
        return PHONEERR_NOMEM;
        
    pHSParam->dwHookSwitchDevice = dwHookSwitchDev;
    pHSParam->dwParam = dwVolume;
        
    // Pass the request to the phone object
    LONG lResult = pPhone->SetVolume (dwRequestId, pHSParam);
    if (ReportError(lResult))
        delete pHSParam;
    return lResult;
   
}// CServiceProvider::phoneSetVolume

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetHookSwitch
//
// This function sets the hook state of the specified open phone's
// hookswitch device to the specified mode.  Only the hookswitch
// state of the hookswitch devices listed is affected.
//
LONG CServiceProvider::phoneSetHookSwitch(
CTSPIPhoneConnection* pPhone,    // Connection
DRV_REQUESTID dwRequestId,       // Request id
DWORD dwHookSwitchDevs,          // Hook switch device
DWORD dwHookSwitchMode)          // Mode to set to
{
    // Allocate a hookswitch param structure                        
    TSPIHOOKSWITCHPARAM* pHSParam = new TSPIHOOKSWITCHPARAM;
    if (pHSParam == NULL)
        return PHONEERR_NOMEM;
        
    pHSParam->dwHookSwitchDevice = dwHookSwitchDevs;
    pHSParam->dwParam = dwHookSwitchMode;
        
    // Pass the request to the phone object
    LONG lResult = pPhone->SetHookSwitch (dwRequestId, pHSParam);
    if (ReportError(lResult))
        delete pHSParam;
    return lResult;

}// CServiceProvider::phoneSetHookSwitch

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetLamp
//
// This function causes the specified lamp to be set on the phone
// device to the specified mode.
//
LONG CServiceProvider::phoneSetLamp(
CTSPIPhoneConnection* pPhone,    // Connection
DRV_REQUESTID dwRequestId,       // Request id   
DWORD dwButtonLampId,            // Lamp to set
DWORD dwLampMode)                // Mode to set to
{   
    // Allocate a button state structure
    TSPISETBUTTONINFO* pButton = new TSPISETBUTTONINFO;
    if (pButton == NULL)
        return PHONEERR_NOMEM;
        
    pButton->dwButtonLampId = dwButtonLampId;
    pButton->dwMode = dwLampMode;
    
    // Submit it to the phone object
    LONG lResult = pPhone->SetLamp (dwRequestId, pButton);
    if (ReportError(lResult))
        delete pButton;
    return lResult;

}// CServiceProvider::phoneSetLamp

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetRing
//
// This function rings the specified open phone device using the
// specified ring mode and volume.
//
LONG CServiceProvider::phoneSetRing(
CTSPIPhoneConnection* pPhone,    // Connection
DRV_REQUESTID dwRequestId,       // Request id
DWORD dwRingMode,                // Ring pattern
DWORD dwVolume)                  // Volume for ringer
{       
    // Allocate a RINGPATTERN structure
    TSPIRINGPATTERN* pRingPattern = new TSPIRINGPATTERN;
    if (pRingPattern == NULL)
        return PHONEERR_NOMEM;
        
    pRingPattern->dwRingMode = dwRingMode;
    pRingPattern->dwVolume = dwVolume;
    
    // Send it to the phone object
    LONG lResult = pPhone->SetRing (dwRequestId, pRingPattern);
    if (ReportError(lResult))
        delete pRingPattern;
    return lResult;

}// CServiceProvider::phoneSetRing

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetDisplay
//
// This method sets the display for the phone device
//
LONG CServiceProvider::phoneSetDisplay(
CTSPIPhoneConnection* pPhone,       // Connection
DRV_REQUESTID dwRequestID,          // Request id
DWORD dwRow,                        // Row
DWORD dwCol,                        // Column
LPCTSTR lpszDisplay,                // Display string
DWORD dwSize)                       // Size of display
{
    // Build a display change request
    TSPIPHONESETDISPLAY* lpDisplay = new TSPIPHONESETDISPLAY;
    if (lpDisplay == NULL)
        return PHONEERR_NOMEM;
        
    lpDisplay->dwRow = dwRow;
    lpDisplay->dwColumn = dwCol;
    if (lpszDisplay != NULL && dwSize > 0)
    {
        lpDisplay->lpvDisplay = (LPVOID) AllocMem(dwSize);
        CopyBuffer(lpDisplay->lpvDisplay, lpszDisplay, dwSize);
        lpDisplay->dwSize = dwSize;
    }

    // Pass it to the phone object
    LONG lResult = pPhone->SetDisplay (dwRequestID, lpDisplay);
    if (ReportError(lResult))
        delete lpDisplay;
    return lResult;

}// CServiceProvider::phoneSetDisplay

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneConfigDialog
//
// Causes the provider of the specified phone device to display a modal 
// dialog (attached to the application's hwndOwner) that allows the user to 
// configure parameters related to the phone device specified by dwDeviceID.
//
LONG CServiceProvider::phoneConfigDialog(
DWORD /*dwDeviceID*/,						// Device ID for phone
CWnd* /*pwndOwner*/,						// Owner window handle
CString& /*strDevClass*/,					// Device class
TUISPIDLLCALLBACK /*lpfnDLLCallback*/)		// DLL instance	
{
	return PHONEERR_OPERATIONUNAVAIL;

}// CServiceProvider::phoneConfigDialog

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneDevSpecific
//
// Is used as a general extension mechanism to enable a Telephony API 
// implementation to provide features not described in the other TAPI functions. 
// The meanings of these extensions are device specific.
//
LONG CServiceProvider::phoneDevSpecific(
CTSPIPhoneConnection* pPhone,
DRV_REQUESTID dwRequestID, 
LPVOID lpParams, 
DWORD dwSize) 
{
    return pPhone->DevSpecificFeature (dwRequestID, lpParams, dwSize);
    
}// CServiceProvider::phoneDevSpecific

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetIcon
//
// Allows an application to retrieve a service phone device-specific 
// (or provider-specific) icon that can be displayed to the user. 
//
LONG CServiceProvider::phoneGetIcon(
CTSPIPhoneConnection* pPhone, 
CString& strDevClass, 
LPHICON lphIcon) 
{
    *lphIcon = NULL;                
    return pPhone->GetIcon (strDevClass, lphIcon);
    
}// CServiceProvider::phoneGetIcon

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetData
//
// Uploads the information from the specified location in the open phone 
// device to the specified buffer.
//
LONG CServiceProvider::phoneGetData(
CTSPIPhoneConnection* pPhone, 
DWORD dwDataId, 
LPVOID lpData, 
DWORD dwSize) 
{                  
    // Move the data to another structure.
    TSPIPHONEDATA* pPhoneData = new TSPIPHONEDATA;
    if (pPhoneData == NULL)
        return PHONEERR_NOMEM;
    
    // Move the data over to local storage.    
    pPhoneData->dwDataID = dwDataId;
    pPhoneData->dwSize = dwSize;
    pPhoneData->lpBuffer = lpData;
        
    // Submit the request to the phone class.  It will delete the pPhoneData
    // automatically for us.
    return pPhone->GetData (pPhoneData);

}// CServiceProvider::phoneGetData

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneGetRing
//
// Enables an application to query the specified open phone device as to 
// its current ring mode.
//
LONG CServiceProvider::phoneGetRing(
CTSPIPhoneConnection* pPhone,
LPDWORD lpdwRingMode, 
LPDWORD lpdwVolume) 
{
    // Pass it onto the phone device.               
    return pPhone->GetRing (lpdwRingMode, lpdwVolume);
    
}// CServiceProvider::phoneGetRing

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSelectExtVersion
//
// This method selects the indicated extension version for the phone 
// device.  Subsequent requests operate according to that extension version.
//
LONG CServiceProvider::phoneSelectExtVersion(
CTSPIPhoneConnection* pPhone, 
DWORD dwExtVersion) 
{                                    
    // We don't support device specific extensions in the base library.
    return PHONEERR_OPERATIONUNAVAIL;
    
}// CServiceProvider::phoneSelectExtVersion

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetData
//
// Downloads the information in the specified buffer to the opened phone 
// device at the selected data ID.
//
LONG CServiceProvider::phoneSetData(
CTSPIPhoneConnection* pPhone, 
DRV_REQUESTID dwRequestId, 
DWORD dwDataId, 
LPCVOID lpData, 
DWORD dwSize) 
{
    // Move the data to another structure.
    TSPIPHONEDATA* pPhoneData = new TSPIPHONEDATA;
    if (pPhoneData == NULL)
        return PHONEERR_NOMEM;
    
    // Move the data over to local storage.    
    pPhoneData->dwDataID = dwDataId;
    pPhoneData->dwSize = dwSize;
    
    if (dwSize > 0)
    {
        pPhoneData->lpBuffer = AllocMem (dwSize);
        if (pPhoneData->lpBuffer == NULL)
        {
            delete pPhoneData;
            return PHONEERR_NOMEM;
        }
        CopyBuffer (pPhoneData->lpBuffer, lpData, dwSize);
    }
    else
        pPhoneData->lpBuffer = NULL;
        
    // Submit the request to the phone class
    LONG lResult = pPhone->SetData (dwRequestId, pPhoneData);        
    if (ReportError(lResult))
        delete pPhoneData;
    return lResult;

}// CServiceProvider::phoneSetData

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::phoneSetStatusMessages
//
// Enables an application to monitor the specified phone device 
// for selected status events.
//
LONG CServiceProvider::phoneSetStatusMessages(
CTSPIPhoneConnection* pPhone, 
DWORD dwPhoneStates, 
DWORD dwButtonModes, 
DWORD dwButtonStates) 
{
    return pPhone->SetStatusMessages (dwPhoneStates, dwButtonModes, dwButtonStates);
    
}// CServiceProvider::phoneSetStatusMessages
