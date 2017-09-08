/*****************************************************************************/
//
// LINEREQ.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the line request processing functions which process
// each of the TAPI requests from the library.
// 
// Copyright (C) 1997-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
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
/*****************************************************************************/

#include "stdafx.h"
#include "dssp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/*----------------------------------------------------------------------------
	GLOBALS
-----------------------------------------------------------------------------*/

// Dialtone states to convert from Emulator to TAPI.
const DWORD g_DialToneStates[] = {
    LINEDIALTONEMODE_INTERNAL,
    LINEDIALTONEMODE_EXTERNAL
};  

// Busy states
const DWORD g_BusyStates[] = {
    LINEBUSYMODE_STATION,
    LINEBUSYMODE_TRUNK
};  

// Call state translation table.
extern DWORD g_CallStates[];
extern LPCTSTR g_ComplMsgs[];

/*****************************************************************************
** Procedure:  CDSLine::processAnswer
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Answer an incoming call.
**
*****************************************************************************/
BOOL CDSLine::processAnswer(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{                             
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // Ask the switch emulator to answer the call - this would be to press
        // the button associated with the offering address.
        case STATE_INITIAL:
            pReq->SetState (STATE_WAITFORCONNECT);
            GetDeviceInfo()->DRV_AnswerCall (pAddr->GetAddressID());
            break;
        
        // Step 2:
        // Address should indicate a connected end-party.
        case STATE_WAITFORCONNECT:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_CONNECT)
                    {   
                        // Since this is an incoming call, mark it connected to our address. 
                        pCall->SetConnectedIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
														  pAddr->GetDialableAddress(), pAddr->GetName());
                        CompleteRequest(pReq, 0);
                        pCall->SetCallState(LINECALLSTATE_CONNECTED);
                    }                        
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;             
                }
            }
            break;                
        
        default:
            ASSERT (FALSE);
            break;
    }
                    
    // If we failed, then idle the call appearance.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processAnswer

/*****************************************************************************
** Procedure:  CDSLine::processSetupXfer
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Setup a transfer request (lineSetupTransfer)
**
*****************************************************************************/
BOOL CDSLine::processSetupXfer (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
    BOOL fProcessed = FALSE;

    // TODO: Implement Call params from pTrans->lpCallParams
    switch (pReq->GetState())
    {                      
        // Step 1:
        // Send a transfer request with no address information.  This will
        // place the current call onHold, and the address will transition to
        // the dialtone state.
        case STATE_INITIAL:   
            pReq->SetState(STATE_CHECKFORDIALTONE);
            if (!GetDeviceInfo()->DRV_Transfer(pAddr->GetAddressID(), ""))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
                  
        // Step 2:
        // Wait for the switch to signal that we are in the proper state.
        case STATE_CHECKFORDIALTONE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_ONHOLD &&
                        lpAddrChange->wStateInfo == HOLDTYPE_TRANSFER)
                    {
                        pTrans->pCall->SetCallState(LINECALLSTATE_ONHOLDPENDTRANSFER);
                    }
                    else if (lpAddrChange->wNewState == ADDRESSSTATE_DIALTONE)
                    {   
                        // Request *must* complete before callstates may be changed!
                        // but as soon as it is completed, request struture is deleted,
                        // so save off the call appearance of the consultation call.
                        ASSERT (lpAddrChange->wStateInfo == DIALTONETYPE_INTERNAL);
                        CTSPICallAppearance* pCall = pTrans->pConsult;                       
                        CompleteRequest(pReq, 0);

                        // Send the initial callstate change for this call - we must
                        // supply a media mode.
                        pCall->SetCallState(LINECALLSTATE_DIALTONE, 
                                        LINEDIALTONEMODE_INTERNAL, 
                                        LINEMEDIAMODE_INTERACTIVEVOICE);
                    }
                    else
                        wResponse = EMRESULT_ERROR;    
                    fProcessed = TRUE;
                }                     
            }
            break;
            
        default:
            ASSERT (FALSE);
            break;
    }
                                    

    // If we failed, then complete the request
    if (wResponse == EMRESULT_ERROR)
    {   
        if (fProcessed || GETADDRID(lpBuff) == (WORD)-1 ||
            GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {   
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processSetupXfer

/*****************************************************************************
** Procedure:  CDSLine::processCompleteXfer
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Complete a transfer request (lineCompleteTransfer)
**
*****************************************************************************/
BOOL CDSLine::processCompleteXfer (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddrConsult = pReq->GetAddressInfo();
    TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
    CTSPIAddressInfo* pAddrCall = pTrans->pCall->GetAddressOwner();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {                      
        // Step 1:
        // Send a transfer request with no address information.  This will
        // complete the transfer request already pending on the switch.
        case STATE_INITIAL:   
            if (pTrans->dwTransferMode == LINETRANSFERMODE_TRANSFER)
            {
                pReq->SetState(STATE_WAITFOROFFLINE);
                if (!GetDeviceInfo()->DRV_Transfer(pAddrCall->GetAddressID(), _T(""), pAddrConsult->GetAddressID()))
                {
                    fProcessed = TRUE;
                    wResponse = EMRESULT_ERROR;
                }
            }
            else if (pTrans->dwTransferMode == LINETRANSFERMODE_CONFERENCE)
            {   
                pReq->SetState(STATE_WAITFORCONF);
                if (GetDeviceInfo()->DRV_Transfer(pAddrCall->GetAddressID(), _T(""), pAddrConsult->GetAddressID(), TRUE))
                    pTrans->pConf->SetCallState(LINECALLSTATE_ONHOLDPENDCONF, 0, LINEMEDIAMODE_INTERACTIVEVOICE);
                else
                {
                    fProcessed = TRUE;
                    wResponse = EMRESULT_ERROR;
                }
            }                
            break;
                  
        // Step 2:
        // Wait for the switch to signal that we are in the proper state.
        case STATE_WAITFOROFFLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD)pAddrCall->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_OFFLINE)
                    {   
                        CTSPICallAppearance* pCall = pTrans->pCall;
                        CTSPICallAppearance* pConsult = pTrans->pConsult;
                        CompleteRequest(pReq, 0);    
                        pCall->SetCallState(LINECALLSTATE_IDLE);
                        pConsult->SetCallState(LINECALLSTATE_IDLE);
                    }
                    else
                        wResponse = EMRESULT_ERROR;    
                    fProcessed = TRUE;
                }                     
            }
            break;
        
        // Step 3:
        // Wait for conference notification.
        case STATE_WAITFORCONF:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD)pAddrCall->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_INCONF)
                    {   
                        CTSPICallAppearance* pCall = pTrans->pCall;
                        CTSPICallAppearance* pConsult = pTrans->pConsult;
                        CTSPICallAppearance* pConf = pTrans->pConf;
                        CompleteRequest(pReq, 0);    
                        pCall->SetCallState(LINECALLSTATE_CONFERENCED);
                        pConsult->SetCallState(LINECALLSTATE_CONFERENCED);
                        pConf->SetCallState(LINECALLSTATE_CONNECTED);
                    }
                    else
                        wResponse = EMRESULT_ERROR;    
                    fProcessed = TRUE;
                }
                // Ignore the consultant call.
                else if (lpAddrChange->wAddressID == (WORD)pAddrConsult->GetAddressID())
                {
                    ASSERT (lpAddrChange->wNewState == ADDRESSSTATE_INCONF);
                    fProcessed = TRUE;                                      
                }
            }
            break;
            
        default:
            ASSERT (FALSE);
            break;
    }

    // If we failed, then complete the request
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || 
            GETADDRID(lpBuff) == (WORD)pAddrCall->GetAddressID() ||
            GETADDRID(lpBuff) == (WORD)pAddrConsult->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processCompleteXfer

