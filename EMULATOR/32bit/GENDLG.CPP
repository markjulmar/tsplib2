/*****************************************************************************/
//
// GENDLG.CPP - Digital Switch Service Provider Sample
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
#include "objects.h"
#include "colorlb.h"
#include "resource.h"
#include "emulator.h"
#include "gendlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::CGenerateDlg
//                           
// Constructor for the digit/pulse generation dialog.
//
CGenerateDlg::CGenerateDlg(CWnd* pParent, CAddressAppearance* pAddr)
    : CDialog(CGenerateDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGenerateDlg)
    m_fUsePulse = FALSE;
    m_pAddr = pAddr;
    //}}AFX_DATA_INIT
    
}// CGenerateDlg::CGenerateDlg

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::DoDataExchange
//
// Dialog data exchange for the pulse/digit generation dialog.
//
void CGenerateDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGenerateDlg)
    DDX_Check(pDX, IDC_CHECK_PULSE, m_fUsePulse);
    //}}AFX_DATA_MAP
    
}// CGenerateDlg::DoDataExchange

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg dialog message map

BEGIN_MESSAGE_MAP(CGenerateDlg, CDialog)
    //{{AFX_MSG_MAP(CGenerateDlg)
    ON_BN_CLICKED(IDC_BUTTON0, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON1, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON2, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON3, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON4, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON5, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON6, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON7, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON8, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON9, OnNumberPress)
    ON_BN_CLICKED(IDC_BUTTON10, OnButton10)
    ON_BN_CLICKED(IDC_BUTTON11, OnButton11)
    ON_BN_CLICKED(IDC_BTN_A, OnBtnA)
    ON_BN_CLICKED(IDC_BTN_B, OnBtnB)
    ON_BN_CLICKED(IDC_BTN_C, OnBtnC)
    ON_BN_CLICKED(IDC_BTN_D, OnBtnD)
    ON_BN_CLICKED(IDC_CHECK_PULSE, OnCheckPulse)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnInitDialog
//
// Initialize the entries on the dialog.
// 
BOOL CGenerateDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the font on all child controls
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd * pWnd = GetWindow (GW_CHILD);
    while (pWnd != NULL && IsChild (pWnd) )
    {
        pWnd->SetFont (& fntAnsi);
        pWnd = pWnd->GetWindow (GW_HWNDNEXT);
    }

    CenterWindow();
    
    // If this is the local dialog, then only show normal digit pad
    // stuff.
    if (m_pAddr == NULL)
    {
        GetDlgItem (IDC_BTN_A)->EnableWindow (FALSE);
        GetDlgItem (IDC_BTN_B)->EnableWindow (FALSE);
        GetDlgItem (IDC_BTN_C)->EnableWindow (FALSE);
        GetDlgItem (IDC_BTN_D)->EnableWindow (FALSE);
        GetDlgItem (IDC_CHECK_PULSE)->EnableWindow (FALSE);    
        SetWindowText ("Local Digit Generation");
    }
    
    return TRUE;

}// CGenerateDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnNumberPress
//
// Generate "0" - "9" codes
//
void CGenerateDlg::OnNumberPress()
{
    CButton * pButton = (CButton *)GetFocus();
    int iButton = pButton->GetDlgCtrlID() - IDC_BUTTON0;
    char cNum = (char)(iButton + '0');
    SendNotification (cNum);
    
}// CGenerateDlg::OnNumberPress

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnButton10
//
// Generate a "*"
//
void CGenerateDlg::OnButton10()
{
    SendNotification('*');
    
}// CGenerateDlg::OnButton10

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnButton11
//
// Generate a "#"
//
void CGenerateDlg::OnButton11()
{
    SendNotification('#');
    
}// CGenerateDlg::OnButton11

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnBtnA
//
// Generate a "A"
//
void CGenerateDlg::OnBtnA()
{
    SendNotification('A');
    
}// CGenerateDlg::OnBtnA

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnBtnB
//
// Generate a "B"
//
void CGenerateDlg::OnBtnB()
{
    SendNotification('B');

}// CGenerateDlg::OnBtnB

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnBtnC
//
// Generate a "C"
//
void CGenerateDlg::OnBtnC()
{
    SendNotification('C');

}// CGenerateDlg::OnBtnC

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnBtnD
//
// Generate a "D"
//
void CGenerateDlg::OnBtnD()
{
    SendNotification('D');

}// CGenerateDlg::OnBtnD

