/*****************************************************************************/
//
// LINEREQ.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the line request functions
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

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/*----------------------------------------------------------------------------
	GLOBAL DATA
-----------------------------------------------------------------------------*/

extern DWORD g_CallStates[10];

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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processAnswer
//
// Answer an offering call
//
BOOL CDSLine::processAnswer(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{                             
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    TSPIMAKECALL* lpMakeCall = (TSPIMAKECALL*) pReq->GetDataPtr();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {
        // Step 1:
        // Ask the switch emulator to answer the call - this would be to press
        // the button associated with the offering address.
        case STATE_INITIAL:
            if (GetDeviceInfo()->DRV_AnswerCall (pAddr->GetAddressID()))
                pReq->SetState (STATE_WAITFORCONNECT);
            else
            {
                fProcessed = TRUE;
                wResponse = EMRESULT_ERROR;
            }
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
                        // Since this is an incoming call, mark it connected to
                        // our address. 
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
            pCall->SetCallState(LINECALLSTATE_IDLE);
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processAnswer

//////////////////////////////////////////////////////////////////////////
// CDSLine::processSetupXfer
//
// Process a setup transfer request for our switch (consultation)
//
BOOL CDSLine::processSetupXfer (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
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
            if (GetDeviceInfo()->DRV_Transfer(pAddr->GetAddressID(), ""))
                pReq->SetState(STATE_CHECKFORDIALTONE);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processCompleteXfer
//
// Process a complete transfer request for our switch (consultation)
//
BOOL CDSLine::processCompleteXfer (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddrConsult = pReq->GetAddressInfo();
    TSPITRANSFER* pTrans = (TSPITRANSFER*) pReq->GetDataPtr();
    CTSPIAddressInfo* pAddrCall = pTrans->pCall->GetAddressInfo();
    BOOL fProcessed = FALSE;

    switch (pReq->GetState())
    {                      
        // Step 1:
        // Send a transfer request with no address information.  This will
        // complete the transfer request already pending on the switch.
        case STATE_INITIAL:   
            if (pTrans->dwTransferMode == LINETRANSFERMODE_TRANSFER)
            {
                if (GetDeviceInfo()->DRV_Transfer(pAddrCall->GetAddressID(), "", 
                                 pAddrConsult->GetAddressID()))
                    pReq->SetState(STATE_WAITFOROFFLINE);
                else
                {
                    fProcessed = TRUE;
                    wResponse = EMRESULT_ERROR;
                }
            }
            else if (pTrans->dwTransferMode == LINETRANSFERMODE_CONFERENCE)
            {   
                if (GetDeviceInfo()->DRV_Transfer(pAddrCall->GetAddressID(), "",
                                 pAddrConsult->GetAddressID(), TRUE))
                {                                 
                    pReq->SetState(STATE_WAITFORCONF);
                    pTrans->pConf->SetCallState(LINECALLSTATE_ONHOLDPENDCONF, 0, LINEMEDIAMODE_INTERACTIVEVOICE);
                }
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processForward
//
// Process a forward request for our phone.
//
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
            if (GetDeviceInfo()->DRV_Forward(-1, lpszDest))
                pReq->SetState (STATE_CHECKLAMP);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processSetupConf
//
// Setup for a new conference call - establishes consultation call.
//
BOOL CDSLine::processSetupConf(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    BOOL fProcessed = FALSE;
    TSPICONFERENCE* pConfData = (TSPICONFERENCE*) pReq->GetDataPtr();

    switch (pReq->GetState())
    {
        // Step 1:   
        // Send the switch a conference command ADD - we should see a request for
        // onHold, followed by a dialtone.
        case STATE_INITIAL:
            if (GetDeviceInfo()->DRV_Conference(pAddr->GetAddressID(), -1L,  CONFCOMMAND_ADD))
                pReq->SetState(STATE_CHECKFORHOLD);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processAddConf
//
// Add a consultation call to the conference.
//
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
            if (GetDeviceInfo()->DRV_Conference (pConfCall->GetAddressInfo()->GetAddressID(),
                                pNewCall->GetAddressInfo()->GetAddressID(),
                                CONFCOMMAND_ADD))
            {                               
                pReq->SetState(STATE_CHECKFORHOLD);
            }                
            else
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
                    (WORD) pConfCall->GetAddressInfo()->GetAddressID())
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
                    (WORD) pNewCall->GetAddressInfo()->GetAddressID())
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
            GETADDRID(lpBuff) == (WORD)pNewCall->GetAddressInfo()->GetAddressID())
        {
            CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
            fProcessed = TRUE;
        }           
    }
    return fProcessed;

}// CDSLine::processAddConf

//////////////////////////////////////////////////////////////////////////
// CDSLine::processRemoveConf
//
// Revert back to original two party call from a conference
//
BOOL CDSLine::processRemoveConf(CTSPIRequest* pReq, WORD /*wResponse*/, const LPVOID /*lpBuff*/)
{
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    TSPICONFERENCE* pConfData = (TSPICONFERENCE*) pReq->GetDataPtr();

    // We assume the remove works.  Call state changes will be reflected automatically.
    if (GetDeviceInfo()->DRV_Conference(pAddr->GetAddressID(), -1L, CONFCOMMAND_REMOVE))
    {
        pConfData->pCall->SetCallState(LINECALLSTATE_IDLE);
        CompleteRequest(pReq, 0);
    }
    else            
        CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
    
    return FALSE;

}// CDSLine::processRemoveConf

//////////////////////////////////////////////////////////////////////////
// CDSLine::processBlindXfer
//
// Process a blind transfer request for our switch
//
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
            if (GetDeviceInfo()->DRV_Transfer(pAddr->GetAddressID(), pDialInfo->strNumber))
                pReq->SetState(STATE_WAITFOROFFLINE);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processMakeCall
//
// This method initiates and steps through the requirements for
// making a call on our emulated device.
//
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
            pReq->SetState(STATE_CHECKFORDIALTONE);
            pCall->SetCallID (pAddr->GetAddressID());    
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
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
	                // Give TAPI a "good" result back indicating that the switch
	                // has begun allocating trunk lines and such - it will notify us
	                // when DIALTONE has been received.
	                //
	                // Do not delete the request since more work needs to be accomplished.
	                CompleteRequest(pReq, 0, TRUE, FALSE);
                
                    // Transition to a dialtone, and move to a DIAL state.
                    if (lpAddrChange->wNewState == ADDRESSSTATE_DIALTONE)
                    {
                        pCall->SetCallState(LINECALLSTATE_DIALTONE, g_DialToneStates[lpAddrChange->wStateInfo],
                                            LINEMEDIAMODE_INTERACTIVEVOICE);
                        
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processGenDigits
//
// Generate a series of DTMF digits on our phone device.
//
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processHold
//
// Place a call on hold on the emulated switch
//
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
            if (GetDeviceInfo()->DRV_HoldCall(pAddr->GetAddressID()))
                pReq->SetState (STATE_CHECKFORHOLD);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processUnhold
//
// Take a call off hold on the emulated switch
//
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
            if (GetDeviceInfo()->DRV_UnholdCall(pAddr->GetAddressID()))
                pReq->SetState (STATE_CHECKFORHOLD);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processSwapHold
//
// Swap two calls (one on hold, and one active)
//
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
            if (pCall->GetAddressInfo() == pHeldCall->GetAddressInfo())
            {
                if (GetDeviceInfo()->DRV_Flash(pAddr->GetAddressID()))
                {
                    pReq->SetStateData(0);
                    pReq->SetState(STATE_WAITFORONLINE);        
                }
                else
                {
                    fProcessed = TRUE;
                    wResponse = EMRESULT_ERROR;
                }     
            }
            else
            { 
                if (GetDeviceInfo()->DRV_HoldCall (pAddr->GetAddressID()))
                    pReq->SetState (STATE_CHECKFORHOLD);
                else
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
                        if (GetDeviceInfo()->DRV_UnholdCall(pHeldCall->GetAddressInfo()->GetAddressID()))
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
                if (lpAddrChange->wAddressID == (WORD) pHeldCall->GetAddressInfo()->GetAddressID())
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processDial
//
// This method initiates and steps through the requirements to 
// dial a number on our emulator.
//
BOOL CDSLine::processDial(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    return HandleDialEvent (pReq, wResponse, lpBuff, (CADObArray*) pReq->GetDataPtr(), pReq->GetDataSize());

}// CDSLine::processDial

//////////////////////////////////////////////////////////////////////////
// CDSLine::processDropCall
//
// This method drops a call on an address.
//
BOOL CDSLine::processDropCall(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    BOOL fProcessed = FALSE;
    BOOL fDropConsult = (pCall->GetCallType() == CALLTYPE_NORMAL && pCall->GetAttachedCall() != NULL);
    
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
            
            // We are connected to a consultant call.  The emulator deals in straight
            // ADDRESS ids - it doesn't track the consultant calls or have any connection
            // to our handles, any operation performed is always on the current call
            // handle, so if our call is onHOLD, then switch over to our call with
            // a flash before the drop occurs.
            if (fDropConsult && 
                (pCall->GetCallState() == LINECALLSTATE_ONHOLDPENDTRANSFER ||
                 pCall->GetCallState() == LINECALLSTATE_ONHOLDPENDCONF))
            {
                GetDeviceInfo()->DRV_Flash(pAddr->GetAddressID());
                pReq->SetState (STATE_WAITFORONLINE);
                break;
            }

        // Fall through is intentional
        case STATE_STARTDROP:
            GetDeviceInfo()->DRV_DropCall (pAddr->GetAddressID());
            pReq->SetState (STATE_WAITFOROFFLINE);
            break;
        
        // Step 1 1/2
        // Wait for the flash to occur
        case STATE_WAITFORONLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState != ADDRESSSTATE_ONHOLD)
                        pReq->SetState(STATE_STARTDROP);
                }
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
                        // Call will automatically go idle in base class.
                        CompleteRequest (pReq, 0);       
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

        default:
            ASSERT(FALSE);
            break;
    }
    return fProcessed;

}// CDSLine::processDropCall

//////////////////////////////////////////////////////////////////////////
// CDSLine::processDropConference
//
// Drop a conference call - send a DROP conference command to the
// switch (i.e. press the conference button).  All involved call
// appearances will go dark.
//
BOOL CDSLine::processDropConference(CTSPIRequest* pReq, WORD /*wResponse*/, const LPVOID /*lpBuff*/)
{                                     
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
        // If we are about to remove the last call from the conference, then
        // complete the request so that the ASYNCH reply is generated BEFORE the
        // conference call itself is IDLE'd.  The conference call automatically
        // moves to an idle state when the last call is removed.
        if (pCall->GetConferenceCount() == 1)
            CompleteRequest(pReq, 0);

        CTSPICallAppearance* pThisCall = pCall->GetConferenceCall(0);
        pThisCall->SetCallState(LINECALLSTATE_IDLE);
    }

    // Issue the DROP request to the switch
    GetDeviceInfo()->DRV_Conference(pAddr->GetAddressID(), -1L, CONFCOMMAND_DESTROY);
    return FALSE;    

}// CDSLine::processDropConference

//////////////////////////////////////////////////////////////////////////
// CDSLine::processRedirect
//
// Redirect an offering call to another address
//
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
            if (GetDeviceInfo()->DRV_Redirect (pAddr->GetAddressID(), pDialInfo->strNumber, dwCountryCode))
                pReq->SetState (STATE_WAITFOROFFLINE);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processCompleteCall
//
// Complete a call
//
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
                extern const char *g_ComplMsgs[5];
                CString strMessage = "";
                if (pCompleteCall->dwCompletionMode == LINECALLCOMPLMODE_MESSAGE)
                    strMessage = g_ComplMsgs[pCompleteCall->dwMessageId];
                if (GetDeviceInfo()->DRV_CompleteCall (pAddr->GetAddressID(), 
                        pCompleteCall->dwCompletionMode, strMessage))
                    pReq->SetState (STATE_WAITFORCOMPLETE);
                else
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
                if (GETADDRID(lpBuff) == (WORD)pAddr->GetAddressID())
                {   
                    // Save off data which gets deleted when CompleteRequest() occurs.                            
                    DWORD dwCompletionMode = pCompleteCall->dwCompletionMode;
                    DWORD dwCompletionID = pCompleteCall->dwCompletionID;
                    
                    // Save off the switch identifier and complete the request.
                    pCompleteCall->strSwitchInfo = "";
                    pCompleteCall->dwSwitchInfo = (DWORD) GETERROR(lpBuff);
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processPark
//
// Park a call.
//
BOOL CDSLine::processPark (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff)
{                   
    const char * pszNonDirAddr = "5551212";
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
            if (GetDeviceInfo()->DRV_Park(pAddr->GetAddressID(), strNum))
                pReq->SetState (STATE_WAITFORCOMPLETE);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processUnpark
//
// Unpark a call.
//
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
            if (GetDeviceInfo()->DRV_Unpark(pAddr->GetAddressID(), pDialInfo->strNumber))
                pReq->SetState (STATE_WAITFORCOMPLETE);
            else
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::processPickup
//
// Pickup a call.
//
BOOL CDSLine::processPickup (CTSPIRequest* pReq, WORD /*wResponse*/, const LPVOID /*lpBuff*/)
{                             
    // TODO: Implement pickup in emulator.
    CompleteRequest(pReq, LINEERR_OPERATIONFAILED);
    return FALSE;
    
}// CDSLine::processPickup

//////////////////////////////////////////////////////////////////////////
// CDSLine::ProcessAsynchDeviceResponse
//
// Process an asynchronous response from the device.
//
void CDSLine::ProcessAsynchDeviceResponse(WORD wResult, const LPVOID lpData)
{                           
    static DWORD dwCompletionID = 0L;

    // If the request is an APPEXIT notification, then the provider can no longer
    // function - our device has gone out of service.
    if (wResult == EMRESULT_APPEXIT)
    {
        // TODO: Report switch disconnect to TAPI
    }    
    
    switch (wResult)
    {   
        // An address has changed states on the device - see if it is an
        // offering call.  If so, create a new call on the address in question.
        case EMRESULT_ADDRESSCHANGED:                      
        {   
            LPEMADDRESSCHANGE lpChange = (LPEMADDRESSCHANGE) lpData;
            if (lpChange->wNewState == ADDRESSSTATE_OFFERING)
            {
                HandleNewCall ((DWORD)lpChange->wAddressID, lpChange->dwMediaModes, dwCompletionID);
                dwCompletionID = 0L;
            }   
            else
                UpdateCallState ((DWORD)lpChange->wAddressID, (int)lpChange->wNewState, (int)lpChange->wStateInfo, lpChange->dwMediaModes);
        }
        break;

        // An offering call is ringing.
        case EMRESULT_RING:
            OnRingDetected(0);
            break;
            
        // A call completion request has completed
        case EMRESULT_COMPLRESULT:
            {     
                // Locate the completion request if available.
                DWORD dwReqID = GETERROR(lpData);
                TSPICOMPLETECALL* pComplete = FindCallCompletionRequest(dwReqID, NULL);
                if (pComplete != NULL)
                {                   
                    if (pComplete->dwCompletionMode == LINECALLCOMPLMODE_CAMPON)
                    {
                        // The call for this address is about to go CONNECTed!
                        ASSERT (pComplete->pCall != NULL);
                        pComplete->pCall->GetCallInfo()->dwCompletionID = pComplete->dwCompletionID;
                        pComplete->pCall->SetCallReason (LINECALLREASON_CALLCOMPLETION);
                    }
                    else if (pComplete->dwCompletionMode == LINECALLCOMPLMODE_CALLBACK)
                        dwCompletionID = pComplete->dwCompletionID;
                    RemoveCallCompletionRequest (pComplete->dwCompletionID);
                }                   
                else
                    dwCompletionID = 0L;
            }
            break;
            
        // A tone (specific frequency) was detected on the device.
        case EMRESULT_TONEDETECTED:
            {
                LPEMTONEBUFF lpTone = (LPEMTONEBUFF) lpData;
                CTSPIAddressInfo* pAddr = GetAddress((int)lpTone->wAddressID);
                ASSERT (pAddr != NULL);
                    
                // Call must be connected or proceeding/dialing
                CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_CONNECTED);
                if (pCall == NULL)
                    pCall = pAddr->FindCallByState(LINECALLSTATE_PROCEEDING);
                if (pCall == NULL)
                    pCall = pAddr->FindCallByState(LINECALLSTATE_DIALING);
                    
                // The emulator sends the frequency as three valid entries which
                // can then be checked together.
                if (pCall != NULL)
                    pCall->OnTone (lpTone->dwFreq[0], lpTone->dwFreq[1], lpTone->dwFreq[2]);
            }
            break;
            
        // A digit was detected from the remote side - supply it to the
        // connected call appearance on the address specified for digit
        // monitoring/gathering support.
        case EMRESULT_DIGITDETECTED:
            {               
                static char cLastChar;
                LPEMDIGITBUFF lpDigit = (LPEMDIGITBUFF) lpData;
                CTSPIAddressInfo* pAddr = GetAddress((int)lpDigit->wAddressID);
                ASSERT (pAddr != NULL);
                CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_CONNECTED);
                if (pCall != NULL)
                {   
                    DWORD dwType = LINEDIGITMODE_PULSE;
                    if (lpDigit->fDTMF)
                    {   
                        // The emulator switch sends a ZERO digit to indicate
                        // silence on the line after a DTMF tone detection.  We
                        // interpret this to mean that the digit is UP and fake
                        // a DTMF END tone.
                        if (lpDigit->cDigit != (char)0)
                        {
                            dwType = LINEDIGITMODE_DTMF;
                            cLastChar = lpDigit->cDigit;
                        }
                        else
                            dwType = LINEDIGITMODE_DTMFEND;                            
                    }
                    else
                        cLastChar = lpDigit->cDigit;
                    pCall->OnDigit (dwType, cLastChar);
                }
            }
            break;
            
        // An offering call has callerID information.
        case EMRESULT_CALLERID:                      
            {
                LPEMCALLERID lpCallerInfo = (LPEMCALLERID) lpData;
                CTSPIAddressInfo* pAddr = GetAddress ((DWORD)lpCallerInfo->wAddressID);
                CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_OFFERING);
                if (pCall != NULL)
                    pCall->SetCallerIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                        lpCallerInfo->szAddressInfo, lpCallerInfo->szName);                                    
            }
            break;
            
        // An offering call was redirected from another address
        case EMRESULT_REDIRECTID:
            {
                LPEMCALLERID lpCallerInfo = (LPEMCALLERID) lpData;
                CTSPIAddressInfo* pAddr = GetAddress ((DWORD)lpCallerInfo->wAddressID);
                CTSPICallAppearance* pCall = pAddr->FindCallByState(LINECALLSTATE_OFFERING);
                if (pCall != NULL)
                {
                    // Set the call reason to REDIRECT
                    pCall->SetCallReason(LINECALLREASON_REDIRECT);
                        
                    // Move the redirecting information into the CALLED information
                    // and REDIRECTING information.  Move the original CALLED 
                    // information into the REDIRECTED information.  Since the
                    // emulator cannot redirect itself, we are guarenteed that this
                    // redirection occurred from the redirecting ID.
                    pCall->SetRedirectionIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                        pAddr->GetDialableAddress(), pAddr->GetName());
                    pCall->SetRedirectingIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                        lpCallerInfo->szAddressInfo, lpCallerInfo->szName);
                    pCall->SetCalledIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                        lpCallerInfo->szAddressInfo, lpCallerInfo->szName);
                }                                            
            }
            break;
                
        default:
            break;                
    }
    
}// CDSLine::ProcessAsynchDeviceResponse