/*****************************************************************************
** Procedure:  CDSLine::processForward
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Process a forwarding request (lineForward)
**
*****************************************************************************/
BOOL CDSLine::processForward (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    BOOL fProcessed = FALSE;
    TSPILINEFORWARD* lpForwData = (TSPILINEFORWARD*) pReq->GetDataPtr();
    LPCSTR lpszDest = NULL;
    
    // See if there are any forwarding entries - in our sample we only support
    // one forward request.  If multiple were supported, the 'arrForwardInfo' would
    // have more than one entry in it.
    if (lpForwData->arrForwardInfo.GetSize() > 0)
    {
        TSPIFORWARDINFO* pInfo = (TSPIFORWARDINFO*) lpForwData->arrForwardInfo.GetAt(0);
        ASSERT (pInfo != NULL);                                          
        if (pInfo->arrDestAddress.GetSize() > 0)
        {
            DIALINFO* pDialInfo = (DIALINFO*) pInfo->arrDestAddress.GetAt(0);
            lpszDest = pDialInfo->strNumber;
        }
    }
                                   
    // TODO: Implement "no answer" ring count
    // TODO: Implement management of Call Params in lpForwData->lpCallParams.                                   
                                   
    switch (pReq->GetState())
    {
        // Step 1:
        // Tell the switch to forward or un-forward our calls.
        case STATE_INITIAL:
            pReq->SetState (STATE_CHECKLAMP);
            if (!GetDeviceInfo()->DRV_Forward(0xffffffff, lpszDest))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }                
            break;
            
        // Step 2:
        // Watch for the FORWARD lamp to move to the blinking state
        case STATE_CHECKLAMP:
            if (wResponse == EMRESULT_LAMPCHANGED)
            {                           
                LPEMLAMPCHANGE lpLamp = (LPEMLAMPCHANGE) lpBuff;
                if ((lpszDest == NULL && lpLamp->wLampState == LAMPSTATE_OFF) ||
                    (lpszDest != NULL && lpLamp->wLampState == LAMPSTATE_BLINKING))
                    CompleteRequest(pReq, 0);
            }
            break;        
            
        default:
            ASSERT (FALSE);
            break;
    }

    // If we failed, then complete the request
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)-1)
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;
    
}// CDSLine::processForward