///////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::OnCheckPulse
//
// Turn pulse on/off
// 
void CGenerateDlg::OnCheckPulse()
{
    UpdateData (TRUE);

    // Disable buttons which can't be generated with pulse
    if (m_fUsePulse)
    {
        GetDlgItem (IDC_BUTTON10)->EnableWindow (FALSE);
        GetDlgItem (IDC_BUTTON11)->EnableWindow (FALSE);
        GetDlgItem (IDC_BTN_A)->EnableWindow (FALSE);
        GetDlgItem (IDC_BTN_B)->EnableWindow (FALSE);
        GetDlgItem (IDC_BTN_C)->EnableWindow (FALSE);
        GetDlgItem (IDC_BTN_D)->EnableWindow (FALSE);
    }
    else
    {
        GetDlgItem (IDC_BUTTON10)->EnableWindow (TRUE);
        GetDlgItem (IDC_BUTTON11)->EnableWindow (TRUE);
        GetDlgItem (IDC_BTN_A)->EnableWindow (TRUE);
        GetDlgItem (IDC_BTN_B)->EnableWindow (TRUE);
        GetDlgItem (IDC_BTN_C)->EnableWindow (TRUE);
        GetDlgItem (IDC_BTN_D)->EnableWindow (TRUE);
    }

}// CGenerateDlg::OnCheckPulse

/////////////////////////////////////////////////////////////////////////////
// CGenerateDlg::SendNotification
//
// Send notification about a digit/pulse being generated.
//
void CGenerateDlg::SendNotification (char cDigit)
{
    ASSERT (!m_fUsePulse || cDigit >= '0' && cDigit <= '9');
    
    CEmulatorDlg* pwnd = (CEmulatorDlg*) AfxGetApp()->m_pMainWnd;
    CString strBuff;

    if (m_pAddr == NULL)
    {
        // Send a button down/button up event.
        int iButtonID = 0;
        if (cDigit >= '0' && cDigit <= '9')
            iButtonID = (int)(cDigit - '0');        
        else if (cDigit == '*')
            iButtonID = 10;
        else if (cDigit == '#')
            iButtonID = 11;
        pwnd->OnButtonUpDown (iButtonID);
        
        // Force the emulator to DIAL the digit - locate the address which
        // is in the appropriate state.
        CAddressAppearance * pAddr = pwnd->FindCallByState (ADDRESSSTATE_CONNECT);
        if (pAddr == NULL)
        {
            pAddr = pwnd->FindCallByState(ADDRESSSTATE_DIALTONE);
            if (pAddr == NULL)
                pAddr = pwnd->FindCallByState(ADDRESSSTATE_ONLINE);
        }
        
        if (pAddr != NULL)
        {
            char szBuff[2];
            szBuff[0] = cDigit;
            szBuff[1] = '\0';
            pwnd->DialNumber (pAddr->m_wAddressID, szBuff);
        }            
    }
    else
    {
        EMDIGITBUFF DigitBuff;
        DigitBuff.wAddressID = m_pAddr->m_wAddressID;
        DigitBuff.fDTMF = (WORD) (m_fUsePulse == FALSE);
        DigitBuff.cDigit = cDigit;
    
        strBuff.Format("Call %d <%s Digit %c>", m_pAddr->m_wAddressID, 
                    (m_fUsePulse) ? "Pulse" : "DTMF", DigitBuff.cDigit);

        pwnd->AddDebugInfo (1, strBuff);
        pwnd->SendNotification (EMRESULT_DIGITDETECTED, (LPARAM)&DigitBuff);

        // If this is a DTMF tone, then generate a silence area afterward for UP
        if (!m_fUsePulse)
        {
            DigitBuff.cDigit = 0;
            pwnd->SendNotification (EMRESULT_DIGITDETECTED, (LPARAM)&DigitBuff);
        }
    }        

}// CGenerateDlg::SendNotification
