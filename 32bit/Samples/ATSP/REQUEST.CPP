/******************************************************************************/
//                                                                        
// REQUEST.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all code specific to request management for this
// driver.
// 
// This service provider drives a Hayes compatible AT style modem.  It
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
/******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "atsp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CATSPLine::processAnswer
//
// Answer the phone using the modem.  This is a pretty simplistic
// implementation, it doesn't attempt to verify the call using any
// newer VOICE extensions.
//
void CATSPLine::processAnswer(CTSPIRequest* pRequest, BYTE /*bResult*/)
{
    CTSPICallAppearance* pCall = pRequest->GetCallInfo();

    if (pRequest->GetState() == STATE_INITIAL)
    {
		pRequest->SetState(STATE_IGNORE);
		if (pCall->GetCallState() == LINECALLSTATE_OFFERING &&
			SendString(GetString(IDS_ANSWER)))
		{
			pCall->SetCallState(LINECALLSTATE_CONNECTED);
			CompleteCurrentRequest(0);
		}
		else
			CompleteCurrentRequest(LINEERR_OPERATIONUNAVAIL);
	}		

}// CATSPLine::processAnswer

//////////////////////////////////////////////////////////////////////////
// CATSPLine::processMakeCall
//
// This function processes each result from the modem when the current
// command is a lineMakeCall.
//
void CATSPLine::processMakeCall(CTSPIRequest* pRequest, BYTE bResult)
{                                                  
    CTSPICallAppearance* pCall = pRequest->GetCallInfo();
    TSPIMAKECALL* lpMakeCall = (TSPIMAKECALL*) pRequest->GetDataPtr();
	DWORD dwStateData = pRequest->GetStateData();
	BOOL fTimeout = FALSE;

    switch (pRequest->GetState())
    {
        // Step 1:
		// Open and Reset the modem.
		//
		case STATE_INITIAL:
			// The device should already be open, but verify that the handle is
			// valid, and attempt a re-open if it is closed.
			if (m_hComm != INVALID_HANDLE_VALUE || OpenDevice() == TRUE)
			{
				// If the bearermode is PASSTHROUGH, then simply transition to
				// CONNECTED and release the request.
				if (pCall->GetCallInfo()->dwBearerMode == LINEBEARERMODE_PASSTHROUGH)
				{   
					CompleteCurrentRequest(0);
					pCall->SetCallState(LINECALLSTATE_CONNECTED, 0, LINEMEDIAMODE_UNKNOWN);
				}
				else
				{
					// Send the initilization string to the modem.  This should
					// setup the modem for interactive CONNECT messages, dial timeout,
					// tone/pulse, etc.  
					CString strInit = BuildInitString(lpMakeCall, FALSE);
					if (SendString(strInit))
					{
						pRequest->SetState(STATE_INITMODEM);
						pRequest->SetStateData(INITMODEM_MSEC);
					}
					else
						bResult = MODEM_ERR;
				}
			}
			break;

		case STATE_INITMODEM:
			if (bResult == MODEM_INTERVAL_TIMER)
			{
				// If we haven't seen a response from the modem, then kill the request.
				if (HIWORD(dwStateData) > MODEM_TIMEOUT)
				{
					fTimeout = TRUE;
					CompleteCurrentRequest(LINEERR_NODEVICE);
				}
				// Otherwise increment our timeout..
				else
					pRequest->SetStateData(MAKELONG(LOWORD(dwStateData), HIWORD(dwStateData)+1));
			}
			else if (bResult == MODEM_ERR)
			{
				// Initialization string failed, try a different style of INIT to catch
				// the generic HAYES style modems out there.
				switch (LOWORD(dwStateData))
				{
					// 1st INIT string failed, send one with SECONDS vs. MSECS. (USRobotics)
					case INITMODEM_MSEC: 
					{	
						if (SendString(BuildInitString(lpMakeCall, TRUE)))
						{
							bResult = MODEM_IGNORE;
							pRequest->SetStateData(MAKELONG(INITMODEM_SEC, HIWORD(dwStateData)));
						}
						break;
					}

					// 2nd INIT string failed, try simple version no timeouts.
					case INITMODEM_SEC:
						if (SendString(_T("ATE0V1S0=0\r")))
						{
							bResult = MODEM_IGNORE;
							pRequest->SetStateData(MAKELONG(INITMODEM_GENERIC, HIWORD(dwStateData)));
						}
						break;

					// All failed, no call available.
					default:
						CompleteCurrentRequest(LINEERR_NODEVICE);
						break;
				}
			}

			// Modem is successfully initialized, take it offhook
			else if (bResult == MODEM_OK)
			{
				// Send a ATD; to the modem to initiate a dial sequence.
				// This will force any HAYES compatible modem to go offhook and
				// report whether a dialtone exists.
				if (SendString(GetString(IDS_OFFHOOK)))
				{
					// Create a UI dialog on this application request.
					// It will not be shown until needed, but must be
					// created BEFORE the request is completed.
					CreateTalkDropDialog (pRequest);

					// If we have numbers to dial, then move to a dialing state.
					// otherwise, exit now in dialtone state.
					if (lpMakeCall->arrAddresses.GetSize() > 0)
						pRequest->SetState(STATE_DIALMODEM);
					else
						CompleteCurrentRequest(0);
				}
				// Failed to move offhook?
				else
					CompleteCurrentRequest(LINEERR_NODEVICE);
			}
            break;
        
        // Step 4:
        // Convert the request to a DIAL request.  Tell the call appearance
        // to generate an asynch request for DIALING and then complete this
        // request so it gets run.
        case STATE_DIALMODEM:
            if (bResult != MODEM_OK)
				break;
			else
			{
				// Complete this request as far as TAPI is concerned, 
				// but don't remove it from the queue - the lineMakeCall
				// documentation expects this to return SUCCESS -BEFORE-
				// the first LINECALLSTATE_ change for the call appearance.
				CompleteCurrentRequest(0, TRUE, FALSE);

				// Tell TAPI we have dialtone.  This will be the first callstate
				// message sent to TAPI, so give it the media mode.
				pCall->SetCallState(LINECALLSTATE_DIALTONE, LINEDIALTONEMODE_NORMAL, 
									LINEMEDIAMODE_INTERACTIVEVOICE);
			}
        
        // Fall through intentional.
                
        case STATE_TALKDROP:
        case STATE_CHECKMODEM:
        case STATE_WAITING:
        case STATE_DESTROYTD:
            if (bResult != MODEM_ERR)
                HandleDial (pRequest, bResult, &lpMakeCall->arrAddresses, lpMakeCall->dwCountryCode);
            break;
        
		// Ignore any data - call dropped
		case STATE_IGNORE:
			bResult = MODEM_UNKNOWN;
			break;

        // We should never see any more states for this type
        default:
            ASSERT(FALSE);
            break;
    }

    // If we failed somewhere, then tell TAPI
    if (bResult == MODEM_ERR)
    {
		// If we have gotten past the first stage..
		if (pRequest->GetState() != STATE_INITIAL &&
			pRequest->GetState() != STATE_INITMODEM)
        {
			// Force the modem to reset/hangup.
			if (!fTimeout)
				SendString(GetString(IDS_CR));
            DropCarrier();

			// Dismiss the UI dialog if it was created
			DismissTalkDropDialog();

			// IDLE the call if it is in some connected state
			if (pCall->GetCallState() != LINECALLSTATE_UNKNOWN &&
				pCall->GetCallState() != LINECALLSTATE_IDLE)
				pCall->SetCallState(LINECALLSTATE_IDLE);
        }   
		
		// Force the request to complete.
        CompleteCurrentRequest(LINEERR_CALLUNAVAIL);
    }   

}// CATSPLine::processMakeCall

