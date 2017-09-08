/******************************************************************************/
//                                                                        
// REQUEST.CPP - Source file for the TSPIRequest base class               
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code to manage the asynchronous request     
// objects which are dynamically allocated for each TSP request generated 
// by TAPI.                                                               
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
// Dynamic object creation support

IMPLEMENT_DYNCREATE( CTSPIRequest, CObject )

///////////////////////////////////////////////////////////////////////////
// Memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::CTSPIRequest
//
// Constructor for the CTSPIRequest object
//
CTSPIRequest::CTSPIRequest() : 
	m_evtComplete (FALSE, TRUE), m_wReqType(0), m_dwRequestId(0),
	m_lpData(NULL), m_dwSize(0), m_pConnOwner(NULL), m_iReqState(STATE_UNINITIALIZED),
	m_pCall(NULL), m_fResponseSent(FALSE), m_lResult(-1L), m_dwAppData(0)
{
	m_evtComplete.ResetEvent();

}// CTSPIRequest::CTSPIRequest()

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::Init
//
// Initialize a CTSPIRequest object
//
VOID CTSPIRequest::Init(
CTSPIConnection* pConn,     // Connection for this request
CTSPIAddressInfo* pAddr,    // Address line for this request
CTSPICallAppearance* pCall, // Call appearance
WORD wReqType,              // Request type (REQTYPE_xxx)
DRV_REQUESTID dwReqId,      // Asynch request id from TAPI
LPVOID lpData,              // Optional data
DWORD dwDataSize)           // Optional size
{
    m_pConnOwner = pConn;
    m_pAddress = pAddr;
    m_pCall = pCall;
    m_wReqType = wReqType;
    m_dwRequestId = dwReqId;
    m_lpData = lpData;
    m_dwSize = dwDataSize; 

	// Increment the CALL object reference count indicating that we have 
	// requests associated with it.
	if (m_pCall != NULL)
		m_pCall->IncRefCount();

}// TSPIRequest::Init

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::~CTSPIRequest
//
// Destructor for the request class
//
// WARNING: IF YOU ALLOCATE MEMORY FOR YOURSELF, YOU ARE REQUIRED
// TO FREE IT YOURSELF WITHIN THE DESTRUCTOR!!!
//
CTSPIRequest::~CTSPIRequest()
{
	if (m_pCall != NULL)
		m_pCall->DecRefCount();

	// Make sure threads are never blocked indefinately.
	m_evtComplete.SetEvent();

	// If it is an object, then it MUST be derived from CObject
    switch (m_wReqType)
    {
        case REQUEST_FORWARD:
        case REQUEST_SETCALLPARAMS:
        case REQUEST_PARK:
        case REQUEST_PICKUP:
        case REQUEST_COMPLETEXFER:
        case REQUEST_SETUPXFER:
        case REQUEST_ADDCONF:
        case REQUEST_PREPAREADDCONF:
        case REQUEST_REMOVEFROMCONF:
        case REQUEST_SETUPCONF:
        case REQUEST_COMPLETECALL:
        case REQUEST_SETTERMINAL:
        case REQUEST_SETDISPLAY:
        case REQUEST_MAKECALL:
        case REQUEST_GENERATEDIGITS:
        case REQUEST_GENERATETONE:
        case REQUEST_MEDIACONTROL:
        case REQUEST_SETPHONEDATA:
        case REQUEST_GETPHONEDATA:
        case REQUEST_SETBUTTONINFO:
        case REQUEST_SETRING:
        case REQUEST_SETHOOKSWITCHVOL:
        case REQUEST_SETHOOKSWITCHGAIN:
        case REQUEST_SETHOOKSWITCH:
        case REQUEST_BLINDXFER:
		case REQUEST_SETCALLDATA:
		case REQUEST_SETQOS:
        case REQUEST_DIAL:
        case REQUEST_UNPARK:
        case REQUEST_REDIRECT:
			{
				CObject* pObj = (CObject*)m_lpData;
				if (pObj->IsKindOf(RUNTIME_CLASS(CObject)) == FALSE)
				{
					DTRACE(TRC_MIN, _T("Warning: Request object had invalid data buffer - supposed to be object!\r\n"));
					FreeMem (m_lpData);
				}
				else
					delete pObj;
			}
			break;

        case REQUEST_ACCEPT:
        case REQUEST_SENDUSERINFO:
        case REQUEST_DROPCALL:
        case REQUEST_ANSWER:
			FreeMem (m_lpData);
			break;

		default:
#ifdef _DEBUG
			if (m_wReqType >= REQUEST_END && m_lpData != NULL)
				DTRACE(TRC_MIN, _T("User-defined request - make sure to delete m_lpData in destructor\r\n"));
#endif
			break;
	}

}// CTSPIRequest::~CTSPIRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::IsAddressOk
//
// Return result as to whether the passed address is still valid.
//
BOOL CTSPIRequest::IsAddressOk(LPVOID lpBuff, DWORD dwSize)
{
    return (BOOL) (IsBadReadPtr(lpBuff, (UINT)dwSize) == 0);

}// CTSPIRequest::IsAddressOk

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::Complete
//
// This causes the request to be completed.  It is invoked by 
// the device when the request is finished.
//
VOID CTSPIRequest::Complete (LONG lResult, BOOL fSendTapiNotification)
{
#ifdef _DEBUG
    DTRACE (TRC_MIN, _T("Request <0x%lx %s> completing with RC=0x%lx\r\n"), (DWORD)this, GetRequestTypeName(), lResult);
#endif

    // Mark this as completed.
    m_fResponseSent = fSendTapiNotification;
    m_lResult = lResult;

    // Tell the connection, address, and call that the request completed.
    m_pConnOwner->OnRequestComplete (this, lResult);
    if (m_pAddress != NULL)
        m_pAddress->OnRequestComplete (this, lResult);
    if (m_pCall != NULL)  
        m_pCall->OnRequestComplete (this, lResult);

    // If we have waiting threads, then unblock them and give them
    // a chance to run.
	m_evtComplete.SetEvent ();
	Sleep(0);

}// CTSPIRequest::Complete

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::WaitForCompletion
//
// This function pauses the current thread waiting for this request
// to complete.
//
// WARNING: DO NOT LET THE INPUT THREAD GET PAUSES OR THE SERVICE PROVIDER
// WILL SHUTDOWN THE SYSTEM!
//
LONG CTSPIRequest::WaitForCompletion (DWORD dwMsec)
{   
    // Pause this thread until the event is signalled.
	DTRACE(TRC_MIN, _T("Pausing thread 0x%lx on request 0x%lx\r\n"), GetCurrentThreadId(), (DWORD)this);
	if (WaitForSingleObject (m_evtComplete, dwMsec) != WAIT_TIMEOUT) 
		return m_lResult;
	return -1L;
        
}// CTSPIRequest::WaitForCompletion

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::Dump
//
// Debug method to dump out the TSPI Request block
//
void CTSPIRequest::Dump(CDumpContext& /*de*/) const
{
	DTRACE(TRC_STRUCT, _T("CTSPIRequest: 0x%lx %s Conn:0x%lx, Call:0x%lx, State:%d, Data=0x%lx, TAPIComplete=%s\r\n"), 
			(DWORD)this, GetRequestTypeName(), GetConnectionInfo(), GetCallInfo(), 
			GetState(), GetStateData(), HaveSentResponse() ? _T("Yes") : _T("No"));

}// CTSPIRequest::Dump

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetRequestTypeName
//
// Debug method to retrieve the request name
//
LPCTSTR CTSPIRequest::GetRequestTypeName() const
{
    switch(GetCommand())
    {
        case REQUEST_ACCEPT:          return _T("lineAccept");
        case REQUEST_ADDCONF:         return _T("lineAddConf");
        case REQUEST_ANSWER:          return _T("lineAnswer");
        case REQUEST_BLINDXFER:       return _T("lineBlindXfer");
        case REQUEST_COMPLETECALL:    return _T("lineCompleteCall");
        case REQUEST_COMPLETEXFER:    return _T("lineCompleteXfer");
        case REQUEST_DIAL:            return _T("lineDial");
        case REQUEST_DROPCALL:        return _T("lineDropCall");
        case REQUEST_FORWARD:         return _T("lineForward");
        case REQUEST_HOLD:            return _T("lineHold");
        case REQUEST_MAKECALL:        return _T("lineMakeCall");
        case REQUEST_PARK:            return _T("linePark");
        case REQUEST_PICKUP:          return _T("linePickup");
        case REQUEST_REDIRECT:        return _T("lineRedirect");
        case REQUEST_REMOVEFROMCONF:  return _T("lineRemoveFromConf");
        case REQUEST_SECURECALL:      return _T("lineSecure");
        case REQUEST_SENDUSERINFO:    return _T("lineSendUser");
        case REQUEST_SETCALLPARAMS:   return _T("lineSetCallParams");
        case REQUEST_SETTERMINAL:     return _T("lineSetTerminal");
        case REQUEST_SETUPCONF:       return _T("lineSetupConf");
        case REQUEST_SETUPXFER:       return _T("lineSetupXfer");
        case REQUEST_SWAPHOLD:        return _T("lineSwapHold");
        case REQUEST_UNHOLD:          return _T("lineUnhold");
        case REQUEST_UNCOMPLETECALL:  return _T("lineUncompleteCall");
        case REQUEST_UNPARK:          return _T("lineUnpark");
        case REQUEST_MEDIACONTROL:    return _T("lineSetMediaControl");
        case REQUEST_PREPAREADDCONF:  return _T("linePrepareAddToConf");
        case REQUEST_GENERATEDIGITS:  return _T("lineGenerateDigits");
        case REQUEST_GENERATETONE:    return _T("lineGenerateTone");
        case REQUEST_SETBUTTONINFO:   return _T("phoneSetButtonInfo");
        case REQUEST_SETDISPLAY:      return _T("phoneSetDisplay");
        case REQUEST_SETHOOKSWITCHGAIN: return _T("phoneSetGain");
        case REQUEST_SETHOOKSWITCH:   return _T("phoneSetHookswitch");
        case REQUEST_SETLAMP:         return _T("phoneSetLamp");
        case REQUEST_SETRING:         return _T("phoneSetRing");
        case REQUEST_SETHOOKSWITCHVOL: return _T("phoneSetVolume");
        case REQUEST_SETPHONEDATA:    return _T("phoneSetData");
        case REQUEST_GETPHONEDATA:    return _T("Internal Get Phone Data");
        default:                      return _T("Derived class defined");
    }

}// CTSPIRequest::GetRequestTypeName

#endif // _DEBUG

