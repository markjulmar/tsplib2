/******************************************************************************/
//                                                                        
// SP.CPP - Service Provider Base source code                             
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the general methods for the "CServiceProvider" class    
// which is the main CWinApp derived class in the SPLIB C++ library.      
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
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// This is ignored by the loader, so doesn't impact load time or image size.
#pragma comment (exestr, "TSP++ 2.24 Copyright (C) 2000 JulMar Technology, Inc.")

/*-----------------------------------------------------------------------------*/
// GLOBALS and CONSTANTS
/*-----------------------------------------------------------------------------*/
const UINT LIBRARY_INTERVAL = 500;
LPCTSTR gszDevice = _T("Device%ld");
LPCTSTR gszTelephonyKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Telephony");

#pragma data_seg ("init_data")
// These strings should be placed into a swappable segment since they 
// are only used during INIT to determine the capabilities of the provider.
// TAPI 2.x and above no longer exports by ordinal.
static LPCSTR gszEntryPoints[] = {
    "TSPI_lineAccept",							//	0
    "TSPI_lineAddToConference",					//	1
    "TSPI_lineAnswer",							//	2
    "TSPI_lineBlindTransfer",					//	3
    "TSPI_lineCompleteCall",					//	4
    "TSPI_lineCompleteTransfer",				//	5	
	"TSPI_lineConditionalMediaDetection",		//	6 
	"TSPI_lineDevSpecific",						//  7 
	"TSPI_lineDevSpecificFeature",				//  8
    "TSPI_lineDial",							//  9
    "TSPI_lineForward",							// 10
	"TSPI_lineGatherDigits",					// 11
	"TSPI_lineGenerateDigits",					// 12
	"TSPI_lineGenerateTone",					// 13
	"TSPI_lineGetDevConfig",					// 14
	"TSPI_lineGetExtensionID",					// 15
	"TSPI_lineGetIcon",							// 16
	"TSPI_lineGetID",							// 17
	"TSPI_lineGetLineDevStatus",				// 18
    "TSPI_lineHold",							// 19	
    "TSPI_lineMakeCall",						// 20	
	"TSPI_lineMonitorDigits",					// 21
	"TSPI_lineMonitorMedia",					// 22
	"TSPI_lineMonitorTones",					// 23
	"TSPI_lineNegotiateExtVersion",				// 24
    "TSPI_linePark",							// 25
    "TSPI_linePickup",							// 26
	"TSPI_linePrepareAddToConference",		    // 27
    "TSPI_lineRedirect",						// 28
	"TSPI_lineReleaseUserUserInfo",				// 29
    "TSPI_lineRemoveFromConference",			// 30
    "TSPI_lineSecureCall",						// 31
	"TSPI_lineSelectExtVersion",				// 32
    "TSPI_lineSendUserUserInfo",				// 33
    "TSPI_lineSetCallData",						// 34
    "TSPI_lineSetCallParams",					// 35	
    "TSPI_lineSetCallQualityOfService",			// 36
    "TSPI_lineSetCallTreatment",				// 37
	"TSPI_lineSetDevConfig",					// 38
    "TSPI_lineSetLineDevStatus",				// 39
	"TSPI_lineSetMediaControl",					// 40
    "TSPI_lineSetTerminal",						// 41	
    "TSPI_lineSetupConference",					// 42
    "TSPI_lineSetupTransfer",					// 43
    "TSPI_lineSwapHold",						// 44	
    "TSPI_lineUncompleteCall",					// 45
    "TSPI_lineUnhold",							// 46
    "TSPI_lineUnpark",							// 47
	"TSPI_phoneDevSpecific",					// 48
	"TSPI_phoneGetButtonInfo",					// 49
	"TSPI_phoneGetData",						// 50
	"TSPI_phoneGetDisplay",						// 51
	"TSPI_phoneGetExtensionID",					// 52
	"TSPI_phoneGetGain",						// 53
	"TSPI_phoneGetHookSwitch",					// 54
	"TSPI_phoneGetIcon",						// 55
	"TSPI_phoneGetID",							// 56
	"TSPI_phoneGetLamp",						// 57
	"TSPI_phoneGetRing",						// 58
	"TSPI_phoneGetVolume",						// 59
	"TSPI_phoneNegotiateExtVersion",			// 60
	"TSPI_phoneSelectExtVersion",				// 61
	"TSPI_phoneSetButtonInfo",					// 62
	"TSPI_phoneSetData",						// 63
	"TSPI_phoneSetDisplay",						// 64
	"TSPI_phoneSetGain",						// 65
	"TSPI_phoneSetHookSwitch",					// 66
	"TSPI_phoneSetLamp",						// 67
	"TSPI_phoneSetRing",						// 68
	"TSPI_phoneSetVolume",						// 69
	"TSPI_providerCreateLineDevice",			// 70
	"TSPI_providerCreatePhoneDevice",			// 71
};
#pragma data_seg ()

///////////////////////////////////////////////////////////////////////////
// Pull in the inline functions here as full code if we are not using
// inline functions.
#ifdef _NOINLINES_
#include "splib.inl"
#endif

#ifdef _DEBUG
// From CRTDBG.H
extern "C" int __cdecl _CrtSetReportMode(int,int);
#endif