/*****************************************************************************
** Procedure:  CDSLine::processSetupConf
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Setup a conference event (linePrepareConference)
**
*****************************************************************************/
BOOL CDSLine::processSetupConf(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    BOOL fProcessed = FALSE;
    TSPICONFERENCE* pConfData = (TSPICONFERENCE*) pReq->GetDataPtr();

    switch (pReq->GetState())
    {
        // Step 1:   
        // Send the switch a conference command ADD - we should see a request for
        // onHold, followed by a dialtone.
        case STATE_INITIAL:
            pReq->SetState(STATE_CHECKFORHOLD);
			if (!GetDeviceInfo()->DRV_Conference (pAddr->GetAddressID(), 0xffff, CONFCOMMAND_ADD))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
            
        // Step 2:
        // Watch for our address changes
        case STATE_CHECKFORHOLD:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_ONHOLD &&
                        lpAddrChange->wStateInfo == HOLDTYPE_CONFERENCE)
                    {                                                
                        // Send back response of OK since we see we are moving
                        // to the conference.
                        CompleteRequest(pReq, 0, TRUE, FALSE);
                        if (pConfData->pCall != NULL)
                            pConfData->pCall->SetCallState(LINECALLSTATE_CONFERENCED);
                        pConfData->pConfCall->SetCallState(LINECALLSTATE_ONHOLDPENDCONF, 0, LINEMEDIAMODE_INTERACTIVEVOICE);
                    }
                    else if (lpAddrChange->wNewState == ADDRESSSTATE_DIALTONE)
                    {                                                             
                        ASSERT (lpAddrChange->wStateInfo == DIALTONETYPE_INTERNAL);
                        // Supply media mode for initial callstate - we only support
                        // interactive voice for conferenced calls.
                        pConfData->pConsult->SetCallState(LINECALLSTATE_DIALTONE, LINEDIALTONEMODE_INTERNAL, 
                                            LINEMEDIAMODE_INTERACTIVEVOICE);
                        CompleteRequest(pReq, 0);
                    } 
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;
                }                     
            }
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    // If we failed, then complete the request
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)-1 ||
            GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {   
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processSetupConf

/*****************************************************************************
** Procedure:  CDSLine::processAddConf
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Add a new party to a conference (lineAddConference)
**
*****************************************************************************/
BOOL CDSLine::processAddConf(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pConfCall = pReq->GetCallInfo();
    BOOL fProcessed = FALSE;
    TSPICONFERENCE* pConfData = (TSPICONFERENCE*) pReq->GetDataPtr();
    CTSPICallAppearance* pNewCall = pConfData->pConsult;

    switch (pReq->GetState())
    {
        // Step 1:
        // Tell the switch to complete the conference and tie the two
        // calls together.
        case STATE_INITIAL:
            pReq->SetState(STATE_CHECKFORHOLD);
            if (!GetDeviceInfo()->DRV_Conference (pConfCall->GetAddressOwner()->GetAddressID(),
                                pNewCall->GetAddressOwner()->GetAddressID(),
                                CONFCOMMAND_ADD))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
            
        // Step 2:
        // Watch for our address changes
        case STATE_CHECKFORHOLD:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == 
                    (WORD) pConfCall->GetAddressOwner()->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_INCONF)
                    {
                        pNewCall->SetCallState(LINECALLSTATE_CONFERENCED);
                        pConfCall->SetCallState(LINECALLSTATE_CONNECTED);
                        CompleteRequest(pReq, 0);
                    }
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;
                }                     
                else if (lpAddrChange->wAddressID ==
                    (WORD) pNewCall->GetAddressOwner()->GetAddressID())
                {                     
                    ASSERT (lpAddrChange->wNewState == ADDRESSSTATE_INCONF);
                    fProcessed = TRUE;
                }
            }
            break;
            
        default:
            ASSERT(FALSE);
            break;
    }

    // If we failed, then complete the request
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)-1 ||
            GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID() ||
            GETADDRID(lpBuff) == (WORD)pNewCall->GetAddressOwner()->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processAddConf

/*****************************************************************************
** Procedure:  CDSLine::processRemoveConf
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Remove a party from an existing conference call
**
*****************************************************************************/
BOOL CDSLine::processRemoveConf(CTSPIRequest* pReq, WORD /*wResponse*/, const LPVOID /*lpBuff*/)
{
	if (pReq->GetState() == STATE_INITIAL)
	{
		pReq->SetState(STATE_WAITFORCOMPLETE);
		CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
		TSPICONFERENCE* pConfData = (TSPICONFERENCE*) pReq->GetDataPtr();

		// We assume the remove works.  Call state changes will be reflected automatically.
		if (GetDeviceInfo()->DRV_Conference(pAddr->GetAddressID(), (DWORD)-1L, CONFCOMMAND_REMOVE) != 0)
		{
			// Complete the request BEFORE idle'ing the call or TAPI will return an
			// error indicating that the remove failed.
			CTSPICallAppearance* pCall = pConfData->pCall;
			CompleteRequest(pReq, 0);
			pCall->SetCallState(LINECALLSTATE_IDLE);
		}
		else            
			CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
	}
    
	// Let it continue to process the emulator response since we didn't.
    return FALSE;

}// CDSLine::processRemoveConf

