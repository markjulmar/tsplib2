/******************************************************************************/
//                                                                        
// SPLINE.CPP - Service Provider lineXXX source code                             
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the methods specifiec to a line for 
// the "CServiceProvider" class which is the main CWinApp derived 
// class in the SPLIB C++ library.      
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

#pragma warning(disable:4100)   // We don't use all parameters so turn off the warning

/////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineNegotiateTSPIVersion
//
// This method is called when TAPI wishes to negotiate available
// versions with us for any line device installed.  The low and high
// version numbers passed are ones which the installed TAPI.DLL supports,
// and we are expected to return the highest value which we can support
// so TAPI knows what type of calls to make to us.
// 
// This method may be called more than once during the life of our
// SP.
//
LONG CServiceProvider::lineNegotiateTSPIVersion(
DWORD dwDeviceId,             // Valid line device -or- INITIALIZE_NEGOTIATION
DWORD dwLowVersion,           // Lowest TAPI version known
DWORD dwHiVersion,            // Highest TAPI version known
LPDWORD lpdwTSPVersion)       // Return version
{
	// Validate the parameter
	if (lpdwTSPVersion == NULL || IsBadWritePtr(lpdwTSPVersion, sizeof(DWORD)))
		return LINEERR_INVALPARAM;

    // If this is a specific line initialize request, then locate
    // it and make sure it belongs to us.
    if (dwDeviceId != INITIALIZE_NEGOTIATION)
    {   
    	CTSPILineConnection* pLine = GetConnInfoFromLineDeviceID(dwDeviceId);
    	if (pLine == NULL)
            return LINEERR_BADDEVICEID;
    }
    
    // Do a SERVICE PROVIDER negotiation.
    *lpdwTSPVersion = GetSupportedVersion();
    if (dwLowVersion > *lpdwTSPVersion) // The app is too new for us
        return LINEERR_INCOMPATIBLEAPIVERSION;

    // If the version supported is LESS than what we support,
    // then drop to the version it allows.  The library can handle
    // down to TAPI 1.3.
    if (dwHiVersion < *lpdwTSPVersion)
    {
        if (dwHiVersion < TAPIVER_13)
            return LINEERR_INCOMPATIBLEAPIVERSION;
        *lpdwTSPVersion = dwHiVersion;
    }

    // Everything looked Ok.
    return FALSE;

}// CServiceProvider::lineNegotiateTSPIVersion

/////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineNegotiateExtVersion
//
// This function returns the highest extension version number the SP is
// willing to operate under for the device given the range of possible
// extension versions.
//
LONG CServiceProvider::lineNegotiateExtVersion(
DWORD dwDeviceID,                   // Device Id (0-# of TAPI devices)
DWORD dwTSPIVersion,                // Exchanged TSPI versions
DWORD /*dwLowVersion*/,             // Low extension version
DWORD /*dwHiVersion*/,              // Hi extension version
LPDWORD lpdwExtVersion)             // Return buffer
{
	// Validate the parameter
	if (lpdwExtVersion == NULL || IsBadWritePtr(lpdwExtVersion, sizeof(DWORD)))
		return LINEERR_INVALPARAM;

    // Set it to ZERO
    *lpdwExtVersion = 0;

    if (GetConnInfoFromLineDeviceID(dwDeviceID) == NULL)
    {
        DTRACE(TRC_MIN, _T("Line negotiation failure, device Id out of range\r\n"));
        return LINEERR_BADDEVICEID;
    }

    // Ok, device Id looks ok, do the version negotiation.
    if (dwTSPIVersion != GetSupportedVersion())
        return LINEERR_INCOMPATIBLEAPIVERSION;

    // We don't natively support EXT versions.  Derived function should
    // override this and look for this return code and supply its own
    // checking for extended version support.
    return LINEERR_OPERATIONUNAVAIL;

}// CServiceProvider::lineNegotiateExtVersion

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSelectExtVersion
//
// This function selects the indicated Extension version for the 
// indicated line device.  Subsequent requests operate according to that 
// Extension version.
//
LONG CServiceProvider::lineSelectExtVersion(
CTSPILineConnection* pLine,
DWORD dwExtVersion) 
{
    // We don't natively support EXT versions.  Derived function should
    // override this and look for this return code and supply its own
    // checking for extended version support.
    return LINEERR_OPERATIONUNAVAIL;
    
}// CServiceProvider::lineSelectExtVersion

/////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetAppSpecific
//
// This method is called when the TAPI application wants to associate
// a specific application data element with the  line.
//
LONG CServiceProvider::lineSetAppSpecific(CTSPICallAppearance* pCall, DWORD dwAppSpecific)
{
    pCall->SetAppSpecificData(dwAppSpecific);
    return FALSE;

}// CServiceProvider::lineSetAppSpecific

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineAccept
//
// This function is used in Telephony environments such as ISDN which
// allow alerting associated with incoming calls to be seperate from the
// initial offering of the call.  When a call comes in, the call is offered
// and may be redirected, dropped, answered or accepted.  Once accepted,
// ringing begins at both parties.
//
LONG CServiceProvider::lineAccept(
CTSPICallAppearance* pCall,         // Call appearance which is being offered
DRV_REQUESTID dwRequestID,          // Asynch request id
LPCSTR lpszUserUserInfo,            // Optional user->user info
DWORD dwSize)                       // Size of above buffer
{
    // Allocate a new buffer for the UserUserInfo.
    LPSTR lpBuff = NULL;
    if (lpszUserUserInfo != NULL && dwSize > 0)
    {   
        CTSPILineConnection* pLine = pCall->GetLineOwner();
        // If we don't support this type of UserUserInfo, then simply
        // ignore it - many apps send it and don't expect a TOOBIG error.
        if (pLine->GetLineDevCaps()->dwUUIAcceptSize > 0)
        {
            // Validate the UserUser information.
            if (pLine->GetLineDevCaps()->dwUUIAcceptSize < dwSize)
                return LINEERR_USERUSERINFOTOOBIG;
        
            // Allocate a buffer to store off the UserUser Information.
            lpBuff = (LPSTR) AllocMem (dwSize);
            if (lpBuff == NULL)
                return LINEERR_NOMEM;
            
            CopyBuffer (lpBuff, lpszUserUserInfo, dwSize);
        }
        else
            dwSize = 0;            
    }
    else
        dwSize = 0;

    // Send the request to the call appearance - if it fails, free the memory involved.
    LONG lResult = pCall->Accept (dwRequestID, lpBuff, dwSize);
    if (ReportError(lResult))
        FreeMem (lpBuff);
    return lResult;

}// CServiceProvider::lineAccept