///////////////////////////////////////////////////////////////////////////
// IntervalTimerThread
//
// Runs the interval timer for the service provider.
//
UINT IntervalTimerThread(LPVOID pParam)
{
    CServiceProvider* pProvider = (CServiceProvider*) pParam;
    return pProvider->IntervalTimer();

}// IntervalTimerThread

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::CServiceProvider
//
// Base class constructor
//
CServiceProvider::CServiceProvider(LPCTSTR pszAppName, LPCTSTR pszProviderInfo, DWORD dwVersion) :
    CWinApp(pszAppName), m_evtShutdown(FALSE, TRUE)
{
	// Setup our timeouts and version/provider information
    m_hProvider		= NULL;
    m_lTimeout		= MAX_WAIT_TIMEOUT;
	m_pThreadI		= NULL;
    m_dwCurrentLocation = 0;
    m_dwTAPIVersionFound = 0;
    m_pszProviderInfo = pszProviderInfo;

    // Setup the default runtime objects.  These may be overriden by
    // calling the member "SetRuntimeObjects"   
    m_pRequestObj	= RUNTIME_CLASS(CTSPIRequest);
    m_pLineObj		= RUNTIME_CLASS(CTSPILineConnection);
    m_pPhoneObj		= RUNTIME_CLASS(CTSPIPhoneConnection);
    m_pDeviceObj	= RUNTIME_CLASS(CTSPIDevice);
    m_pCallObj		= RUNTIME_CLASS(CTSPICallAppearance);
    m_pAddrObj		= RUNTIME_CLASS(CTSPIAddressInfo);
    m_pConfCallObj	= RUNTIME_CLASS(CTSPIConferenceCall);

    // Version must be at least 2.0 according to TAPI documentation.
    if (dwVersion < TAPIVER_20)
        dwVersion = TAPIVER_20;
    m_dwTapiVerSupported = dwVersion;

}// CServiceProvider::CServiceProvider

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::SetRuntimeObjects
//
// This method replaces some or all of the runtime objects used by
// the service provider class.  This allows the derived class to 
// override the functionallity of ANY of the used base classes, but
// still keep most of the creation/initialization hidden away.
//
void CServiceProvider::SetRuntimeObjects(
CRuntimeClass* pDevObj,       // New CRuntimeClass for CTSPIDevice
CRuntimeClass* pReqObj,       // New CRuntimeClass for CTSPIRequest
CRuntimeClass* pLineObj,      // New CRuntimeClass for CTSPILineConnection
CRuntimeClass* pAddrObj,      // New CRuntimeClass for CTSPIAddressInfo
CRuntimeClass* pCallObj,      // New CRuntimeClass for CTSPICallAppearance
CRuntimeClass* pConfCallObj,  // New CRuntimeClass for CTSPIConferenceCall
CRuntimeClass* pPhoneObj)     // New CRuntimeClass for CTSPIPhoneConnection
{
    if (pReqObj)
        m_pRequestObj = pReqObj;

    if (pLineObj)
        m_pLineObj = pLineObj;

    if (pPhoneObj)
        m_pPhoneObj = pPhoneObj;

    if (pDevObj)
        m_pDeviceObj = pDevObj;

    if (pCallObj)
        m_pCallObj = pCallObj;

    if (pAddrObj)
        m_pAddrObj = pAddrObj;

    if (pConfCallObj)
        m_pConfCallObj = pConfCallObj;

}// CServiceProvider::SetRuntimeObjects

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::InitInstance
//
// This is called when the driver DLL is loaded by Windows through
// a OpenDriver call.
//
BOOL CServiceProvider::InitInstance()
{
#ifdef _DEBUG
	// Change the default C-Runtime error reporting to go to
	// the debug output facility.  Otherwise, it might attempt to
	// output to the GUI which results in a deadlock.
	for (int i = 0; i < 3; i++)
		_CrtSetReportMode (i, 0x2);
#endif

    // Determine what provider abilities are exported
    // from this module.
    DetermineProviderCapabilities();

    // Return 'Ok' response to the DLL loading.
    return TRUE;

}// CServiceProvider::InitInstance

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::ExitInstance
//
// This is called when the driver DLL is being unloaded by Windows
//
int CServiceProvider::ExitInstance()
{
    // If the service provider is still running, then providerShutdown
    // never got called.  This can happen in TAPI if the providerInit
    // function failed (generally by the derived provider) and we still
    // initialized our device.
    while (GetDeviceCount() > 0)
    {
        CTSPIDevice* pDevice = GetDeviceByIndex(0);
        ASSERT (pDevice != NULL);
        DTRACE(TRC_MIN, _T("Forcing shutdown of abandoned device 0x%lx\r\n"), pDevice->GetProviderID());
        providerShutdown (GetSystemVersion(), pDevice);
    }

    return FALSE;

}// CServiceProvider::ExitInstance

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::IntervalTimer
//
// Walk through all the devices we are running and give it the
// interval timer.
//
BOOL CServiceProvider::IntervalTimer()
{
	CSingleLock lck(&m_csProvider);
    for (;;)
    {
        // Wait for either our object to signal (meaning our provider
        // is being shutdown), or for our timeout value to elapse.
        LONG lResult = WaitForSingleObject(m_evtShutdown, LIBRARY_INTERVAL);
        if (lResult == WAIT_OBJECT_0 || lResult == WAIT_ABANDONED)
            break;

		// Walk through any calls which need timeout values.
		lck.Lock();
		for (POSITION pos = m_lstTimedCalls.GetHeadPosition(); pos != NULL;)
		{
			POSITION posLast = pos;
			CTSPICallAppearance* pCall = (CTSPICallAppearance*) m_lstTimedCalls.GetNext(pos);
			if (pCall->OnInternalTimer())
				m_lstTimedCalls.RemoveAt(posLast);
		}
		lck.Unlock();
    }

	// Reset our thread pointer
	m_pThreadI = NULL;
    return TRUE;
   
}// CServiceProvider::IntervalTimer