/*****************************************************************************
** Procedure:  CDSLine::processBlindXfer
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Process an automated (blind) transfer of a call (lineBlindTransfer)
**
*****************************************************************************/
BOOL CDSLine::processBlindXfer (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    CObArray* parrDestAddr = (CObArray*) pReq->GetDataPtr();
    DIALINFO* pDialInfo = (DIALINFO*) parrDestAddr->GetAt(0);
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // Ask the switch to perform the transfer - since all the data is
        // here to perform the transfer, we can simply wait for the appearance
        // to go idle.
        case STATE_INITIAL:
            pReq->SetState(STATE_WAITFOROFFLINE);
            if (!GetDeviceInfo()->DRV_Transfer(pAddr->GetAddressID(), pDialInfo->strNumber))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
            
        // Step 2:
        // The call should go idle.
        case STATE_WAITFOROFFLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_OFFLINE)
                    {   
                        pCall->SetCallState(LINECALLSTATE_IDLE);
                        CompleteRequest(pReq, 0);
                    }
                    else if (lpAddrChange->wNewState == ADDRESSSTATE_ONHOLD &&
                             lpAddrChange->wStateInfo == HOLDTYPE_TRANSFER)
                    {
                        pCall->SetCallState(LINECALLSTATE_ONHOLDPENDTRANSFER);
                    }
                    else
                        wResponse = EMRESULT_ERROR;    
                    fProcessed = TRUE;
                }                     
            }
            break;
            
        default:
            ASSERT (FALSE);
            break;
    }

    // If we failed, then error the request.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_CALLUNAVAIL);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processBlindXfer

/*****************************************************************************
** Procedure:  CDSLine::processMakeCall
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Handle the mechanics of taking the switch off hook and 
**              dialing another party (lineMakeCall)
**
*****************************************************************************/
BOOL CDSLine::processMakeCall(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    TSPIMAKECALL* lpMakeCall = (TSPIMAKECALL*) pReq->GetDataPtr();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // Attach the call appearance to an address on our device using the
        // address ID information.  Take the address off hook in preparation for
        // dialing.  We inserted the addresses in the order that the switch gave them
        // to us, so the address ID should match what the switch says.
        case STATE_INITIAL:
            pCall->SetCallID (pAddr->GetAddressID());    
            pReq->SetState(STATE_CHECKFORDIALTONE);
            if (!GetDeviceInfo()->DRV_PrepareCall (pAddr->GetAddressID()))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
        
        // State 2:
        // Transition to the "DIALTONE" state when we get a notification that
        // the phone is in DIALTONE.
        case STATE_CHECKFORDIALTONE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                // Give TAPI a "good" result back indicating that the switch
                // has begun allocating trunk lines and such - it will notify us
                // when DIALTONE has been received.
                //
                // Do not delete the request since more work needs to be accomplished.
                CompleteRequest(pReq, 0, TRUE, FALSE);

				// Determine which address changed.
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    // Transition to a dialtone, and move to a DIAL state.
                    if (lpAddrChange->wNewState == ADDRESSSTATE_DIALTONE)
                    {
						// Set the new callstate
                        pCall->SetCallState(LINECALLSTATE_DIALTONE, g_DialToneStates[lpAddrChange->wStateInfo],
                                            LINEMEDIAMODE_INTERACTIVEVOICE);

						// Set a call-id for the call - the emulator doesn't use
						// call-ids so we fabricate one here.
						pCall->SetCallID(GetTickCount());
                        
                        // If we have addresse(s) to dial, then move to the
                        // dialing state, otherwise complete the request here and
                        // leave the call in the "dialtone" state.
                        if (lpMakeCall->arrAddresses.GetSize() == 0)
                            CompleteRequest(pReq, 0);
                        else
                        {
                            pReq->SetState(STATE_DIALING);
                            HandleDialEvent (pReq, 0, lpBuff, &lpMakeCall->arrAddresses, 
                                             lpMakeCall->dwCountryCode);
                        }    
                    }
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;                        
                }
            }
            break;                
        
        // Step 3-xx
        // Process the dial request.  Send off to a master handler for
        // dialing which will field requests from a MAKECALL and DIAL event.
        case STATE_DIALING:
        case STATE_WAITFORONLINE:
        case STATE_WAITFORCONNECT:
            return HandleDialEvent (pReq, wResponse, lpBuff, &lpMakeCall->arrAddresses, 
                                    lpMakeCall->dwCountryCode);
        
        // Should never get here.    
        default:
            ASSERT(FALSE);
            break;
    }
    
    // If we failed, then idle the call appearance.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            pCall->SetCallState(LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_UNAVAIL);
            pCall->SetCallState(LINECALLSTATE_IDLE);
            CompleteRequest(pReq, LINEERR_CALLUNAVAIL);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processMakeCall

