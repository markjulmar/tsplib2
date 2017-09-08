/******************************************************************************/
//                                                                        
// SP.CPP - Service Provider Base source code                             
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the general methods for the "CServiceProvider" class    
// which is the main CWinApp derived class in the SPLIB C++ library.      
//
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
//                                                                        
/******************************************************************************/

#include "stdafx.h"
#include <ctype.h>
#if _MFC_VER==0x0252
#include <..\src\auxdata.h>
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// Globals

extern int g_iRefCount;

// Fix for memory leak under MFC 2.52
#if _MFC_VER==0x252
extern void PASCAL _AfxExitDelete(HGDIOBJ hObject);
#endif

// Copyright string.  This does not impact the load image.
#pragma comment(exestr, "TSP++ 1.56a Copyright (C) 1999 JulMar Technology, Inc.")

// INI file strings for mapping to registry.
LPCSTR gszProvider = "%s Device%ld";

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::CServiceProvider
//
// Base class constructor
//
CServiceProvider::CServiceProvider(const char* pszAppName, const char *pszExeName, 
                                    DWORD dwVersion, const char *pszProviderInfo) :
         CWinApp(pszAppName)
{
    // Setup the default runtime objects.  These may be overriden by
    // calling the member "SetRuntimeObjects"   
    m_pRequestObj = RUNTIME_CLASS(CTSPIRequest);
    m_pLineObj = RUNTIME_CLASS(CTSPILineConnection);
    m_pPhoneObj = RUNTIME_CLASS(CTSPIPhoneConnection);
    m_pDeviceObj = RUNTIME_CLASS(CTSPIDevice);
    m_pCallObj = RUNTIME_CLASS(CTSPICallAppearance);
    m_pAddrObj = RUNTIME_CLASS(CTSPIAddressInfo);
    m_pConfCallObj = RUNTIME_CLASS(CTSPIConferenceCall);
    m_lTimeout = MAX_WAIT_TIMEOUT;
    m_dwCurrentLocation = 0;
    m_dwTAPIVersionFound = 0;
    m_pszExeName = pszExeName;
    m_pszProviderInfo = pszProviderInfo;
    m_dwTapiVerSupported = dwVersion;
    
}// CServiceProvider::CServiceProvider

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetSupportedVersion
//
// Return the highest level of TAPI support this service provider
// has decided to conform to.
//
DWORD CServiceProvider::GetSupportedVersion() const
{                               
    return m_dwTapiVerSupported;

}// CServiceProvider::GetSupportedVersion

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetSystemVersion
//
// Return the version which TAPI.DLL is at on this computer.
//
DWORD CServiceProvider::GetSystemVersion() const
{                                     
    return m_dwTAPIVersionFound;

}// CServiceProvider::GetSystemVersion

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

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetCurrentLocation
//
// Return the current location set by TAPI and adjusted by the 
// dial property page.
//
DWORD CServiceProvider::GetCurrentLocation() const
{
    return m_dwCurrentLocation;
    
}// CServiceProvider::GetCurrentLocation

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetProviderName
//
// Return string name (passed to CWinApp constructor)
//
const char * CServiceProvider::GetProviderName() const
{                                              
    return m_pszAppName;

}// CServiceProvider::GetProviderName
                     