///////////////////////////////////////////////////////////////////////////
// CTSPIServiceProvider::ConvertDialableToCanonical
//
// This function is used to convert a dialable number into
// a standard TAPI canonical number.  So, if the number
// is 12143647464, then it will be formatted to "+1 (214) 3647464".
//                               
CString CServiceProvider::ConvertDialableToCanonical(LPCTSTR pszNumber, DWORD /*dwCountryCode*/)
{   
    CString strInput = GetDialableNumber (pszNumber);
    CString strOutput;

    // This function assumes North American standards, i.e. a country code (1)
    // will be followed by an area code (214), followed by an exchange (364) and
    // finally extension (7464).
    
    // Since there may be toll, billing, outside line access, or call-waiting
    // information at the front of the string, walk through it backwards to 
    // get our "stuff".
    if (strInput.GetLength() == 10)
    {   
        CString strAreaCode, strNumber;
    
        // We assume the exchange/extension is the final 7 digits.
        strNumber = strInput.Right(7);                            
        strInput = strInput.Left (strInput.GetLength()-7);
        
        // The area code should now be the final 3 digits if available.
        // If it is not there, then get our current area code and use it -
        // this assumes that the call is local.
        if (strInput.GetLength() >= 3)
            strAreaCode = strInput.Right(3);
            
        // Build the final string.
        if (!strAreaCode.IsEmpty())                                              
            strOutput = _T("+1 (") + strAreaCode + _T(") ");
        strOutput += strNumber;
    }
    
    // If we failed to break the string down, then return the straight
    // digit information.
    if (strOutput.IsEmpty())
        strOutput = strInput;
    return strOutput;    
    
}// CTSPIServiceProvider::ConvertDialableToCanonical

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetDialableNumber
//
// This method will go through a dialable string and return the 
// portion that is considered the "number" (ie: digits only) which 
// can be used to represent an address.
//
CString CServiceProvider::GetDialableNumber (LPCTSTR pszNumber, LPCTSTR pszAllow) const
{                 
    CString strReturn;
    if (pszNumber != NULL)
    {
        while (*pszNumber)
        {   
            if (_istdigit(*pszNumber) || (pszAllow && _tcschr(pszAllow, *pszNumber)))
                strReturn += *pszNumber;
            pszNumber++;
        }
    }                
    
    return strReturn;

}// CServiceProvider::GetDialableNumber

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::CheckDialableNumber
//
// Check the provided string to determine if our service provider
// supports the type of number given.
//
// This function should be overriden to provide additional checks on the
// dialable number (ie: billing info, etc.)
//
LONG CServiceProvider::CheckDialableNumber(           
CTSPILineConnection* pLine, // Line for this check
CTSPIAddressInfo* pAddr,    // Address for this check
LPCTSTR lpszDigits,         // Original input
CObArray* parrEntries,      // Entry array for broken number information
DWORD  /*dwCountry*/,       // Country code for this number
LPCTSTR lpszValidChars)     // Valid characters for the number
{
    LPCTSTR szCRLF = _T("\r\n");
	if (lpszValidChars == NULL)
		lpszValidChars = _T("0123456789ABCD*#!WPT@$+,");

    // Move the buffer to another string so we can modify it as we go.
    CString strNumber = lpszDigits;

    // If the prompt is in the string, this is an error and the
    // application has sent us a bad dial string.
    if (strNumber.Find(_T('?')) >= 0)
        return LINEERR_DIALPROMPT;
    
    // If more than one address is listed, and we don't support multiplexing
    // on this line device, then error it.
    if (strNumber.Find(szCRLF) >= 0)
    {
        if (pLine &&
            (pLine->GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_MULTIPLEADDR) == 0)
        {
            DTRACE(TRC_MIN, _T("Multiple addresses listed in dialable address, ignoring all but first.\r\n"));
            strNumber = strNumber.Left (strNumber.Find(szCRLF));
        }
    }                
    
    // Final result code
    LONG lResult = 0;
    
    // Now go through the string breaking each up into a seperate dial string (if
    // more than one is there).
    while (!strNumber.IsEmpty())
    {   
        CString strBuff;
        int iPos = strNumber.Find(szCRLF);
        if (iPos >= 0)          
        {
            strBuff = strNumber.Left(iPos);
            strNumber = strNumber.Mid(iPos+2);
        }
        else
        {
            strBuff = strNumber; 
            strNumber.Empty();
        }
        
        CString strSubAddress;
        CString strName;
        
        // Break the number up into its component parts.  Check to see if an
        // ISDN subaddress is present.
        iPos = strBuff.Find(_T('|'));
        if (iPos >= 0)
        {   
            strSubAddress = strBuff.Mid(iPos+1);
            int iEndPos = strSubAddress.FindOneOf(_T("+|^"));
            if (iEndPos >= 0)
                strSubAddress = strSubAddress.Left (iEndPos);
        }
        
        // Now grab the NAME if present in the string.
        iPos = strBuff.Find(_T('^'));
        if (iPos >= 0)
            strName = strBuff.Mid(iPos+1);

        // Strip off all the ISDN/Name info
        iPos = strBuff.FindOneOf(_T("|^"));
        if (iPos >= 0)
            strBuff = strBuff.Left(iPos);
        
        // Verify the size of the digit section
        if (strBuff.GetLength() > TAPIMAXDESTADDRESSSIZE)
        {
            lResult = LINEERR_INVALPOINTER;
            break;
        }
        
        // Validate the things in the digit buffer.
        strBuff.MakeUpper();
        while (strBuff.Right(1) == _T(' '))
            strBuff = strBuff.Left(strBuff.GetLength()-1);
        
        // Check to see if partial dialing is allowed.
        BOOL fPartialDial = FALSE;
        if (strBuff.Right(1) == _T(';'))
        {
            fPartialDial = TRUE;
            strBuff = strBuff.Left(strBuff.GetLength()-1);
        }
        
        // Remove anything which we don't understand.  Typically, the app
        // will do this for us, but just in case, remove any dashes, parens,
        // etc. from a phonebook entry.
        CString strValidChars(lpszValidChars);
        CString strNewBuff;
        for (iPos = 0; iPos < strBuff.GetLength(); iPos++)
        {
            if (strValidChars.Find(strBuff[iPos]) >= 0)
                strNewBuff += strBuff[iPos];
        }
        strBuff = strNewBuff;
        
        // Check the address capabilities against specific entries in our
        // dial string.
        if (fPartialDial && pAddr &&
            (pAddr->GetAddressCaps()->dwAddrCapFlags & LINEADDRCAPFLAGS_PARTIALDIAL) == 0)
        {
            lResult = LINEERR_INVALPOINTER;
            break;
        }
        
        if (strBuff.Find(_T('$')) >= 0 &&
            pLine &&
            (pLine->GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_DIALBILLING) == 0)
        {
            lResult = LINEERR_DIALBILLING;
            break;
        }
       
        if (strBuff.Find(_T('@')) >= 0 &&
            pLine &&
            (pLine->GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_DIALQUIET) == 0)
        {
            lResult = LINEERR_DIALQUIET;
            break;
        }
        
        if (strBuff.Find(_T('W')) >= 0 &&
            pLine &&
            (pLine->GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_DIALDIALTONE) == 0)
        {
            lResult = LINEERR_DIALDIALTONE;
            break;
        }
     
        // Now store the information into a DIALINFO structure.
        DIALINFO* pDialInfo = new DIALINFO;
        pDialInfo->fIsPartialAddress = fPartialDial;
        pDialInfo->strNumber = strBuff;
        pDialInfo->strName = strName;
        pDialInfo->strSubAddress = strSubAddress;
        parrEntries->Add(pDialInfo);
    }        

    // If it failed somewhere, then spin through and delete ALL the
    // dialinfo requests already broken out.
    if (lResult != 0)
    {
        for (int i = 0; i < parrEntries->GetSize(); i++)
        {
            DIALINFO* pDialInfo = (DIALINFO*) parrEntries->GetAt(i);
            delete pDialInfo;
        }   
        parrEntries->RemoveAll();
    }
    return lResult;

}// CServiceProvider::CheckDialableNumber

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetDevice
//
// Returns the device information structure for the specified
// permanent provider id.
//
CTSPIDevice* CServiceProvider::GetDevice(DWORD dwPPid) const
{
    int iCount = GetDeviceCount();
    for (int i = 0; i < iCount; i++)
    {
        CTSPIDevice* pDevice = GetDeviceByIndex(i);
        if (pDevice->GetProviderID() == dwPPid)
            return pDevice;
    }
    return NULL;
   
}// CServiceProvider::GetDevice

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetDeviceByIndex
//
// Return a device object based on an index into our device
// map.
//
CTSPIDevice* CServiceProvider::GetDeviceByIndex (int iPos) const
{
    CTSPIDevice* pDevice = NULL;
    if (GetDeviceCount() > (DWORD) iPos)
        pDevice = (CTSPIDevice*) m_arrDevices[iPos];
    return pDevice;

}// CServiceProvider::GetDeviceByIndex

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::SearchForConnInfo
//
// This function searches all the connection information blocks for one
// which matches the criteria passed.
//
// Valid types are:
//
//    wReqType                                 dwId
//    -----------------------------------------------------
//       0:  Physical TAPI line device index   (DWORD)
//       1:  Physical TAPI phone device index  (DWORD)
//
CTSPIConnection* CServiceProvider::SearchForConnInfo(DWORD dwId, WORD wReqType)
{
    CTSPIConnection* pConn = NULL;
    ASSERT(wReqType == 0 || wReqType == 1);

    int iCount = GetDeviceCount();
    for (int i = 0; i < iCount; i++)
    {
        CTSPIDevice* pDevice = GetDeviceByIndex(i);
        if (pDevice)
        {
            if (wReqType == 0)  // Line?
                pConn = pDevice->FindLineConnectionByDeviceID (dwId);
            else if (wReqType == 1) // Phone
                pConn = pDevice->FindPhoneConnectionByDeviceID (dwId);
            if (pConn != NULL)
                break;
        }
    }
    return pConn;
   
}// CServiceProvider::SearchForConnInfo

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ProcessCallParameters
//
// Check the parameters in a LINECALLPARAMS structure and verify that
// the fields are capable of being handled on the line/address/call.
//
// It is guarenteed that the line will be valid, the address and call can
// be NULL, but if a call is present, an address will always be present.
//  
// This function changed significantly in v1.21.  In previous releases
// it was required that this function was supplied by the derived class.
// It now passes control down to the specific objects in question which
// can perform most of the validation based on how the derived class sets up
// the LINECAPS structures.  It still may be overriden, but it is no
// longer necessary.
// 
LONG CServiceProvider::ProcessCallParameters(CTSPILineConnection* pLine, LPLINECALLPARAMS lpCallParams)
{                                                     
    LONG lResult;
    
    // Fill in the default values if not supplied.
    if (lpCallParams == NULL)
        return LINEERR_INVALCALLPARAMS;
    
    // Set the defaults up if not supplied.  TAPI should be filling these
    // out, but the documentation is a bit vague as to who is really responsible
    // so we will just make sure that our values are ALWAYS valid.    
    if (lpCallParams->dwBearerMode == 0)
        lpCallParams->dwBearerMode = LINEBEARERMODE_VOICE;
    if (lpCallParams->dwMaxRate == 0)
        lpCallParams->dwMaxRate = pLine->GetLineDevCaps()->dwMaxRate;
    if (lpCallParams->dwMediaMode == 0)
        lpCallParams->dwMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
    if (lpCallParams->dwAddressMode == 0)
        lpCallParams->dwAddressMode = LINEADDRESSMODE_ADDRESSID;
    
    // Make sure the DIAL parameters are all filled in.    
    if (lpCallParams->DialParams.dwDialPause == 0)
        lpCallParams->DialParams.dwDialPause = pLine->GetLineDevCaps()->DefaultDialParams.dwDialPause;
    if (lpCallParams->DialParams.dwDialSpeed == 0)
        lpCallParams->DialParams.dwDialSpeed = pLine->GetLineDevCaps()->DefaultDialParams.dwDialSpeed;
    if (lpCallParams->DialParams.dwDigitDuration == 0)
        lpCallParams->DialParams.dwDigitDuration = pLine->GetLineDevCaps()->DefaultDialParams.dwDigitDuration;
    if (lpCallParams->DialParams.dwWaitForDialtone == 0)
        lpCallParams->DialParams.dwWaitForDialtone = pLine->GetLineDevCaps()->DefaultDialParams.dwWaitForDialtone;
    
    // Pass it to the line object - it will determine if the address specified
    // or any address can support the call.
    lResult = pLine->CanSupportCall (lpCallParams);
        
    return lResult;  

}// CServiceProvider::ProcessCallParameters

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::CanHandleRequest
//
// This function is used to dynamically determine what the capabilities
// of our service provider really is.
//
BOOL CServiceProvider::CanHandleRequest (CTSPIConnection* /*pConn*/,
                                         CTSPIAddressInfo* /*pAddr*/, 
                                         CTSPICallAppearance* pCall,
                                         WORD wRequest, DWORD /*dwData*/)
{   
    // If the call appearance is in "pass-through" mode, then don't allow
    // any of the functions not able to work with it.
    if (pCall && pCall->GetCallInfo()->dwBearerMode == LINEBEARERMODE_PASSTHROUGH)
    {
        if (wRequest != REQUEST_SETCALLPARAMS)
            return FALSE;            
    }

	if (wRequest >= 0 && wRequest <= TSPI_ENDOFLIST)
		return m_arrProviderCaps [wRequest];
	return FALSE;

}// CServiceProvider::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::DetermineProviderCapabilities
//
// Determine what the service provider can and cannot do based on 
// what is exported from the TSP.  These are later used to provide
// the basic "CanHandleRequest" function.
//
VOID CServiceProvider::DetermineProviderCapabilities()
{                              
    HINSTANCE hInstance = AfxGetInstanceHandle();
    ASSERT (hInstance != NULL);
    for (int i = 0; i <= TSPI_ENDOFLIST; i++)
    {
        if (GetProcAddress(hInstance, gszEntryPoints[i]))
		{
			DTRACE(TRC_MIN, _T("Supports %s\r\n"), gszEntryPoints[i]);
            m_arrProviderCaps.SetAt(i, TRUE);
		}
    }

}// CServiceProvider::DetermineProviderCapabilities

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::MatchTones
//
// This function matches a series of tone frequencies against each other
// and determines if they are equal.  It is provided to allow for "fuzzy"
// searches rather than exact frequency matches.
//
BOOL CServiceProvider::MatchTones (
DWORD dwSFreq1,             // Search frequency 1 (what we are looking for)
DWORD dwSFreq2,             // Search frequency 2 (what we are looking for)
DWORD dwSFreq3,             // Search frequency 3 (what we are looking for)
DWORD dwTFreq1,             // Target frequency 1 (what we found)
DWORD dwTFreq2,             // Target frequency 2 (what we found)
DWORD dwTFreq3)             // Target frequency 3 (what we found)
{                               
    // The default is to to direct matching (exact) against any of the three frequency 
    // components.  If you require a filter or some "fuzzy" testing of the tones, then
    // override this function.
    if ((dwSFreq1 == dwTFreq1 || dwSFreq1 == dwTFreq2 || dwSFreq1 == dwTFreq3) &&
        (dwSFreq2 == dwTFreq1 || dwSFreq2 == dwTFreq2 || dwSFreq2 == dwTFreq3) &&
        (dwSFreq3 == dwTFreq1 || dwSFreq3 == dwTFreq2 || dwSFreq3 == dwTFreq3))
        return TRUE;
    return FALSE;        
    
}// CServiceProvider::MatchTones

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::CheckCallFeatures
//
// Called when any call features change on a call.  This allows the
// derived provider to adjust the list as necessary for the H/W involved.
//
// When this is called, all the features have been adjusted by the library,
// so any changes will be recorded properly.
//
DWORD CServiceProvider::CheckCallFeatures(CTSPICallAppearance* /*pCall*/, DWORD dwCallFeatures)
{   
    // Return the same list passed in.                                   
    return dwCallFeatures;
    
}// CServiceProvider::CheckCallFeatures

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerRemove
//
// This method is invoked when the TSP is being removed from the
// system.  It should remove all its files and .INI settings.
//
// This method should be overriden by the derived class.
//
LONG CServiceProvider::providerRemove(
DWORD dwPermanentProviderID,            // Provider ID (unique across providers)
CWnd* /*pwndOwner*/,					// Owner window to supply and UI for.
TUISPIDLLCALLBACK /*lpfnDLLCallback*/)	// Callback for provider.
{                                   
	// Open the TELEPHONY key
    HKEY hKeyTelephony;
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, KEY_ALL_ACCESS, &hKeyTelephony)
		!= ERROR_SUCCESS)
		return LINEERR_OPERATIONFAILED;

	// Delete our pointer to our section from TAPI
	CString strKey;
	strKey.Format(_T("Device%ld"), dwPermanentProviderID);
	RegDeleteValue (hKeyTelephony, strKey);

    // Delete our section from the profile.
    IntRegDeleteKey (hKeyTelephony, m_pszProviderInfo);
    RegCloseKey (hKeyTelephony);

    return FALSE;

}// CServiceProvider::providerRemove

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerInit
//
// This method is called when the service provider is first initialized.
// It supplies the base line/phone ids for us and our permanent provider
// id which has been assigned by TAPI.  It will be called right after
// the INITIALIZE_NEGOTIATION.
//
LONG CServiceProvider::providerInit(
DWORD dwTSPVersion,              // Version required for TAPI.DLL
DWORD dwProviderId,              // Our permanent provider Id.
DWORD dwLineBase,                // Our device line base id.
DWORD dwPhoneBase,               // Our device phone base id.
DWORD dwNumLines,                // Number of lines TAPI expects us to run
DWORD dwNumPhones,               // Number of phones TAPI expects us to run
ASYNC_COMPLETION lpfnCallback,   // Asynchronous completion callback.
LPDWORD lpdwTSPIOptions)         // TSPI options
{
    // Make sure we don't already have this provider in our
    // device array.
    if (GetDevice(dwProviderId) != NULL)
        return TAPIERR_DEVICEINUSE;

    // The library is fully re-entrant, the derived service provider
    // should set the re-entrancy flag if it is not re-enterant.
    *lpdwTSPIOptions = 0L;

    // Allocate a device information object for this id and add it to
    // our device map.  This device object maintains the connection lists
    // and line information.
    CTSPIDevice* pDevice = (CTSPIDevice*) GetTSPIDeviceObj()->CreateObject();
    ASSERT(pDevice->IsKindOf(RUNTIME_CLASS(CTSPIDevice)));
    pDevice->Init(dwProviderId, dwLineBase, dwPhoneBase, dwNumLines, dwNumPhones, m_hProvider, lpfnCallback);

    // Add the device to the list.
    if (m_arrDevices.Add(pDevice) == 0)
    {
        m_dwTAPIVersionFound = dwTSPVersion;
        m_evtShutdown.ResetEvent();
        m_pThreadI = AfxBeginThread (IntervalTimerThread, (LPVOID)this);
    }

    return FALSE;

}// CServiceProvider::providerInit   

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerShutdown
//
// This method is called to shutdown our service provider.  It will
// be called directly before the unload of our driver.  This should
// only be called when ALL line/phone devices supported by this service provider
// are to be shutdown.
//
LONG CServiceProvider::providerShutdown(DWORD /*dwTSPVersion*/, 
                        CTSPIDevice* pShutdownDevice)
{   
    // Locate the device in our array and remove it.
    int iCount = GetDeviceCount();
    for (int i = 0; i < iCount; i++)
    {
        CTSPIDevice* pDevice = GetDeviceByIndex(i);
        if (pDevice == pShutdownDevice)
        {
            m_arrDevices.RemoveAt(i);
            break;
        }
    }

#ifdef _DEBUG
    if (iCount == i)
        DTRACE(TRC_MIN, _T("ERR: Could not locate Device 0x%lx\r\n"), pShutdownDevice->GetProviderID());
#endif

    // If that was the last provider, shutdown the interval timer.
    if (GetDeviceCount() == 0)
	{
        m_evtShutdown.SetEvent();

		// Wait for the interval timer thread to exit.
		if (m_pThreadI)
		{
			HANDLE hThread = m_pThreadI->m_hThread;
			if (WaitForSingleObject (hThread, 1000) == WAIT_TIMEOUT)
			{
				TerminateThread (hThread, 0);
				delete m_pThreadI;
			}
		}
	}

    // Delete our device.
    delete pShutdownDevice;    
    return FALSE;

}// CServiceProvider::providerShutdown

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerCreateLineDevice
//
// This function is called by TAPI in response to a LINE_CREATE message
// being sent from us.  This allows TAPI to assign the line device id.
//
// This function is specific to TAPI version 1.4
//
LONG CServiceProvider::providerCreateLineDevice(
DWORD dwTempId,                     // Specifies the line device ID used in our LINE_CREATE
DWORD dwDeviceId)                   // Specifies TAPIs new line device id.
{                    
    // If we cannot support this function, then return an error.
    if (GetSupportedVersion() < TAPIVER_14)
        return LINEERR_OPERATIONUNAVAIL;

    // Locate the line device 
    CTSPILineConnection* pLine = GetConnInfoFromLineDeviceID(dwTempId);
    if (pLine)
    {
        // Assign the NEW device id.
        pLine->SetDeviceID(dwDeviceId);
        return FALSE;
    }

    // Couldn't find the device id in our table?
    return LINEERR_BADDEVICEID;

}// CServiceProvider::providerCreateLineDevice

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerCreatePhoneDevice
//
// This function is called by TAPI in response to a PHONE_CREATE message
// being sent from us.  This allows TAPI to assign the phone device id.
//
// This function is specific to TAPI version 1.4
//
LONG CServiceProvider::providerCreatePhoneDevice(
DWORD dwTempId,                     // Specifies the phone device ID used in our PHONE_CREATE
DWORD dwDeviceId)                   // Specifies TAPIs new phone device id.
{
    // If we cannot support this function, then return an error.
    if (GetSupportedVersion() < TAPIVER_14)
        return LINEERR_OPERATIONUNAVAIL;

    // Locate the phone device 
    CTSPIPhoneConnection* pPhone = GetConnInfoFromPhoneDeviceID(dwTempId);
    if (pPhone)
    {
        // Assign the NEW device id.
        pPhone->SetDeviceID(dwDeviceId);
        return FALSE;
    }

    // Couldn't find the device id in our table?
    return PHONEERR_BADDEVICEID;

}// CServiceProvider::providerCreatePhoneDevice

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerFreeDialogInstance
//
// Informs the provider about a dialog instance terminating.
//
LONG CServiceProvider::providerFreeDialogInstance (HDRVDIALOGINSTANCE hdDlgInstance)
{
    // Cast the handle back to our LINEUIDIALOG structure.
    LINEUIDIALOG* pLineDialog = (LINEUIDIALOG*) hdDlgInstance;
    ASSERT (pLineDialog != NULL);

    // Ask the line connection to free the specific dialog instance.
    CTSPILineConnection* pLine = pLineDialog->pLineOwner;
    ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    return pLine->FreeDialogInstance(pLineDialog->htDlgInstance);

}// CServiceProvider::providerFreeDialogInstance

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerGenericDialogData 
//
// This is called when the UI DLL sends data back to our 
// provider.
//
LONG CServiceProvider::providerGenericDialogData (
CTSPIDevice* pDevice, 
CTSPILineConnection* pLine, 
CTSPIPhoneConnection* pPhone, 
HDRVDIALOGINSTANCE hdDlgInstance, 
LPVOID lpBuff, 
DWORD dwSize)
{
    // Notify the appropriate object
    if (pPhone)
        return pPhone->GenericDialogData (lpBuff, dwSize);
    else if (pLine)
        return pLine->GenericDialogData (NULL, lpBuff, dwSize);
    else if (pDevice)
        return pDevice->GenericDialogData (lpBuff, dwSize);

    // Only one left is the dialog instance.
    LINEUIDIALOG* pLineDialog = (LINEUIDIALOG*) hdDlgInstance;
    ASSERT (pLineDialog != NULL);

    pLine = pLineDialog->pLineOwner;
    return pLine->GenericDialogData (pLineDialog, lpBuff, dwSize);
    
}// CServiceProvider::providerGenericDialogData 

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerConfig
//
// This method is invoked when the user selects our ServiceProvider
// icon in the control panel.  It should invoke the configuration dialog
// which must be provided by the derived class.
//
// This method should be overriden by the derived class to supply
// a dialog.
//
LONG CServiceProvider::providerConfig(
DWORD /*dwPermanentProviderID*/,    // Provider ID (unique across providers)
CWnd* /*pwndOwner*/,                // Owner window to supply and UI for.
TUISPIDLLCALLBACK /*lpfnDLLCallback*/) // Callback for provider.
{
    return FALSE;
    
}// CServiceProvider::providerConfig

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerInstall
//
// This method is invoked when the TSP is to be installed via the
// TAPI install code.  It should insure that all the correct files
// are there, and write out the initial .INI settings.
//
// This method should be overriden by the derived class.
//
LONG CServiceProvider::providerInstall(
DWORD dwPermanentProviderID,			// Provider ID (unique across providers)
CWnd* /*pwndOwner*/,					// Owner window to supply and UI for.
TUISPIDLLCALLBACK /*lpfnDLLCallback*/)	// Callback for provider.
{
	// Open the TELEPHONY key
    HKEY hKeyTelephony;
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, KEY_ALL_ACCESS, &hKeyTelephony)
		!= ERROR_SUCCESS)
		return LINEERR_OPERATIONFAILED;

	// Add a section to the telephony section which points to our
	// full provider section.  This is used by the DbgSetLevel program
	// supplied by JulMar to locate the provider section properly.
	CString strKey;
	strKey.Format(_T("Device%ld"), dwPermanentProviderID);
	RegSetValueEx (hKeyTelephony, strKey, 0, REG_SZ, 
			(LPBYTE)m_pszProviderInfo, lstrlen(m_pszProviderInfo)+1);

	// Close our key
	RegCloseKey(hKeyTelephony);

	// Add an entry marking this provider as "TSP++" compatible for other
	// programs which search for it.
	WriteProfileDWord(0, _T("UsesTSPLib"), 1);

    return FALSE;
    
}// CServiceProvider::providerInstall

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerEnumDevices
//
// TAPI calls this function before providerInit to determine the number
// of line and phone devices supported by the service provider.  This
// allows the service provider to read a configuration OTHER than
// the TELEPHON.INI to gather line/phone counts.  This is especially
// important in devices which support Plug&Play.
//
// THIS IS A MANDATORY FUNCTION IN TAPI 2.0
//
LONG CServiceProvider::providerEnumDevices(
DWORD /*dwProviderId*/,             // Our Provider ID
LPDWORD lpNumLines,                 // Number of lines (return)
LPDWORD lpNumPhones,                // Number of phones (return)    
HPROVIDER hProvider,                // TAPIs HANDLE to our service provider
LINEEVENT lpfnLineCreateProc,       // LINEEVENT for dynamic line creation
PHONEEVENT lpfnPhoneCreateProc)     // PHONEEVENT for dynamic phone creation
{   
    // Store off the line/phone event procedures.
    m_lpfnLineCreateProc = lpfnLineCreateProc;
    m_lpfnPhoneCreateProc = lpfnPhoneCreateProc;
    m_hProvider = hProvider;

    // Override to return a correct count for lines/phones.
    *lpNumLines = 0L;
    *lpNumPhones = 0L;

    return FALSE;

}// CServiceProvider::providerEnumDevices

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerUIIdentify 
//
// Return the name passed in the constructor for this TSP.
//
LONG CServiceProvider::providerUIIdentify (LPWSTR lpszUIDLLName)
{
#ifdef _UNICODE
    _tcscpy (lpszUIDLLName, AfxGetAppName());
#else
    MultiByteToWideChar (CP_ACP, 0, AfxGetAppName(), -1, lpszUIDLLName, _MAX_PATH);
#endif
    return FALSE;

}// CServiceProvider::providerUIIdentify 

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerGenericDialog
//
// This method is called for a UI dialog DLL
//
LONG CServiceProvider::providerGenericDialog (
HTAPIDIALOGINSTANCE /*htDlgInst*/,          // Dialog instance
LPVOID /*lpParams*/,                        // Parameter block
DWORD /*dwSize*/,                           // Size of above
HANDLE /*hEvent*/,                          // Event when INIT complete
TUISPIDLLCALLBACK /*lpfnDLLCallback*/)      // Callback to TSP
{
    return LINEERR_OPERATIONUNAVAIL;

}// CServiceProvider::providerGenericDialog

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerGenericDialogData
//
// This method is called for the UI DLL when the provider 
// sends information.
//
LONG CServiceProvider::providerGenericDialogData (
HTAPIDIALOGINSTANCE /*htDlgInst*/,          // Dialog instance
LPVOID /*lpParams*/,                        // Parameter block
DWORD /*dwSize*/)                           // Size of above
{
    return LINEERR_OPERATIONUNAVAIL;

}// CServiceProvider::providerGenericDialogData

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ReadProfileString
//
// Read a string from our profile section in the registry.  This
// function is limited to 512 characters.
//
CString CServiceProvider::ReadProfileString (DWORD dwDeviceID, LPCTSTR pszEntry, LPCTSTR lpszDefault/*=""*/)
{
	TCHAR szBuff[512];

    // Open the master registry key.
    HKEY hTelephonyKey;
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, KEY_ALL_ACCESS, &hTelephonyKey) != ERROR_SUCCESS)
        return lpszDefault;

    // Open our Provider section
    HKEY hProviderKey;
    if (RegOpenKeyEx (hTelephonyKey, m_pszProviderInfo, 0, KEY_ALL_ACCESS, &hProviderKey) != ERROR_SUCCESS)
	{
		RegCloseKey (hTelephonyKey);
        return lpszDefault;
	}

	// Open our device section
    DWORD dwDataSize = sizeof(szBuff), dwDataType;
	if (dwDeviceID > 0)
	{
		_stprintf (szBuff, gszDevice, dwDeviceID);
		HKEY hDeviceKey;
		if (RegOpenKeyEx (hProviderKey, szBuff, 0, KEY_ALL_ACCESS, &hDeviceKey) != ERROR_SUCCESS)
		{
			RegCloseKey (hProviderKey);
			RegCloseKey (hTelephonyKey);
			return lpszDefault;
		}

		// Query the value requested.
		if (RegQueryValueEx (hDeviceKey, pszEntry, 0, &dwDataType, (LPBYTE)szBuff, &dwDataSize) == ERROR_SUCCESS)
		{
			lpszDefault = szBuff;
			szBuff[dwDataSize] = _T('\0');
		}
		RegCloseKey (hDeviceKey);
		RegCloseKey (hProviderKey);
		RegCloseKey (hTelephonyKey);

	}
	else
	{
		// Query the value requested.
		if (RegQueryValueEx (hProviderKey, pszEntry, 0, &dwDataType, (LPBYTE)szBuff, &dwDataSize) == ERROR_SUCCESS)
			lpszDefault = szBuff;
		szBuff[dwDataSize] = _T('\0');
		RegCloseKey (hProviderKey);
		RegCloseKey (hTelephonyKey);
	}

    return lpszDefault;

}// CServiceProvider::ReadProfileString

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ReadProfileDWord
//
// Read a DWORD from our profile section in the registry.
//
DWORD CServiceProvider::ReadProfileDWord (DWORD dwDeviceID, LPCTSTR pszEntry, DWORD dwDefault/*=0*/)
{
    // Open the master registry key.
    HKEY hTelephonyKey;
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, KEY_ALL_ACCESS, &hTelephonyKey) != ERROR_SUCCESS)
		return dwDefault;

    // Open our Provider section
    HKEY hProviderKey;
    if (RegOpenKeyEx (hTelephonyKey, m_pszProviderInfo, 0, KEY_ALL_ACCESS, &hProviderKey) != ERROR_SUCCESS)
	{
		RegCloseKey (hTelephonyKey);
		return dwDefault;
	}

	// Open our device section
	DWORD dwDataSize = sizeof(DWORD), dwDataType, dwData;
	if (dwDeviceID > 0)
	{
		TCHAR szBuff[20];
		_stprintf (szBuff, gszDevice, dwDeviceID);

		HKEY hDeviceKey;
		if (RegOpenKeyEx (hProviderKey, szBuff, 0, KEY_ALL_ACCESS, &hDeviceKey) != ERROR_SUCCESS)
		{
			RegCloseKey (hProviderKey);
			RegCloseKey (hTelephonyKey);
			return dwDefault;
		}

		// Query the value requested.
		if (RegQueryValueEx (hDeviceKey, pszEntry, 0, &dwDataType, (LPBYTE)&dwData, &dwDataSize) != ERROR_SUCCESS)
			dwData = dwDefault;

		RegCloseKey (hDeviceKey);
		RegCloseKey (hProviderKey);
		RegCloseKey (hTelephonyKey);
	}
	else
	{
		// Query the value requested.
		if (RegQueryValueEx (hProviderKey, pszEntry, 0, &dwDataType, (LPBYTE)&dwData, &dwDataSize) != ERROR_SUCCESS)
			dwData = dwDefault;
		RegCloseKey (hProviderKey);
		RegCloseKey (hTelephonyKey);
	}

    return dwData;

}// CServiceProvider::ReadProfileDWord

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::WriteProfileString
//
// Write a string into our registry profile.
//
BOOL CServiceProvider::WriteProfileString (DWORD dwDeviceID, LPCTSTR pszEntry, LPCTSTR pszValue)
{
    DWORD dwDisposition;

    // Attempt to create the telephony registry section - it should really exist if our
    // driver has been loaded by TAPI.
    HKEY hKeyTelephony;
    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, _T(""), REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS, NULL, &hKeyTelephony, &dwDisposition) != ERROR_SUCCESS)
        return FALSE;

    // Now create our provider section if necessary.
    HKEY hKeyProvider;
    if (RegCreateKeyEx (hKeyTelephony, m_pszProviderInfo, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                NULL, &hKeyProvider, &dwDisposition) != ERROR_SUCCESS)
	{
		RegCloseKey (hKeyTelephony);
        return FALSE;
	}

	// Create our device section
	if (dwDeviceID > 0)
	{
		TCHAR szBuff[20];
		_stprintf (szBuff, gszDevice, dwDeviceID);
		HKEY hDeviceKey;
		if (RegCreateKeyEx (hKeyProvider, szBuff, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
							NULL, &hDeviceKey, &dwDisposition) != ERROR_SUCCESS)
		{
			RegCloseKey (hKeyProvider);
			RegCloseKey (hKeyTelephony);
			return FALSE;
		}

		// Store the key.
		RegSetValueEx (hDeviceKey, pszEntry, 0, REG_SZ, (LPBYTE)pszValue, (lstrlen(pszValue)*sizeof(TCHAR))+1);
		RegCloseKey (hDeviceKey);
		RegCloseKey (hKeyProvider);
		RegCloseKey (hKeyTelephony);

	}
	else
	{
		RegSetValueEx (hKeyProvider, pszEntry, 0, REG_SZ, (LPBYTE)pszValue, (lstrlen(pszValue)*sizeof(TCHAR))+1);
		RegCloseKey (hKeyProvider);
		RegCloseKey (hKeyTelephony);
	}

	return TRUE;

}// CServiceProvider::WriteProfileString

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::WriteProfileDWord
//
// Write a DWORD into our registry profile.
//
BOOL CServiceProvider::WriteProfileDWord (DWORD dwDeviceID, LPCTSTR pszEntry, DWORD dwValue)
{
    DWORD dwDisposition;

    // Attempt to create the telephony registry section - it should really exist if our
    // driver has been loaded by TAPI.
    HKEY hKeyTelephony;
    if (RegCreateKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, _T(""), REG_OPTION_NON_VOLATILE,
                                    KEY_ALL_ACCESS, NULL, &hKeyTelephony, &dwDisposition) != ERROR_SUCCESS)
        return FALSE;

    // Now create our provider section if necessary.
    HKEY hKeyProvider;
    if (RegCreateKeyEx (hKeyTelephony, m_pszProviderInfo, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                        NULL, &hKeyProvider, &dwDisposition) != ERROR_SUCCESS)
	{
		RegCloseKey (hKeyTelephony);
        return FALSE;
	}

	// Create our device section
	if (dwDeviceID > 0)
	{
		TCHAR szBuff[20];
		_stprintf (szBuff, gszDevice, dwDeviceID);
		HKEY hDeviceKey;
		if (RegCreateKeyEx (hKeyProvider, szBuff, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
							NULL, &hDeviceKey, &dwDisposition) != ERROR_SUCCESS)
		{
			RegCloseKey (hKeyProvider);
			RegCloseKey (hKeyTelephony);
			return FALSE;
		}

		// Store the key.
		RegSetValueEx (hDeviceKey, pszEntry, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
		RegCloseKey (hDeviceKey);
		RegCloseKey (hKeyProvider);
		RegCloseKey (hKeyTelephony);
	}
	else
	{
		// Store the key.
		RegSetValueEx (hKeyProvider, pszEntry, 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
		RegCloseKey (hKeyProvider);
		RegCloseKey (hKeyTelephony);
	}

	return TRUE;

}// CServiceProvider::WriteProfileDWord

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::DeleteProfile
//
// Deletes the registry key directory for a section.
//
BOOL CServiceProvider::DeleteProfile (DWORD dwDeviceID)
{
    // Open the master registry key.
    HKEY hTelephonyKey;
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, KEY_ALL_ACCESS, &hTelephonyKey) != ERROR_SUCCESS)
		return FALSE;

    // Open our Provider section
    HKEY hProviderKey;
    if (RegOpenKeyEx (hTelephonyKey, m_pszProviderInfo, 0, KEY_ALL_ACCESS, &hProviderKey) != ERROR_SUCCESS)
	{
		RegCloseKey (hTelephonyKey);
		return FALSE;
	}

	// Delete the device section.
    TCHAR szBuff[20];
    _stprintf (szBuff, gszDevice, dwDeviceID);
	BOOL fRC = IntRegDeleteKey (hProviderKey, szBuff);

    RegCloseKey (hProviderKey);
    RegCloseKey (hTelephonyKey);

	return fRC;

}// CServiceProvider::DeleteProfile

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::RenameProfile
//
// Moves a profile from one area to another.
//
BOOL CServiceProvider::RenameProfile (DWORD dwOldDevice, DWORD dwNewDevice)
{
	// Ignore requests for the same profile.
	if (dwOldDevice == dwNewDevice)
		return TRUE;

	// First make sure the NEW device doesn't exist already
	if (dwNewDevice > 0 && !DeleteProfile(dwNewDevice))
		return FALSE;

    // Open the master registry key.
    HKEY hTelephonyKey;
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, gszTelephonyKey, 0, KEY_ALL_ACCESS, &hTelephonyKey) != ERROR_SUCCESS)
		return FALSE;

    // Open our Provider section
    HKEY hProviderKey;
    if (RegOpenKeyEx (hTelephonyKey, m_pszProviderInfo, 0, KEY_ALL_ACCESS, &hProviderKey) != ERROR_SUCCESS)
	{
		RegCloseKey (hTelephonyKey);
		return FALSE;
	}

	// Open our device section.
	HKEY hOldDeviceKey;
	TCHAR szBuff[20];
	if (dwOldDevice > 0)
	{
		_stprintf (szBuff, gszDevice, dwOldDevice);
		if (RegOpenKeyEx (hProviderKey, szBuff, 0, KEY_ALL_ACCESS, &hOldDeviceKey) != ERROR_SUCCESS)
		{
			RegCloseKey (hProviderKey);
			RegCloseKey (hTelephonyKey);
			return FALSE;
		}
	}
	else
		hOldDeviceKey = hProviderKey;

	// Create the new section
	HKEY hNewDeviceKey;
	if (dwNewDevice > 0)
	{
		DWORD dwDisposition;
		_stprintf (szBuff, gszDevice, dwNewDevice);
		if (RegCreateKeyEx (hProviderKey, szBuff, 0, _T(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
							NULL, &hNewDeviceKey, &dwDisposition) != ERROR_SUCCESS)
		{
			if (dwOldDevice > 0)
				RegCloseKey(hOldDeviceKey);
			RegCloseKey (hProviderKey);
			RegCloseKey (hTelephonyKey);
			return FALSE;
		}
	}
	else
		hNewDeviceKey = hProviderKey;

	// Get the max size of the name and values.
	DWORD dwNameSize, dwValueSize, dwType;
	if (RegQueryInfoKey (hOldDeviceKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		                 &dwNameSize, &dwValueSize, NULL, NULL) != ERROR_SUCCESS)
	{
		dwNameSize = 1024;
		dwValueSize = 4096;
	}
	else
	{
		dwNameSize++;
		dwValueSize++;
	}

	// Alloc blocks to hold the information.
	LPTSTR pszName = (LPTSTR) AllocMem (dwNameSize * sizeof(TCHAR));
	LPBYTE pszValue = (LPBYTE) AllocMem (dwValueSize * sizeof(TCHAR));

	// Enumerate through all the values within the old key, and move them to the new section.
	DWORD dwIndex = 0;
	while (TRUE)
	{
		// Enumerate through the items.
		DWORD dwNewNameSize = dwNameSize, dwNewValueSize = dwValueSize;
		if (RegEnumValue (hOldDeviceKey, dwIndex++, pszName, &dwNewNameSize, NULL,
									   &dwType, pszValue, &dwNewValueSize) != ERROR_SUCCESS)
			break;

		// Delete the value.
		RegDeleteValue (hOldDeviceKey, pszName);

		// Create the key in our new subkey.
		RegSetValueEx (hNewDeviceKey, pszName, 0, dwType, (LPBYTE)pszValue, dwNewValueSize);
	}

	// We're done with the memory.
	FreeMem (pszName);
	FreeMem (pszValue);

	// Close all the used keys.
	if (dwNewDevice > 0)
		RegCloseKey(hNewDeviceKey);
	if (dwOldDevice > 0)
		RegCloseKey(hOldDeviceKey);
	RegCloseKey (hProviderKey);
	RegCloseKey (hTelephonyKey);

	// Now delete the original section
	if (dwOldDevice > 0)
		DeleteProfile(dwOldDevice);

	return TRUE;

}// CServiceProvider::RenameProfile

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::AddDeviceClassInfo
//
// Add a device class info structure to an array
//
int CServiceProvider::AddDeviceClassInfo (CObArray& arrElem, LPCTSTR pszName, 
                        DWORD dwType, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle)
{
    DEVICECLASSINFO* pDevClass = FindDeviceClassInfo (arrElem, pszName);
    if (pDevClass)
        RemoveDeviceClassInfo(arrElem, pszName);
    pDevClass = new DEVICECLASSINFO (pszName, dwType, lpBuff, dwSize, hHandle);
    return arrElem.Add(pDevClass);

}// CServiceProvider::AddDeviceClassInfo

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::RemoveDeviceClassInfo
//
// Remove a device class structure from our array
//
BOOL CServiceProvider::RemoveDeviceClassInfo (CObArray& arrElem, LPCTSTR pszName)
{
    for (int i = 0; i < arrElem.GetSize(); i++)
    {
        DEVICECLASSINFO* pDevClass = (DEVICECLASSINFO*) arrElem[i];
        if (!pDevClass->strName.CompareNoCase(pszName))
        {
            arrElem.RemoveAt(i);
            delete pDevClass;
            return TRUE;
        }
    }
    return FALSE;

}// CServiceProvider::RemoveDeviceClassInfo

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::FindDeviceClassInfo
//
// Locate and return a device class structure from our array
//
DEVICECLASSINFO* CServiceProvider::FindDeviceClassInfo (CObArray& arrElem, LPCTSTR pszName)
{
    for (int i = 0; i < arrElem.GetSize(); i++)
    {
        DEVICECLASSINFO* pDevClass = (DEVICECLASSINFO*) arrElem[i];
        if (!pDevClass->strName.CompareNoCase(pszName))
            return pDevClass;
    }
    return NULL;

}// CServiceProvider::FindDeviceClassInfo

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::CopyDeviceClass
//
// Copy a DEVICECLASSINFO structure into a VARSTRING
//
LONG CServiceProvider::CopyDeviceClass (DEVICECLASSINFO* pDeviceClass, 
                                        LPVARSTRING lpDeviceID, HANDLE hTargetProcess)
{
    // Copy the basic information into the VARSTRING.
    lpDeviceID->dwUsedSize = sizeof(VARSTRING);
    lpDeviceID->dwNeededSize = sizeof(VARSTRING) + pDeviceClass->dwSize;
    lpDeviceID->dwStringOffset = 0;
    lpDeviceID->dwStringSize = 0;

    if (pDeviceClass->hHandle != INVALID_HANDLE_VALUE)
        lpDeviceID->dwNeededSize += sizeof(DWORD);
    lpDeviceID->dwStringFormat = pDeviceClass->dwStringFormat;

    // If we have enough space, copy the handle and data into our buffer.
    if (lpDeviceID->dwTotalSize >= lpDeviceID->dwNeededSize)
    {
        // Copy the handle first.  It must be duplicated for the process which
        // needs it.
        if (pDeviceClass->hHandle != INVALID_HANDLE_VALUE)
        {
            HANDLE hTargetHandle;
            if (DuplicateHandle(GetCurrentProcess(), pDeviceClass->hHandle, 
                        hTargetProcess, &hTargetHandle, 0, FALSE,
                        DUPLICATE_SAME_ACCESS))
            {
                AddDataBlock (lpDeviceID, lpDeviceID->dwStringOffset, lpDeviceID->dwStringSize,
                              &hTargetHandle, sizeof(HANDLE));
            }
        }

        // Now the buffer if present.
        if (pDeviceClass->dwSize > 0)
            AddDataBlock (lpDeviceID, lpDeviceID->dwStringOffset, lpDeviceID->dwStringSize,
                            pDeviceClass->lpvData, pDeviceClass->dwSize);
    }
    return FALSE;

}// CServiceProvider::CopyDeviceClass

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::OnNewRequest
//
// A request is being added to the connection list.
//
BOOL CServiceProvider::OnNewRequest (CTSPIConnection* /*pConn*/, CTSPIRequest* /*pReq*/, int* /*piPos*/)
{
    // Return TRUE to continue adding the request or FALSE to cancel the request.
    return TRUE;

}// CServiceProvider::OnNewRequest

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::ProcessData
//
// Master function to process all input from every line
// and phone device if no individual line/phone overrides
// are provided.
//
BOOL CServiceProvider::ProcessData (CTSPIConnection* /*pConn*/, 
                    DWORD /*dwData*/, const LPVOID /*lpvData*/, DWORD /*dwSize*/)
{
    // You MUST override this if no line/phone overrides are handled for the ReceiveData() functions!
    ASSERT (FALSE);
    return FALSE;

}// CServiceProvider::ProcessData

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::IntRegDeleteKey
//
// Internal function used to delete a section of the registry under Windows NT
// where we must delete each branch seperately.
//
BOOL CServiceProvider::IntRegDeleteKey (HKEY hKeyTelephony, LPCTSTR pszMainDir)
{
	// Attempt to delete the key directly.  Under Win95, this will also delete
	// any branches under it.
    if (RegDeleteKey (hKeyTelephony, pszMainDir) != ERROR_SUCCESS)
	{
		// Open the top-level key.
		HKEY hKey;
		DWORD dwRC = RegOpenKeyEx(hKeyTelephony, pszMainDir, 0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey);
		if (dwRC == ERROR_SUCCESS)
		{
			DWORD dwReqSize = 1024;
			LPTSTR pszName = (LPTSTR) AllocMem (dwReqSize+1);
			if (pszName == NULL)
			{
				RegCloseKey (hKey);
				return FALSE;
			}
		
			do
			{
				dwRC = RegEnumKeyEx(hKey, 0, pszName, &dwReqSize, NULL, NULL, NULL, NULL);
				if (dwRC == ERROR_NO_MORE_ITEMS)
				{
				   dwRC = RegDeleteKey(hKeyTelephony, pszMainDir);
				   break;
				}
				else if (dwRC == ERROR_SUCCESS)
				   IntRegDeleteKey (hKey, pszName);
			}
			while (dwRC == ERROR_SUCCESS);
			
			RegCloseKey(hKey);
			FreeMem (pszName);
		}
	}
	return TRUE;

}// CServiceProvider::IntRegDeleteKey

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::OnCancelRequest
//
// Cancel a request which has already been started (its state is not
// STATE_INITIAL).  The request is about to be deleted and send an
// error notification back to TAPI.  Generally this happens when the
// call or line is closed.  The service provider code should reset
// the device and take it out of the state.
//
VOID CServiceProvider::OnCancelRequest (CTSPIRequest* /*pReq*/)
{                                  
    /* Do nothing */

}// CServiceProvider::OnCancelRequest

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::TraceOut
//
// This method is called to output the TRACE facility.
//
void CServiceProvider::TraceOut(LPCTSTR pszBuff)
{
	OutputDebugString(pszBuff);

}// CServiceProvider::TraceOut

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::AddTimedCall
//
// This method adds a call to our "need timer" list.
//
void CServiceProvider::AddTimedCall(CTSPICallAppearance* pCall)
{
	CSingleLock lck(&m_csProvider, TRUE);
	for (POSITION pos = m_lstTimedCalls.GetHeadPosition(); pos != NULL;)
	{
		if ((CTSPICallAppearance*)m_lstTimedCalls.GetNext(pos) == pCall)
			return;
	}
	m_lstTimedCalls.AddTail(pCall);

}// CServiceProvider::AddTimedCall

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::RemoveTimedCall
//
// This method removes a call from our "need timer" list.
//
void CServiceProvider::RemoveTimedCall(CTSPICallAppearance* pCall)
{
	CSingleLock lck(&m_csProvider, TRUE);
	for (POSITION pos = m_lstTimedCalls.GetHeadPosition(); pos != NULL;)
	{
		POSITION posCurr = pos;
		if ((CTSPICallAppearance*)m_lstTimedCalls.GetNext(pos) == pCall)
		{
			m_lstTimedCalls.RemoveAt(posCurr);
			return;
		}
	}

}// CServiceProvider::RemoveTimedCall
                     