/*****************************************************************************
** Procedure:  CDSLine::processGenDigits
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Generate a series of tones or pulses on the call. (lineGenerateDigits)
**
*****************************************************************************/
BOOL CDSLine::processGenDigits (CTSPIRequest* pReq, WORD /*wResponse*/, const LPVOID /*lpBuff*/)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    TSPIGENERATE* lpGenStruct = (TSPIGENERATE*) pReq->GetDataPtr();
    
    DIALINFO ds;
    ds.strNumber = lpGenStruct->strDigits;
    if (GetDeviceInfo()->DRV_Dial(pAddr->GetAddressID(), &ds, 0))
        CompleteRequest(pReq, 0);
    else
        CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
    return FALSE;        
        
}// CDSLine::processGenDigits

/*****************************************************************************
** Procedure:  CDSLine::processHold
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Transition a call to the OnHold status (lineHold)
**
*****************************************************************************/
BOOL CDSLine::processHold (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // Ask the switch to place the call on hold
        case STATE_INITIAL:                        
            pReq->SetState (STATE_CHECKFORHOLD);
            if (!GetDeviceInfo()->DRV_HoldCall(pAddr->GetAddressID()))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
 
        // State 2:
        // Transition to the "ONHOLD" state when we get a notification that
        // the phone is in a holding pattern.
        case STATE_CHECKFORHOLD:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_ONHOLD)
                    {
                        pCall->SetCallState(LINECALLSTATE_ONHOLD);
                        CompleteRequest(pReq, 0);
                    }
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;                        
                }
            }
            break;  
            
        default:
            ASSERT (FALSE);
            break;
    }
    
    // If we failed, then kill the request
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processHold

/*****************************************************************************
** Procedure:  CDSLine::processUnhold
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Transition a call which is currently holding on the switch
**              back to the previous state (normally Connected).
**
*****************************************************************************/
BOOL CDSLine::processUnhold (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    CTSPICallAppearance* pConsult = pCall->GetAttachedCall();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // Ask the switch to place the call on hold
        case STATE_INITIAL:                        
            pReq->SetState (STATE_CHECKFORHOLD);
            if (!GetDeviceInfo()->DRV_UnholdCall(pAddr->GetAddressID()))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
 
        // State 2:
        // Transition to the "ONHOLD" state when we get a notification that
        // the phone is in a holding pattern.
        case STATE_CHECKFORHOLD:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState != ADDRESSSTATE_ONHOLD)
                    {                         
                        switch (lpAddrChange->wNewState)
                        {
                            case ADDRESSSTATE_DIALTONE:
                                pCall->SetCallState(LINECALLSTATE_DIALTONE, LINEDIALTONEMODE_UNKNOWN);
                                CompleteRequest(pReq, 0);
                                break;
                            
                            case ADDRESSSTATE_ONLINE:
                                pCall->SetCallState(LINECALLSTATE_PROCEEDING);
                                CompleteRequest(pReq, 0);
                                break;
                                
                            case ADDRESSSTATE_CONNECT:
                                pCall->SetCallState(LINECALLSTATE_CONNECTED);
                                CompleteRequest(pReq, 0);
                                break;

                            case ADDRESSSTATE_DISCONNECT:
                                if (pConsult != NULL)
                                    pConsult->SetCallState(LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_NORMAL);
                                else                                    
                                    pCall->SetCallState(LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_NORMAL);
                                break;
                                
                            case ADDRESSSTATE_OFFLINE:
                                if (pConsult != NULL)
                                    pConsult->SetCallState(LINECALLSTATE_IDLE);
                                else
                                {
                                    pCall->SetCallState(LINECALLSTATE_IDLE);
                                    CompleteRequest(pReq, 0);
                                }                                   
                                break;
                            
                            case ADDRESSSTATE_INCONF:
                                ASSERT (pCall->GetCallType() == CALLTYPE_CONFERENCE);
                                pCall->SetCallState(LINECALLSTATE_CONNECTED);
                                CompleteRequest(pReq, 0);
                                break;
                                
                            default:
                                wResponse = EMRESULT_ERROR;
                                break;
                        }                                                            
                    }
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;                        
                }
            }
            break;  
            
        default:
            ASSERT (FALSE);
            break;
    }
    
    // If we failed, then kill the request
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processUnhold