//////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetupConference
//
// Sets up a conference call for the addition of the third party. 
//
LONG CServiceProvider::lineSetupConference(
CTSPILineConnection* pLine,      // Line connection object 
CTSPICallAppearance* pCall,      // First party of the conference call   
DRV_REQUESTID dwRequestID,       // Asynch. req. id
HTAPICALL htConfCall,            // New conference call TAPI handle
LPHDRVCALL lphdConfCall,         // Returning call handle
HTAPICALL htConsultCall,         // Call to conference in
LPHDRVCALL lphdConsultCall,      // Returning call handle
DWORD dwNumParties,              // Number of parties expected.
LPLINECALLPARAMS const lpCallParams) // Line parameters
{
    // We need an address to create the conference call onto.  If we got
    // a valid call handle to start the call relationship with, then use
    // it as the starting address.
    CTSPIAddressInfo* pAddr = NULL;
    if (pCall != NULL)
        pAddr = pCall->GetAddressOwner();
    else
    {
        ASSERT (pLine != NULL);
        pAddr = pLine->FindAvailableAddress (lpCallParams);
    }

    // If we never found an address capable of supporting this, then cancel out.
    if (pAddr == NULL)
        return LINEERR_INVALLINEHANDLE;

    // If we were passed line parameters, verify them
    LPLINECALLPARAMS lpMyCallParams = NULL;
    if (lpCallParams)
    {
        // Copy the call parameters into our own buffer.
        lpMyCallParams = (LPLINECALLPARAMS) AllocMem (lpCallParams->dwTotalSize);
        if (lpMyCallParams == NULL)
            return LINEERR_NOMEM;
        CopyBuffer (lpMyCallParams, lpCallParams, lpCallParams->dwTotalSize);
        
        // Process the call parameters
        LONG lResult = ProcessCallParameters(pLine, lpMyCallParams);
        if (lResult)
        {
            FreeMem ((LPTSTR)lpMyCallParams);
            return lResult;          
        }
    }

    // Allocate a CONFERENCE structure to pass down to the worker thread.
    TSPICONFERENCE* lpConf = new TSPICONFERENCE;
    if (lpConf == NULL)
    {
        FreeMem ((LPTSTR)lpMyCallParams);
        return LINEERR_NOMEM;    
    }

    // Setup our conference structure.
    lpConf->pCall = pCall;
    lpConf->dwPartyCount = dwNumParties;
    lpConf->lpCallParams = lpMyCallParams;

    // Let the address do the work of setting up the conference.
    LONG lResult = pAddr->SetupConference (dwRequestID, lpConf, htConfCall, lphdConfCall, htConsultCall, lphdConsultCall);
    if (ReportError(lResult))
        delete lpConf;
    return lResult;

}// CServiceProvider::lineSetupConference

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::linePrepareAddToConference
//
// Prepares an existing conference call for the addition of another party.
//
LONG CServiceProvider::linePrepareAddToConference(
CTSPIConferenceCall* pConfCall,         // Existing conference call
DRV_REQUESTID dwRequestID,              // Asynch request id
HTAPICALL htConsultCall,                // Newly created consultation call
LPHDRVCALL lphdConsultCall,             // Our opaque handle for the consultation call    
LPLINECALLPARAMS lpCallParams)          // Call parameters required
{
    // If we were passed line parameters, verify them
    LPLINECALLPARAMS lpMyCallParams = NULL;
    if (lpCallParams)
    {
        // Copy the call parameters into our own buffer.
        lpMyCallParams = (LPLINECALLPARAMS) AllocMem (lpCallParams->dwTotalSize);
        if (lpMyCallParams == NULL)
            return LINEERR_NOMEM;
        CopyBuffer (lpMyCallParams, lpCallParams, lpCallParams->dwTotalSize);
        
        // Process the call parameters
        LONG lResult = ProcessCallParameters(pConfCall, lpMyCallParams);
        if (lResult)
        {
            FreeMem ((LPTSTR)lpMyCallParams);
            return lResult;          
        }
    }

    // Allocate a TSPICONFERENCE structure to pass down to the worker thread.
    TSPICONFERENCE* lpConf = new TSPICONFERENCE;
    if (lpConf == NULL)            
    {
        FreeMem ((LPTSTR)lpMyCallParams);
        return LINEERR_NOMEM;    
    }

    lpConf->pConfCall = pConfCall;
    lpConf->dwPartyCount = 0L;
    lpCallParams = lpMyCallParams;

    // Let the conference call manage it.
    LONG lResult = pConfCall->PrepareAddToConference (dwRequestID, htConsultCall, lphdConsultCall, lpConf);
    if (ReportError(lResult))
        delete lpConf;
    return lResult;

}// CServiceProvider::linePrepareAddToConference

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineAddToConference
//
// This function adds the call specified by 'hdConsultCall' to the
// conference call specified by 'hdConfCall'.
//
LONG CServiceProvider::lineAddToConference(
CTSPIConferenceCall* pConfCall,        // Existing conference call
CTSPICallAppearance* pConsultCall,     // New call to add to conference
DRV_REQUESTID dwRequestID)             // Request id to associate with conf.
{
    // Allocate a TSPICONFERENCE structure to pass down to the 
    // worker thread.
    TSPICONFERENCE* lpConf = new TSPICONFERENCE;
    if (lpConf == NULL)
        return LINEERR_NOMEM;

    // Fill in the structure.
    lpConf->pConfCall = pConfCall;
    lpConf->pConsult = pConsultCall;

    // Let the conference call manage it.
    LONG lResult = pConfCall->AddToConference (dwRequestID, pConsultCall, lpConf);
    if (ReportError(lResult))
        delete lpConf;
    return lResult;

}// CServiceProvider::lineAddToConference

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineRemoveFromConference
//
// This function removes the specified call from the conference call to
// which it currently belongs.  The remaining calls in the conference call
// are unaffected.
//
LONG CServiceProvider::lineRemoveFromConference(
CTSPICallAppearance* pCall,         // Call appearance
DRV_REQUESTID dwRequestID)          // Asynch req. id.
{
    // Locate the conference call - it should be stored in the related call
    // field of the CALLINFO record.
    CTSPIConferenceCall* pConfCall = pCall->GetConferenceOwner();
    if (pConfCall == NULL || pConfCall->GetCallType() != CALLTYPE_CONFERENCE)
		return LINEERR_NOCONFERENCE;

    // Allocate a CONFERENCE structure to pass down to the worker thread.
    TSPICONFERENCE* lpConf = new TSPICONFERENCE;
    if (lpConf == NULL)
        return LINEERR_NOMEM;

    // Fill in the structure.
    lpConf->pConfCall = pConfCall;
    lpConf->pCall = pCall;

    // Pass the request onto the conference call.
    LONG lResult = pConfCall->RemoveFromConference (dwRequestID, pCall, lpConf);
    if (ReportError(lResult))
        delete lpConf;
    return lResult;

}// CServiceProvider::lineRemoveFromConference

//////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSecureCall
//
// This function secures the call from any interruptions or interference
// that may affect the call's media stream.
//
LONG CServiceProvider::lineSecureCall(CTSPICallAppearance* pCall, DRV_REQUESTID dwReqId)
{
    return pCall->Secure(dwReqId);

}// CServiceProvider::lineSecureCall

//////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSendUserUserInfo
//
// This function sends user-to-user information to the remote party on the
// specified call.
//
LONG CServiceProvider::lineSendUserUserInfo(
CTSPICallAppearance* pCall,         // Call appearance
DRV_REQUESTID dwRequestID,          // Asynch req. id.
LPCSTR lpszUserUserInfo,            // User information
DWORD dwSize)                       // Size of above
{     
    // If the parameters are bad, don't continue.
    if (lpszUserUserInfo == NULL || dwSize == 0L)
        return LINEERR_OPERATIONFAILED;

    // Validate the UserUser information.
    CTSPILineConnection* pLine = pCall->GetLineOwner();
    if (pLine->GetLineDevCaps()->dwUUISendUserUserInfoSize < dwSize)
        return LINEERR_USERUSERINFOTOOBIG;

    // Allocate a new buffer for the UserUserInfo.
    LPSTR lpBuff = (LPSTR) AllocMem (dwSize);
    if (lpBuff == NULL)
        return LINEERR_NOMEM;

    // Copy the buffer over
    CopyBuffer (lpBuff, lpszUserUserInfo, dwSize);

    // Send the request to the call appearance - if it fals, free the memory
    // involved.
    LONG lResult = pCall->SendUserUserInfo(dwRequestID, lpBuff, dwSize);
    if (ReportError(lResult))
        FreeMem (lpBuff);
    return lResult;

}// CServiceProvider::lineSendUserUserInfo

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineOpen
//
// This method opens the specified line device based on the device
// id passed and returns a handle for the line.  The TAPI.DLL line
// handle must also be retained for further interaction with this
// device.
//
LONG CServiceProvider::lineOpen(
CTSPILineConnection* pLine,      // Specific line for above device
HTAPILINE htLine,                // TAPI opaque handle to use for line
LPHDRVLINE lphdLine,             // Return area for SP handle
DWORD dwTSPIVersion,             // Version of TSPI to synchronize to
LINEEVENT lpfnEventProc)         // Event procedure address for notifications
{
    // Store our service provider line handle to return.  This
    // is the handle which TAPI will use henceforth to talk to us about
    // this particular line.
    LONG lResult = pLine->Open(htLine, lpfnEventProc, dwTSPIVersion);
    if (ReportError (lResult) == FALSE)
        *lphdLine = (HDRVLINE) pLine;

    return lResult;

}// CServiceProvider::lineOpen

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineClose
//
// This method closes the specified open line after stopping all
// asynchronous requests on the line.
//
LONG CServiceProvider::lineClose(CTSPILineConnection* pLine)
{
    return pLine->Close();

}// CServiceProvider::lineClose

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineCloseCall
//
// This method deallocates a call after completing or aborting all
// outstanding asynchronous operations on the call.  The call handle
// is no longer valid after this call.
//
LONG CServiceProvider::lineCloseCall(CTSPICallAppearance* pCall)
{
    return pCall->Close();

}// CServiceProvider::lineCloseCall

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineDrop
//
// This method drops or disconnects the specified call.  User-to-User
// information can optionally be transmitted as part of the call
// disconnect.  This method may be called at any point in the conversation,
// and when this method returns, the call will be idle but still open.
//
LONG CServiceProvider::lineDrop(
CTSPICallAppearance* pCall,            // Call appearance
DRV_REQUESTID dwRequestId,             // Asynch. request id
LPCSTR lpszUserUserInfo,               // User-to-User information
DWORD dwSize)                          // size of the above user-to-user info
{
    // Allocate a new buffer for the UserUserInfo.
    LPSTR lpBuff = NULL;
    if (lpszUserUserInfo && dwSize > 0)
    {   
        CTSPILineConnection* pLine = pCall->GetLineOwner();
        // If we don't support this type of UserUserInfo, then simply
        // ignore it - many apps send it and don't expect a TOOBIG error.
        if (pLine->GetLineDevCaps()->dwUUIDropSize > 0)
        {
            // Validate the UserUser information.
            if (pLine->GetLineDevCaps()->dwUUIDropSize < dwSize)
                return LINEERR_USERUSERINFOTOOBIG;

            lpBuff = (LPSTR) AllocMem (dwSize);
            if (lpBuff == NULL)
                return LINEERR_NOMEM;

            // Copy the buffer over
            CopyBuffer (lpBuff, lpszUserUserInfo, dwSize);
        }
        else
            dwSize = 0L;            
    }
    else 
        dwSize = 0L;

    // Send the request to the call appearance - if it fals, free the memory
    // involved.
    LONG lResult = pCall->Drop (dwRequestId, lpBuff, dwSize);
    if (ReportError(lResult) && lpBuff)
        FreeMem (lpBuff);

    return lResult;

}// CServiceProvider::lineDrop

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineAnswer
//
// This method allows an offering state call to be answered.
//
LONG CServiceProvider::lineAnswer(
CTSPICallAppearance* pCall,            // Call appearance for anwering
DRV_REQUESTID dwReq,                   // Asynch. request id
LPCSTR lpszUserUserInfo,               // User/User info
DWORD dwSize)                          // Size of above information
{                              
    // Allocate a new buffer for the UserUserInfo.
    LPSTR lpBuff = NULL;
    if (lpszUserUserInfo && dwSize > 0)
    {                                    
        CTSPILineConnection* pLine = pCall->GetLineOwner();
        if (pLine->GetLineDevCaps()->dwUUIAnswerSize > 0)
        {
            // Validate the UserUser information.
            if (pLine->GetLineDevCaps()->dwUUIAnswerSize < dwSize)
                return LINEERR_USERUSERINFOTOOBIG;
                   
            lpBuff = (LPSTR) AllocMem (dwSize);
            if (lpBuff == NULL)
                return LINEERR_NOMEM;

            // Copy the buffer over
            CopyBuffer (lpBuff, lpszUserUserInfo, dwSize);
        }
        else 
            dwSize = 0L;            
    } 
    else
        dwSize = 0L;

    // Send the request to the call appearance - if it fals, free the memory involved.
    LONG lResult = pCall->Answer(dwReq, lpBuff, dwSize);
    if (ReportError(lResult) && lpBuff)
        FreeMem (lpBuff);

    return lResult;

}// CServiceProvider::lineAnswer

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineBlindTransfer
//
// This function performs a blind transfer of the specified call to
// the specified address.
//
LONG CServiceProvider::lineBlindTransfer(
CTSPICallAppearance* pCall,         // Call appearance to transfer
DRV_REQUESTID dwRequestId,          // Asynch. request id.
LPCTSTR lpszDestAddr,               // Destination to transfer to
DWORD dwCountryCode)                // Country code
{
    // If the destination address is not there, then error out.
    if (lpszDestAddr == NULL || *lpszDestAddr == _T('\0'))
        return LINEERR_INVALADDRESS;

    // Verify the destination address and move it into another buffer.
    CADObArray* pArray = new CADObArray;
    LONG lResult = CheckDialableNumber(pCall->GetLineOwner(), pCall->GetAddressOwner(),
                             lpszDestAddr, pArray, dwCountryCode);
    if (ReportError(lResult))
    {
        delete pArray;
        return lResult;
    }
                  
    // Pass it down, if the call appearance fails the transfer, then delete
    // the address buffer.   
    lResult = pCall->BlindTransfer (dwRequestId, pArray, dwCountryCode);
    if (ReportError (lResult))
        delete pArray;
    return lResult;

}// CServiceProvider::lineBlindTransfer

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineCompleteCall
//
// This method is used to specify how a call that could not be
// connected normally should be completed instead.  The network or
// switch may not be able to complete a call because the network
// resources are busy, or the remote station is busy or doesn't answer.
//
LONG CServiceProvider::lineCompleteCall(
CTSPICallAppearance* pCall,      // Call appearance to complete
DRV_REQUESTID dwRequestId,       // Asynch. request id   
LPDWORD lpdwCompletionID,        // Completion Id address
DWORD dwCompletionMode,          // Completion mode
DWORD dwMessageID)               // Message to send when completed
{   
    // Allocate a buffer for the COMPLETECALL structure
    TSPICOMPLETECALL* lpCompCall = new TSPICOMPLETECALL;
    if (lpCompCall == NULL)
        return LINEERR_NOMEM;
    
    // Fill in the structure                              
    lpCompCall->dwCompletionMode = dwCompletionMode;
    lpCompCall->dwMessageId = dwMessageID;
    lpCompCall->dwCompletionID = (DWORD) lpCompCall;
    lpCompCall->pCall = pCall;
    lpCompCall->dwSwitchInfo = 0L;
    lpCompCall->strSwitchInfo = "";

    // Pass it down to the call appearance to process
    LONG lResult = pCall->CompleteCall(dwRequestId, lpdwCompletionID, lpCompCall);
    if (ReportError (lResult))
        delete lpCompCall;
    return lResult;

}// CServiceProvider::lineCompleteCall

