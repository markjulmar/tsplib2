/******************************************************************************/
//                                                                        
// ATSP.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our driver code for the ATSP sample.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider drives a Hayes compatible AT style modem.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
/******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "atsp.h"
#include "atspint.h"
#include "config.h"
#include "talkdrop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////
// Globals

CATSPProvider theSP;

////////////////////////////////////////////////////////////////////////////
// CATSPProvider::CATSPProvider
//
// Constructor for the service provider
//
CATSPProvider::CATSPProvider() : 
    CServiceProvider ("ATSPV2", STR_EXENAME, TAPIVER_14, 
                      "ATSP C++ Service Provider")
{
	// Turn on the WM_CTLCOLOR hook for our configuration dialogs.
	SetDialogBkColor();

	// Setup our line device object override.
	SetRuntimeObjects (NULL, NULL, RUNTIME_CLASS(CATSPLine));
    
}// CATSPProvider::CATSPProvider
                    
////////////////////////////////////////////////////////////////////////////
// CATSPProvider::providerEnumDevices
//
// Return the number of supported lines/phones
//
LONG CATSPProvider::providerEnumDevices(DWORD dwProviderId, LPDWORD lpNumLines,
                                 LPDWORD lpNumPhones, HPROVIDER hProvider,
                                 LINEEVENT lpfnLineCreateProc, 
                                 PHONEEVENT lpfnPhoneCreateProc)
{   
    // Enforce the number of lines/phones we have.                               
    LONG lResult = CServiceProvider::providerEnumDevices(dwProviderId, lpNumLines,
                        lpNumPhones, hProvider, lpfnLineCreateProc, lpfnPhoneCreateProc);
    *lpNumLines = 1;
    *lpNumPhones = 0;
    return lResult;

}// CATSPProvider::providerEnumDevices

/////////////////////////////////////////////////////////////////////////////
// CATSPProvider::lineGetID
//
// Override to supply COMM handle for an open call.
//
LONG CATSPProvider::lineGetID(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, 
		CTSPICallAppearance* pCall, CString& strDevClass, LPVARSTRING lpDeviceID)
{                           
    // If we have the stuff we need to give back a COMM handle, then ask
    // the companion application for it.
    if (strDevClass == "comm/handle16" && pCall && 
        pCall->GetCallState() != LINECALLSTATE_IDLE)
    {                                   
        HANDLE hComm = (HANDLE) SendThreadRequest (COMMAND_GETCOMMHANDLE, pConn->GetPermanentDeviceID());
        if (hComm > 0)
        {
            lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
            if (lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize)
            {
                lpDeviceID->dwUsedSize     = lpDeviceID->dwNeededSize;
                lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
                lpDeviceID->dwStringSize   = sizeof(DWORD);
                lpDeviceID->dwStringOffset = sizeof(VARSTRING);
                LPDWORD lpDW = (LPDWORD)((LONG)lpDeviceID + sizeof(VARSTRING));
                *lpDW = (DWORD)(UINT)hComm;
                return FALSE;
            }
            return LINEERR_STRUCTURETOOSMALL;
        }
    }
    return CServiceProvider::lineGetID (pConn, pAddr, pCall, strDevClass, lpDeviceID);

}// CATSPProvider::lineGetID

///////////////////////////////////////////////////////////////////////////
// CATSPProvider::OpenDevice
//
// Open a device and return a handle to the device.
//
BOOL CATSPProvider::OpenDevice (CTSPIConnection* pConn)
{   
    TRACE("Opening device 0x%lX\r\n", pConn->GetPermanentDeviceID());
    SERIALINFO si;
    si.iPort = (int) (GetSP()->ReadProfileDWord(pConn->GetDeviceInfo()->GetProviderID(), GetString(IDS_PORT), 1));
    si.dwConnId = pConn->GetPermanentDeviceID();
    si.dwBaud = GetSP()->ReadProfileDWord(pConn->GetDeviceInfo()->GetProviderID(), GetString(IDS_BAUD), 9600);
    return SendThreadRequest (COMMAND_OPENSERIAL, (LPARAM)&si, TRUE, sizeof(SERIALINFO));

}// CServiceProvider::OpenDevice

////////////////////////////////////////////////////////////////////////////
// GetString
//
// Returns a character string from the resource table
//
const char * GetString(UINT iResId)
{
   static CString strBuff;
   strBuff.LoadString(iResId);
   return (const char *) strBuff;

}// GetString