/*****************************************************************************
** Procedure:  CDSLine::processSwapHold
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Swap the current call with the call waiting on hold (lineSwapHold).
**
*****************************************************************************/
BOOL CDSLine::processSwapHold (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    CTSPICallAppearance* pHeldCall = (CTSPICallAppearance*) pReq->GetDataPtr();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // If the two calls are on different addresses, then place the 
        // active call into the holding pattern.  Otherwise, if they are
        // on the same address (consultation) then issue a "flash" command
        // to the switch.
        case STATE_INITIAL:                        
            if (pCall->GetAddressOwner() == pHeldCall->GetAddressOwner())
            {
                pReq->SetState(STATE_WAITFORONLINE);        
                pReq->SetStateData(0);
                if (!GetDeviceInfo()->DRV_Flash(pAddr->GetAddressID()))
                {
                    fProcessed = TRUE;
                    wResponse = EMRESULT_ERROR;
                }     
            }
            else
            { 
                pReq->SetState (STATE_CHECKFORHOLD);
                if (!GetDeviceInfo()->DRV_HoldCall (pAddr->GetAddressID()))
                {
                    fProcessed = TRUE;
                    wResponse = EMRESULT_ERROR;
                }     
            }                
            break;
 
        // State 2:
        // Watch for the ACTIVE call to move to the hold state, and
        // when it does, re-activate the original held call (take it offhold).
        case STATE_CHECKFORHOLD:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_ONHOLD)
                    {                                                  
                        if (GetDeviceInfo()->DRV_UnholdCall(pHeldCall->GetAddressOwner()->GetAddressID()))
                        {   
                            pCall->SetCallState(LINECALLSTATE_ONHOLD);
                            pReq->SetState(STATE_WAITFORCONNECT);
                        }
                        else
                            wResponse = EMRESULT_ERROR;
                    }
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;                        
                }
            }
            break;  
            
        // State 3:
        // Watch for the held call to move out of the held state.  It might
        // not move to the connected state - it could be at a dialtone, or
        // ringing, etc.
        case STATE_WAITFORCONNECT:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pHeldCall->GetAddressOwner()->GetAddressID())
                {   
                    if (lpAddrChange->wNewState != ADDRESSSTATE_ONHOLD)
                    {
                        pHeldCall->SetCallState(g_CallStates[lpAddrChange->wNewState]);
                        CompleteRequest(pReq, 0);
                    }
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;                        
                }
            }
            break;  
        
        // State 2:2
        // Watch for the swap signal on the same address.  We simply will watch
        // for a non-onHold state.  This is used in the case of a swap on two
        // call appearances on a single address (i.e. consultation call)
        case STATE_WAITFORONLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState != ADDRESSSTATE_ONHOLD)
                    {   
                        // Make sure to switch with the correct type of hold - 
                        // otherwise completeXfer and completeConf won't work
                        // with this call anymore.
                        DWORD dwHoldType = pHeldCall->GetCallState();
                        ASSERT (dwHoldType == LINECALLSTATE_ONHOLD ||
                                dwHoldType == LINECALLSTATE_ONHOLDPENDCONF ||
                                dwHoldType == LINECALLSTATE_ONHOLDPENDTRANSFER);  
                        pCall->SetCallState(dwHoldType);
                        pHeldCall->SetCallState(g_CallStates[lpAddrChange->wNewState]);
                        CompleteRequest(pReq, 0);
                    }
                    fProcessed = TRUE;                        
                }
            }
            break;  
            
        default:
            ASSERT (FALSE);
            break;
    }
    
    // If we failed, then kill the request
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processSwapHold

/*****************************************************************************
** Procedure:  CDSLine::processDial
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Dial digits on a call (lineDial).
**
*****************************************************************************/
BOOL CDSLine::processDial(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    return HandleDialEvent (pReq, wResponse, lpBuff, (CADObArray*) pReq->GetDataPtr(), pReq->GetDataSize());

}// CDSLine::processDial

/*****************************************************************************
** Procedure:  CDSLine::processDropCall
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Drop a call off the switch (lineDrop).
**
*****************************************************************************/
BOOL CDSLine::processDropCall(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    BOOL fProcessed = FALSE;
    
    // If this is a conference call, then don't send the drop request to the
    // switch - it is a "fake" call appearance.
    if (pCall->GetCallType() == CALLTYPE_CONFERENCE)
        return processDropConference(pReq, wResponse, lpBuff);
    
    switch (pReq->GetState())
    {   
        // Step 1:
        // Ask the switch to drop the call.
        case STATE_INITIAL:
            // Don't allow conferenced calls to be dropped individually.  Our emulated
            // switch doesn't support this - the conference must be dropped or
            // the call may be removed.
            if (pCall->GetCallState() == LINECALLSTATE_CONFERENCED)
            {
                CompleteRequest(pReq, LINEERR_INVALCALLSTATE);        
                return FALSE;
            }

			// If the call is already dropped (possibly due to a conference owner drop)
			// then simply complete the request.
			else if (pCall->GetCallState() == LINECALLSTATE_IDLE)
				CompleteRequest(pReq, 0);
			else
			{
	            pReq->SetState (STATE_WAITFOROFFLINE);
	            GetDeviceInfo()->DRV_DropCall (pAddr->GetAddressID());
			}
            break;
        
        // Step 2:
        // When the address goes offline, idle the call appearance.
        case STATE_WAITFOROFFLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    // Is it offline?
                    if (lpAddrChange->wNewState == ADDRESSSTATE_OFFLINE)
                    {   
                        CompleteRequest (pReq, 0);       
                        pCall->SetCallState(LINECALLSTATE_IDLE);
                        fProcessed = TRUE;                        
                    }
                    // Disconnected - offline should follow.
                    else if (lpAddrChange->wNewState == ADDRESSSTATE_DISCONNECT)
                    {
                        pCall->SetCallState(LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_NORMAL);
                        fProcessed = TRUE;                        
                    }
                }
            }
            break;
    }
    return fProcessed;

}// CDSLine::processDropCall

