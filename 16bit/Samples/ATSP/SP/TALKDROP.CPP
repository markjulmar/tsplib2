/******************************************************************************/
//                                                                        
// TALKDROP.CPP - Talk/Drop dialog for the ATSP service provider
//                                                                        
// This file contains the dialog definitions and code for the Talk/Drop
// dialog which is used to facilitate action from the user when a
// call is connected using the modem.
//
// Copyright (C) 1994, 1995 Mark C. Smith
// All rights reserved                                                    
//                                                                        
// This service provider drives a Hayes compatible AT style modem.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
// Change History
// -------------------------------------------------------------------
// 09/04/94 Mark Smith (MCS)
//      Initial revision.
//
/******************************************************************************/

#include "stdafx.h"
#include "atsp.h"
#include "resource.h"
#include "talkdrop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////
// Constants

const UINT IDT_TIMER = 100;

////////////////////////////////////////////////////////////////////////
// CTalkDropDlg::CTalkDropDlg
//
// Constructor for the Talk/Drop dialog for ATSP
// 
CTalkDropDlg::CTalkDropDlg(CTSPIRequest* pRequest)
   : CDialog(CTalkDropDlg::IDD, NULL)
{
    //{{AFX_DATA_INIT(CTalkDropDlg)
    m_pCurrentRequest = pRequest;
    m_eReason = None;
    //}}AFX_DATA_INIT
    
    // Auto create the dialog (modaless)
    CDialog::Create (CTalkDropDlg::IDD, NULL);
    
}// CTalkDropDlg::CTalkDropDlg

////////////////////////////////////////////////////////////////////////
// CTalkDropDlg::DoDataExchange
//
// Dialog data exchange for the Talk/Drop dialog
//
VOID CTalkDropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTalkDropDlg)
    DDX_Text (pDX, IDC_ELAPSEDTIME, m_strElapsedTime);
    //}}AFX_DATA_MAP
    
}// CTalkDropDlg::DoDataExchange

////////////////////////////////////////////////////////////////////////
// CTalkDropDlg Message Map
//
BEGIN_MESSAGE_MAP(CTalkDropDlg, CDialog)
    //{{AFX_MSG_MAP(CTalkDropDlg)
    ON_BN_CLICKED(IDC_DROP, OnDrop)
    ON_BN_CLICKED(IDC_TALK, OnTalk)
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////
// CTalkDropDlg::OnInitDialog
//
// Initialize the Talk/Drop dialog for the ATSP sample.
//
BOOL CTalkDropDlg::OnInitDialog()
{
    // Set the font for all the controls to something
    // a little better than the default.
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd* pwndChild = GetWindow (GW_CHILD);
    while (pwndChild != NULL && IsChild (pwndChild))
    {
        pwndChild->SetFont (&fntAnsi);
        pwndChild = pwndChild->GetWindow (GW_HWNDNEXT);
    }
   
    // Center the window on the screen.
    CenterWindow(GetDesktopWindow());

    // Setup a timer for the dialog to show elapsed time.
    m_dwCurrTime = GetTickCount();
    SetTimer (IDT_TIMER, 1000L, NULL);
           
    // And connect up (through DDX) the controls on the screen
    // to the elements in our dialog class.           
    return CDialog::OnInitDialog();

}// CTalkDropDlg::OnInitDialog

////////////////////////////////////////////////////////////////////////
// CTalkDropDlg::OnDrop
//
// Process a DROP command for the AT service provider.  This will
// adjust the state of the current request and force the service
// provider to take action.
//
VOID CTalkDropDlg::OnDrop()
{
    m_eReason = DropPressed;
    GetDlgItem (IDC_TALK)->EnableWindow (FALSE);
    GetDlgItem (IDC_DROP)->EnableWindow (FALSE);
    KillTimer(IDT_TIMER);    
    
    if (m_pCurrentRequest->GetState() == STATE_CHECKMODEM ||
        m_pCurrentRequest->GetState() == STATE_WAITING)
    {
        m_pCurrentRequest->SetState (STATE_DESTROYTD);
        m_pCurrentRequest->GetConnectionInfo()->ReceiveData(0, NULL, 0L);
    }

}// CTalkDropDlg::OnDrop

///////////////////////////////////////////////////////////////////////
// CTalkDropDlg::OnTalk
//
// Process a TALK command for the AT service provider
//
VOID CTalkDropDlg::OnTalk()
{
    m_eReason = TalkPressed;
    GetDlgItem (IDC_TALK)->EnableWindow (FALSE);
    GetDlgItem (IDC_DROP)->EnableWindow (FALSE);
    KillTimer(IDT_TIMER);    

    if (m_pCurrentRequest->GetState() == STATE_CHECKMODEM ||
        m_pCurrentRequest->GetState() == STATE_WAITING)
    {
        m_pCurrentRequest->SetState (STATE_DESTROYTD);
        m_pCurrentRequest->GetConnectionInfo()->ReceiveData(0, NULL, 0L);
    }
    
}// CTalkDropDlg::OnTalk

////////////////////////////////////////////////////////////////////////
// CTalkDropDlg::GetReasonCode
//
// Return the reason code for dismissal.
//
int CTalkDropDlg::GetReasonCode() const
{                              
    return (int) m_eReason;

}// CTalkDropDlg::GetReasonCode

///////////////////////////////////////////////////////////////////////
// CTalkDropDlg::OnTimer
//
// Elapsed timer for the dialog - shows an elapsed time while
// the dialog is up.
//
void CTalkDropDlg::OnTimer (UINT idEvent)
{                                       
    if (idEvent == IDT_TIMER)
    {
        DWORD dwElapsed = (GetTickCount() - m_dwCurrTime) / 1000;
        wsprintf (m_strElapsedTime.GetBuffer(20), "%ld second%c", dwElapsed, (dwElapsed == 1) ? ' ' : 's');
        UpdateData (FALSE);    
    }        
    
}// CTalkDropDlg::OnTimer

