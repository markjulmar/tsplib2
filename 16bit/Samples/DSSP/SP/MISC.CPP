/*****************************************************************************/
//
// MISC.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains misc. functions called for the service provider.
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

//////////////////////////////////////////////////////////////////////////
// CDSLine::HandleDialEvent
//
// Master function to process a dial request.  This is called from
// both the lineMakeCall events and the lineDial events.
//
BOOL CDSLine::HandleDialEvent (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff,
                                   CADObArray* parrAddress, DWORD dwCountryCode)
{                                   
    CTSPIAddressInfo* pAddr = pReq->GetAddressInfo();
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    BOOL fProcessed = FALSE;
    
    // Get the address information to dial.                      
    DIALINFO* pDialInfo = (DIALINFO*) parrAddress->GetAt(0);
    ASSERT (pDialInfo != NULL);
                          
    switch (pReq->GetState())
    {   
        // State 1:
        // Send a dial string to the emulator device.  We only send the 
        // first address given in the object array.  Country code is
        // ignored.
        case STATE_INITIAL:           
        case STATE_DIALING:
            if (GetDeviceInfo()->DRV_Dial (pAddr->GetAddressID(), pDialInfo, dwCountryCode))
            {         
            	if (pCall->GetCallState() == LINECALLSTATE_DIALING)
            	{                                      
                	if (pDialInfo->fIsPartialAddress)
                		pCall->SetCallState(LINECALLSTATE_PROCEEDING);
                	CompleteRequest (pReq, 0);
            	}
            	else
            	{
                	pCall->SetCallState(LINECALLSTATE_DIALING);
                	pReq->SetState (STATE_WAITFORONLINE);
                }
            }
            else
                wResponse = EMRESULT_ERROR;
            fProcessed = TRUE;
            break;

        // State 2:
        // Look for the address moving to a "ONLINE" state.  This indicates
        // that we are not connected to a call, and not in dialtone, but
        // transitioning somehow between them.
        case STATE_WAITFORONLINE:
            if (wResponse == EMRESULT_ADDRESSCHANGED)
            {
                const LPEMADDRESSCHANGE lpAddrChange = (const LPEMADDRESSCHANGE) lpBuff;
                if (lpAddrChange->wAddressID == (WORD) pAddr->GetAddressID())
                {   
                    if (lpAddrChange->wNewState == ADDRESSSTATE_ONLINE)
                    {   
                        // Move the the "proceeding" state on this call.
                        if (!pDialInfo->fIsPartialAddress)
							pCall->SetCallState(LINECALLSTATE_PROCEEDING);
						else
                        	pCall->SetCallState(LINECALLSTATE_DIALING);
                        // Go ahead and complete the request.  If the call changes
                        // to something other than "ONLINE", then the Update function
                        // will change the callstate.
                        CompleteRequest (pReq, 0);
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

}// CDSLine::HandleDialEvent

/////////////////////////////////////////////////////////////////////////////
// CDSLine::UpdateCallState
//
// Update an existing call's call state for an address.  Emulator has
// changed the state of a call.
//
void CDSLine::UpdateCallState (DWORD dwAddressID, int iNewState, int iStateInfo, 
                               DWORD /*dwMediaModes*/)
{
    CTSPIAddressInfo* pAddr = GetAddress(dwAddressID);
    CTSPICallAppearance* pCall = NULL;

    // Locate the call appearance this is referring to.  It will either be a 
    // direct call appearance, or a consultation call appearance.
    if (pAddr->GetAddressStatus()->dwNumInUse == 1)
        pCall = pAddr->GetCallInfo(0);
    else
    {   
        // Look for a call in a particular state - the following is the order
        // we search in.  The emulator switch always works with the active call, i.e.
        // the connected call will always become offline *before* the onHold call
        // moves back to connected.
        static DWORD dwStateOrder[] = {
           LINECALLSTATE_CONNECTED, 
           LINECALLSTATE_PROCEEDING,
           LINECALLSTATE_ONHOLD, 
           LINECALLSTATE_ONHOLDPENDTRANSFER,
           LINECALLSTATE_ONHOLDPENDCONF, 
           LINECALLSTATE_CONFERENCED,
           LINECALLSTATE_DIALTONE,
           LINECALLSTATE_BUSY,
           -1L
        };
        
        for (int i = 0; dwStateOrder[i] != -1L; i++)
        {
            pCall = pAddr->FindCallByState(dwStateOrder[i]);
            if (pCall)
                break;
        }
    }        
    
    // All we really care about is if the call is forced offline by the
    // phone emulator - in this case, we need to do the same to our call.
    if (pCall && pCall->GetCallState() != LINECALLSTATE_IDLE)
    {   
        if (iNewState == ADDRESSSTATE_OFFLINE)
        {
            if (pCall->GetCallState() == LINECALLSTATE_CONNECTED)
                pCall->SetCallState(LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_UNAVAIL);
            pCall->SetCallState (LINECALLSTATE_IDLE);
        }
        else if (iNewState == ADDRESSSTATE_ONHOLD)
        {
            if (pCall->GetCallState() != LINECALLSTATE_ONHOLD &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDTRANSFER &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDCONF)
            {
                switch (iStateInfo)
                {
                    case HOLDTYPE_NORMAL:
                        pCall->SetCallState(LINECALLSTATE_ONHOLD);
                        break;
                    case HOLDTYPE_TRANSFER:
                        pCall->SetCallState(LINECALLSTATE_ONHOLDPENDTRANSFER);
                        break;
                    case HOLDTYPE_CONFERENCE:
                        pCall->SetCallState(LINECALLSTATE_ONHOLDPENDCONF);
                        break;                                            
                    default:
                        ASSERT (FALSE);
                        break;
                }
            }
        }
        else if (iNewState == ADDRESSSTATE_INCONF)
        {
            ASSERT (pCall->GetCallState() == LINECALLSTATE_ONHOLDPENDCONF);
            pCall->SetCallState (LINECALLSTATE_CONNECTED);
        }                                                    
        else if (iNewState == ADDRESSSTATE_CONNECT)
        {   
            // If this call isn't transitioning from a HOLDing pattern, then
            // mark the connection ID (we assume that it is transitioning from
            // DIAL or PROCEED state).
            if (pCall->GetCallState() != LINECALLSTATE_ONHOLD &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDTRANSFER &&
                pCall->GetCallState() != LINECALLSTATE_ONHOLDPENDCONF)
            {
                if (pCall->GetCallInfo()->dwReason != LINECALLREASON_DIRECT)                
                {
                    pCall->SetConnectedIDInformation (LINECALLPARTYID_NAME | LINECALLPARTYID_ADDRESS,
                                pAddr->GetDialableAddress(), pAddr->GetName());
                }
            }                               
        
            // If a conference call just reverted to two-party call due to
            // either a remove conference or an unhold, then idle the conference
            // call itself and transition the final party into the two party call.
            if (pCall->GetCallState() == LINECALLSTATE_CONNECTED && 
                pCall->GetCallType() == CALLTYPE_CONFERENCE)
            {
                CTSPIConferenceCall* pConfCall = (CTSPIConferenceCall*) pCall;
                ASSERT (pConfCall->GetConferenceCount() == 1);
                pCall = pConfCall->GetConferenceCall(0);
                pConfCall->SetCallState(LINECALLSTATE_IDLE);
                pCall->SetCallState(LINECALLSTATE_CONNECTED);
            }    
            
            // If this was a PROCEEDING call which just connected, transition to
            // the RINGBACK state for a second to see "ring events".
            else if (pCall->GetCallState() == LINECALLSTATE_PROCEEDING)
            {
                pCall->SetCallState(LINECALLSTATE_RINGBACK);
                pCall->SetCallState(LINECALLSTATE_CONNECTED);
            }
            
            // Otherwise simply transition the call
            else if (pCall->GetCallState() != LINECALLSTATE_CONNECTED)
                pCall->SetCallState(LINECALLSTATE_CONNECTED);
        }
        
        else if (iNewState == ADDRESSSTATE_DIALTONE ||
                 iNewState == ADDRESSSTATE_ONLINE ||
                 iNewState == ADDRESSSTATE_BUSY)
        {
            if (pCall->GetCallState() != g_CallStates[iNewState])
                pCall->SetCallState(g_CallStates[iNewState]);
        }
    }        
    
}// CDSLine::UpdateCallState

//////////////////////////////////////////////////////////////////////////
// CDSLine::HandleNewCall
//
// Do all the setup for a new call appearance showing up on our phone.
//
void CDSLine::HandleNewCall (DWORD dwAddressID, DWORD dwMediaModes, DWORD dwCompletionID)
{   
    // Default the media mode.
    if (dwMediaModes == 0)
        dwMediaModes = LINEMEDIAMODE_INTERACTIVEVOICE;

    // Now, if the line doesn't have anybody watching for this type of call,
    // then drop the call and don't tell TAPI about it.
    if ((GetDefaultMediaDetection() & dwMediaModes) == 0)
    {
        TRACE ("NewCall dropped, media mode=0x%lx, required=0x%lx\r\n", dwMediaModes, GetDefaultMediaDetection());
        GetDeviceInfo()->DRV_DropCall (dwAddressID);
        return;
    }

    // Create a call appearance on the address in question.
    CTSPIAddressInfo* pAddress = GetAddress (dwAddressID);
    ASSERT (pAddress != NULL);
    CTSPICallAppearance* pCall = pAddress->CreateCallAppearance (NULL, 0, 
                                    LINECALLORIGIN_INBOUND, LINECALLREASON_UNAVAIL);

    // If this has a completion ID, then mark it.
    if (dwCompletionID != 0L)
    {
        pCall->GetCallInfo()->dwCompletionID = dwCompletionID;
        pCall->SetCallReason (LINECALLREASON_CALLCOMPLETION);   
    }

    // Reset the ring counter if necessary.
    OnRingDetected (0, TRUE);

    // Report an offering call to TAPI.
    pCall->SetCallState(LINECALLSTATE_OFFERING, 0, dwMediaModes);

}// CDSLine::HandleNewCall