/*****************************************************************************
** Procedure:  CDSLine::processDropConference
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Drop a created conference - dropping all calls within.
**
*****************************************************************************/
BOOL CDSLine::processDropConference(CTSPIRequest* pReq, WORD /*wResponse*/, const LPVOID /*lpBuff*/)
{                                     
	if (pReq->GetState() == STATE_INITIAL)
	{
		pReq->SetState(STATE_WAITFORCOMPLETE);

		CTSPIConferenceCall* pCall = (CTSPIConferenceCall*) pReq->GetCallInfo();
		CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();

		// Idle any attached call to the conference.  This would be a consultation
		// call created for adding a new member but not yet in conferenced state.
		CTSPICallAppearance* pThisCall = pCall->GetAttachedCall();
		if (pThisCall != NULL)
			pThisCall->SetCallState(LINECALLSTATE_IDLE);
    
		// Idle all the calls in this conference.  As each call is IDLE'd,
		// it is automatically removed from the conference array.
		while (pCall->GetConferenceCount() > 0)
		{   
			CTSPICallAppearance* pThisCall = pCall->GetConferenceCall(0);
			pThisCall->SetCallState(LINECALLSTATE_IDLE);
		}

		// Complete the request
		CompleteRequest(pReq, 0);

		// Issue the DROP request to the switch
		GetDeviceInfo()->DRV_Conference(pAddr->GetAddressID(), (DWORD)-1L, CONFCOMMAND_DESTROY);
	}

    return FALSE;    

}// CDSLine::processDropConference

/*****************************************************************************
** Procedure:  CDSLine::processRedirect
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description:  Redirect a call to another station.
**
*****************************************************************************/
BOOL CDSLine::processRedirect(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{                             
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    CObArray* parrAddress = (CObArray*) pReq->GetDataPtr();
    DWORD dwCountryCode = pReq->GetDataSize();
    BOOL fProcessed = FALSE;

    // Get the address to redirect to.  We only use the first address in the
    // array.                      
    DIALINFO* pDialInfo = NULL;
    if (parrAddress->GetSize() > 0)
        pDialInfo = (DIALINFO*) parrAddress->GetAt(0);

    switch (pReq->GetState())
    {
        // Step 1:
        // Ask the switch emulator to answer the call - this would be to press
        // the button associated with the offering address.
        case STATE_INITIAL:
            pReq->SetState (STATE_WAITFOROFFLINE);
            if (!GetDeviceInfo()->DRV_Redirect (pAddr->GetAddressID(), pDialInfo->strNumber, dwCountryCode))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;
        
        // Step 2:
        // Address should indicate an offline condition.
        case STATE_WAITFOROFFLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_OFFLINE)
                    {
                        CompleteRequest(pReq, 0);
                        pCall->SetCallState(LINECALLSTATE_IDLE);
                    }                        
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;             
                }
            }
            break;                
        
        default:
            ASSERT (FALSE);
            break;
    }
                    
    // If we failed, then idle the call appearance.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processRedirect