//////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineUncompleteCall
//
// This function is used to cancel the specified call completion request
// on the specified line.
//
LONG CServiceProvider::lineUncompleteCall(
CTSPILineConnection* pLine,         // Line connection
DRV_REQUESTID dwRequestID,          // Asynch. request id
DWORD dwCompletionID)               // Completion id to cancel
{
    // Pass it to the line object to complete.
    return pLine->UncompleteCall (dwRequestID, dwCompletionID);

}// CServiceProvider::lineUncompleteCall

//////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetupTransfer
//
// This function sets up a call for transfer to a destination address.
// A new call handle is created which represents the destination
// address.
//
LONG CServiceProvider::lineSetupTransfer(
CTSPICallAppearance *pCall,         // Call appearance to transfer
DRV_REQUESTID dwRequestID,          // Asynch. request id
HTAPICALL htConsultCall,            // Consultant call to create
LPHDRVCALL lphdConsultCall,         // Return handle for call to create   
LPLINECALLPARAMS const lpCallParams)   // Calling parameters
{
    // If there are call parameters, then force the derived class to
    // deal with them since there are device specific flags in the
    // set.
    LPLINECALLPARAMS lpMyCallParams = NULL;
    if (lpCallParams)
    {
        // Copy the call parameters into our own buffer.
        lpMyCallParams = (LPLINECALLPARAMS) AllocMem (lpCallParams->dwTotalSize);
        if (lpMyCallParams == NULL)
            return LINEERR_NOMEM;
        CopyBuffer (lpMyCallParams, lpCallParams, lpCallParams->dwTotalSize);
        
        // Process the call parameters
        LONG lResult = ProcessCallParameters(pCall, lpMyCallParams);
        if (lResult)
        {
            FreeMem ((LPTSTR)lpMyCallParams);
            return lResult;          
        }
    }

    // Allocate a transfer request to send down to the worker thread.
    TSPITRANSFER* lpTransfer = new TSPITRANSFER;
    if (lpTransfer == NULL)        
    {
        FreeMem ((LPTSTR)lpMyCallParams);
        return LINEERR_NOMEM;    
    }

    // Fill in what we know
    lpTransfer->lpCallParams = lpMyCallParams;
    lpTransfer->pCall = pCall;
    lpTransfer->pConf = NULL;
    lpTransfer->dwTransferMode = 0L;
    
    // Determine which address to use based on our CALLPARAMS.  If they indicate
    // a different address, then this is a cross-address transfer.
    CTSPILineConnection* pLine = pCall->GetLineOwner();
    CTSPIAddressInfo* pAddr = pCall->GetAddressOwner();
    if (lpMyCallParams)
    {
        if (lpMyCallParams->dwAddressMode == LINEADDRESSMODE_ADDRESSID)
        {
            pAddr = pLine->GetAddress(lpMyCallParams->dwAddressID);
            if (pAddr == NULL)
            {
                delete lpTransfer;
                return LINEERR_INVALCALLPARAMS;
            }
        }            
    }
    
    // Now tell the address to setup the transfer.
    LONG lResult = pAddr->SetupTransfer (dwRequestID, lpTransfer, htConsultCall, lphdConsultCall);
    if (ReportError(lResult))
        delete lpTransfer;
    return lResult;

}// CServiceProvider::lineSetupTransfer

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineCompleteTransfer
//
// This method completes the transfer of the specified call to the
// party connected in the consultation call.  If 'dwTransferMode' is
// LINETRANSFERMODE_CONFERENCE, the original call handle is changed
// to a conference call.  Otherwise, the service provider should send
// callstate messages change all the calls to idle.
//
LONG CServiceProvider::lineCompleteTransfer(
CTSPICallAppearance* pCall,         // Call appearance for this transfer
DRV_REQUESTID dwRequestId,          // Asynch, request id
CTSPICallAppearance* pConsult,      // Specifies the destination of xfer
HTAPICALL htConfCall,               // Conference call handle if needed.
LPHDRVCALL lphdConfCall,            // Return SP handle for conference
DWORD dwTransferMode)               // Transfer mode
{             
    // Allocate a transfer request to send down to the worker thread.
    TSPITRANSFER* lpTransfer = new TSPITRANSFER;
    if (lpTransfer == NULL)
        return LINEERR_NOMEM;

    // Fill in what we know
    lpTransfer->pCall = pCall;
    lpTransfer->pConsult = pConsult;
    lpTransfer->dwTransferMode = dwTransferMode;

    // Tell the address to complete the transfer
    CTSPIAddressInfo* pAddr = pCall->GetAddressOwner();
    LONG lResult = pAddr->CompleteTransfer (dwRequestId, lpTransfer, htConfCall, lphdConfCall);
    if (ReportError(lResult))
        delete lpTransfer;
    return lResult;

}// CServiceProvider::lineCompleteTransfer

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineDial
//
// This method dials the specified dialable number on the specified
// call appearance.
//
LONG CServiceProvider::lineDial(
CTSPICallAppearance* pCall,            // Call appearance for this dial
DRV_REQUESTID dwRequestId,             // Asynch. request id
LPCTSTR lpszDestAddress,               // Destination to be dialed
DWORD dwCountryCode)                   // Country code of the destination
{
    // If the destination address is not there, then error out.
    if (lpszDestAddress == NULL || *lpszDestAddress == _T('\0'))
        return LINEERR_INVALADDRESS;
    
    CADObArray* pArray = new CADObArray;
    LONG lResult = CheckDialableNumber(pCall->GetLineOwner(), pCall->GetAddressOwner(),
                             lpszDestAddress, pArray, dwCountryCode);
    if (ReportError(lResult))
    {
        delete pArray;
        return lResult;
    }
                  
    // Pass it down, if the call appearance fails the transfer, then delete
    // the address buffer.   
    lResult = pCall->Dial (dwRequestId, pArray, dwCountryCode);
    if (ReportError (lResult))
        delete pArray;
    return lResult;

}// CServiceProvider::lineDial

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineForward
//
// This function forwards calls destined for the specified address on
// the specified line, according to the specified forwarding instructions.
// When an origination address is forwarded, the incoming calls for that
// address are deflected to the other number by the switch.  This function
// provides a combination of forward and do-not-disturb features.  This
// function can also cancel specific forwarding currently in effect.
//
LONG CServiceProvider::lineForward(
CTSPILineConnection* pLine,            // Line Connection (for All)
CTSPIAddressInfo* pAddr,               // Address to forward to (NULL=all)
DRV_REQUESTID dwRequestId,             // Asynchronous request id
LPLINEFORWARDLIST const lpForwardList, // Forwarding instructions
DWORD dwNumRingsAnswer,                // Rings before "no answer"
HTAPICALL htConsultCall,               // New TAPI call handle if necessary
LPHDRVCALL lphdConsultCall,            // Our return call handle if needed
LPLINECALLPARAMS const lpCallParams)   // Used if creating a new call
{
    // Verify the call parameters if present.
    LPLINECALLPARAMS lpMyCallParams = NULL;
    if (lpCallParams)
    {
        // Copy the call parameters into our own buffer.
        lpMyCallParams = (LPLINECALLPARAMS) AllocMem (lpCallParams->dwTotalSize);
        if (lpMyCallParams == NULL)
            return LINEERR_NOMEM;
        CopyBuffer (lpMyCallParams, lpCallParams, lpCallParams->dwTotalSize);
        
        // Process the call parameters
        LONG lResult = ProcessCallParameters(pLine, lpMyCallParams);
        if (lResult)
        {
            FreeMem ((LPTSTR)lpMyCallParams);
            return lResult;          
        }
    }

    // Allocate a buffer for the forwarding information so we don't
    // lose the pointer during the asynch processing.  It is freed
    // automatically by the CTSPIRequest class.
    TSPILINEFORWARD* lpForwardInfo = new TSPILINEFORWARD;
    if (lpForwardInfo == NULL)
    {
        FreeMem ((LPTSTR)lpMyCallParams);
        return LINEERR_NOMEM;    
    }

    // Save off the call parameters
    LONG lResult;
    lpForwardInfo->lpCallParams = lpMyCallParams;    
    lpForwardInfo->pCall = NULL;
    *lphdConsultCall = NULL;
        
    // Copy and validate the forwarding information.
    if (lpForwardList && lpForwardList->dwNumEntries > 0)
    {   
        // Validate the forward list passed.
        if (lpForwardList->dwTotalSize < sizeof (LINEFORWARDLIST))
        {
            delete lpForwardInfo;
            return LINEERR_STRUCTURETOOSMALL;
        }
    
        // Allocate seperate line forward requests which we stick into a ptr
        // array.
        for (int iCount = 0; iCount < (int)lpForwardList->dwNumEntries; iCount++)
        {   
            TSPIFORWARDINFO* lpForward = new TSPIFORWARDINFO;
            if (lpForward == NULL)
            {
                delete lpForwardInfo;
                return LINEERR_NOMEM;
            }
            
            // Insert it into our array
            lpForwardInfo->arrForwardInfo.Add (lpForward);
            
            // Get the passed forwarding information
            LPLINEFORWARD lpLineForward = &lpForwardList->ForwardList[iCount];

            // Copy it over - validate each of the numbers (if available)
            lpForward->dwForwardMode = lpLineForward->dwForwardMode;
            lpForward->dwDestCountry = lpLineForward->dwDestCountryCode;
            
            // First the caller address - it will always be offset from the OWNING structure
            // which in this case is the forward list.
            if (lpLineForward->dwCallerAddressSize > 0 && lpLineForward->dwCallerAddressOffset > 0)
            {
				wchar_t* lpszBuff = (wchar_t*)((LPBYTE)lpForwardList + lpLineForward->dwCallerAddressOffset);
#ifndef UNICODE
				CString strAddress = ConvertWideToAnsi(lpszBuff);
#else
				CString strAddress = lpszBuff;
#endif
                lResult = CheckDialableNumber (pLine, pAddr, strAddress, &lpForward->arrCallerAddress, 0);
                if (ReportError(lResult))
                {   
                    delete lpForwardInfo;
                    return lResult;
                }
            }
            
            // Now the destination address
            if (lpLineForward->dwDestAddressSize > 0 && lpLineForward->dwDestAddressOffset > 0)
            {
                wchar_t* lpszBuff = (wchar_t*)((LPBYTE)lpForwardList + lpLineForward->dwDestAddressOffset);
#ifndef UNICODE
				CString strAddress = ConvertWideToAnsi(lpszBuff);
#else
				CString strAddress = lpszBuff;
#endif
                lResult = CheckDialableNumber (pLine, pAddr, strAddress, &lpForward->arrDestAddress, 0);
                if (ReportError(lResult))
                {   
                    delete lpForwardInfo;
                    return lResult;
                }
            }

			// Calculate the size used for this forwarding information object so
			// we don't have to do it everytime TAPI requests our configuration.
			// This size is what is needed in TAPI terms for the forwarding information.
			lpForward->dwTotalSize = sizeof(LINEFORWARD);
            for (int j = 0; j < lpForward->arrCallerAddress.GetSize(); j++)
            {
                DIALINFO* pDialInfo = (DIALINFO*) lpForward->arrCallerAddress[j];
                if (!pDialInfo->strNumber.IsEmpty())
                {
					int iLen = pDialInfo->strNumber.GetLength() + sizeof(TCHAR);
					if (j > 0)
						iLen += sizeof(TCHAR);
					while ((iLen%4) != 0)
						iLen++;
					lpForward->dwTotalSize += iLen;
                }
            }

            for (j = 0; j < lpForward->arrDestAddress.GetSize(); j++)
            {
                DIALINFO* pDialInfo = (DIALINFO*) lpForward->arrDestAddress[j];
                if (!pDialInfo->strNumber.IsEmpty())
                {
					int iLen = pDialInfo->strNumber.GetLength() + sizeof(TCHAR);
					if (j > 0)
						iLen += sizeof(TCHAR);
					while ((iLen%4) != 0)
						iLen++;
					lpForward->dwTotalSize += iLen;
                }
            }                
        }            
    }
    
    // Save off the number of rings before the call is considered a "no answer".        
    lpForwardInfo->dwNumRings = dwNumRingsAnswer;

    // Now tell the line to forward the address (or all of its addresses)
    lResult = pLine->Forward (dwRequestId, pAddr, lpForwardInfo, htConsultCall, lphdConsultCall);
    if (ReportError (lResult))
        delete lpForwardInfo;
    return lResult;

}// CServiceProvider::lineForward

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetCallAddressID
//
// This method retrieves the address ID for the specified call.
// The information returned from here will allow TAPI to complete
// the lineNewCalls function.
//
LONG CServiceProvider::lineGetCallAddressID(
CTSPICallAppearance* pCall,         // Call appearance
LPDWORD lpdwAddressId)              // Return address id (index)
{
    *lpdwAddressId = pCall->GetAddressOwner()->GetAddressID();
    return FALSE;
   
}// CServiceProvider::lineGetCallAddressID

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetAddressID
//
// This method returns the address ID associated with this line
// in the specified format.
//
LONG CServiceProvider::lineGetAddressID(
CTSPILineConnection* pLine,            // Connection information object
LPDWORD lpdwAddressId,                 // DWORD for return address ID
DWORD dwAddressMode,                   // Address mode in lpszAddress
LPCTSTR lpszAddress,                   // Address of the specified line
DWORD dwSize)                          // Size of the above string/buffer
{
    return pLine->GetAddressID (lpdwAddressId, dwAddressMode, lpszAddress, dwSize);

}// CServiceProvider::lineGetAddressID

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetExtensionID
//
// This function returns the extension ID that the service provider
// supports for the indicated line device.
//
LONG CServiceProvider::lineGetExtensionID(
CTSPILineConnection* pLine,            // Line connection object
DWORD dwTSPIVersion,                   // TSPI agreed version
LPLINEEXTENSIONID lpExtensionID)       // Extension information
{
    // The base class supports no direct extensions.
    lpExtensionID->dwExtensionID0 = 
    lpExtensionID->dwExtensionID1 =
    lpExtensionID->dwExtensionID2 =
    lpExtensionID->dwExtensionID3 = 0;
    return FALSE;

}// CServiceProvider::lineGetExtensionID

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineHold
//
// This method places the specified call on hold
//
LONG CServiceProvider::lineHold(
CTSPICallAppearance* pCall,            // Call appearance to put on hold
DRV_REQUESTID dwRequestID)             // Asynchronous request ID
{                    
    return pCall->Hold (dwRequestID);

}// CServiceProvider::lineHold

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineMakeCall
//
// This method places a call on the specified line to the specified
// destination address.  Optionally, the call parameters can be
// specified if anything but a default call setup is required.
//
LONG CServiceProvider::lineMakeCall(
CTSPILineConnection* pLine,            // Connection information object
DRV_REQUESTID dwRequestID,             // Asynchronous request ID
HTAPICALL htCall,                      // TAPI opaque call handle
LPHDRVCALL lphdCall,                   // return addr for TSPI call handle
LPCTSTR lpszDestAddr,                  // Address to call
DWORD dwCountryCode,                   // Country code (SP specific)
LPLINECALLPARAMS const lpCallParams)   // Optional call parameters
{
    // Validate the UserUser information.
    LPLINECALLPARAMS lpMyCallParams = NULL;
    if (lpCallParams)
    {                                    
        if (lpCallParams->dwUserUserInfoOffset > 0 &&
            pLine->GetLineDevCaps()->dwUUIMakeCallSize > 0 &&
            pLine->GetLineDevCaps()->dwUUIMakeCallSize < lpCallParams->dwUserUserInfoSize)
            return LINEERR_USERUSERINFOTOOBIG;

        // Copy the call parameters into our own buffer.
        lpMyCallParams = (LPLINECALLPARAMS) AllocMem (lpCallParams->dwTotalSize);
        if (lpMyCallParams == NULL)
            return LINEERR_NOMEM;
        CopyBuffer (lpMyCallParams, lpCallParams, lpCallParams->dwTotalSize);
    }
                  
    // Allocate an object to send down to our worker code.
    TSPIMAKECALL* lpMakeCall = new TSPIMAKECALL;
    if (lpMakeCall == NULL)
    {
        FreeMem ((LPTSTR)lpMyCallParams);
        return LINEERR_NOMEM;    
    }

    // Save off the country code and call parameters.
    lpMakeCall->dwCountryCode = dwCountryCode;
    lpMakeCall->lpCallParams = lpMyCallParams;
    
    // Verify the destination address if it exists.    
    LONG lResult = 0L;
    if (lpszDestAddr && *lpszDestAddr != _T('\0'))
    {
        lResult = CheckDialableNumber(pLine, NULL, lpszDestAddr, &lpMakeCall->arrAddresses, dwCountryCode);
        if (ReportError (lResult))
        {
            delete lpMakeCall;
            return lResult;
        }
    }
    
    // Verify the call parameters if present.
    if (lpCallParams)
    {
        lResult = ProcessCallParameters(pLine, lpMyCallParams);
        if (lResult)
        {
            delete lpMakeCall;
            return lResult;
        }
    }
                      
    // Pass it down, if the call appearance fails the transfer, then delete
    // the address buffer.   
    lResult = pLine->MakeCall (dwRequestID, htCall, lphdCall, lpMakeCall);
    if (ReportError (lResult))
        delete lpMakeCall;
    return lResult;

}// CServiceProvider::lineMakeCall

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::linePark
//
// This function parks the specified call according to the specified
// park mode.
//
LONG CServiceProvider::linePark(
CTSPICallAppearance* pCall, 
DRV_REQUESTID dwRequestID, 
DWORD dwParkMode, 
LPCTSTR lpszDirAddr, 
LPVARSTRING lpNonDirAddress)
{
    // Allocate a park structure which is associated with this request.
    TSPILINEPARK* lpPark = new TSPILINEPARK;
    if (lpPark == NULL)
        return LINEERR_NOMEM;

    // Save off the park information
    lpPark->dwParkMode = dwParkMode;
    lpPark->lpNonDirAddress = lpNonDirAddress;

    // If the directed address is supplied, then copy into an internal
    // buffer and verify the contents of it.
    if (dwParkMode == LINEPARKMODE_DIRECTED)
    {
        if (lpszDirAddr != NULL)
        {   
            // Validate the buffer
            LONG lResult = CheckDialableNumber(pCall->GetLineOwner(), pCall->GetAddressOwner(),
                                               lpszDirAddr, &lpPark->arrAddresses, 0);
            if (ReportError(lResult))
            {
                delete lpPark;
                return lResult;
            }
        }
        else
        {
            delete lpPark;
            return LINEERR_INVALADDRESS;
        }
    }

    // Pass it to the call appearance
    LONG lResult = pCall->Park (dwRequestID, lpPark);
    if (ReportError (lResult))
        delete lpPark;
    return lResult;

}// CServiceProvider::linePark

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineUnpark
//
// This function retrieves the call parked at the specified
// address and returns a call handle for it.
//
LONG CServiceProvider::lineUnpark(
CTSPIAddressInfo* pAddr,            // Address to unpark to
DRV_REQUESTID dwRequestID,          // Asynch req. id
HTAPICALL htCall,                   // New unparked call
LPHDRVCALL lphdCall,                // Return address for unparked call
LPCTSTR lpszDestAddr)               // Address where call is parked
{
    // If there is not a destination address, error out.
    if (lpszDestAddr == NULL || *lpszDestAddr == _T('\0'))
        return LINEERR_INVALADDRESS;
    
    CADObArray* pArray = new CADObArray;
    LONG lResult = CheckDialableNumber(pAddr->GetLineOwner(), pAddr, lpszDestAddr, pArray, 0);
    if (ReportError(lResult))
    {
        delete pArray;
        return lResult;
    }

    // Pass it to the address to unpark
    lResult = pAddr->Unpark (dwRequestID, htCall, lphdCall, pArray);
    if (ReportError(lResult))
        delete pArray;
    return lResult;

}// CServiceProvider::lineUnpark

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::linePickup
//
// This function picks up a call alerting at the specified destination
// address and returns a call handle for the picked up call.  If invoked
// with a NULL for the 'lpszDestAddr' parameter, a group pickup is performed.
// If required by the device capabilities, 'lpszGroupID' specifies the
// group ID to which the alerting station belongs.
//
LONG CServiceProvider::linePickup(
CTSPIAddressInfo* pAddr,         // Address to pickup call onto
DRV_REQUESTID dwRequestID,       // Asynch request id.   
HTAPICALL htCall,                // New call handle
LPHDRVCALL lphdCall,             // Return address for call handle
LPCTSTR lpszDestAddr,            // Destination address
LPCTSTR lpszGroupID)             // Group id
{
    // Allocate a buffer to hold the pickup request internally.
    TSPILINEPICKUP* lpPickup = new TSPILINEPICKUP;
    if (lpPickup == NULL)
        return LINEERR_NOMEM;

    // Save off the group id if this is a group pickup.
    lpPickup->strGroupID = lpszGroupID;

    // Check the dest address if it is there.
    if (lpszDestAddr && *lpszDestAddr != _T('\0'))
    {
        // Copy and validate the buffer
        LONG lResult = CheckDialableNumber(pAddr->GetLineOwner(), pAddr, lpszDestAddr,
                                           &lpPickup->arrAddresses, 0);
        if (ReportError(lResult))
        {
            delete lpPickup;
            return lResult; 
        }
    }

    // Pass on control to the address pickup
    LONG lResult = pAddr->Pickup (dwRequestID, htCall, lphdCall, lpPickup);
    if (ReportError (lResult))
        delete lpPickup;
    return lResult;

}// CServiceProvider::linePickup

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineRedirect
//
// This function redirects the specified offering call to the specified
// destination address.
//
LONG CServiceProvider::lineRedirect(
CTSPICallAppearance* pCall,            // Call appearance
DRV_REQUESTID dwRequestID,             // Asynch. request id
LPCTSTR lpszDestAddr,                  // Destination to direct to
DWORD dwCountryCode)                   // Country of destination
{
    // Copy and validate the desintation address.
    if (lpszDestAddr == NULL || *lpszDestAddr == _T('\0'))
        return LINEERR_INVALADDRESS;

    CADObArray* pArray = new CADObArray;
    LONG lResult = CheckDialableNumber(pCall->GetLineOwner(), pCall->GetAddressOwner(),
                                       lpszDestAddr, pArray, dwCountryCode);
    if (ReportError (lResult))
    {
        delete pArray;
        return lResult;
    }

    // Pass it to the call to redirect.
    lResult = pCall->Redirect (dwRequestID, pArray, dwCountryCode);
    if (ReportError(lResult))
        delete pArray;
    return lResult;

}// CServiceProvider::lineRedirect

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetCallParams
//
// This function sets certain parameters for an existing call.
//
LONG CServiceProvider::lineSetCallParams(
CTSPICallAppearance *pCall,      // Call appearance
DRV_REQUESTID dwRequestID,       // Asynch req. id.
DWORD dwBearerMode,              // New bearer mode
DWORD dwMinRate,                 // New data minimum rate
DWORD dwMaxRate,                 // New data maximum rate
LPLINEDIALPARAMS const lpDialParams)   // New dialing parameters
{
    // Allocate a CALLPARAMS structure
    TSPICALLPARAMS* lpSCP = new TSPICALLPARAMS;
    if (lpSCP == NULL)
        return LINEERR_NOMEM;

    // Fill it in from our passed parameters.
    lpSCP->dwBearerMode = dwBearerMode;
    lpSCP->dwMinRate = dwMinRate;
    lpSCP->dwMaxRate = dwMaxRate;
    if (lpDialParams)
        memcpy(&lpSCP->DialParams, lpDialParams, sizeof(LINEDIALPARAMS));

    // Pass the request to the call.
    LONG lResult = pCall->SetCallParams (dwRequestID, lpSCP);
    if (ReportError(lResult))
        delete lpSCP;
    return lResult;

}// CServiceProvider::lineSetCallParams