/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetProviderInfo
//
// Return the provider information
//
const char * CServiceProvider::GetProviderInfo() const
{
    return m_pszProviderInfo;

}// CServiceProvider::GetProviderInfo

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIRequestObj
//
// Return the TSPI request object dynamic creation descriptor
//
CRuntimeClass* CServiceProvider::GetTSPIRequestObj() const
{
    ASSERT (m_pRequestObj != NULL);
    return m_pRequestObj;

}// CServiceProvider::GetTSPIRequestObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPILineObj
//
// Return the TSPI line object dynamic creation descriptor
//
CRuntimeClass* CServiceProvider::GetTSPILineObj() const
{
    ASSERT (m_pLineObj != NULL);
    return m_pLineObj;

}// CServiceProvider::GetTSPILineObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIPhoneObj
//
// Return the TSPI phone object dynamic creation descriptor
//
CRuntimeClass* CServiceProvider::GetTSPIPhoneObj() const
{
    ASSERT (m_pPhoneObj != NULL);
    return m_pPhoneObj;

}// CServiceProvider::GetTSPIPhoneObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIDeviceObj
//
// Return the TSPI device object dynamic creation descriptor
//
CRuntimeClass* CServiceProvider::GetTSPIDeviceObj() const
{
    ASSERT (m_pDeviceObj != NULL);
    return m_pDeviceObj;

}// CServiceProvider::GetTSPIDeviceObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPICallObj
//
// Return the TSPI call object dynamic creation descriptor
//
CRuntimeClass* CServiceProvider::GetTSPICallObj() const
{
    ASSERT (m_pCallObj != NULL);
    return m_pCallObj;

}// CServiceProvider::GetTSPICallObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIConferenceCallObj
//
// Return the TSPI conference call object dynamic creation descriptor
//
CRuntimeClass* CServiceProvider::GetTSPIConferenceCallObj() const
{
    ASSERT (m_pConfCallObj != NULL);
    return m_pConfCallObj;

}// CServiceProvider::GetTSPIConferenceCallObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIAddressObj
//
// Return the TSPI address object dynamic creation descriptor
//
CRuntimeClass* CServiceProvider::GetTSPIAddressObj() const
{
    ASSERT (m_pAddrObj != NULL);
    return m_pAddrObj;

}// CServiceProvider::GetTSPIAddressObj

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

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetLineCreateProc
//
// Return the line creation procedure if available.
//
// WARNING: This may be NULL!
//
LINEEVENT CServiceProvider::GetLineCreateProc() const
{
    return m_lpfnLineCreateProc;

}// CServiceProvider::GetLineCreateProc

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetPhoneCreateProc
//
// Return the phone creation procedure if available.
//
// WARNING: This may be NULL!
//
PHONEEVENT CServiceProvider::GetPhoneCreateProc() const
{
    return m_lpfnPhoneCreateProc;

}// CServiceProvider::GetPhoneCreateProc

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTimeout
//
// Return the standard timeout value used for the service provider.
//
LONG CServiceProvider::GetTimeout() const
{
    return m_lTimeout;

}// CServiceProvider::GetTimeout

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::SetTimeout
//
// This method sets the default timeout used throughout the library.
//
VOID CServiceProvider::SetTimeout (LONG lTimeout)
{
    // We should always wait at LEAST 1/2 sec!
    ASSERT (lTimeout > 500);
    m_lTimeout = lTimeout;

}// CServiceProvider::SetTimeout

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetConnInfoFromLineDeviceID
//
// Return the CTSPIConnection object from a line device id.
//
CTSPILineConnection* CServiceProvider::GetConnInfoFromLineDeviceID (DWORD dwDevId)
{
    return (CTSPILineConnection*) SearchForConnInfo(dwDevId, 0);

}// CServiceProvider::GetConnInfoFromLineDeviceID

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetConnInfoFromPhoneDeviceID
//
// Return the CTSPIConnection object from a phone device id.
//
CTSPIPhoneConnection* CServiceProvider::GetConnInfoFromPhoneDeviceID (DWORD dwDevId) 
{
    return (CTSPIPhoneConnection*) SearchForConnInfo(dwDevId, 1);

}// CServiceProvider::GetConnInfoFromPhoneDeviceID

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::UnknownDeviceNotify
//
// This method is called when an unknown device notify command
// is received from the companion application.  If the application
// has been written with new commands or responses, this method needs
// to be overriden.
//
VOID CServiceProvider::UnknownDeviceNotify(WORD /*wCommand*/, DWORD /*dwConnId*/, 
                            DWORD /*dwResult*/, LPVOID /*lpvData*/, DWORD /*dwSize*/)
{   
    // MUST be overriden if new commands are defined!                        
    ASSERT (FALSE);
        
}// CServiceProvider::UnknownDeviceNotify

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::InitInstance
//
// This is called when the driver DLL is loaded by Windows through
// a OpenDriver call.
//
BOOL CServiceProvider::InitInstance()
{
    // Set our .INI file to be telephon.ini so all calls into the 
    // profile stuff go there.
    m_pszProfileName = "telephon.ini";

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
    TRACE ("ExitInstance entered\r\n");
    
    // Release pens which MFC doesn't delete properly in VC 1.52
    // This was fixed in VC 1.52b.
#if _MFC_VER==0x252
    _AfxExitDelete (afxData.hpenBtnShadow);
    _AfxExitDelete (afxData.hpenBtnHilite);
    _AfxExitDelete (afxData.hpenBtnText);
#endif    

    // Run through and delete the device map.
    for (POSITION pos = m_mapDevices.GetStartPosition(); pos != NULL; )
    {
        CTSPIDevice* pDev;
        WORD w;
        m_mapDevices.GetNextAssoc(pos, w, (CObject*&)pDev);
        delete pDev;
    }
    m_mapDevices.RemoveAll();

    // Do not call the normal CWinApp stuff since it writes out the
    // garbage to the .INI file.
    return FALSE;

}// CServiceProvider::ExitInstance

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::IntervalTimer
//
// Walk through all the devices we are running and give it the
// interval timer.
//
void CServiceProvider::IntervalTimer()
{
    for (POSITION pos = m_mapDevices.GetStartPosition(); pos != NULL; )
    {
        CTSPIDevice* pDev;
        WORD w;
        m_mapDevices.GetNextAssoc(pos, w, (CObject*&)pDev);
        if (pDev)
            pDev->OnTimer();
    }
   
}// CServiceProvider::IntervalTimer

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::OnTimer
//
// This method is called back by the device interval timer if the
// derived connection class doesn't override it.  It should be 
// overriden by the derived service provider class in order to do 
// whatever work needs to be accomplished.
//
VOID CServiceProvider::OnTimer (CTSPIConnection* /*pConn*/)
{
    /* Do nothing */

}// CServiceProvider::OnTimer

