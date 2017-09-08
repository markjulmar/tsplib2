/******************************************************************************/
//                                                                        
// REQUEST.CPP - Source file for the TSPIRequest base class               
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the code to manage the asynchronous request     
// objects which are dynamically allocated for each TSP request generated 
// by TAPI.                                                               
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
CTSPIRequest::CTSPIRequest()
{
    m_wReqType = (WORD)-1;
    m_dwRequestId = (DWORD)-1L;
    m_lpData = NULL;
    m_dwSize = 0L;
    m_pConnOwner = NULL;
    m_iReqState = STATE_INITIAL;
    m_pCall = NULL;
    m_fResponseSent = FALSE;
    m_lResult = -1L;
    m_hEvent = NULL;

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
// WARNING: IF YOU OVERRIDE THIS OBJECT AND ALLOCATE ANY MEMORY,
// YOU ARE RESPONSIBLE FOR FREE'ING IT IN YOUR OWN DESTRUCTOR!
//
CTSPIRequest::~CTSPIRequest()
{
	// Decrement our reference to the call.
	if (m_pCall != NULL)
		m_pCall->DecRefCount();

	// Make sure threads are never blocked indefinately.
    m_hEvent = NULL;
    Yield();

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
        case REQUEST_GATHERDIGITS:
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
        case REQUEST_DIAL:
        case REQUEST_UNPARK:
        case REQUEST_REDIRECT:
			{
				CObject* pObj = (CObject*)m_lpData;
				if (pObj->IsKindOf(RUNTIME_CLASS(CObject)) == FALSE)
				{
					TRACE("Warning: Request object had invalid data buffer - supposed to be object!\r\n");
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
				TRACE("User-defined request - make sure to delete m_lpData in destructor\r\n");
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
// CTSPIRequest::SetCommand
//
// Set the command for this request object.  This is useful when
// a packet is transitioned through two commands (like MakeCall to Dial).
//
VOID CTSPIRequest::SetCommand(WORD wCommand)
{
    m_wReqType = wCommand;
    SetState (STATE_INITIAL);

}// CTSPIRequest::SetCommand

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetDataPtr
//
// Set the state data pointer for this request
//
VOID CTSPIRequest::SetDataPtr(LPVOID lpBuff)
{
	m_lpData = lpBuff;

}// CTSPIRequest::SetDataPtr

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetDataSize
//
// Set the state data size for this request
//
VOID CTSPIRequest::SetDataSize(DWORD dwSize)
{
	m_dwSize = dwSize;

}// CTSPIRequest::SetDataSize

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetCommand
//
// Return the command request for this packet
//
WORD CTSPIRequest::GetCommand() const
{
    return m_wReqType;

}// CTSPIRequest::GetCommand

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetAsynchRequestId
//
// Return the TAPI asynchronous request id associated with this
// command.
//
DRV_REQUESTID CTSPIRequest::GetAsynchRequestId() const 
{
    return m_dwRequestId;

}// CTSPIRequest::GetAsynchRequestId

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetDataPtr
//
// Return the optional data associated with this request.
//
LPVOID CTSPIRequest::GetDataPtr() const
{ 
    return m_lpData;

}// CTSPIRequest::GetDataPtr

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetDataSize
//
// Return the size of the data block in our data pointer
//
DWORD CTSPIRequest::GetDataSize() const 
{
    return m_dwSize;

}// CTSPIRequest::GetDataSize

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetState
//
// Set the current state of this request.
//
VOID CTSPIRequest::SetState(int iState) 
{ 
    m_iReqState = iState;

}// CTSPIRequest::SetState

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetState
//
// Get the state of this request
//
int CTSPIRequest::GetState() const 
{
    return m_iReqState;

}// CTSPIRequest::GetState

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetStateData
//
// Set the state data for this request
//
VOID CTSPIRequest::SetStateData(DWORD dwData)
{
    m_dwAppData = dwData;

}// CTSPIRequest::SetStateData

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetStateData
//
// Return the state data for this request (USER-DEFINED)
//
DWORD CTSPIRequest::GetStateData() const
{
    return m_dwAppData;

}// CTSPIRequest::GetStateData

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::HaveSentResponse
//
// Returns BOOL indicating whether a response has been sent to TAPI
// about this request.  If this has no request id, then always return
// as if we sent a response.
//
BOOL CTSPIRequest::HaveSentResponse() const 
{ 
    if (GetAsynchRequestId() > 0)
        return m_fResponseSent;
    return TRUE;

}// CTSPIRequest::HaveSentResponse

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetConnectionInfo
//
// Return the CTSPIConnection pointer this packet relates to
//
CTSPIConnection* CTSPIRequest::GetConnectionInfo() const
{ 
    return m_pConnOwner;

}// CTSPIRequest::GetConnectionInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetAddressInfo
//
// Return the address information for a request packet.
//
CTSPIAddressInfo* CTSPIRequest::GetAddressInfo() const
{                               
    return m_pAddress;
    
}// CTSPIRequest::GetAddressInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetCallInfo
//
// Return the CTSPICallAppearance associated with this request.
//
CTSPICallAppearance* CTSPIRequest::GetCallInfo() const
{ 
    return m_pCall;

}// CTSPIRequest::GetCallInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetCallInfo
//
// Return the CTSPICallAppearance associated with this request.
//
void CTSPIRequest::SetCallInfo(CTSPICallAppearance* pCall)
{ 
	// Decrement the previous call.
	if (m_pCall != pCall)
	{
		if (m_pCall != NULL)
			m_pCall->DecRefCount();

		m_pCall = pCall;
		if (pCall != NULL)
		{
			m_pCall->IncRefCount();
			m_pAddress = pCall->GetAddressInfo();
			m_pConnOwner = pCall->GetLineConnectionInfo();
		}
	}

}// CTSPIRequest::SetCallInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::Complete
//
// This causes the request to be completed.  It is invoked by 
// the device when the request is finished.
//
VOID CTSPIRequest::Complete (LONG lResult, BOOL fSendTapiNotification)
{
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
    if (m_hEvent != NULL)
    {
        // Let all the other thread "wake up".
        m_hEvent = NULL;
        Yield();
    }

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
#ifdef _DEBUG
    TRACE ("Thread waiting for request <0x%lx %s> to complete, <%ld>\r\n", (DWORD)this, GetRequestTypeName(), (DWORD)(UINT)m_hEvent);
#endif
    
    // Mark that we are waiting for an event to complete.                     
    m_hEvent = (HANDLE) ((DWORD)(UINT)m_hEvent + 1);

    LONG lResult = -1;
        
    // Now begin yielding the processor until the event clears.
    // Do NOT call PeekMessage/GetMessage/TranslateMessage/DispatchMessage!
    // We are in the context of a calling application, and various problems
    // can occur if we dispatch messages - the most significant being
    // (1) Reentrancy into application window procedures which don't expect it.
    // (2) Under Windows 95, if the client application is 32-bit, a 16-bit DispatchMessage()
    //     will strip off the HIWORD of the 32-bit WPARAM - possibly resulting in GPFs if
    //     it had a 32-bit pointer in it.
    DWORD dwStartTime = GetTickCount();
    while (m_hEvent != NULL)
    {   
        // If we timeout, return (-1L)
        if (dwMsec > 0 && dwStartTime+dwMsec < GetTickCount())
        {
            TRACE ("Thread timed-out, Event=%ld\r\n", (DWORD)(UINT)m_hEvent);
            break;
        }
        Yield();    
    }
    
    // Save off the result for the request.
    if (m_hEvent == NULL)
        lResult = m_lResult;
    
    // Now force the thread to give up control again, so we get a chance to 
    // complete the main companion application thread correctly.  Otherwise,
    // TAPI will re-enter us on this thread.    
    Yield();
    TRACE ("Thread finished wait, rc=%ld\r\n", lResult);
    return lResult;
        
}// CTSPIRequest::WaitForCompletion

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::Dump
//
// Debug method to dump out the TSPI Request block
//
void CTSPIRequest::Dump(CDumpContext&) const
{
	TRACE("CTSPIRequest: 0x%lx %s Conn:0x%lx, Call:0x%lx, State:%d, Data=0x%lx, TAPIComplete=%s\r\n", 
			(DWORD)this, GetRequestTypeName(), GetConnectionInfo(), GetCallInfo(), 
			GetState(), GetStateData(), HaveSentResponse() ? "Yes" : "No");

}// CTSPIRequest::Dump

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetRequestTypeName
//
// Debug method to retrieve the request name
//
LPCSTR CTSPIRequest::GetRequestTypeName() const
{
    switch(GetCommand())
    {
        case REQUEST_ACCEPT:          return "lineAccept";
        case REQUEST_ADDCONF:         return "lineAddConf";
        case REQUEST_ANSWER:          return "lineAnswer";
        case REQUEST_BLINDXFER:       return "lineBlindXfer";
        case REQUEST_COMPLETECALL:    return "lineCompleteCall";
        case REQUEST_COMPLETEXFER:    return "lineCompleteXfer";
        case REQUEST_DIAL:            return "lineDial";
        case REQUEST_DROPCALL:        return "lineDropCall";
        case REQUEST_FORWARD:         return "lineForward";
        case REQUEST_HOLD:            return "lineHold";
        case REQUEST_MAKECALL:        return "lineMakeCall";
        case REQUEST_PARK:            return "linePark";
        case REQUEST_PICKUP:          return "linePickup";
        case REQUEST_REDIRECT:        return "lineRedirect";
        case REQUEST_REMOVEFROMCONF:  return "lineRemoveFromConf";
        case REQUEST_SECURECALL:      return "lineSecure";
        case REQUEST_SENDUSERINFO:    return "lineSendUser";
        case REQUEST_SETCALLPARAMS:   return "lineSetCallParams";
        case REQUEST_SETTERMINAL:     return "lineSetTerminal";
        case REQUEST_SETUPCONF:       return "lineSetupConf";
        case REQUEST_SETUPXFER:       return "lineSetupXfer";
        case REQUEST_SWAPHOLD:        return "lineSwapHold";
        case REQUEST_UNHOLD:          return "lineUnhold";
        case REQUEST_UNCOMPLETECALL:  return "lineUncompleteCall";
        case REQUEST_UNPARK:          return "lineUnpark";
        case REQUEST_MEDIACONTROL:    return "lineSetMediaControl";
        case REQUEST_MONITORDIGITS:   return "lineMonitorDigits";
        case REQUEST_MONITORMEDIA:    return "lineMonitorMedia";
        case REQUEST_MONITORTONES:    return "lineMonitorTones";
        case REQUEST_PREPAREADDCONF:  return "linePrepareAddToConf";
        case REQUEST_GATHERDIGITS:    return "lineGatherDigits";
        case REQUEST_GENERATEDIGITS:  return "lineGenerateDigits";
        case REQUEST_GENERATETONE:    return "lineGenerateTone";
        case REQUEST_SETBUTTONINFO:   return "phoneSetButtonInfo";
        case REQUEST_SETDISPLAY:      return "phoneSetDisplay";
        case REQUEST_SETHOOKSWITCHGAIN: return "phoneSetGain";
        case REQUEST_SETHOOKSWITCH:   return "phoneSetHookswitch";
        case REQUEST_SETLAMP:         return "phoneSetLamp";
        case REQUEST_SETRING:         return "phoneSetRing";
        case REQUEST_SETHOOKSWITCHVOL: return "phoneSetVolume";
        case REQUEST_SETPHONEDATA:    return "phoneSetData";
        case REQUEST_GETPHONEDATA:    return "Internal Get Phone Data";
        default:                      return "Derived class defined";
    }

}// CTSPIRequest::GetRequestTypeName

#endif // _DEBUG

