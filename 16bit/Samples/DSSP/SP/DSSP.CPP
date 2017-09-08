/*****************************************************************************/
//
// DSSP.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the main service provider override class.
// All driver related calls are run through this class.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
/*****************************************************************************/

#include "stdafx.h"
#include "dssp.h"
#include <mmsystem.h>

IMPLEMENT_DYNCREATE(CDSLine, CTSPILineConnection)
IMPLEMENT_DYNCREATE(CDSPhone, CTSPIPhoneConnection)
IMPLEMENT_DYNCREATE(CDSDevice, CTSPIDevice)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////
// Globals

CDSProvider theSP;  // the one and only application object

////////////////////////////////////////////////////////////////////////////
// CDSProvider::CDSProvider
//
// Constructor for the TBridge service provider
//
CDSProvider::CDSProvider() : 
    CServiceProvider("DSSP", "DSSPEXE.EXE", TAPIVER_14, 
            "Sample Digital Switch Service Provider")
{   
	// Setup our line device and phone device object override.
	SetRuntimeObjects (RUNTIME_CLASS(CDSDevice),					// Our device override
					   NULL,										// Request object
					   RUNTIME_CLASS(CDSLine),						// Line override
					   NULL, NULL, NULL,							// Address/Call/ConfCall objects
					   RUNTIME_CLASS(CDSPhone));					// Phone override	

}// CDSProvider::CDSProvider

////////////////////////////////////////////////////////////////////////////
// CDSProvider::providerEnumDevices
//
// Force our provider to export a single line and phone device regardless
// of the values recorded in the TELEPHON.INI.
//
LONG CDSProvider::providerEnumDevices(DWORD dwProviderId, LPDWORD lpNumLines,
                                 LPDWORD lpNumPhones, HPROVIDER hProvider,
                                 LINEEVENT lpfnLineCreateProc, 
                                 PHONEEVENT lpfnPhoneCreateProc)
{   
    // Enforce the number of lines/phones we have.                               
    LONG lResult = CServiceProvider::providerEnumDevices(dwProviderId, lpNumLines,
                        lpNumPhones, hProvider, lpfnLineCreateProc, lpfnPhoneCreateProc);
    *lpNumLines = 1;
    *lpNumPhones = 1;
    return lResult;

}// CDSProvider::providerEnumDevices

/////////////////////////////////////////////////////////////////////////////
// CDSProvider::lineGetID
//
// Supply multimedia WAVE input/output device ids for our emulated
// digital provider.  Use the default multimedia device as our I/O
// device.
//
LONG CDSProvider::lineGetID(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, 
                CTSPICallAppearance* pCall, CString& strDevClass, LPVARSTRING lpDeviceID)
{                           
	if (pCall && pCall->GetCallState() != LINECALLSTATE_IDLE)
	{
	    // For WAVE input, return the default microphone device for the PC
	    if (strDevClass == "wave/in" && waveInGetNumDevs() > 0)
        {
            // Always return the FIRST entry.
            lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
            if (lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize)
            {
                lpDeviceID->dwUsedSize     = lpDeviceID->dwNeededSize;
                lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
                lpDeviceID->dwStringSize   = sizeof(DWORD);
                lpDeviceID->dwStringOffset = sizeof(VARSTRING);
                LPDWORD lpDW = (LPDWORD)((LONG)lpDeviceID + sizeof(VARSTRING));
                *lpDW = 0L;
                return FALSE;
            }
            return LINEERR_STRUCTURETOOSMALL;
	    }
	    // For WAVE output, return the default output device (speaker)
	    else if (strDevClass == "wave/out" && waveOutGetNumDevs() > 0)
        {
            // Always return the FIRST entry.
            lpDeviceID->dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);
            if (lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize)
            {
                lpDeviceID->dwUsedSize     = lpDeviceID->dwNeededSize;
                lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
                lpDeviceID->dwStringSize   = sizeof(DWORD);
                lpDeviceID->dwStringOffset = sizeof(VARSTRING);
                LPDWORD lpDW = (LPDWORD)((LONG)lpDeviceID + sizeof(VARSTRING));
                *lpDW = 0L;
                return FALSE;
            }
            return LINEERR_STRUCTURETOOSMALL;
        }
	}
    return CServiceProvider::lineGetID (pConn, pAddr, pCall, strDevClass, lpDeviceID);

}// CDSProvider::lineGetID
