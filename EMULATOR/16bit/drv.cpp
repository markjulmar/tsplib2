//*****************************************************************************
//
// DRV.CPP
//
// Service provider request management functions
//
// Change History:
// ----------------------------------------------------------------------
// 11/02/94 Mark Smith (MCS)    Initial revision.
//
//*****************************************************************************

#include "stdafx.h"
#include "pingint.h"
#include "colorlb.h"
#include "resource.h"
#include "objects.h"
#include "baseprop.h"
#include "emulator.h"
#include "call.h"
#include "dial.h"
#include "confgdlg.h"
#include "gendlg.h"
#include "addrset.h"
#include "gensetup.h"
#include <windowsx.h>

#undef SubclassWindow

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
    
//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnEmulator
//
// This method processes commands/responses from the service provider (SP).
// The SP will send the structure pointer that needs filled out (based on the command)
// in the LPARAM of the message, so no allocating or freeing is required by this app.
//
LRESULT CEmulatorDlg::OnEmulator (WPARAM wCommand, LPARAM lpData)
{                   
    // Process the message from the SP
    switch (wCommand)
    {
        //---------------------------------------------------------------------
        // Init. command from the SP
        //---------------------------------------------------------------------
        case EMCOMMAND_INIT:
            m_hwndTExe = (HWND)LOWORD(lpData);
            if (IsWindow (m_hwndTExe))
            {
                AddDebugInfo (2, "Connected to DSSP");
                SendNotification(EMRESULT_INIT, MAKELPARAM(GetSafeHwnd(), 0));
                UpdateStates();
            }        
            else
                m_hwndTExe = NULL;
            break;
            
        //---------------------------------------------------------------------
        // The SP is going to stop talking to us
        //---------------------------------------------------------------------
        case EMCOMMAND_TSPEXIT:
            AddDebugInfo (2, "Disconnected from DSSP");
            m_hwndTExe = NULL;
            UpdateStates();
            break;

        //---------------------------------------------------------------------
        // Query capabilities
        //---------------------------------------------------------------------
        case EMCOMMAND_QUERYCAPS:
            ASSERT (lpData);
            DRV_QueryCaps ((LPEMSETTINGS)lpData);
            break;

        //---------------------------------------------------------------------
        // Query the emulator version
        //---------------------------------------------------------------------
        case EMCOMMAND_GETVERSION:
            ASSERT (lpData);
            DRV_QueryVersion ((LPEMVERSIONINFO)lpData);
            break;

        //---------------------------------------------------------------------
        // Query the address information
        //---------------------------------------------------------------------
        case EMCOMMAND_GETADDRESSINFO:
            ASSERT (lpData);
            DRV_GetAddressInfo ((LPEMADDRESSINFO)lpData);
            break;

        //---------------------------------------------------------------------
        // Process a "Prepare Address" command
        //---------------------------------------------------------------------
        case EMCOMMAND_PREPAREADDR:
            DRV_PrepareAddress ((int)lpData);
            break;

        //---------------------------------------------------------------------
        // Process a "Park Call" command
        //---------------------------------------------------------------------
        case EMCOMMAND_PARKCALL:
            ASSERT (lpData);
            DRV_ParkCall ((LPEMPARKINFO)lpData);
            break;
            
        //---------------------------------------------------------------------
        // Process an "Unpark call" command
        //---------------------------------------------------------------------
        case EMCOMMAND_UNPARKCALL:
            ASSERT (lpData);
            DRV_UnparkCall ((LPEMPARKINFO)lpData);
            break;        

        //---------------------------------------------------------------------
        // Process a "Complete Call" command
        //---------------------------------------------------------------------
        case EMCOMMAND_COMPLETECALL:
            ASSERT (lpData);
            DRV_CompleteCall ((LPEMCOMPLETECALL)lpData);
            break;

        //---------------------------------------------------------------------
        // Process a "Dial" command
        //---------------------------------------------------------------------
        case EMCOMMAND_DIAL:
            ASSERT (lpData);
            DRV_Dial ((LPEMADDRESSINFO)lpData);
            break;

        //---------------------------------------------------------------------
        // Process a Drop Call (release) command
        //---------------------------------------------------------------------
        case EMCOMMAND_DROPCALL:
            DRV_DropCall ((int)lpData);
            break;
        
        //---------------------------------------------------------------------
        // Process an answer request from the service provider
        //---------------------------------------------------------------------
        case EMCOMMAND_ANSWER:
            DRV_Answer ((int)lpData);
            break;        

        //---------------------------------------------------------------------
        // Process a forwarding request from the service provider
        //---------------------------------------------------------------------
        case EMCOMMAND_FORWARD:
            DRV_Forward ((LPEMFORWARDINFO)lpData);
            break;        

        //---------------------------------------------------------------------
        // Process a redirection request from the service provider
        //---------------------------------------------------------------------
        case EMCOMMAND_REDIRECT:
            DRV_Redirect ((LPEMFORWARDINFO)lpData);
            break;

        //---------------------------------------------------------------------
        // Set our current ringer mode
        //---------------------------------------------------------------------
        case EMCOMMAND_SETRINGMODE:
            SetRingStyle ((WORD)lpData);
            break;

        //---------------------------------------------------------------------
        // Process a conference request from the service provider
        //---------------------------------------------------------------------
        case EMCOMMAND_CONFERENCE:
            DRV_Conference ((LPEMCONFERENCEINFO)lpData);
            break;        

        //---------------------------------------------------------------------
        // Process a hold request from the service provider
        //---------------------------------------------------------------------
        case EMCOMMAND_HOLDCALL:
            DRV_HoldCall ((int)lpData);
            break;        

        //---------------------------------------------------------------------
        // Process a hookswitch flash
        //---------------------------------------------------------------------
        case EMCOMMAND_FLASH:
            DRV_Flash ((int)lpData);
            break;

        //---------------------------------------------------------------------
        // Process an unhold request from the service provider
        //---------------------------------------------------------------------
        case EMCOMMAND_UNHOLDCALL:
            DRV_UnholdCall ((int)lpData);
            break;        

        //---------------------------------------------------------------------
        // Process a transfer request from the service provider
        //---------------------------------------------------------------------
        case EMCOMMAND_TRANSFER:
            DRV_Transfer ((LPEMTRANSFERINFO)lpData);
            break;        

        //---------------------------------------------------------------------
        // Phone vol/mic level change
        //---------------------------------------------------------------------
        case EMCOMMAND_SETLEVEL:
            DRV_SetLevel ((LPEMLEVELCHANGE)lpData);
            break;

        //---------------------------------------------------------------------
        // Phone mic hookswitch state change
        //---------------------------------------------------------------------
        case EMCOMMAND_SETHOOKSWITCH:
            DRV_SetHookswitch ((LPEMHOOKSWITCHCHANGE)lpData);
            break;

        //---------------------------------------------------------------------
        // Unknown request
        //---------------------------------------------------------------------
        default:
            AddDebugInfo (2, "SP Command Unknown");
            break;
    }

    return 0L;

}// CEmultorDlg::OnEmulator

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_QueryCaps
//
// Fill in the EMSETTINGS structure to return to the SP
//
void CEmulatorDlg::DRV_QueryCaps (LPEMSETTINGS lpSettings)
{                    
	CMfxString strBuff;
	strBuff.Sprintf("SP Query Caps <0x%lx %s>", (DWORD)lpSettings, (IsBadWritePtr(lpSettings, sizeof(EMSETTINGS)) == TRUE) ? "BAD" : "OK");
    AddDebugInfo (0, (LPCSTR)strBuff);

    if (lpSettings && !IsBadWritePtr(lpSettings, sizeof(EMSETTINGS)))
    {
        // Set the total number of call appearances available
        lpSettings->wAddressCount = (WORD)m_arrAppearances.GetSize();

        // Set the switchhook state
        if (! m_fSpeaker && ! m_fMicrophone)
            lpSettings->wHandsetHookswitch = HSSTATE_ONHOOK;
        else if (m_fSpeaker && ! m_fMicrophone)
            lpSettings->wHandsetHookswitch = HSSTATE_OFFHOOKSPEAKER;
        else if (! m_fSpeaker && m_fMicrophone)
            lpSettings->wHandsetHookswitch = HSSTATE_OFFHOOKMIC;
        else
            lpSettings->wHandsetHookswitch = HSSTATE_OFFHOOKMICSPEAKER;

        // Get the volume and gain settings
        lpSettings->wVolHandset = (WORD) m_iVolumeLevel;
        lpSettings->wGainHandset = (WORD) m_iGainLevel;
        lpSettings->wRingMode = (WORD) m_cbRinger.GetCurSel();

        // Put all our button information into place.
        for (int i = 0 ; i < BUTTON_COUNT; i++)
        {   
            CEmulButton* pButton = GetButton(i);
            lpSettings->wButtonModes[i] = (WORD) pButton->m_iButtonFunction;
            if (pButton->m_pLampWnd)
                lpSettings->wLampStates[i] = (WORD) pButton->m_pLampWnd->m_iState;
            else
                lpSettings->wLampStates[i] = LAMPSTATE_NONE;
        }

        // Get the current display text
        CopyDisplay (lpSettings->szDisplay); 

        // Get the user name info.
        CString strName = AfxGetApp()->GetProfileString ("General", "Name", "");
        strncpy (lpSettings->szOwner, (LPCSTR)strName, OWNER_SIZE);
    }

}// CEmulatorDlg::DRV_QueryCaps

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_QueryVersion
//
// Fill in the EMVERSIONINFO structure to return to the SP
//
void CEmulatorDlg::DRV_QueryVersion (LPEMVERSIONINFO lpEMVersion)
{
    AddDebugInfo (0, "SP Query Version");

    if (lpEMVersion && !IsBadWritePtr(lpEMVersion, sizeof(EMVERSIONINFO)))
    {
        lpEMVersion->dwVersion = HIWORD(2) + LOWORD(0);
        CString strSwInfo = AfxGetApp()->GetProfileString ("General", "SwInfo", "");
        if (strSwInfo.IsEmpty() == FALSE)
        	strncpy (lpEMVersion->szSwitchInfo, (LPCSTR)strSwInfo, SWITCHINFO_SIZE);
    }

}// CEmulatorDlg::DRV_QueryVersion

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_GetAddressInfo
// 
// Return the address information for the specified address
// 
void CEmulatorDlg::DRV_GetAddressInfo (LPEMADDRESSINFO lpAddrInfo)
{
    CMfxString strMsg;
    int iID = -1;
    
    if (lpAddrInfo && !IsBadWritePtr(lpAddrInfo, sizeof(EMADDRESSINFO)))
    {
        // The wAddressID contains the index for the address the SP wants info. on
        iID = (int)lpAddrInfo->wAddressID;
        memset (lpAddrInfo, 0, sizeof(EMADDRESSINFO));
        
        strMsg.Sprintf("SP Get AddressInfo <ID = %d>", iID);
    	AddDebugInfo (0, (LPCSTR)strMsg);
        
        CAddressAppearance * pAddrApp = GetAddress (iID);
        if (pAddrApp)
        {
            lpAddrInfo->wAddressID = (WORD) iID;
            strncpy (lpAddrInfo->szAddress, (LPCSTR)pAddrApp->m_strNumber, ADDRESS_SIZE);
            lpAddrInfo->wAddressState = (WORD)pAddrApp->m_Call.m_iState;
            lpAddrInfo->wStateInfo = (WORD) pAddrApp->m_Call.m_iStateInfo;
            lpAddrInfo->dwMediaMode = pAddrApp->m_Call.m_dwMediaModes;
        }
    }

}// CEmulatorDlg::DRV_GetAddressInfo

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_PrepareAddress
//
// Prepare an address for an outgoing call.
//
void CEmulatorDlg::DRV_PrepareAddress (int iAddressID)
{
    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP PrepareAddr <ID = %d>", iAddressID);
    AddDebugInfo (0, (LPCSTR)strMsg);
    
    // Release the SP thread that sent the message
    ReleaseTSPThread();

    // See if we have an address for the ID passed
    CAddressAppearance * pAddrApp = GetAddress (iAddressID);
    
    // Check the address ID to make sure the address is idle
    if (pAddrApp == NULL || pAddrApp->m_Call.m_iState != ADDRESSSTATE_OFFLINE)
    {
        // Send the correct error response
        LPARAM lParam;
        if (pAddrApp == NULL)
            lParam = MAKEERR(iAddressID, EMERROR_INVALADDRESSID);
        else
            lParam = MAKEERR(iAddressID, EMERROR_INVALADDRESSSTATE);
        SendNotification(EMRESULT_ERROR, lParam);
        return;
    }

    // Set the address to the dialtone state
    pAddrApp->SetState (ADDRESSSTATE_DIALTONE, DIALTONETYPE_INTERNAL);
    CEmulButton* pButton = GetButton(pAddrApp->m_iButtonIndex);
    pButton->SetLampState(LAMPSTATE_STEADY);
    
    // Turn on the MIC/SPEAKER.
    SetHookSwitch (HSDEVICE_HANDSET, HSSTATE_OFFHOOKMICSPEAKER);
    
    UpdateStates();

}// CEmulatorDlg::DRV_PrepareAddress

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_Dial
//
// Process a dial request from the service provider.
// 
void CEmulatorDlg::DRV_Dial (LPEMADDRESSINFO lpAddrInfo)
{
    if (IsBadReadPtr(lpAddrInfo, sizeof(EMADDRESSINFO)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP Dial <ID = %d>", (int)lpAddrInfo->wAddressID);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a local copy if the structure before releasing the SP thread
    EMADDRESSINFO sAddrInfo;
    memcpy (&sAddrInfo, lpAddrInfo, sizeof (EMADDRESSINFO));

    // Release the SP thread that sent the message
    ReleaseTSPThread();

    // Call the worker to dial the number
    int iRC = DialNumber (sAddrInfo.wAddressID, sAddrInfo.szAddress);
    if (iRC)
        SendNotification(EMRESULT_ERROR, MAKEERR(sAddrInfo.wAddressID, iRC));

}// CEmulatorDlg::DRV_Dial

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_Answer
//
// Answer an offering call appearance.  It will transition to the
// "connected" state.
//
void CEmulatorDlg::DRV_Answer (int iAddressID)
{                           
    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP AnswerCall <ID = %d>", iAddressID);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Release the SP thread that sent the message
    ReleaseTSPThread();

    // Kill the ring timer
    KillTimer (TIMER_RING);
    
    // Grab the address appearance to answer
    CAddressAppearance* pAddr = GetAddress(iAddressID);
    if (pAddr == NULL || pAddr->m_Call.m_iState != ADDRESSSTATE_OFFERING)
    {
        // Send the correct error response
        LPARAM lParam;
        if (pAddr == NULL)
            lParam = MAKEERR(iAddressID, EMERROR_INVALADDRESSID);
        else
            lParam = MAKEERR(iAddressID, EMERROR_INVALADDRESSSTATE);
        SendNotification (EMRESULT_ERROR, lParam);
        return;
    }
    
    // If there are any active calls, then destroy them.
    for (int i = 0; i < m_arrAppearances.GetSize(); i++)
    {                                                 
        CAddressAppearance* pOldAddr = GetAddress(i);
        if (pOldAddr != pAddr)
        {
            if (pOldAddr->m_Call.IsActive())
            {
                pOldAddr->SetState(ADDRESSSTATE_OFFLINE);
                CEmulButton* pButton = GetButton(pOldAddr->m_iButtonIndex);
                pButton->SetLampState(LAMPSTATE_OFF);
            }                
        }
    }
    
    // Move it to the CONNECTED state and adjust the indicator appropriately.
    pAddr->SetState (ADDRESSSTATE_CONNECT);
    CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
    pButton->SetLampState(LAMPSTATE_STEADY);

    UpdateStates();
    
}// CEmulatorDlg::DRV_Answer

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_DropCall
// 
// Drop an existing active call
//
void CEmulatorDlg::DRV_DropCall (int iAddressID)
{
    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP DropCall <ID = %d>", iAddressID);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Release the SP thread that sent the message
    ReleaseTSPThread();

    // Call the worker to drop the call
    int iRC = PerformDrop(iAddressID);
    if (iRC)
        SendNotification (EMRESULT_ERROR, MAKEERR(iAddressID, iRC));

}// CEmulatorDlg::DRV_DropCall

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_HoldCall
//
// Place a connected call onhold
//
void CEmulatorDlg::DRV_HoldCall(int iAddressID)
{
    CMfxString strMsg;
    strMsg.Sprintf("SP HoldCall <ID = %d>", iAddressID);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Release the SP thread that sent the message
    ReleaseTSPThread();
    
    // Call the worker to hold the call
    int iRC = PerformHold (iAddressID);
    if (iRC)
        SendNotification (EMRESULT_ERROR, MAKEERR(iAddressID, iRC));

}// CEmulatorDlg::DRV_HoldCall 

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_UnholdCall
//
// Remove a call from HOLD
//
void CEmulatorDlg::DRV_UnholdCall(int iAddressID)
{
    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP UnholdCall <ID = %d>", iAddressID);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Release the SP thread that sent the message
    ReleaseTSPThread();

    // Call the worker to hold the call
    int iRC = PerformUnhold (iAddressID);
    if (iRC)
        SendNotification (EMRESULT_ERROR, MAKEERR(iAddressID, iRC));

}// CEmulatorDlg::DRV_UnholdCall

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_Forward
//
// Process a forwarding request from the service provider.
//
// In this release, we only support ALL addresses being forwarded.
//
void CEmulatorDlg::DRV_Forward(LPEMFORWARDINFO lpForwardInfo)
{                            
    if (IsBadReadPtr(lpForwardInfo, sizeof(EMFORWARDINFO)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP Forward <%s>", lpForwardInfo->szAddress);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the forwarding request
    EMFORWARDINFO fwdInfo;
    memcpy (&fwdInfo, lpForwardInfo, sizeof(EMFORWARDINFO));
    lpForwardInfo = &fwdInfo;
    
    // Release the TSP thread    
    ReleaseTSPThread();

    CEmulButton* pButton = FindButtonByFunction (BUTTONFUNCTION_FORWARD);
    if (pButton == NULL)
    {
        SendNotification (EMRESULT_ERROR, MAKEERR(-1, EMERROR_INVALIDFUNCTION));
        return;
    }
    
    // Verify that ALL addresses are listed.
    if (lpForwardInfo->wAddressID != -1)
    {
        SendNotification (EMRESULT_ERROR, MAKEERR(-1, EMERROR_INVALADPARAM));
        return;
    }
    
    // If this is a request to UNFORWARD the phone, then do so.
    if (lpForwardInfo->szAddress[0] == '\0')
    {                                       
        if (m_iActiveFeature == Forward)
        {
            m_iActiveFeature = None;
            m_strFeatureData.Empty();
        }
        pButton->SetLampState(LAMPSTATE_OFF);
    }
    else
    {
        // There can be no active call appearances.
        for (int i = 0; i < m_arrAppearances.GetSize(); i++)
        {
            CAddressAppearance* pAddr = GetAddress(i);
            if (pAddr->m_Call.IsActive())
            {   
                SendNotification (EMRESULT_ERROR, MAKEERR(-1, EMERROR_INVALADDRESSSTATE));
                return;
            }
        }
        
        m_iActiveFeature = Forward;
        m_strFeatureData = lpForwardInfo->szAddress;
        pButton->SetLampState(LAMPSTATE_BLINKING);
    }

    ResetDisplay (TRUE);
    UpdateStates();
    
}// CEmulatorDlg::DRV_Forward

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_Transfer
//
// Perform blind and consultation transfer for the provider
//
void CEmulatorDlg::DRV_Transfer (LPEMTRANSFERINFO lpTransInfo)
{   
    if (IsBadReadPtr(lpTransInfo, sizeof(EMTRANSFERINFO)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP Transfer <%d %s>", lpTransInfo->wAddressID, lpTransInfo->szAddress);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Copy the transfer information over                          
    EMTRANSFERINFO tranInfo;
    memcpy(&tranInfo, lpTransInfo, sizeof(EMTRANSFERINFO));
    lpTransInfo = &tranInfo;

    // Release the TSP thread    
    ReleaseTSPThread();

    // Get the address we are transferring    
    CAddressAppearance * pAddr = GetAddress (lpTransInfo->wAddressID);
    if (pAddr == NULL)
    {
        SendNotification (EMRESULT_ERROR, EMERROR_INVALADDRESSID);
        return;
    }
    
    // Locate the transfer button.  If it doesn't exist, exit out with an error.
    CEmulButton* pTransButton = FindButtonByFunction(BUTTONFUNCTION_TRANSFER);
    if (pTransButton == NULL)
    {
        SendNotification (EMRESULT_ERROR, MAKEERR(-1, EMERROR_INVALIDFUNCTION));
        return;
    }
        
    // Now, determine what our 1st step is.  If a number is listed, then
    // this is a straight transfer to a known number.
    if (lpTransInfo->szAddress[0] != '\0')
    {   
        // Call must be in an ACTIVE state (CONNECT,ONLINE) to be transferred.
        if (!pAddr->m_Call.IsActive())
        {
            SendNotification (EMRESULT_ERROR, EMERROR_INVALADDRESSSTATE);
            return;
        }
        
        // Transition the call to ONHOLD PENDING TRANSFER then idle it.
        pAddr->SetState(ADDRESSSTATE_ONHOLD, HOLDTYPE_TRANSFER);
        pAddr->SetState(ADDRESSSTATE_OFFLINE);        
        
        // Turn off the button lamp
        CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
        pButton->SetLampState(LAMPSTATE_OFF);
        
        // Turn off the hookswitch
        SetHookSwitch (HSDEVICE_HANDSET, HSSTATE_ONHOOK);
        ResetDisplay(TRUE);
    }
    
    // Otherwise, this is a consultation transfer.    
    else
    {
        // Check to see if this is a two address transfer (CROSS-ADDRESS)
        if (lpTransInfo->wTransferAddress != -1 &&
            lpTransInfo->wTransferAddress != lpTransInfo->wAddressID)
        {
            // Get the call appearance for the destination address
            CAddressAppearance * pAddrDst = GetAddress (lpTransInfo->wTransferAddress);
            if (pAddrDst == NULL)
            {
                SendNotification (EMRESULT_ERROR,
                                  MAKEERR(lpTransInfo->wTransferAddress,
                                  EMERROR_INVALADDRESSID));
                return;
            }   
            
            // Verify that the source address is in the ONHOLD state
            if (pAddr->m_Call.m_iState != ADDRESSSTATE_ONHOLD)
            {
                SendNotification (EMRESULT_ERROR,
                                  MAKEERR(lpTransInfo->wAddressID,
                                  EMERROR_INVALADDRESSSTATE));
                return;
            }
            
            // Verify that the destination address is either in the
            // CONNECT or ONLINE state
            if (pAddrDst->m_Call.m_iState != ADDRESSSTATE_CONNECT &&
                pAddrDst->m_Call.m_iState != ADDRESSSTATE_ONLINE)
            {
                SendNotification (EMRESULT_ERROR,
                                  MAKEERR(lpTransInfo->wTransferAddress,
                                  EMERROR_INVALADDRESSSTATE));
                return;
            }

            // Turn off the transfer lamp if it's on.
            pTransButton->SetLampState (LAMPSTATE_OFF);
            
            // Moving to a conference?
            if (lpTransInfo->fMoveToConference)
            {
                // Add the address to our conference list.
                CString strAddr = pAddr->m_Call.m_strDestNumber;
                pAddr->m_lstConf.AddTail(strAddr);
                wsprintf (strAddr.GetBuffer(20), "@%02d", pAddrDst->m_wAddressID);
                strAddr.ReleaseBuffer();
                pAddr->m_lstConf.AddTail(strAddr);
                
                // Setup the other side of the link.
                strAddr = pAddrDst->m_Call.m_strDestNumber;
                pAddrDst->m_lstConf.AddTail (strAddr);
                wsprintf (strAddr.GetBuffer(20), "@%02d", pAddr->m_wAddressID);
                strAddr.ReleaseBuffer();
                pAddrDst->m_lstConf.AddTail(strAddr);
                
                // Transition BOTH calls to in-conferenced state.
                pAddr->SetState(ADDRESSSTATE_INCONF);
                CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
                pButton->SetLampState (LAMPSTATE_STEADY);

                pAddrDst->SetState (ADDRESSSTATE_INCONF);
                pButton = GetButton (pAddrDst->m_iButtonIndex);                
                pButton->SetLampState (LAMPSTATE_STEADY);
                
                // Turn on the conference button.
                pButton = FindButtonByFunction (BUTTONFUNCTION_CONFERENCE);
                if (pButton)
                    pButton->SetLampState (LAMPSTATE_BLINKING);

                // Reset the display
                ShowDisplay (pAddr);
            }
            else // COMPLETING TRANSFER CROSS-ADDRESS
            {
                // Error checking out of the way, we can now idle both addresses and
                // turn the lamps off
                pAddr->SetState (ADDRESSSTATE_OFFLINE);
                CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
                pButton->SetLampState (LAMPSTATE_OFF);

                // If the destination address was in a CONNECT state, change it to
                // DISCONNECT before setting it to idle
                if (pAddrDst->m_Call.m_iState == ADDRESSSTATE_CONNECT)
                    pAddrDst->SetState (ADDRESSSTATE_DISCONNECT);
                pAddrDst->SetState (ADDRESSSTATE_OFFLINE);
                
                // Turn off the destination lamp
                CEmulButton * pButtonDst = GetButton (pAddrDst->m_iButtonIndex);
                pButtonDst->SetLampState (LAMPSTATE_OFF);
                
                // Turn off the hookswitch
                SetHookSwitch (HSDEVICE_HANDSET, HSSTATE_ONHOOK);
                ResetDisplay (TRUE);
            }                
        }
        
        // If this is the first time we have heard about this, then move
        // the call info to our consultation call data and mark it onHold.
        else if (pAddr->m_ConsultationCall.m_iState == ADDRESSSTATE_UNKNOWN &&
                 lpTransInfo->fMoveToConference == FALSE)
        {   
            pTransButton->SetLampState (LAMPSTATE_BLINKING);
            pAddr->m_Call.m_iLastState = pAddr->m_Call.m_iState;
            pAddr->SetState (ADDRESSSTATE_ONHOLD, HOLDTYPE_TRANSFER);
            pAddr->m_ConsultationCall = pAddr->m_Call;
            pAddr->SetState (ADDRESSSTATE_DIALTONE, DIALTONETYPE_INTERNAL);
            ShowDisplay (pAddr);
        }
        
        // Otherwise, final stage is completing a SINGLE-ADDRESS TRANSFER
        else
        {
            ASSERT (pAddr->m_ConsultationCall.m_iState == ADDRESSSTATE_ONHOLD);
            
            // Turn off the transfer lamp if it's on.
            pTransButton->SetLampState (LAMPSTATE_OFF);
            
            // If we are moving into a conference, then it is a single address conference.
            if (lpTransInfo->fMoveToConference)
            {
                // Add both addresses to our conference array
                CString strAddr = pAddr->m_Call.m_strDestNumber;
                pAddr->m_lstConf.AddTail (strAddr);
                strAddr = pAddr->m_ConsultationCall.m_strDestNumber;
                pAddr->m_lstConf.AddTail (strAddr);
                
                // Move to the CONFERENCED state
                pAddr->SetState(ADDRESSSTATE_INCONF);
                CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
                pButton->SetLampState (LAMPSTATE_STEADY);
                
                // Turn on the conference button.
                pButton = FindButtonByFunction (BUTTONFUNCTION_CONFERENCE);
                if (pButton)
                    pButton->SetLampState (LAMPSTATE_BLINKING);
            } 
            // Completing a SINGLE-ADDRESS transfer
            else
            {
                pAddr->SetState (ADDRESSSTATE_OFFLINE);
                pAddr->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
                EmptyCallInfo (lpTransInfo->wAddressID);
                CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
                pButton->SetLampState(LAMPSTATE_OFF);
                SetHookSwitch (HSDEVICE_HANDSET, HSSTATE_ONHOOK);
                ResetDisplay (TRUE);
            }                
        }
    }
    
    // Update our button states
    UpdateStates();

}// CEmulatorDlg::DRV_Transfer

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_Flash
//
// Process a "HOOKSWITCH" flash event
//
void CEmulatorDlg::DRV_Flash (int iAddressID)
{                          
    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP Flash <%d>", iAddressID);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Release the TSP thread
    ReleaseTSPThread();
                           
    // Get the address we are transferring or conferenced to
    CAddressAppearance * pAddr = GetAddress (iAddressID);
    if (pAddr == NULL)
    {
        SendNotification (EMRESULT_ERROR, EMERROR_INVALADDRESSID);
        return;
    }

    // If we don't have a consultation call, then ignore this.
    if (pAddr->m_ConsultationCall.m_iState == ADDRESSSTATE_UNKNOWN)
        return;

    // If the connected call is in an illegal state, then return an error.
    if (pAddr->m_Call.m_iState == ADDRESSSTATE_OFFLINE ||
        pAddr->m_Call.m_iState == ADDRESSSTATE_DISCONNECT ||
        pAddr->m_Call.m_iState == ADDRESSSTATE_ONHOLD)
    {
        SendNotification (EMRESULT_ERROR, EMERROR_INVALADDRESSSTATE);
        return;
    }        
                           
    // Otherwise, record a swap of the two - signal that it goes on hold
    // followed by the state of the original call.
    pAddr->SetState(ADDRESSSTATE_ONHOLD, HOLDTYPE_NORMAL);
    CAddrCall CallInfo;
    CallInfo = pAddr->m_Call;
    pAddr->m_Call = pAddr->m_ConsultationCall;
    pAddr->m_ConsultationCall = CallInfo;
    pAddr->SetState(pAddr->m_Call.m_iLastState);
    
    // Update the display
    ShowDisplay (pAddr);
    UpdateStates();
    
}// CEmulatorDlg::DRV_Flash

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_SetLevel
//
// Set the level of the volume/gain
//
void CEmulatorDlg::DRV_SetLevel(LPEMLEVELCHANGE lpChange)
{   
    if (IsBadReadPtr(lpChange, sizeof(EMLEVELCHANGE)))
        return;
                          
    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP SetLevel <%s 0x%x>", 
            (lpChange->wLevelType == LEVELTYPE_MIC) ? "Mic" : "Spkr", lpChange->wLevel);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the data structures involved.
    EMLEVELCHANGE emChange;
    memcpy (&emChange, lpChange, sizeof(EMLEVELCHANGE));

    // Release the TSP thread
    ReleaseTSPThread();

    // Change the level of the appropriate device
    switch (emChange.wLevelType)
    {
        case LEVELTYPE_MIC:
            OnGainChanged (emChange.wLevel);
            break;
            
        case LEVELTYPE_SPEAKER:
            OnVolumeChanged (emChange.wLevel);
            break;
            
        default:
            SendNotification (EMRESULT_ERROR, MAKEERR(-1, EMERROR_INVALADPARAM));
            break;
    }    

}// CEmulatorDlg::DRV_SetLevel

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_SetHookswitch
//
// Set the state of the MIC hookswitch.
//
void CEmulatorDlg::DRV_SetHookswitch (LPEMHOOKSWITCHCHANGE lpChange)
{                                  
    if (IsBadReadPtr(lpChange, sizeof(EMHOOKSWITCHCHANGE)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP SetHookswitch <MIC=%s>", 
            (lpChange->wHookswitchState >= HSSTATE_OFFHOOKMICSPEAKER && 
            lpChange->wHookswitchState <= HSSTATE_OFFHOOKMIC) ?
            "On" : "Off");
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the data structures involved.
    EMHOOKSWITCHCHANGE emChange;
    memcpy (&emChange, lpChange, sizeof(EMHOOKSWITCHCHANGE));

    // Release the TSP thread
    ReleaseTSPThread();

    // Turn the mic on or off based on what the hookswitch is right now.
    ASSERT (emChange.wHookswitchID == HSDEVICE_HANDSET);
    
    UpdateData(TRUE);
    
    if (emChange.wHookswitchState >= HSSTATE_OFFHOOKMICSPEAKER &&
        emChange.wHookswitchState <= HSSTATE_OFFHOOKMIC)
        SetHookSwitch (emChange.wHookswitchID, (m_fSpeaker) ? HSSTATE_OFFHOOKMICSPEAKER : HSSTATE_OFFHOOKMIC);
    else
        SetHookSwitch (emChange.wHookswitchID, (m_fSpeaker) ? HSSTATE_OFFHOOKSPEAKER : HSSTATE_ONHOOK);        

}// CEmulatorDlg::DRV_SetHookswitch

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_Conference
//
// Manage the simple conference support in the emulator.
//
void CEmulatorDlg::DRV_Conference(LPEMCONFERENCEINFO lpConfInfo)
{   
    if (IsBadReadPtr(lpConfInfo, sizeof(EMCONFERENCEINFO)))
        return;
                            
    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP Conference <%d, %s>", lpConfInfo->wAddressID, 
                    (lpConfInfo->wCommand == CONFCOMMAND_ADD) ? "Add" : "Remove");
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the data structures involved.
    EMCONFERENCEINFO ConfInfo;
    memcpy (&ConfInfo, lpConfInfo, sizeof(EMCONFERENCEINFO));
    lpConfInfo = &ConfInfo;

    // Release the TSP thread
    ReleaseTSPThread();
    
    // Locate the conference button.
    CEmulButton* pConfButton = FindButtonByFunction(BUTTONFUNCTION_CONFERENCE);
    if (pConfButton == NULL)
    {
        SendNotification (EMRESULT_ERROR, MAKEERR(-1, EMERROR_INVALIDFUNCTION));
        return;
    }

    // Get the address specified
    CAddressAppearance* pAddr = GetAddress(ConfInfo.wAddressID);
    if (pAddr == NULL)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(ConfInfo.wAddressID, 
                    EMERROR_INVALADDRESSID));
        return;
    }

    // Check the command
    if (lpConfInfo->wCommand == CONFCOMMAND_ADD)
    {
        // There are two states for an add.  The first is if we have no consultation
        // call currently - i.e. a conference has NOT been established.
        if (pAddr->m_ConsultationCall.m_iState == ADDRESSSTATE_UNKNOWN)
        {   
            if (pAddr->m_Call.m_iState != ADDRESSSTATE_CONNECT &&
                pAddr->m_Call.m_iState != ADDRESSSTATE_INCONF)
            {
                SendNotification (EMRESULT_ERROR, 
                            MAKEERR(ConfInfo.wAddressID, 
                            EMERROR_INVALADDRESSSTATE));
                return;
            }
            
            // If this is the FIRST addition to our conference, then add the initial address
            // to our address array
            if (pAddr->m_lstConf.IsEmpty())
            {
                CString strAddress = pAddr->m_Call.m_strDestNumber;
                pAddr->m_lstConf.AddTail (strAddress);
            }
            
            // Establish the consultation call presense on our switch.
            pAddr->m_Call.m_iLastState = pAddr->m_Call.m_iState;
            pAddr->SetState (ADDRESSSTATE_ONHOLD, HOLDTYPE_CONFERENCE);
            pAddr->m_ConsultationCall = pAddr->m_Call;
            pAddr->SetState (ADDRESSSTATE_DIALTONE, DIALTONETYPE_INTERNAL);
            pConfButton->SetLampState(LAMPSTATE_BLINKING);
        }
        else // CONFERENCE
        {   
            // SINGLE-ADDRESS conference?              
            if (ConfInfo.wConfAddress == (WORD) -1 ||
                ConfInfo.wConfAddress == ConfInfo.wAddressID)
            {
                // In this case, we have established a consultation call, now conference
                // the two together.
                if (pAddr->m_ConsultationCall.m_iState != ADDRESSSTATE_ONHOLD ||
                    pAddr->m_ConsultationCall.m_iStateInfo != HOLDTYPE_CONFERENCE)
                {
                    SendNotification (EMRESULT_ERROR, 
                                MAKEERR(ConfInfo.wAddressID, 
                                EMERROR_INVALADDRESSSTATE));
                    return;
                }   
        
                // Grab the current address information, add it to our conference address
                // array.
                CString strAddr = pAddr->m_Call.m_strDestNumber;
            
                // Move the consultation call information back into the call.
                pAddr->m_Call = pAddr->m_ConsultationCall;
                pAddr->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
            
                // Add the conferenced address
                pAddr->m_lstConf.AddTail (strAddr);
        
                // Perform the conference notification - this moves the conference into the
                // CONNECTED state.
                pAddr->SetState(ADDRESSSTATE_INCONF);
                CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
                pButton->SetLampState(LAMPSTATE_STEADY);
            }
            else // CROSS-ADDRESS conference
            {                               
                // Get the call appearance for the destination address
                CAddressAppearance * pAddrDst = GetAddress (ConfInfo.wConfAddress);
                if (pAddrDst == NULL)
                {
                    SendNotification (EMRESULT_ERROR,
                                      MAKEERR(ConfInfo.wConfAddress,
                                      EMERROR_INVALADDRESSID));
                    return;
                }   
            
                // Verify that the source address is in the ONHOLD state
                if (pAddr->m_Call.m_iState != ADDRESSSTATE_ONHOLD)
                {
                    SendNotification (EMRESULT_ERROR,
                                      MAKEERR(ConfInfo.wAddressID,
                                      EMERROR_INVALADDRESSSTATE));
                    return;
                }
            
                // Verify that the destination address is either in the
                // CONNECT or ONLINE state
                if (pAddrDst->m_Call.m_iState != ADDRESSSTATE_CONNECT &&
                    pAddrDst->m_Call.m_iState != ADDRESSSTATE_ONLINE)
                {
                    SendNotification (EMRESULT_ERROR,
                                      MAKEERR(ConfInfo.wConfAddress,
                                      EMERROR_INVALADDRESSSTATE));
                    return;
                }

                // Add the destination number to our conference array pointing at the
                // address.
                CString strAddress;         
                wsprintf (strAddress.GetBuffer(20), "@%02d", ConfInfo.wConfAddress);
                strAddress.ReleaseBuffer();
                pAddr->m_lstConf.AddTail (strAddress);
                
                // Add the original conference to our destination array
                strAddress = pAddrDst->m_Call.m_strDestNumber;
                pAddrDst->m_lstConf.AddTail(strAddress);
                wsprintf (strAddress.GetBuffer(20), "@%02d", ConfInfo.wAddressID);
                strAddress.ReleaseBuffer();
                pAddrDst->m_lstConf.AddTail (strAddress);
                
                // Transition both into the conferenced state.
                pAddrDst->SetState(ADDRESSSTATE_INCONF);
                pAddr->SetState(ADDRESSSTATE_INCONF);
                pAddr->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
                pAddrDst->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
                
                CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
                pButton->SetLampState(LAMPSTATE_STEADY);
                pButton = GetButton (pAddrDst->m_iButtonIndex);
                pButton->SetLampState(LAMPSTATE_STEADY);
            }
        }           
    } 
    else if (lpConfInfo->wCommand == CONFCOMMAND_REMOVE)
    {                          
        // If we are not currently in a conference, then error out.
        if (pAddr->m_Call.m_iState != ADDRESSSTATE_INCONF)
        {
            SendNotification (EMRESULT_ERROR, 
                              MAKEERR(ConfInfo.wAddressID, 
                              EMERROR_INVALADDRESSSTATE));
            return;
        }
        
        // This is an ERROR!  Should never happen.
        if (pAddr->m_lstConf.GetCount() == 0)
        {
            pAddr->SetState (ADDRESSSTATE_OFFLINE);
            EmptyCallInfo (pAddr->m_wAddressID);
            if (FindCallByState(ADDRESSSTATE_INCONF) == NULL)
                pConfButton->SetLampState(LAMPSTATE_OFF);
            CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
            pButton->SetLampState(LAMPSTATE_OFF);
            UpdateStates();
            return;
        }
        
        // We always remove the LAST entry added which should be the tail
        // entry in our string array.  
        CString strAddress = pAddr->m_lstConf.GetTail();
        pAddr->m_lstConf.RemoveTail();
        
        // If this is a CROSS-ADDRESS conference pointer, then de-activate the conference on that
        // other address.
        if (strAddress.Left(1) == '@')
        {
            CMfxString strNum = strAddress.Mid(1);
            int iAddressID = strNum;
            CAddressAppearance* pAddrDst = GetAddress(iAddressID);
            ASSERT (pAddrDst != NULL);
            if (pAddrDst)
            {
                pAddrDst->m_lstConf.RemoveAll();
                pAddrDst->SetState(ADDRESSSTATE_OFFLINE);
                CEmulButton* pButton = GetButton(pAddrDst->m_iButtonIndex);
                pButton->SetLampState(LAMPSTATE_OFF);
                EmptyCallInfo(pAddrDst->m_wAddressID);
            }
        }
        
        // If only a single address is left in the array, then switch it 
        // back to a two party call.
        if (pAddr->m_lstConf.GetCount() == 1)
        {
            CString strAddress = pAddr->m_lstConf.GetHead();
            ASSERT (strAddress.Left(1) != '@');
            pAddr->m_lstConf.RemoveAll();
            pAddr->SetState (ADDRESSSTATE_CONNECT);
            pAddr->m_Call.m_strDestNumber = strAddress;
            if (FindCallByState(ADDRESSSTATE_INCONF) == NULL)
                pConfButton->SetLampState(LAMPSTATE_OFF);
        }               
    }   
    else if (lpConfInfo->wCommand == CONFCOMMAND_DESTROY)
    {   
        if (pAddr->m_Call.m_iState != ADDRESSSTATE_INCONF)
        {
            SendNotification (EMRESULT_ERROR, 
                              MAKEERR(ConfInfo.wAddressID, 
                              EMERROR_INVALADDRESSSTATE));
            return;
        }
        PerformDropConf(pAddr);
    }
    
#ifdef _DEBUG    
    else     
        ASSERT (FALSE);
#endif        

    UpdateStates();

}// CEmulatorDlg::DRV_Conference

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_Redireect
//
// Redirect an offering call.
//
void CEmulatorDlg::DRV_Redirect (LPEMFORWARDINFO lpForwardInfo)
{                            
    if (IsBadReadPtr(lpForwardInfo, sizeof(EMFORWARDINFO)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP Redirect <%d, %s>", lpForwardInfo->wAddressID, lpForwardInfo->szAddress);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the forwarding request
    EMFORWARDINFO fwdInfo;
    memcpy (&fwdInfo, lpForwardInfo, sizeof(EMFORWARDINFO));
    lpForwardInfo = &fwdInfo;
    
    // Release the TSP thread    
    ReleaseTSPThread();
                               
    // Make sure the address is offering.
    CAddressAppearance* pAddr = GetAddress(lpForwardInfo->wAddressID);
    if (pAddr == NULL)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpForwardInfo->wAddressID, 
                    EMERROR_INVALADDRESSID));
        return;
    }
    
    if (pAddr->m_Call.m_iState != ADDRESSSTATE_OFFERING)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpForwardInfo->wAddressID, 
                    EMERROR_INVALADDRESSSTATE));
        return;
    }

    // Save off the caller information for this call.
    CString strName = pAddr->m_Call.m_strDestName;
    CString strNumber = pAddr->m_Call.m_strDestNumber;
    DWORD dwMediaModes = pAddr->m_Call.m_dwMediaModes;
    
    // Now turn off the call.
    pAddr->SetState (ADDRESSSTATE_OFFLINE);
    CEmulButton* pButton = GetButton (pAddr->m_iButtonIndex);
    pButton->SetLampState (LAMPSTATE_OFF);
    
    if (FindCallByState(ADDRESSSTATE_OFFERING) == NULL)
        KillTimer (TIMER_RING);
    
    // If the call redirection address is another address on this
    // phone, then show it offering THERE.
    for (int i = 0; i < m_arrAppearances.GetSize(); i++)
    {
        CAddressAppearance* pTAddr = GetAddress(i);
        if (!pTAddr->m_strNumber.CompareNoCase(lpForwardInfo->szAddress))
        {                                          
            if (pTAddr->m_Call.m_iState == ADDRESSSTATE_OFFLINE)
            {   
                pTAddr->m_iRingCount = 0;
                pTAddr->m_Call.m_fIncoming = TRUE;
                pTAddr->m_Call.m_strDestName = strName;
                pTAddr->m_Call.m_strDestNumber = strNumber;
                pTAddr->m_Call.m_dwMediaModes = dwMediaModes;
                pTAddr->SetState (ADDRESSSTATE_OFFERING);
                CEmulButton* pButton = GetButton (pTAddr->m_iButtonIndex);
                pButton->SetLampState (LAMPSTATE_FLASHING);
                
                // Send redirecting ID information.
                if (!pAddr->m_strNumber.IsEmpty() || !m_strStationName.IsEmpty())
                {
                    EMCALLERID CallerID;
                    CallerID.wAddressID = pTAddr->m_wAddressID;
                    if (!pAddr->m_strNumber.IsEmpty())
                        strncpy (CallerID.szAddressInfo, (LPCSTR)pAddr->m_strNumber, ADDRESS_SIZE);
                    if (!m_strStationName.IsEmpty())                    
                        strncpy (CallerID.szName, (LPCSTR)m_strStationName, OWNER_SIZE);
                    SendNotification (EMRESULT_REDIRECTID, (LPARAM)&CallerID);
                }
                                
                SetTimer (TIMER_RING, 3000, NULL);
                ShowDisplay (pAddr);
            }
            break;
        }
    }

    EmptyCallInfo (pAddr->m_wAddressID);

    UpdateStates();
                                   
}// CEmulatorDlg::DRV_Redireect

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_CompleteCall
//
// Complete a call which is in the BUSY or ONLINE state.
//
void CEmulatorDlg::DRV_CompleteCall (LPEMCOMPLETECALL lpCompleteCall)
{                                 
    if (IsBadReadPtr(lpCompleteCall, sizeof(EMCOMPLETECALL)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP CompleteCall <%d, %d, \"%s\">", lpCompleteCall->wAddressID, 
                    lpCompleteCall->wCompletionType, lpCompleteCall->szMessage);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the completion request
    EMCOMPLETECALL ComplInfo;
    memcpy (&ComplInfo, lpCompleteCall, sizeof(EMCOMPLETECALL));
    lpCompleteCall = &ComplInfo;
    
    // Release the TSP thread    
    ReleaseTSPThread();
                               
    // Make sure the address is offering.
    CAddressAppearance* pAddr = GetAddress(lpCompleteCall->wAddressID);
    if (pAddr == NULL)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpCompleteCall->wAddressID, 
                    EMERROR_INVALADDRESSID));
        return;
    }
    
    if (pAddr->m_Call.m_iState != ADDRESSSTATE_BUSY &&
        pAddr->m_Call.m_iState != ADDRESSSTATE_ONLINE)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpCompleteCall->wAddressID, 
                    EMERROR_INVALADDRESSSTATE));
        return;
    }

    // Create an internal COMPLETION request.
    WORD wPos = CreateCompletionRequest (lpCompleteCall->wAddressID, lpCompleteCall->wCompletionType);
    
    // Send back an OK response to the completion request.
    SendNotification (EMRESULT_COMPLRESULT, MAKEERR(lpCompleteCall->wAddressID, wPos));

    // If this is a CAMP ON request, then idle the address.
    if (lpCompleteCall->wCompletionType == CALLCOMPLTYPE_CALLBACK)
    {
        pAddr->SetState (ADDRESSSTATE_OFFLINE);
        CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
        pButton->SetLampState (LAMPSTATE_OFF);
    }

    // Update our display and such.
    UpdateStates();

}// CEmulatorDlg::DRV_CompleteCall

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_ParkCall
//
// Park the call (idle it and add it to our park list)
//
void CEmulatorDlg::DRV_ParkCall (LPEMPARKINFO lpParkInfo)
{                                 
    if (IsBadReadPtr(lpParkInfo, sizeof(EMPARKINFO)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP ParkCall <%d, \"%s\">", lpParkInfo->wAddressID, lpParkInfo->szAddress);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the park request
    EMPARKINFO ParkInfo;
    memcpy (&ParkInfo, lpParkInfo, sizeof(EMPARKINFO));
    lpParkInfo = &ParkInfo;
    
    // Release the TSP thread    
    ReleaseTSPThread();
                               
    // Make sure the address is active.
    CAddressAppearance* pAddr = GetAddress(lpParkInfo->wAddressID);
    if (pAddr == NULL)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpParkInfo->wAddressID, 
                    EMERROR_INVALADDRESSID));
        return;
    }
    
    if (!pAddr->m_Call.IsActive())
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpParkInfo->wAddressID, 
                    EMERROR_INVALADDRESSSTATE));
        return;
    }

    // Add it to our "parked" list.
    PARKREQ* pPark = new PARKREQ;
    strncpy (pPark->szParkAddress, lpParkInfo->szAddress, ADDRESS_SIZE);
    strncpy (pPark->szOrigAddress, (LPCSTR)pAddr->m_Call.m_strDestNumber, ADDRESS_SIZE);
    strncpy (pPark->szOrigName, (LPCSTR)pAddr->m_Call.m_strDestName, OWNER_SIZE);
    
    // Remove any existing park information for this address
    for (int i = 0; i < m_arrParks.GetSize(); i++)
    {
        PARKREQ* pReq = (PARKREQ*) m_arrParks[i];
        if (!stricmp(lpParkInfo->szAddress, pReq->szParkAddress))
        {
            m_arrParks.RemoveAt(i--);
            delete pReq;
        }
    }
        
    m_arrParks.Add(pPark);

    // Offline the address
    pAddr->SetState (ADDRESSSTATE_OFFLINE);
    CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
    pButton->SetLampState (LAMPSTATE_OFF);
    EmptyCallInfo(pAddr->m_wAddressID);

    // Update our display and such.
    UpdateStates();

}// CEmulatorDlg::DRV_ParkCall

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DRV_UnparkCall
//
// Unpark a parked call
//
void CEmulatorDlg::DRV_UnparkCall (LPEMPARKINFO lpParkInfo)
{                                 
    if (IsBadReadPtr(lpParkInfo, sizeof(EMPARKINFO)))
        return;

    // Put out a status message
    CMfxString strMsg;
    strMsg.Sprintf("SP UnparkCall <%d, \"%s\">", lpParkInfo->wAddressID, lpParkInfo->szAddress);
    AddDebugInfo (0, (LPCSTR)strMsg);

    // Make a copy of the park request
    EMPARKINFO ParkInfo;
    memcpy (&ParkInfo, lpParkInfo, sizeof(EMPARKINFO));
    lpParkInfo = &ParkInfo;
    
    // Release the TSP thread    
    ReleaseTSPThread();
                               
    // Make sure the address is offline
    CAddressAppearance* pAddr = GetAddress(lpParkInfo->wAddressID);
    if (pAddr == NULL)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpParkInfo->wAddressID, 
                    EMERROR_INVALADDRESSID));
        return;
    }
    
    if (pAddr->m_Call.m_iState != ADDRESSSTATE_OFFLINE)
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpParkInfo->wAddressID, 
                    EMERROR_INVALADDRESSSTATE));
        return;
    }
    
    // Locate any park information for this.
    for (int i = 0; i < m_arrParks.GetSize(); i++)
    {
        PARKREQ* pReq = (PARKREQ*) m_arrParks[i];
        if (!stricmp(lpParkInfo->szAddress, pReq->szParkAddress))
        {
            m_arrParks.RemoveAt(i--);
            pAddr->m_Call.m_strDestNumber = pReq->szOrigAddress;
            pAddr->m_Call.m_strDestName = pReq->szOrigName;
            pAddr->SetState (ADDRESSSTATE_CONNECT);
            CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
            pButton->SetLampState (LAMPSTATE_STEADY);
            
            // Force CALLERID information to be accurate.
            EMCALLERID sCallerID;
            sCallerID.wAddressID = pAddr->m_wAddressID;
            strncpy (sCallerID.szAddressInfo, (LPCSTR)pAddr->m_Call.m_strDestNumber, ADDRESS_SIZE);
            strncpy (sCallerID.szName, (LPCSTR)pAddr->m_Call.m_strDestName, OWNER_SIZE);
            SendNotification (EMRESULT_CALLERID, (LPARAM)& sCallerID);
            
            delete pReq;
            break;
        }
    }
    
    // If we never found an address, error out.
    if (i == m_arrParks.GetSize())
    {
        SendNotification (EMRESULT_ERROR, 
                    MAKEERR(lpParkInfo->wAddressID, 
                    EMERROR_NOPARKINFO));
    }

    // Update our display and such.
    UpdateStates();

}// CEmulatorDlg::DRV_ParkCall