///////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetTerminal
//
// This operation enables TAPI.DLL to specify to which terminal information
// related to a specified line, address, or call is to be routed.  This
// can be used while calls are in progress on the line, to allow events
// to be routed to different devices as required.
//
LONG CServiceProvider::lineSetTerminal(
CTSPILineConnection* pLine, 
CTSPIAddressInfo* pAddr,
CTSPICallAppearance* pCall, 
DRV_REQUESTID dwRequestID, 
DWORD dwTerminalModes, 
DWORD dwTerminalID, 
BOOL bEnable)
{
    // Allocate a LINESETTERMINAL structure to pass with the request
    TSPILINESETTERMINAL* lpLine = new TSPILINESETTERMINAL;
    if (lpLine == NULL)
        return LINEERR_NOMEM;

    lpLine->pLine = pLine;
    lpLine->pAddress = pAddr;
    lpLine->pCall = pCall;
    lpLine->dwTerminalModes = dwTerminalModes;
    lpLine->dwTerminalID = dwTerminalID;
    lpLine->bEnable = bEnable;

    // Pass it to the lowest object.
    LONG lResult;
    if (pCall != NULL)
        lResult = pCall->SetTerminal (dwRequestID, lpLine);
    else if (pAddr != NULL)
        lResult = pAddr->SetTerminal (dwRequestID, lpLine);
    else
        lResult = pLine->SetTerminal (dwRequestID, lpLine);

    if (ReportError (lResult))
        delete lpLine;
    return lResult;

}// CServiceProvider::lineSetTerminal