//////////////////////////////////////////////////////////////////////////
// CATSPLine::processSetCallParams
//
// This processes a REQUEST_SETCALLPARAMS request from a 
// lineSetCallParams.  This only supports changing to/from
// a passthrough mode.
//
void CATSPLine::processSetCallParams(CTSPIRequest* pRequest, BYTE /*bResult*/)
{
    CTSPICallAppearance* pCall = pRequest->GetCallInfo();
    TSPICALLPARAMS* pCallParams = (TSPICALLPARAMS*) pRequest->GetDataPtr();

    // Support changing bearer mode to/from passthrough.    
    if (pCallParams->dwBearerMode == LINEBEARERMODE_PASSTHROUGH &&
        pCall->GetCallInfo()->dwBearerMode != LINEBEARERMODE_PASSTHROUGH)
    {
        // CallInfo will be set on success return.
        pCall->SetCallState(LINECALLSTATE_CONNECTED);
    }
    
    // Say ok so the spLIB++ moves our dial params and bearermode over to the call.
    CompleteCurrentRequest(0);
    
}// CATSPLine::processSetCallParams

//////////////////////////////////////////////////////////////////////////
// CATSPLine::processDial
//
// This function is the Hayes Modem specific routine to dial a call
// on the presented line.
//
void CATSPLine::processDial(CTSPIRequest* pReq, BYTE bResult)
{
    HandleDial (pReq, bResult, (CADObArray*) pReq->GetDataPtr(), pReq->GetDataSize());

}// CATSPLine::processDial