/*****************************************************************************
** Procedure:  CDSLine::processCompleteCall
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Wait for a call completion on an existing call. (lineCompleteCall)
**
*****************************************************************************/
BOOL CDSLine::processCompleteCall(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{                             
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    TSPICOMPLETECALL* pCompleteCall = (TSPICOMPLETECALL*) pReq->GetDataPtr();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // Tell the switch to complete the call.
        case STATE_INITIAL:       
            {                    
                pReq->SetState (STATE_WAITFORCOMPLETE);
                CString strMessage = "";
                if (pCompleteCall->dwCompletionMode == LINECALLCOMPLMODE_MESSAGE)
                    strMessage = g_ComplMsgs[pCompleteCall->dwMessageId];
                if (!GetDeviceInfo()->DRV_CompleteCall (pAddr->GetAddressID(), 
                        pCompleteCall->dwCompletionMode, strMessage))
                {
                    fProcessed = TRUE;
                    wResponse = EMRESULT_ERROR;
                }
            }
            break;

        // Step 2:
        // Switch sends back OK response for call completion.
        case STATE_WAITFORCOMPLETE:
            if (wResponse == EMRESULT_COMPLRESULT)
            {
				LPEMCOMPLETECALL emComplete = (LPEMCOMPLETECALL) lpBuff;
                if (emComplete->wAddressID == (WORD)pAddr->GetAddressID())
                {   
                    // Save off data which gets deleted when CompleteRequest() occurs.                            
                    DWORD dwCompletionMode = pCompleteCall->dwCompletionMode;
                    DWORD dwCompletionID = pCompleteCall->dwCompletionID;
                    
                    // Save off the switch identifier and complete the request.
                    pCompleteCall->strSwitchInfo = "";
                    pCompleteCall->dwSwitchInfo = (DWORD) emComplete->wCompletionType;
                    CompleteRequest(pReq, 0);
                    
                    // If this is a message/intrude completion request, then
                    // remove the completion request from the list.  We will still
                    // get notified from the switch, but this is easier in hooking
                    // the call up.
                    if (dwCompletionMode == LINECALLCOMPLMODE_MESSAGE)
                        RemoveCallCompletionRequest (dwCompletionID);
                    else if (dwCompletionMode == LINECALLCOMPLMODE_INTRUDE)
                    {
                        pCall->GetCallInfo()->dwCompletionID = dwCompletionID;
                        pCall->SetCallReason(LINECALLREASON_CALLCOMPLETION);
                        RemoveCallCompletionRequest (dwCompletionID);
                    }
                    fProcessed = TRUE;
                }
            }
            break;
            
        default:
            ASSERT (FALSE);
            break;
    }
    
    // If we failed, then idle the call appearance.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processCompleteCall

/*****************************************************************************
** Procedure:  CDSLine::processPark
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Park a call on the switch
**
*****************************************************************************/
BOOL CDSLine::processPark (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{                   
    const TCHAR * pszNonDirAddr = _T("5551212");
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    TSPILINEPARK* pPark = (TSPILINEPARK*) pReq->GetDataPtr();
    CString strNum = pszNonDirAddr;
    if (pPark->arrAddresses.GetSize() > 0)
    {
        DIALINFO* pDialInfo = (DIALINFO*) pPark->arrAddresses[0];
        strNum = pDialInfo->strNumber;
    }
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // If this is a NON-directed park, then fill it in with a bogus address
        // which we park everything into.
        case STATE_INITIAL:
            if (pPark->dwParkMode == LINEPARKMODE_NONDIRECTED)
                CopyVarString (pPark->lpNonDirAddress, pszNonDirAddr);
            pReq->SetState (STATE_WAITFORCOMPLETE);
            if (!GetDeviceInfo()->DRV_Park(pAddr->GetAddressID(), strNum))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;

        // Step 2:
        // Switch idles call
        case STATE_WAITFORCOMPLETE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_OFFLINE)
                    {
                        CompleteRequest(pReq, 0);
                        pCall->SetCallState(LINECALLSTATE_IDLE);
                    }                        
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;             
                }
            }
            break;                
            
        default:
            ASSERT (FALSE);
            break;
    }
    
    // If we failed, then idle the call appearance.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processPark

/*****************************************************************************
** Procedure:  CDSLine::processUnpark
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Unpark a call from the switch (lineUnpark)
**
*****************************************************************************/
BOOL CDSLine::processUnpark (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{                             
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    BOOL fProcessed = FALSE;
    CObArray* parrAddress = (CObArray*) pReq->GetDataPtr();
    DIALINFO* pDialInfo = (DIALINFO*) parrAddress->GetAt(0);
                              
    switch (pReq->GetState())
    {
        // Step 1:
        // Attempt to unpark a call at the specified address.  The emulator
        // will return an error if no call is parked there.
        case STATE_INITIAL:
            pReq->SetState (STATE_WAITFORCOMPLETE);
            if (!GetDeviceInfo()->DRV_Unpark(pAddr->GetAddressID(), pDialInfo->strNumber))
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
            break;

        // Step 2:
        // Switch idles call
        case STATE_WAITFORCOMPLETE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_CONNECT)
                    {
                        CompleteRequest(pReq, 0);
                        pCall->SetCallReason (LINECALLREASON_UNPARK);
                        pCall->SetCallState(LINECALLSTATE_CONNECTED, 0, LINEMEDIAMODE_INTERACTIVEVOICE);
                    }                        
                    else
                        wResponse = EMRESULT_ERROR;
                    fProcessed = TRUE;             
                }
            }
            break;                
            
        default:
            ASSERT (FALSE);
            break;
    }
    
    // If we failed, then idle the call appearance.
    if (wResponse == EMRESULT_ERROR)
    {                               
        if (fProcessed || GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;
                              
}// CDSLine::processUnpark

/*****************************************************************************
** Procedure:  CDSLine::processPickup
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Pickup a call from a remote station. (linePickup)
**
*****************************************************************************/
BOOL CDSLine::processPickup (CTSPIRequest* pReq, WORD /*wResponse*/, const LPVOID /*lpBuff*/)
{                             
    // TODO: Implement pickup in emulator.
    CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
    return FALSE;
    
}// CDSLine::processPickup

/*****************************************************************************
** Procedure:  CDSLine::processWaitReq
**
** Arguments:  'pReq' - Request object we are working with
**             'wResponse' - Current response from emulator
**             'lpBuff' - Current data structure from emulator
**
** Returns:    TRUE/FALSE success code
**
** Description: Wait for a VERSIONINFO, EMSETTINGS, or GETADDRESSINFO structure.
**
*****************************************************************************/
BOOL CDSLine::processWaitReq (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff, DWORD dwSize)
{                          
	if (wResponse == EMRESULT_RECEIVED && lpBuff != NULL && dwSize == pReq->GetDataSize())
	{
		memcpy (pReq->GetDataPtr(), lpBuff, dwSize);
		CompleteRequest(pReq, 0);
		return TRUE;
	}
    return FALSE;
    
}// CDSLine::processWaitReq