//////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetAddressCaps
//
// This function is called by TAPI to retreive the capabilities of one
// of our lines.
//
LONG CServiceProvider::lineGetAddressCaps(
CTSPIAddressInfo* pAddr,               // Address to get capabilities for
DWORD dwTSPIVersion,                   // Version of SPI expected
DWORD dwExtVersion,                    // Driver specific version
LPLINEADDRESSCAPS lpAddressCaps)       // Address CAPS structure to fill in
{
    // Pass it onto our address object
    return pAddr->GatherCapabilities(dwTSPIVersion, dwExtVersion, lpAddressCaps);

}// CServiceProvider::lineGetAddressCaps

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetAddressStatus
//
// This method returns the status of the particular address on a line.
// 
LONG CServiceProvider::lineGetAddressStatus(
CTSPIAddressInfo* pAddr,               // Address to get status for
LPLINEADDRESSSTATUS lpAddressStatus)
{
    // Pass it onto our address object
    return pAddr->GatherStatusInformation (lpAddressStatus);

}// CServiceProvider::lineGetAddressStatus

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetCallInfo
//
// This method retrieves all the information about a particular call.
//
LONG CServiceProvider::lineGetCallInfo(
CTSPICallAppearance* pCall,         // Call appearance
LPLINECALLINFO lpCallInfo)          // Call information structure
{
    // Pass it onto the call appearance.
    return pCall->GatherCallInformation (lpCallInfo);

}// CServiceProvider::lineGetCallInfo

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetCallStatus
//
// This method returns the current status of the specified call
// appearance.
//
LONG CServiceProvider::lineGetCallStatus(
CTSPICallAppearance* pCall,      // Call appearance
LPLINECALLSTATUS lpCallStatus)   // Return buffer
{
    // Pass it onto the call appearance.
    return pCall->GatherStatusInformation(lpCallStatus);

}// CServiceProvider::lineGetCallStatus

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetDevCaps
//
// This method retrieves the line capabilities for this TAPI device.
//
LONG CServiceProvider::lineGetDevCaps(
CTSPILineConnection* pLine,      // Connection
DWORD dwTSPIVersion,             // TSP version expected   
DWORD dwExtVer,                  // Driver extensions version
LPLINEDEVCAPS lpLineCaps)        // Line capabilities
{
    // Pass it to the line device
    return pLine->GatherCapabilities (dwTSPIVersion, dwExtVer, lpLineCaps);

}// CServiceProvider::lineGetDevCaps