//////////////////////////////////////////////////////////////////////////
// CATSPLine::processDropCall
//
// This function is the Hayes Modem specific routine to drop a
// call on the presented line.
//
void CATSPLine::processDropCall(CTSPIRequest* pReq, BYTE bResult)
{
	CTSPICallAppearance* pCall = pReq->GetCallInfo();
	BOOL fIsVoiceCall = (pCall && pCall->GetCallInfo()->dwMediaMode & LINEMEDIAMODE_INTERACTIVEVOICE);

    if (pReq->GetState() == STATE_INITIAL)
	{
		// If the call is in the DISCONNECTED state then ignore it.
		if (pCall->GetCallState() == LINECALLSTATE_DISCONNECTED)
		{
			pCall->SetCallState(LINECALLSTATE_IDLE);
			CompleteRequest(pReq, 0);
			return;
		}

		// Kill any talk/drop dialog for this call.
		DismissTalkDropDialog();

		// Turn device filtering back on
		Resume();

		// If this is not a voice call, then force the modem into the command state.
		if (fIsVoiceCall == FALSE)
		{
			// Force the modem into the command state
			SendString(GetString(IDS_COMMANDSTATE));

			// Move to the next state
			pReq->SetState (STATE_CHECKMODEM);
			pReq->SetStateData(0);
		}
		else
		{
			// Force the modem to hangup.
			if (pCall != NULL)
			{
				if (pCall->GetCallState() == LINECALLSTATE_DIALTONE ||
					pCall->GetCallState() == LINECALLSTATE_DIALING)
					SendString(GetString(IDS_HANGUP));
				else if (pCall->GetCallState() != LINECALLSTATE_IDLE &&
					     pCall->GetCallState() != LINECALLSTATE_CONNECTED)
					SendString(GetString(IDS_CR));
			}
			pReq->SetState(STATE_WAITING);
		}

	}
	else if (pReq->GetState() == STATE_CHECKMODEM)
	{
		if (bResult != MODEM_OK)
		{
			DWORD dwStateData = pReq->GetStateData();
			if (++dwStateData > 5)
			{
				// Let it go through the hangup code anyway to
				// disconnect the call even if the modem doesn't seem
				// to be responding to us.
				bResult = MODEM_OK;			
			}
			else
			{
				// Reset our timeout
				pReq->SetStateData(dwStateData);

				// Force the modem into the command state
				SendString(GetString(IDS_COMMANDSTATE));
			}
		}

		if (bResult == MODEM_OK)
		{
			// If the call still exists
			if (pCall != NULL)
			{
				// If the call is OFFERING, then answer it and drop it.
				if (pCall->GetCallState() == LINECALLSTATE_OFFERING)
				{
					SendString(GetString(IDS_ANSWER));
					Sleep(1000L);
				}
				
				// Or if the call is CONNECTED, then hangup.
				else if (pCall->GetCallState() == LINECALLSTATE_CONNECTED)
					SendString(GetString(IDS_HANGUP));

				// If the call is not connected or IDLE,
				// assume the modem is in the command state.
				else if (pCall->GetCallState() != LINECALLSTATE_IDLE)
					SendString(GetString(IDS_CR));

				// If the call appearance is BUSY, then show a disconnect
				// due to BUSY.  This isn't required, but it gives more information
				// to the applications.
				if (pCall->GetCallState() == LINECALLSTATE_BUSY)
					pCall->SetCallState (LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_BUSY);
			}

			// Force any active call to drop and reset our line.
			DropCarrier();

			// Allow the next interval timer to shutdown the request.
			pReq->SetState (STATE_WAITING);
		}
	}
	else if (pReq->GetState() == STATE_WAITING)
	{
		// Idle the call before we complete so it is still valid.
		if (pCall)
			pCall->SetCallState(LINECALLSTATE_IDLE);

		// Complete the request - WARNING - since this is a DROPCALL request,
		// it may be in the process of a shutdown, which means that the device
		// may be invalid after this completes!!
		CompleteRequest(pReq, 0);
	}
    
}// CATSPLine::processDropCall

