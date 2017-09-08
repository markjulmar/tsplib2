//*****************************************************************************
//
// OBJECTS.CPP
//
// Digital switch emulator program
//
// Written 04/23/96
// Copyright (C) 1996 Mark C. Smith
// All rights reserved.
//
//*****************************************************************************

#include "stdafx.h"
#include "colorlb.h"
#include "objects.h"
#include "resource.h"
#include "emulator.h"
#include "gendlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Globals

const char * g_pszStates[] = {
    { "Unk" }, {"Connect"}, {"Busy"}, 
    {"Disconnect"}, {"Dialtone"}, 
    {"Online"}, {"Offline"}, {"Offering"}, 
    {"OnHold"}, {"Conf"}
};    

const char * g_pszLamps[] = {
    {"None"}, {"Off"}, {"Steady"}, 
    {"Blinking"}, {"Flashing"}
};    

///////////////////////////////////////////////////////////////////////////////
// CEmulButton::CEmulButton
//
// Button class constructor
//
CEmulButton::CEmulButton()
{                           
    m_iButtonFunction = -1;
    m_pLampWnd = NULL;

}// CEmulButton::CEmulButton

///////////////////////////////////////////////////////////////////////////////
// CEmulButton::~CEmulButton
//
// Destructor for the emulator button.
//
CEmulButton::~CEmulButton()
{                            
    delete m_pLampWnd;

}// CEmulButton::~CEmulButton

///////////////////////////////////////////////////////////////////////////////
// CEmulButton::SetLampState
//
// Set the state of the lamp associated with this button.
//
void CEmulButton::SetLampState(int iState)
{
    if (m_pLampWnd != NULL)
    {
        CMfxString strBuff;
        strBuff.Sprintf("Lamp %d <%s>", m_iLampButtonID, g_pszLamps[iState]);
        ((CEmulatorDlg*)AfxGetApp()->m_pMainWnd)->AddDebugInfo(1,(LPCSTR)strBuff);
    
        m_pLampWnd->SetState(iState);
    
        // Notify the service provider    
        EMLAMPCHANGE sLampChange;
        sLampChange.wButtonLampID = (WORD) m_iLampButtonID;
        sLampChange.wLampState = (WORD) iState;
        ((CEmulatorDlg*)AfxGetApp()->m_pMainWnd)->SendNotification (EMRESULT_LAMPCHANGED, (LPARAM)&sLampChange);
    }
    
}// CEmulButton::SetLampState

///////////////////////////////////////////////////////////////////////////////
// CAddrCall::CAddrCall
//
// Constructor for the call appearance object
//
CAddrCall::CAddrCall()
{
    m_fIncoming = FALSE;
    m_iState = ADDRESSSTATE_OFFLINE;
    m_iLastState = ADDRESSSTATE_UNKNOWN;
    m_iStateInfo = 0;
    m_dwMediaModes = MEDIAMODE_INTERACTIVEVOICE;
    
}// CAddrCall::CAddrCall

///////////////////////////////////////////////////////////////////////////////
// CAddrCall::~CAddrCall
//
// Destructor for the address call appearance
//
CAddrCall::~CAddrCall()
{ 
    /* Do nothing */
    
}// CAddrCall::~CAddrCall

///////////////////////////////////////////////////////////////////////////////
// CAddrCall::operator=
//
// Assignment operator for the call appearance
//
const CAddrCall& CAddrCall::operator=(const CAddrCall& src)
{                       
    m_fIncoming = src.m_fIncoming;
    m_strDestName = src.m_strDestName;
    m_strDestNumber = src.m_strDestNumber;
    m_iState = src.m_iState;
    m_iStateInfo = src.m_iStateInfo;
    m_iLastState = src.m_iLastState;
    m_dwMediaModes = src.m_dwMediaModes;
    return *this;
    
}// CAddrCall::operator=

///////////////////////////////////////////////////////////////////////////////
// CAddrCall::IsActive
//
// Return whether this call is in an active state
//
BOOL CAddrCall::IsActive() const
{
    if (m_iState == ADDRESSSTATE_CONNECT ||
        m_iState == ADDRESSSTATE_BUSY ||
        m_iState == ADDRESSSTATE_DIALTONE ||
        m_iState == ADDRESSSTATE_ONLINE)
        return TRUE;
    return FALSE;                
    
}// CAddrCall::IsActive

///////////////////////////////////////////////////////////////////////////////
// CAddrAppearance::CAddrAppearance
//
// Constructor for the address appearance
//
CAddressAppearance::CAddressAppearance()
{
    m_iButtonIndex = -1;
    m_iRingCount = 0;
    
}// CAddrAppearance::CAddrAppearance

///////////////////////////////////////////////////////////////////////////////
// CAddrAppearance::~CAddrAppearance
//
// Destructor for the address appearance.
//
CAddressAppearance::~CAddressAppearance()
{
    /* Do nothing */
    
}// CAddrAppearance::~CAddrAppearance

///////////////////////////////////////////////////////////////////////////////
// CAddrAppearance::SetState
//             
// Method to set the state information for this call and notify
// the service provider.
//
BOOL CAddressAppearance::SetState (int iNewState, int iStateInfo)
{
    int iLastState = m_Call.m_iState;

    CMfxString strBuff;
    strBuff.Sprintf("Call %d <%s to %s>", m_wAddressID+1, g_pszStates[iLastState],
    				g_pszStates[iNewState]);
    ((CEmulatorDlg*)AfxGetApp()->m_pMainWnd)->AddDebugInfo(1,(LPCSTR)strBuff);

    if (m_Call.m_iState != iNewState)
        m_Call.m_iState = iNewState;
    m_Call.m_iStateInfo = iStateInfo;
    
    // Empty the consultation call once we move to connected and we don't have
    // a real call on hold.
	// Make sure we have a media mode if connected to a call.
	if (iNewState == ADDRESSSTATE_CONNECT)
	{
	    if (m_ConsultationCall.m_iState != ADDRESSSTATE_ONHOLD)
    		m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
		if (m_Call.m_dwMediaModes == 0)
			m_Call.m_dwMediaModes = MEDIAMODE_INTERACTIVEVOICE;
	}				    		
    
    // Notify provider
    EMADDRESSCHANGE sAddrChange;

    // Fill in the address change structure
    sAddrChange.wAddressID = m_wAddressID;
    sAddrChange.wNewState = (WORD) iNewState;
    sAddrChange.wStateInfo = (WORD) iStateInfo;
    sAddrChange.dwMediaModes = m_Call.m_dwMediaModes;

    ((CEmulatorDlg*)AfxGetApp()->m_pMainWnd)->SendNotification(EMRESULT_ADDRESSCHANGED,
                       (LPARAM)& sAddrChange);                
                       
    return TRUE;
                                        
}// CAddrAppearance::SetState