/////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetNumAddressIds
//
// This function is called to determine the number of addresses on
// this line.
//
LONG CServiceProvider::lineGetNumAddressIDs(
CTSPILineConnection* pLine, 
LPDWORD lpNumAddr)
{
   *lpNumAddr = pLine->GetAddressCount();
   return FALSE;

}// CServiceProvider::lineGetNumAddressIDs

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetLineDevStatus
//
// This method retrieves the features of the particular line.
//
LONG CServiceProvider::lineGetLineDevStatus(
CTSPILineConnection* pLine,         // Connection
LPLINEDEVSTATUS lpLineDevStatus)    // Return structure
{
    return pLine->GatherStatus (lpLineDevStatus);

}// CServiceProvider::lineGetLineDevStatus

/////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetID
//
// This method returns specific ID information about the specified
// line, address, or call.
// 
LONG CServiceProvider::lineGetID(
CTSPILineConnection* pLine,      // Connection (may be NULL)
CTSPIAddressInfo* pAddr,         // Address information (may be NULL)
CTSPICallAppearance* pCall,      // Particular call appearance (may be NULL)
CString& strDevClass,            // Device class to retrieve
LPVARSTRING lpDeviceID,          // Memory for return buffer
HANDLE hTargetProcess)			 // Process to dup handle for
{                
    // Pass it to the call if it exists (and the call takes it)
    if (pCall && pCall->GetID(strDevClass, lpDeviceID, hTargetProcess) == 0)
        return 0;
    
    // Try the address next.
    if (pAddr && pAddr->GetID(strDevClass, lpDeviceID, hTargetProcess) == 0)
        return 0;
        
    // Lastly, pass it to the line.
    return pLine->GetID (strDevClass, lpDeviceID, hTargetProcess);

}// CServiceProvider::lineGetID

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSwapHold
//
// This function swaps the specified active call with the specified
// call on hold.
//
LONG CServiceProvider::lineSwapHold(
CTSPICallAppearance* pCall, 
DRV_REQUESTID dwRequestID, 
CTSPICallAppearance* pHeldCall)
{
    // Tell the call appearance to swap the hold
    return pCall->SwapHold (dwRequestID, pHeldCall);

}// CServiceProvider::lineSwapHold

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineUnhold
//
// This function retrieves the specified call off hold
//
LONG CServiceProvider::lineUnhold(
CTSPICallAppearance* pCall,         // Call appearance
DRV_REQUESTID dwRequestID)          // Request Id
{  
    return pCall->Unhold (dwRequestID);    

}// CServiceProvider::lineUnhold

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGatherDigits
//
// Initiates the buffered gathering of digits on the specified call. 
// The application specifies a buffer in which to place the digits and the 
// maximum number of digits to be collected.
//
LONG CServiceProvider::lineGatherDigits(
CTSPICallAppearance* pCall,         // Call appearance to gather digits on
DWORD dwEndToEndID,                 // Unique identifier for this request
DWORD dwDigitModes,                 // LINEDIGITMODE_xxx 
LPWSTR lpszDigits,                  // Buffer for the call to place collected digits
DWORD dwNumDigits,                  // Number of digits before finished
LPCTSTR lpszTerminationDigits,      // List of digits to terminate upon
DWORD dwFirstDigitTimeout,          // mSec timeout for first digit
DWORD dwInterDigitTimeout)          // mSec timeout between any digits
{
    // Validate the numDigits field.
    if (dwNumDigits == 0L && lpszDigits != NULL)
        return LINEERR_INVALPARAM;
                  
    // Allocate our TSPIDIGITGATHER structure
    TSPIDIGITGATHER* lpGather = new TSPIDIGITGATHER;
    if (lpGather == NULL)
        return LINEERR_NOMEM;
    
    // Fill it in with our parameters.
    lpGather->dwEndToEndID = dwEndToEndID;
    lpGather->dwDigitModes = dwDigitModes;
    lpGather->lpBuffer = lpszDigits;
    lpGather->dwSize = dwNumDigits;
    lpGather->strTerminationDigits = lpszTerminationDigits;
    lpGather->dwFirstDigitTimeout = dwFirstDigitTimeout;
    lpGather->dwInterDigitTimeout = dwInterDigitTimeout;    
    
    // And call our call appearance.
    LONG lResult = pCall->GatherDigits (lpGather);
    if (ReportError (lResult))
        delete lpGather;
    return lResult;
            
}// CServiceProvider::lineGatherDigits

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGenerateDigits
//
// Initiates the generation of the specified digits on the specified 
// call as inband tones using the specified signaling mode. Invoking this 
// function with a NULL value for lpszDigits aborts any digit generation 
// currently in progress.  Invoking lineGenerateDigits or lineGenerateTone 
// while digit generation is in progress aborts the current digit generation 
// or tone generation and initiates the generation of the most recently 
// specified digits or tone. 
//
LONG CServiceProvider::lineGenerateDigits(
CTSPICallAppearance* pCall,             // Line connection to generate digits on
DWORD dwEndToEndID,                     // Identifier for this request
DWORD dwDigitMode,                      // LINEDIGITMODE_xxxx
LPCTSTR lpszDigits,                     // Digits to be generated
DWORD dwDuration)                       // mSec duration for digits and inter-digit generation.
{
    // Allocate a structure to pass to our worker code.
    TSPIGENERATE* lpGenerate = new TSPIGENERATE;
    if (lpGenerate == NULL)
        return LINEERR_NOMEM;
        
    // Fill in the structure
    lpGenerate->dwEndToEndID = dwEndToEndID;
    lpGenerate->dwMode = dwDigitMode;
    lpGenerate->dwDuration = dwDuration;
    lpGenerate->strDigits = lpszDigits;

    // Call down to the call appearance to actually process this request.
    LONG lResult = pCall->GenerateDigits (lpGenerate);
    if (ReportError(lResult))
        delete lpGenerate;
    return lResult;

}// CServiceProvider::lineGenerateDigits

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGenerateTone
//
// Generates the specified inband tone over the specified call. Invoking 
// this function with a zero for dwToneMode aborts the tone generation 
// currently in progress on the specified call. Invoking lineGenerateTone or
// lineGenerateDigits while tone generation is in progress aborts the current 
// tone generation or digit generation and initiates the generation of 
// the newly specified tone or digits.
//
LONG CServiceProvider::lineGenerateTone(
CTSPICallAppearance* pCall,             // Call appearance to generate tones on
DWORD dwEndToEndID,                     // Unique identifier for this request
DWORD dwToneMode,                       // Tone mode
DWORD dwDuration,                       // Duration
DWORD dwNumTones,                       // Number of custom tone entries in below array
LPLINEGENERATETONE lpTones)             // Array of tones
{
    // Allocate a structure to pass to our worker code.
    TSPIGENERATE* lpGenerate = new TSPIGENERATE;
    if (lpGenerate == NULL)
        return LINEERR_NOMEM;
        
    // Fill in the structure
    lpGenerate->dwEndToEndID = dwEndToEndID;
    lpGenerate->dwMode = dwToneMode;
    lpGenerate->dwDuration = dwDuration;

    // If there are any custom tone blocks, copy them over in our
    // array.
    if (dwNumTones > 0)
    {   
        for (int i = 0; i < (int)dwNumTones; i++)
        {
            LPLINEGENERATETONE lpMyTone = new LINEGENERATETONE;
            if (lpMyTone == NULL)
            {
                delete lpGenerate;
                return LINEERR_NOMEM;
            }
            
            CopyBuffer (lpMyTone, lpTones, sizeof(LINEGENERATETONE));
            lpGenerate->arrTones.Add (lpMyTone);
            lpTones++;
        }
    }

    // Call down to the call appearance to actually process this request.
    LONG lResult = pCall->GenerateTone (lpGenerate);
    if (ReportError(lResult))
        delete lpGenerate;
    return lResult;
                                      
}// CServiceProvider::lineGenerateTone

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineMonitorDigits
//
// Enables and disables the unbuffered detection of digits received on the 
// call. Each time a digit of the specified digit mode(s) is detected, a 
// message is sent to the application indicating which digit has been detected.
//
LONG CServiceProvider::lineMonitorDigits(
CTSPICallAppearance* pCall, 
DWORD dwDigitModes) 
{
    return pCall->MonitorDigits(dwDigitModes);
    
}// CServiceProvider::lineMonitorDigits

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineMonitorMedia
//
// Enables and disables the detection of media modes on the specified call. 
// When a media mode is detected, a message is sent to the application.
//
LONG CServiceProvider::lineMonitorMedia(
CTSPICallAppearance* pCall, 
DWORD dwMediaModes) 
{
    return pCall->MonitorMedia (dwMediaModes);
    
}// CServiceProvider::lineMonitorMedia

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineMonitorTones
//
// Enables and disables the detection of inband tones on the call. Each 
// time a specified tone is detected, a message is sent to the application. 
//
LONG CServiceProvider::lineMonitorTones(
CTSPICallAppearance* pCall, 
DWORD dwToneListID, 
LPLINEMONITORTONE const lpToneList, 
DWORD dwNumEntries) 
{   
    // Verify the parameter values.                         
    if (lpToneList && dwNumEntries == 0L)
        return LINEERR_INVALPARAM;

    // Allocate a new TONEMONITOR structure.
    TSPITONEMONITOR* lpMon = new TSPITONEMONITOR;
    if (lpMon == NULL)
        return LINEERR_NOMEM;
    
    // Copy additional information.
    lpMon->dwToneListID = dwToneListID;                                 
                                         
    // Copy over all the tone entries    
    for (int i = 0; i < (int) dwNumEntries; i++)
    {
        LPLINEMONITORTONE lpTone = new LINEMONITORTONE;
        if (lpTone == NULL)
        {
            delete lpMon;
            return LINEERR_NOMEM;
        }          
        
        CopyBuffer(lpTone, lpToneList, sizeof(LINEMONITORTONE));
        lpMon->arrTones.Add (lpTone);
    }        

    // And pass it to the call appearance.        
    LONG lResult = pCall->MonitorTones (lpMon);
    if (ReportError (lResult))
        delete lpMon;
    return lResult;
    
}// CServiceProvider::lineMonitorTones

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetCurrentLocation
//
// This function is called by TAPI whenever the address translation location
// is changed by the user (in the Dial Helper dialog or 
// 'lineSetCurrentLocation' function.  SPs which store parameters specific
// to a location (e.g. touch-tone sequences specific to invoke a particular
// PBX function) would use the location to select the set of parameters 
// applicable to the new location.
//
// Added for TAPI 1.4
// 
LONG CServiceProvider::lineSetCurrentLocation(DWORD dwLocation)
{                    
	// If we cannot support this function, then return an error.
	if (GetSupportedVersion() < TAPIVER_14)
		return LINEERR_OPERATIONUNAVAIL;

    m_dwCurrentLocation = dwLocation;
    return 0L;
    
}// CServiceProvider::lineSetCurrentLocation

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetMediaControl
//
// This function enables and disables control actions on the media stream 
// associated with the specified line, address, or call. Media control 
// actions can be triggered by the detection of specified digits, media modes, 
// custom tones, and call states.  The new specified media controls replace 
// all the ones that were in effect for this line, address, or call prior 
// to this request.
//
LONG CServiceProvider::lineSetMediaControl(
CTSPILineConnection *pLine,                         // Line connection
CTSPIAddressInfo* pAddr,                            // Address (may be NULL)
CTSPICallAppearance* pCall,                         // Call (may be NULL)
LPLINEMEDIACONTROLDIGIT const lpDigitListIn,        // Digits to trigger actions
DWORD dwNumDigitEntries,                            // Count of digits
LPLINEMEDIACONTROLMEDIA const lpMediaListIn,        // Media modes to be monitored    
DWORD dwNumMediaEntries,                            // Count of media modes
LPLINEMEDIACONTROLTONE const lpToneListIn,          // Tones to be monitored    
DWORD dwNumToneEntries,                             // Count of tone
LPLINEMEDIACONTROLCALLSTATE const lpCallStateListIn, // Callstates to be monitored
DWORD dwNumCallStateEntries)                        // Count of call states
{                                       
	// Make sure we can perform this action now.
	if ((pLine->GetLineDevStatus()->dwLineFeatures & LINEFEATURE_SETMEDIACONTROL) == 0 ||
		(pAddr && pAddr->GetAddressStatus()->dwAddressFeatures & LINEADDRFEATURE_SETMEDIACONTROL) == 0 ||
		(pCall && pCall->GetCallStatus()->dwCallFeatures & LINECALLFEATURE_SETMEDIACONTROL) == 0)
		return LINEERR_OPERATIONUNAVAIL;
	
    // Allocate a media control structure
    TSPIMEDIACONTROL* lpMediaControl = new TSPIMEDIACONTROL;
    if (lpMediaControl)
        return LINEERR_NOMEM;
    
    LPLINEMEDIACONTROLDIGIT lpDigitList = (LPLINEMEDIACONTROLDIGIT) lpDigitListIn;
    LPLINEMEDIACONTROLMEDIA lpMediaList = (LPLINEMEDIACONTROLMEDIA) lpMediaListIn;
    LPLINEMEDIACONTROLTONE lpToneList = (LPLINEMEDIACONTROLTONE) lpToneListIn;
    LPLINEMEDIACONTROLCALLSTATE lpCallStateList = (LPLINEMEDIACONTROLCALLSTATE) lpCallStateListIn;
        
    // Run through each array and copy over the values.
    for (int i = 0; i < (int) dwNumDigitEntries; i++)
    {
        LPLINEMEDIACONTROLDIGIT lpDigit = new LINEMEDIACONTROLDIGIT;
        if (lpDigit == NULL)
        {
            delete lpMediaControl;
            return LINEERR_NOMEM;
        }                                                                
        
        CopyBuffer (lpDigit, lpDigitList, sizeof(LINEMEDIACONTROLDIGIT));
        lpMediaControl->arrDigits.Add (lpDigit);
        lpDigitList++;
    }    

    for (i = 0; i < (int) dwNumMediaEntries; i++)
    {
        LPLINEMEDIACONTROLMEDIA lpMedia = new LINEMEDIACONTROLMEDIA;
        if (lpMedia == NULL)
        {
            delete lpMediaControl;
            return LINEERR_NOMEM;
        }                        
        
        CopyBuffer (lpMedia, lpMediaList, sizeof (LINEMEDIACONTROLMEDIA));
        lpMediaControl->arrMedia.Add (lpMedia);
        lpMediaList++;
    }
    
    for (i = 0; i < (int) dwNumToneEntries; i++)
    {
        LPLINEMEDIACONTROLTONE lpTone = new LINEMEDIACONTROLTONE;
        if (lpTone == NULL)
        {
            delete lpMediaControl;
            return LINEERR_NOMEM;
        }
        
        CopyBuffer (lpTone, lpToneList, sizeof(LINEMEDIACONTROLTONE));
        lpMediaControl->arrTones.Add (lpTone);
        lpToneList++;
    }        
    
    for (i = 0; i < (int) dwNumCallStateEntries; i++)
    {
        LPLINEMEDIACONTROLCALLSTATE lpCallState = new LINEMEDIACONTROLCALLSTATE;        
        if (lpCallState == NULL)
        {
            delete lpMediaControl;
            return LINEERR_NOMEM;
        }                        
        
        CopyBuffer (lpCallState, lpCallStateList, sizeof(LINEMEDIACONTROLCALLSTATE));
        lpMediaControl->arrCallStates.Add (lpCallState);
        lpCallStateList++;
    }
    
    // Validate the parameters in the media control list
    LONG lResult = pLine->ValidateMediaControlList (lpMediaControl);
    if (ReportError(lResult))
    {
        delete lpMediaControl;
        return lResult;
    }

    // Submit an asynch request on behalf off the object.
	CTSPIRequest* pRequest = NULL;
    if (pCall)
		pRequest = pCall->AddAsynchRequest(REQUEST_MEDIACONTROL, 0, lpMediaControl);
    else if (pAddr)
        pRequest = pAddr->AddAsynchRequest(REQUEST_MEDIACONTROL, 0, lpMediaControl);
    else
        pRequest = pLine->AddAsynchRequest(NULL, NULL, REQUEST_MEDIACONTROL, 0, lpMediaControl);

	// If the add was successful, wait on the request.
	if (pRequest != NULL)
	{
		LONG lResult = pRequest->WaitForCompletion(INFINITE);
		if (lResult != 0)
			delete lpMediaControl;
	}
	else
		delete lpMediaControl;

    return lResult;    
    
}// CServiceProvider::lineSetMediaControl

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetDefaultMediaDetection
//
// This procedure tells the Service Provider the new set of Media Modes to 
// detect for the indicated line (replacing any previous set). It also sets 
// the initial set of Media Modes that should be monitored for on subsequent 
// calls (inbound or outbound) on this line. 
//
LONG CServiceProvider::lineSetDefaultMediaDetection(
CTSPILineConnection* pLine, 
DWORD dwMediaModes)
{
    return pLine->SetDefaultMediaDetection (dwMediaModes);        

}// CServiceProvider::lineSetDefaultMediaDetection

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetMediaMode
//
// This function changes the call's media as stored in the call's 
// LINECALLINFO structure.
//
LONG CServiceProvider::lineSetMediaMode(
CTSPICallAppearance* pCall,                 // Call appearance
DWORD dwMediaMode)                          // LINEMEDIAMODE_xxxx
{
    return pCall->SetMediaMode (dwMediaMode);

}// CServiceProvider::lineSetMediaMode

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetStatusMessages
//
// This operation enables the TAPI DLL to specify which notification 
// messages the Service Provider should generate for events related to 
// status changes for the specified line or any of its addresses.
//
LONG CServiceProvider::lineSetStatusMessages(
CTSPILineConnection* pLine,             // Line connection
DWORD dwLineStates,                     // LINESTATE notifications required
DWORD dwAddressStates)                  // ADDRESSSTATE notifications required
{
    return pLine->SetStatusMessages (dwLineStates, dwAddressStates);
    
}// CServiceProvider::lineSetStatusMessages

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineConfigDialog
//
// This function causes the service provider of the line device to
// display a modal dialog as a child of the specified owner.  The dialog
// should allow the user to configure parameters related to the line device.
//
LONG CServiceProvider::lineConfigDialog(
DWORD /*dwDeviceID*/,					// Device ID
CWnd* /*pwndOwner*/,                    // Application window owner
CString& /*strDeviceClass*/,            // Specific subscreen to display (device)
TUISPIDLLCALLBACK /*lpfnDLLCallback*/)	// DLL instance	
{
	return LINEERR_OPERATIONUNAVAIL;
	
}// CServiceProvider::lineConfigDialog

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineConfigDialogEdit
//
// This function causes the service provider to display a modal dialog as 
// a child window of the specified parent.  The dialog should allow the 
// user to configure parameters related to the line device.  The parameters
// for the configuration are passed in and passed back once the dialog is
// dismissed.
//
LONG CServiceProvider::lineConfigDialogEdit(
DWORD /*dwDeviceID*/,					// Device ID
CWnd* /*pwndOwner*/,                    // Application window owner
CString& /*strDeviceClass*/,            // Specific subscreen to display (device)
LPVOID const /*lpDeviceConfigIn*/,      // Returned from lineGetDevConfig
DWORD /*dwSize*/,                       // Size of above
LPVARSTRING /*lpDeviceConfigOut*/,      // Device configuration being returned.
TUISPIDLLCALLBACK /*lpfnDLLCallback*/)	// DLL instance	
{                    
	return LINEERR_OPERATIONUNAVAIL;
    
}// CServiceProvider::lineConfigDialogEdit

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetIcon
//
// This function retrieves a service line device-specific icon for display
// in user-interface dialogs.
//
LONG CServiceProvider::lineGetIcon(
CTSPILineConnection* pLine,                 // Line connection 
CString& strDevClass,                       // Specific device type (data/modem, etc)
LPHICON lphIcon)                            // Return handle
{                   
    *lphIcon = NULL;                
    return pLine->GetIcon (strDevClass, lphIcon);
        
}// CServiceProvider::lineGetIcon

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineDevSpecific
//
// Device specific extensions to the service provider.
//
LONG CServiceProvider::lineDevSpecific(
CTSPILineConnection* pLine,                 // Line connection
CTSPIAddressInfo* pAddr,                    // Address on line    
CTSPICallAppearance* pCall,                 // Call appearance on address
DRV_REQUESTID dwRequestId,                  // Asynch. request id.
LPVOID lpParams,                            // Parameters (device specific)
DWORD dwSize)                               // Size of parameters
{
    return LINEERR_OPERATIONUNAVAIL;
    
}// CServiceProvider::lineDevSpecific

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineDevSpecificFeature
//
// Device specific extensions to the service provider.
//
LONG CServiceProvider::lineDevSpecificFeature(
CTSPILineConnection* pLine,                 // Line connection
DWORD dwFeature,                            // Feature being invoked.
DRV_REQUESTID dwRequestId,                  // Asynch. request id
LPVOID lpParams,                            // Parameters (feature specific)
DWORD dwSize)                               // Size of parameters.
{
    return pLine->DevSpecificFeature (dwFeature, dwRequestId, lpParams, dwSize);
    
}// CServiceProvider::lineDevSpecificFeature

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineGetDevConfig
//
// Returns an "opaque" data structure object the contents of which are 
// specific to the line (service provider) and device class. The data 
// structure object stores the current configuration of a media-stream 
// device associated with the line device.
//
LONG CServiceProvider::lineGetDevConfig(
CTSPILineConnection* pLine,                 // Line connection
CString& strDeviceClass,                    // Device class to query
LPVARSTRING lpDeviceConfig)                 // Return result
{
    return pLine->GetDevConfig (strDeviceClass, lpDeviceConfig);
    
}// CServiceProvider::lineGetDevConfig

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetDevConfig
//
// Allows the application to restore the configuration of a media 
// stream device on a line device to a setup previously obtained 
// using lineGetDevConfig.
//
LONG CServiceProvider::lineSetDevConfig(
CTSPILineConnection* pLine,                 // Line connection
LPVOID const lpDevConfig,                   // Configuration (from lineGetDevConfig)
DWORD dwSize,                               // Size of configuration
CString& strDevClass)                       // Device class to set.
{ 
    return pLine->SetDevConfig (strDevClass, lpDevConfig, dwSize);
        
}// CServiceProvider::lineSetDevConfig

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineConditionalMediaDetection
//
// This method is invoked by TAPI.DLL when the application requests a
// line open using the LINEMAPPER.  This method will check the 
// requested media modes and return an acknowledgement based on whether 
// we can monitor all the requested modes.
//
LONG CServiceProvider::lineConditionalMediaDetection(
CTSPILineConnection* pLine,                     // Line connection
DWORD dwMediaModes,                             // LINEMEDIAMODE_xxxx (modes of interest)
LPLINECALLPARAMS const lpCallParams)            // Line parameters we need to be able to support
{
    // Pass it onto the line object
    return pLine->ConditionalMediaDetection(dwMediaModes, lpCallParams);

}// CServiceProvider::lineConditionalMediaDetection

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineReleaseUserUserInfo
//
// This method is provided to release a block of UserUser information
// for a call appearance.
//
LONG CServiceProvider::lineReleaseUserUserInfo(
CTSPICallAppearance* pCall,                 // Call appearance
DRV_REQUESTID dwRequestId)                  // Asynch. request id.
{                    
    return pCall->ReleaseUserUserInfo (dwRequestId);
    
}// CServiceProvider::lineReleaseUserUserInfo

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetCallData
//
// Set calldata into the calls CALLINFO record.
//
LONG CServiceProvider::lineSetCallData(
CTSPICallAppearance* pCall,                 // Call appearance
DRV_REQUESTID dwRequestId,                  // Asynch. request id.
LPVOID lpCallData,							// Call data buffer
DWORD dwSize)								// Size of buffer
{
	// Copy the data buffer into local storage.
	TSPICALLDATA* pCallData = new TSPICALLDATA;
	pCallData->dwSize = dwSize;
	if (dwSize > 0)
	{
		pCallData->lpvCallData = (LPVOID) AllocMem(dwSize);
		if (pCallData->lpvCallData == NULL)
		{
			delete pCallData;
			return LINEERR_NOMEM;
		}
		CopyBuffer((LPTSTR)pCallData->lpvCallData, lpCallData, dwSize);
	}
	else
		pCallData->lpvCallData = NULL;

	// Pass down to CALL level - delete if error.
	LONG lResult = pCall->SetCallData (dwRequestId, pCallData);
	if (ReportError(lResult))
		delete pCallData;
	return lResult;

}// CServiceProvider::lineSetCallData

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetQualityOfService
//
// Negotiates a new QOS for the call.
//
LONG CServiceProvider::lineSetQualityOfService(
CTSPICallAppearance* pCall,                 // Call appearance
DRV_REQUESTID dwRequestId,                  // Asynch. request id.
LPVOID lpSendingFlowSpec,					// Sending WinSock FLOWSPEC
DWORD dwSendingFlowSpecSize,				// Size
LPVOID lpReceivingFlowSpec,					// Recieving WinSock FLOWSPEC
DWORD dwReceivingFlowSpecSize)				// Size
{
	// Copy the Win32 FLOWSPEC buffers to local storage.
	TSPIQOS* pQOS = new TSPIQOS;
	
	pQOS->dwSendingSize = dwSendingFlowSpecSize;
	pQOS->dwReceivingSize = dwReceivingFlowSpecSize;

	if (dwSendingFlowSpecSize > 0)
	{
		pQOS->lpvSendingFlowSpec = (LPVOID) AllocMem (dwSendingFlowSpecSize);
		if (pQOS->lpvSendingFlowSpec == NULL)
		{
			pQOS->lpvReceivingFlowSpec = NULL;
			delete pQOS;
			return LINEERR_NOMEM;
		}
		CopyBuffer((LPTSTR)pQOS->lpvSendingFlowSpec, lpSendingFlowSpec, dwSendingFlowSpecSize);
	}
	else
		pQOS->lpvSendingFlowSpec = NULL;

	if (dwReceivingFlowSpecSize > 0)
	{
		pQOS->lpvReceivingFlowSpec = (LPVOID) AllocMem (dwReceivingFlowSpecSize);
		if (pQOS->lpvReceivingFlowSpec == NULL)
		{
			delete pQOS;
			return LINEERR_NOMEM;
		}
		CopyBuffer((LPTSTR)pQOS->lpvReceivingFlowSpec, lpReceivingFlowSpec, dwReceivingFlowSpecSize);
	}
	else
		pQOS->lpvReceivingFlowSpec = NULL;

	// Pass it down to the CALL layer.
	LONG lResult = pCall->SetQualityOfService(dwRequestId, pQOS);
	if (ReportError(lResult))
		delete pQOS;
	return lResult;

}// CServiceProvider::lineSetQualityOfService

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetCallTreatment
//
// The specified call treatment value is stored off in the
// LINECALLINFO record and TAPI is notified of the change.  If
// the call is currently in a state where the call treatment is
// relevent, then it goes into effect immediately.  Otherwise,
// the treatment will take effect the next time the call enters a
// relevent state.
//
LONG CServiceProvider::lineSetCallTreatment (CTSPICallAppearance* pCall, 
					DRV_REQUESTID dwRequestID, DWORD dwCallTreatment)
{
	return pCall->SetCallTreatment (dwRequestID, dwCallTreatment);

}// CServiceProvider::lineSetCallTreatment

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::lineSetLineDevStatus
//
// The service provider will set the device status as indicated,
// sending the appropriate LINE_LINEDEVSTATE messages to indicate
// the new status.
//
LONG CServiceProvider::lineSetLineDevStatus (CTSPILineConnection* pLine,
					DRV_REQUESTID dwRequestID, DWORD dwStatusToChange,
					DWORD fStatus)
{
	return pLine->SetLineDevStatus (dwRequestID, dwStatusToChange,
					(fStatus != 0) ? TRUE : FALSE);

}// CServiceProvider::lineSetLineDevStatus