///////////////////////////////////////////////////////////////////////////
// CTSPIServiceProvider::ConvertDialableToCanonical
//
// This function is used to convert a dialable number into
// a standard TAPI canonical number.  So, if the number
// is 12143647464, then it will be formatted to "+1 (214) 3647464".
//                               
CString CServiceProvider::ConvertDialableToCanonical(const char * pszNumber, DWORD /*dwCountryCode*/)
{   
    CString strInput = GetDialableNumber (pszNumber);
    CString strOutput;

    // This function assumes North American standards, i.e. a country code (1)
    // will be followed by an area code (214), followed by an exchange (364) and
    // finally extension (7464).
    
    // Since there may be toll, billing, outside line access, or call-waiting
    // information at the front of the string, walk through it backwards to 
    // get our "stuff".
    if (strInput.GetLength() >= 7)
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
        {
            wsprintf(strOutput.GetBuffer(20), "+1 (%.3s) ", (LPCSTR)strAreaCode);
            strOutput.ReleaseBuffer();
        }
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
CString CServiceProvider::GetDialableNumber (const char *pszNumber, const char *pszAllow) const
{                 
    CString strReturn;
    if (pszNumber)
    {
        while (*pszNumber)
        {   
            if (isdigit(*pszNumber) || (pszAllow && strchr(pszAllow, *pszNumber)))
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
LPCSTR lpszDigits,          // Original input
CObArray* parrEntries,      // Entry array for broken number information
DWORD  /*dwCountry*/,       // Country code for this number
LPCSTR lpszValidChars)      // Valid characters for the number
{
    // Move the buffer to another string so we can modify it as we go.
    CString strNumber = lpszDigits;
	if (lpszValidChars == NULL)
		lpszValidChars = "0123456789ABCD*#!WPT@$+,";

    // If the prompt is in the string, this is an error and the
    // application has sent us a bad dial string.
    if (strNumber.Find('?') >= 0)
        return LINEERR_DIALPROMPT;
    
    // If more than one address is listed, and we don't support multiplexing
    // on this line device, then error it.
    if (strNumber.Find("\r\n") >= 0)
    {
        if (pLine &&
            (pLine->GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_MULTIPLEADDR) == 0)
        {
            TRACE("Multiple addresses listed in dialable address, ignoring all but first.\r\n");
            strNumber = strNumber.Left (strNumber.Find("\r\n"));
        }
    }                
    
    // Final result code
    LONG lResult = 0;
    
    // Now go through the string breaking each up into a seperate dial string (if
    // more than one is there).
    while (!strNumber.IsEmpty())
    {   
        CString strBuff;
        int iPos = strNumber.Find("\r\n");
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
        iPos = strBuff.Find('|');
        if (iPos >= 0)
        {   
            strSubAddress = strBuff.Mid(iPos+1);
            int iEndPos = strSubAddress.FindOneOf("+|^");
            if (iEndPos >= 0)
                strSubAddress = strSubAddress.Left (iEndPos);
        }
        
        // Now grab the NAME if present in the string.
        iPos = strBuff.Find('^');
        if (iPos >= 0)
            strName = strBuff.Mid(iPos+1);

        // Strip off all the ISDN/Name info
        iPos = strBuff.FindOneOf("|^");
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
        while (strBuff.Right(1) == ' ')
            strBuff = strBuff.Left(strBuff.GetLength()-1);
        
        // Check to see if partial dialing is allowed.
        BOOL fPartialDial = FALSE;
        if (strBuff.Right(1) == ';')
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
        
        if (strBuff.Find('$') >= 0 &&
            pLine &&
            (pLine->GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_DIALBILLING) == 0)
        {
            lResult = LINEERR_DIALBILLING;
            break;
        }
       
        if (strBuff.Find('@') >= 0 &&
            pLine &&
            (pLine->GetLineDevCaps()->dwDevCapFlags & LINEDEVCAPFLAGS_DIALQUIET) == 0)
        {
            lResult = LINEERR_DIALQUIET;
            break;
        }
        
        if (strBuff.Find('W') >= 0 &&
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
CTSPIDevice* CServiceProvider::GetDevice(DWORD dwId) const
{
    CObject* pDev = NULL;
    m_mapDevices.Lookup((WORD)dwId, pDev);
    return (CTSPIDevice*) pDev;
   
}// CServiceProvider::GetDevice

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
    ASSERT(wReqType == 0 || wReqType == 1);
    TRACE("Looking for connection info, Type=%d, Id=%lx\r\n", wReqType, dwId);

    CTSPIConnection* pConn = NULL;

    // Walk through all the service provider device structures and locate this entry.
    for (POSITION pos = m_mapDevices.GetStartPosition(); pos != NULL && pConn == NULL;)
    {
        CTSPIDevice* pDev;
        WORD w;
        m_mapDevices.GetNextAssoc(pos, w, (CObject*&)pDev);
        if (pDev != NULL)
        {
            if (wReqType == 0)  // Line?
                pConn = pDev->FindLineConnectionByDeviceID (dwId);
            else
                pConn = pDev->FindPhoneConnectionByDeviceID (dwId);
        }
    }
    return pConn;
   
}// CServiceProvider::SearchForConnInfo

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ProcessCallParameters
//
// This function is used to determine if the passed call parameters
// are valid for the device they are being used for.
//
LONG CServiceProvider::ProcessCallParameters (CTSPIAddressInfo* pAddr, LPLINECALLPARAMS lpCallParams)
{
    return ProcessCallParameters (pAddr->GetLineOwner(), lpCallParams);

}// CServiceProvider::ProcessCallParameters

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ProcessCallParameters
//
// This function is used to determine if the passed call parameters
// are valid for the device they are being used for.
//
LONG CServiceProvider::ProcessCallParameters (CTSPICallAppearance* pCall, LPLINECALLPARAMS lpCallParams)
{   
    return ProcessCallParameters (pCall->GetLineConnectionInfo(), lpCallParams);

}// CServiceProvider::ProcessCallParameters

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
BOOL CServiceProvider::CanHandleRequest (CTSPIConnection* pConn, WORD wRequest, DWORD dwData)
{
    return CanHandleRequest (pConn, (CTSPIAddressInfo*)NULL, 
                             (CTSPICallAppearance*)NULL, wRequest, dwData);

}// CServiceProvider::CanHandleRequest

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::CanHandleRequest
//
// This function is used to dynamically determine what the capabilities
// of our service provider really is.
//
BOOL CServiceProvider::CanHandleRequest (CTSPIAddressInfo* pAddr, WORD wRequest, DWORD dwData)
{
    return CanHandleRequest (pAddr->GetLineOwner(), pAddr, (CTSPICallAppearance*)NULL, wRequest, dwData);

}// CServiceProvider::CanHandleRequest

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::CanHandleRequest
//
// This function is used to dynamically determine what the capabilities
// of our service provider really is.
//
BOOL CServiceProvider::CanHandleRequest (CTSPICallAppearance* pCall, WORD wRequest, DWORD dwData)
{
    return CanHandleRequest (pCall->GetLineConnectionInfo(),
                             pCall->GetAddressInfo(), pCall, wRequest, dwData);

}// CServiceProvider::CanHandleRequest

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
        if (wRequest != REQUEST_SETCALLPARAMS &&
            wRequest != REQUEST_DROPCALL &&
            wRequest != REQUEST_DROPCALLONCLOSE &&
            wRequest != REQUEST_DROPNOOWNER)
            return FALSE;            
    }
    
    return m_arrProviderCaps [wRequest];

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
    HINSTANCE hInst = AfxGetInstanceHandle();
    ASSERT (hInst != NULL);
    
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEACCEPT, 0)))
        m_arrProviderCaps.SetAt(REQUEST_ACCEPT, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEADDTOCONFERENCE, 0)))
        m_arrProviderCaps.SetAt(REQUEST_ADDCONF, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEANSWER, 0)))
        m_arrProviderCaps.SetAt(REQUEST_ANSWER, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEBLINDTRANSFER, 0)))
        m_arrProviderCaps.SetAt(REQUEST_BLINDXFER, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINECOMPLETECALL, 0)))
        m_arrProviderCaps.SetAt(REQUEST_COMPLETECALL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINECOMPLETETRANSFER, 0)))
        m_arrProviderCaps.SetAt(REQUEST_COMPLETEXFER, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEDIAL, 0)))
        m_arrProviderCaps.SetAt(REQUEST_DIAL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEDROP, 0)))
        m_arrProviderCaps.SetAt(REQUEST_DROPCALL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEFORWARD, 0)))
        m_arrProviderCaps.SetAt(REQUEST_FORWARD, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEHOLD, 0)))
        m_arrProviderCaps.SetAt(REQUEST_HOLD, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEMAKECALL, 0)))
        m_arrProviderCaps.SetAt(REQUEST_MAKECALL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEPARK, 0)))
        m_arrProviderCaps.SetAt(REQUEST_PARK, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEPICKUP, 0)))
        m_arrProviderCaps.SetAt(REQUEST_PICKUP, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEREDIRECT, 0)))
        m_arrProviderCaps.SetAt(REQUEST_REDIRECT, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEREMOVEFROMCONFERENCE, 0)))
        m_arrProviderCaps.SetAt(REQUEST_REMOVEFROMCONF, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESECURECALL, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SECURECALL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESENDUSERUSERINFO, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SENDUSERINFO, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESETCALLPARAMS, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETCALLPARAMS, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESETTERMINAL, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETTERMINAL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESETUPCONFERENCE, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETUPCONF, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESETUPTRANSFER, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETUPXFER, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESWAPHOLD, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SWAPHOLD, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEUNCOMPLETECALL, 0)))
        m_arrProviderCaps.SetAt(REQUEST_UNCOMPLETECALL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEUNHOLD, 0)))
        m_arrProviderCaps.SetAt(REQUEST_UNHOLD, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEUNPARK, 0)))
        m_arrProviderCaps.SetAt(REQUEST_UNPARK, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINESETMEDIACONTROL, 0)))
        m_arrProviderCaps.SetAt(REQUEST_MEDIACONTROL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEMONITORDIGITS, 0)))
        m_arrProviderCaps.SetAt(REQUEST_MONITORDIGITS, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEMONITORMEDIA, 0)))
        m_arrProviderCaps.SetAt(REQUEST_MONITORMEDIA, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEMONITORTONES, 0)))
        m_arrProviderCaps.SetAt(REQUEST_MONITORTONES, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEPREPAREADDTOCONFERENCE, 0)))
        m_arrProviderCaps.SetAt(REQUEST_PREPAREADDCONF, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEGATHERDIGITS, 0)))
        m_arrProviderCaps.SetAt(REQUEST_GATHERDIGITS, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEGENERATEDIGITS, 0)))
        m_arrProviderCaps.SetAt(REQUEST_GENERATEDIGITS, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEGENERATETONE, 0)))
        m_arrProviderCaps.SetAt(REQUEST_GENERATETONE, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEDROPONCLOSE, 0)))
        m_arrProviderCaps.SetAt(REQUEST_DROPCALLONCLOSE, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINEDROPNOOWNER, 0)))
        m_arrProviderCaps.SetAt(REQUEST_DROPNOOWNER, TRUE); 
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_LINERELEASEUSERUSERINFO, 0)))
        m_arrProviderCaps.SetAt(REQUEST_RELEASEUSERINFO, TRUE);        
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETDISPLAY, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETDISPLAY, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETGAIN, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETHOOKSWITCHGAIN, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETHOOKSWITCH, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETHOOKSWITCH, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETLAMP, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETLAMP, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETRING, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETRING, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETBUTTONINFO, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETBUTTONINFO, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETVOLUME, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETHOOKSWITCHVOL, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONESETDATA, 0)))
        m_arrProviderCaps.SetAt(REQUEST_SETPHONEDATA, TRUE);
    if (GetProcAddress (hInst, (LPCSTR)MAKELONG(TSPI_PHONEGETDATA, 0)))
        m_arrProviderCaps.SetAt(REQUEST_GETPHONEDATA, TRUE);

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
DWORD /*dwPPID*/,                     // Provider ID (unique across providers)
CWnd* /*pwndOwner*/)                  // Owner window to supply dialog for.
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
DWORD /*dwPermanentProviderID*/,    // Provider ID (unique across providers)
CWnd* /*pwndOwner*/)                // Owner window to supply and UI for.
{
    return FALSE;
    
}// CServiceProvider::providerInstall

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerRemove
//
// This method is invoked when the TSP is being removed from the
// system.  It should remove all its files and .INI settings.
//
// This method should be overriden by the derived class.
//
LONG CServiceProvider::providerRemove(
DWORD dwPermanentProviderID,    // Provider ID (unique across providers)
CWnd* /*pwndOwner*/)            // Owner window to supply and UI for.
{                    
	// Remove any profile settings under our provider key.  Since we may not
	// actually be loaded right now (if simply invoked) we don't know what
	// our devices are, so run through all listed in the registry.
	int iCount = CWinApp::GetProfileInt	("Providers", "NumProviders", -1)+1;
	for (int i = 0; i < iCount; i++)
		DeleteProfile ((DWORD)i);
		
	// Delete any PROVIDER oriented profile section
	DeleteProfile (dwPermanentProviderID);		
		
	// Remove the "Provider" section which details the line/phone count.
	char szBuff[30];
	wsprintf (szBuff, "Provider%ld", dwPermanentProviderID);
	WritePrivateProfileString (szBuff, NULL, NULL, m_pszProfileName);
		
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
ASYNC_COMPLETION lpfnCallback)   // Asynchronous completion callback.
{                    
    // Do one-time initialization for the first provider instance
    if (g_iRefCount == 0)
    {
        // Save off the version of TAPI we are running under.
        m_dwTAPIVersionFound = dwTSPVersion;
    
        // Determine what capabilities the device supports.
        DetermineProviderCapabilities();
    
        // If the application has not started, then run it now.  Note
        // that the deriving class can disable the companion application
        // by simply setting the global to NULL.
        if (m_pMainWnd == NULL && m_pszExeName != NULL)
        {
            ASSERT(m_pMainWnd == NULL);
            UINT uiErr = WinExec(m_pszExeName, SW_SHOWMINIMIZED);
            if (uiErr <= (WORD) HINSTANCE_ERROR)
            {
                TRACE("Problem starting companion application <%s>, rc=0x%x.", m_pszExeName, uiErr);
                return LINEERR_NODRIVER;
            }
        }
#ifdef _DEBUG
        else
            ASSERT(m_pMainWnd != NULL);
#endif
    }
    else
    {
        ASSERT (m_dwTAPIVersionFound == dwTSPVersion);
        if (m_pMainWnd == NULL && m_pszExeName != NULL)
            return LINEERR_NODRIVER;
    }
       
    // Allocate a device information object for this id and add it to
    // our device map.  This device object maintains the connection lists
    // and line information.
    CTSPIDevice* pDev = (CTSPIDevice*) GetTSPIDeviceObj()->CreateObject();
    ASSERT(pDev->IsKindOf(RUNTIME_CLASS(CTSPIDevice)));
    pDev->Init(dwProviderId, dwLineBase, dwPhoneBase, dwNumLines, dwNumPhones, lpfnCallback);
    m_mapDevices.SetAt((WORD)dwProviderId, pDev);
    return FALSE;

}// CServiceProvider::providerInit   

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerShutdown
//
// This method is called to shutdown our service provider.  It will
// be called directly before the unload of our driver.
//
LONG CServiceProvider::providerShutdown(DWORD /*dwTSPVersion*/)
{   
    // Devices will be deleted when the ExitInstance is called to ensure
    // proper shutdown in all cases.
    if (g_iRefCount == 0)
        // Terminate our provider thread if it is active
        TerminateRequestThread();
    return FALSE;

}// CServiceProvider::providerShutdown

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::providerEnumDevices
//
// TAPI calls this function before providerInit to determine the number
// of line and phone devices supported by the service provider.  This
// allows the service provider to read a configuration OTHER than
// the TELEPHON.INI to gather line/phone counts.  This is especially
// important in devices which support Plug&Play.
//
LONG CServiceProvider::providerEnumDevices(
DWORD dwProviderId,                 // Our Provider ID
LPDWORD lpNumLines,                 // Number of lines (return)
LPDWORD lpNumPhones,                // Number of phones (return)    
HPROVIDER /*hProvider*/,            // TAPIs HANDLE to our service provider
LINEEVENT lpfnLineCreateProc,       // LINEEVENT for dynamic line creation
PHONEEVENT lpfnPhoneCreateProc)     // PHONEEVENT for dynamic phone creation
{   
    // If we cannot support this function, then return an error.
    if (GetSupportedVersion() < TAPIVER_14)
        return LINEERR_OPERATIONUNAVAIL;

    // Store off the line/phone event procedures.
    m_lpfnLineCreateProc = lpfnLineCreateProc;
    m_lpfnPhoneCreateProc = lpfnPhoneCreateProc;

    // Now read off the TELEPHON.INI what our lines/phones count is.
    // Override this if your device supports dynamic querying of the device
    // for line counts.
    char szBuff[30];
    wsprintf (szBuff, "Provider%ld", dwProviderId);

    *lpNumLines = (DWORD) GetProfileInt (szBuff, "NumLines", 0);
    *lpNumPhones = (DWORD) GetProfileInt (szBuff, "NumPhones", 0);

    return FALSE;

}// CServiceProvider::providerEnumDevices

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
// This method is called as the master request handler if no overrides
// are provided on a line/phone connection basis.  It must be overriden
// if no overrides are provided for the CTSPILineConnection/CTSPIPhoneConnection.
//
BOOL CServiceProvider::ProcessData(CTSPIConnection* /*pConn*/, DWORD /*dwData*/, 
                    const LPVOID /*lpBuff*/, DWORD /*dwSize*/)
{                                
    // You MUST override if you hit this ASSERT!
    ASSERT (FALSE);
    return FALSE;

}// CServiceProvider::ProcessData

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ReadProfileString
//
// Read a string from our profile section in the telephon.ini.  This
// function is limited to 256 characters.
//
CString CServiceProvider::ReadProfileString (DWORD dwDeviceID, LPCSTR pszEntry, LPCSTR pszDefault)
{
	CString strGroup;
	wsprintf (strGroup.GetBuffer(512), gszProvider, m_pszProviderInfo, dwDeviceID);
	strGroup.ReleaseBuffer();
	strGroup = CWinApp::GetProfileString (strGroup, pszEntry);
	if (strGroup.IsEmpty())
		return pszDefault;
	return strGroup;

}// CServiceProvider::ReadProfileString

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ReadProfileDWord
//
// Read a DWORD from our profile section in the registry.
//
DWORD CServiceProvider::ReadProfileDWord (DWORD dwDeviceID, LPCSTR pszEntry, DWORD dwDefault)
{
	CString strGroup;
	int iLen = lstrlen(gszProvider)+lstrlen(m_pszProviderInfo)+10;
	wsprintf (strGroup.GetBuffer(iLen), gszProvider, m_pszProviderInfo, dwDeviceID);
	strGroup.ReleaseBuffer();

	CString strValue = CWinApp::GetProfileString (strGroup, pszEntry);
	if (strValue.IsEmpty() == FALSE)
		return (DWORD) atol(strValue);
	return dwDefault;

}// CServiceProvider::ReadProfileDWord

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::WriteProfileString
//
// Write a string into our registry profile.
//
BOOL CServiceProvider::WriteProfileString (DWORD dwDeviceID, LPCSTR pszEntry, LPCSTR pszValue)
{
	CString strGroup;
	int iLen = lstrlen(gszProvider)+lstrlen(m_pszProviderInfo)+10;
	wsprintf (strGroup.GetBuffer(iLen), gszProvider, m_pszProviderInfo, dwDeviceID);
	strGroup.ReleaseBuffer();

    return CWinApp::WriteProfileString (strGroup, pszEntry, pszValue);

}// CServiceProvider::WriteProfileString

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::WriteProfileDWord
//
// Write a DWORD into our registry profile.
//
BOOL CServiceProvider::WriteProfileDWord (DWORD dwDeviceID, LPCSTR pszEntry, DWORD dwValue)
{
    char szValue[30];

	CString strGroup;            
	int iLen = lstrlen(gszProvider)+lstrlen(m_pszProviderInfo)+10;
	wsprintf (strGroup.GetBuffer(iLen), gszProvider, m_pszProviderInfo, dwDeviceID);
	strGroup.ReleaseBuffer();

    wsprintf (szValue, "%lu", dwValue);
    return CWinApp::WriteProfileString (strGroup, pszEntry, szValue);

}// CServiceProvider::WriteProfileDWord

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::DeleteProfile
//
// Remove a profile from the system.
//
BOOL CServiceProvider::DeleteProfile(DWORD dwDeviceID)
{
	CString strGroup;
	int iLen = lstrlen(gszProvider)+lstrlen(m_pszProviderInfo)+10;
	wsprintf (strGroup.GetBuffer(iLen), gszProvider, m_pszProviderInfo, dwDeviceID);
	strGroup.ReleaseBuffer();

	// Delete the section.  We cannot use the CWinApp::WriteProfileString here since
	// it ASSERTs on the "Entry" being NULL.
	return WritePrivateProfileString (strGroup, NULL, NULL, m_pszProfileName);

}// CServiceProvider::DeleteProfile