///////////////////////////////////////////////////////////////////////////
// CATSPLine::HandleDial
//
// This is a common function used for MAKECALL and DIAL to actually
// perform the dial.  Multiple addresses are not supported, so there
// should only be a single address in the array (checked by base class).
//
void CATSPLine::HandleDial (CTSPIRequest* pReq, BYTE bResult, 
                            CADObArray* parrAddresses, DWORD dwCountryCode)
{
    CTSPICallAppearance* pCall = pReq->GetCallInfo();
    DIALINFO* pDialInfo = (DIALINFO*) parrAddresses->GetAt(0);
   
    switch(pReq->GetState())
    {   
        // Step 1:
        // Send the dial string to the modem.  This should move us to
        // a DIALING state.  Again, TAPI expects the request to asynchronously
        // complete when the dialing is initiated, but we need to track the
        // request further, so leave the asynch request in the list but tell
        // TAPI is completed successfully.
        case STATE_INITIAL:
			CreateTalkDropDialog (pReq);

			// Fall through intentional.

        case STATE_DIALMODEM:
            if (SendString(BuildDialString (pCall, pDialInfo, dwCountryCode)))
            {
                pCall->SetCallState(LINECALLSTATE_DIALING);
                CompleteCurrentRequest(0, TRUE, FALSE);
                
                // If we are waiting on a DATAMODEM, then move to the proceeding
                // state since we won't get notified until either a connect or
                // no carrier.
                if (pCall->GetCallInfo()->dwMediaMode == LINEMEDIAMODE_DATAMODEM)
				{
                    pCall->SetCallState(LINECALLSTATE_PROCEEDING);
                    pReq->SetState(STATE_CHECKMODEM);
				}
				else
					pReq->SetState(STATE_TALKDROP);
            }
            break;
                
        // Step 2:
        // Call is proceeding, show a TALK/DROP dialog to allow the
		// user to pickup the handset of the phone connected to our modem.
        case STATE_TALKDROP:  
            if (bResult == MODEM_OK)
            {
                // If this is a partial, don't display the dialog until
                // we complete the number.  Therefore, this request is
                // essentially complete.  Kill the packet.
                if (pDialInfo->fIsPartialAddress)
                    CompleteCurrentRequest(0);
                else
                {
                    // Display the TALK/DROP dialog.
					ShowTalkDropDialog ();

                    // Start the busy detection.
                    pCall->SetCallState(LINECALLSTATE_PROCEEDING);
                    if (SendString(GetString(IDS_MODEMATDT)))
                    {
                        pReq->SetState(STATE_CHECKMODEM);
                        pReq->SetStateData (0);
                    }
                    else
                        bResult = MODEM_ERR;                        
                }                        
            }
            // Found a DATA response
            else if (bResult == MODEM_CONNECT)
            {   
                // Inform TAPI of a media change if necessary.
                if (pCall->GetCallInfo()->dwMediaMode == LINEMEDIAMODE_DATAMODEM)
                {
                    pCall->SetCallState (LINECALLSTATE_CONNECTED, LINECONNECTEDMODE_ACTIVE, LINEMEDIAMODE_DATAMODEM);
					Suspend();
                    CompleteCurrentRequest(0);
                }                    
            }
            break;
        
        // Step 3:
        // Wait for a response from the modem indicating either
        // (a) the call was connected
        // (b) the call is busy
		// (c) talk/drop dialog interference
        case STATE_CHECKMODEM:
            // This should be entered by a result from the modem.
            switch(bResult)
            {
                case MODEM_BUSY:
					DismissTalkDropDialog();
                    pCall->SetCallState(LINECALLSTATE_BUSY, LINEBUSYMODE_STATION);
					pCall->SetCallState (LINECALLSTATE_DISCONNECTED, LINEDISCONNECTMODE_BUSY);
					SendString(GetString(IDS_CR));
                    CompleteCurrentRequest(0);
					DropCarrier();
                    break;

                case MODEM_CONNECT:
					DismissTalkDropDialog();
                    if (pCall->GetCallInfo()->dwMediaMode != LINEMEDIAMODE_DATAMODEM)
                        pCall->OnDetectedNewMediaModes (LINEMEDIAMODE_DATAMODEM);
					pCall->SetCallState (LINECALLSTATE_CONNECTED, LINECONNECTEDMODE_ACTIVE, LINEMEDIAMODE_DATAMODEM);
					Suspend();
                    CompleteCurrentRequest(0);
                    break;

                case MODEM_OK:
                    pCall->SetCallState(LINECALLSTATE_CONNECTED, LINECONNECTEDMODE_ACTIVE);
                    break;

				case MODEM_TALKDROP_TALK:
					DismissTalkDropDialog();
					pCall->SetCallState(LINECALLSTATE_CONNECTED);
					SendString(GetString(IDS_CR));
					CompleteCurrentRequest(0);
					Suspend();
					break;

				case MODEM_TALKDROP_DROP:             
					bResult = MODEM_ERR;  // Cause it to IDLE
					break;

                default:
                    break;
            }
            break;
        
         default:
            break;
    }

    // If we failed somewhere, then tell TAPI
    if (bResult == MODEM_ERR)
    {
		pReq->SetState (STATE_IGNORE);

        // Dismiss the talk/drop dialog if it's up.
        DismissTalkDropDialog();        
        
		// If the call is not connected or IDLE,
		// assume the modem is in the command state.
		if (pCall->GetCallState() != LINECALLSTATE_CONNECTED &&
			pCall->GetCallState() != LINECALLSTATE_IDLE)
			SendString(GetString(IDS_CR));

		// Drop DTR on the line.
		DropCarrier();

        // We failed for some reason, IDLE the call and delete the request
        pCall->SetCallState(LINECALLSTATE_IDLE);
        CompleteCurrentRequest(LINEERR_CALLUNAVAIL);
    }   
   
}// CATSPLine::HandleDial

///////////////////////////////////////////////////////////////////////////
// CATSPLine::processRingEvent
//
// Manage the RING event - either create a new call or show a RING detected
// on the line.
//
void CATSPLine::processRingEvent()
{
	// Mark activity on the device.  This will be used to IDLE the call when
	// rings stop appearing.
	m_dwLastActivity = GetTickCount();

	// If the line doesn't have anybody watching for DATAMODEM support,
	// then don't bother to tell TAPI about it since it will just turn around and
	// drop the call and not inform any apps anyway.  We don't support FAX in this
	// sample driver.
	if ((GetDefaultMediaDetection() & LINEMEDIAMODE_DATAMODEM) == 0)
		return;

	// Make sure we don't have a call appearance already - if so, ignore the RING
	// since it is simply a ring for an existing call which has not been answered.
	CTSPIAddressInfo* pAddress = GetAddress((DWORD)0L);
	CTSPICallAppearance* pCall = pAddress->FindCallByState(LINECALLSTATE_OFFERING);
	if (pCall)
	{
		// Mark a RING event.
		OnRingDetected(0);
	}
	else
	{
		// No call exists, create a new call.
		pCall = pAddress->CreateCallAppearance (NULL, 0, LINECALLORIGIN_INBOUND, LINECALLREASON_UNAVAIL);

		// Reset the ring counter if necessary.
		OnRingDetected (0, TRUE);

		// Report an offering call to TAPI.  We don't know what media mode it is yet (could be voice
		// or data) so tell TAPI it is still unknown.
		pCall->SetCallState(LINECALLSTATE_OFFERING, 0, (LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_UNKNOWN));
	}
	
}// CATSPLine::processRingEvent
