/*****************************************************************************/
//
// CALL.CPP - Digital Switch Service Provider Sample
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
#include "resource.h"
#include "call.h"
#include <tapi.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// Structure to hold TAPI media type info.
struct
{
    const char * pszText;		// Description
    DWORD dwType;             	// TAPI media mode key
    
} g_pszMediaTypes[] = {
    {"ADSI",                 MEDIAMODE_ADSI},
    {"Automated Voice",      MEDIAMODE_AUTOMATEDVOICE},
    {"Data Modem",           MEDIAMODE_DATAMODEM},
    {"Digital Data",         MEDIAMODE_DIGITALDATA},
    {"G3 FAX",               MEDIAMODE_G3FAX},
    {"G4 FAX",               MEDIAMODE_G4FAX},
    {"Interactive Voice",    MEDIAMODE_INTERACTIVEVOICE},
    {"Mixed",                MEDIAMODE_MIXED},
    {"TDD",                  MEDIAMODE_TDD},
    {"TeleTex",              MEDIAMODE_TELETEX},
    {"Telex",                MEDIAMODE_TELEX},
    {"VideoTex",             MEDIAMODE_VIDEOTEX},
    {"Voice View",           MEDIAMODE_VOICEVIEW},
    {NULL,                   0L}
};

/////////////////////////////////////////////////////////////////////////////
// CCallDlg::CCallDlg
//
// Constructor for the Call dialog
//
CCallDlg::CCallDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CCallDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CCallDlg)
    m_strCaller = "";
    m_strCallerNum = "";
    m_iLine = 0;
	m_fExternal = FALSE;
	//}}AFX_DATA_INIT
    
}// CCallDlg::CCallDlg

/////////////////////////////////////////////////////////////////////////////
// CCallDlg::DoDataExchange
//
// Dialog Data Exchange
//
void CCallDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCallDlg)
    DDX_Control(pDX, IDC_MEDIALIST, m_lbMediaModes);
    DDX_Control(pDX, IDC_CALLINGNUM, m_edtCallingNum);
    DDX_Control(pDX, IDC_CALLINGNAME, m_edtCallingName);
    DDX_Control(pDX, IDC_STATCALL1, m_statCall1);
    DDX_Control(pDX, IDC_STATCALL2, m_statCall2);
    DDX_Text(pDX, IDC_CALLINGNAME, m_strCaller);
    DDX_Text(pDX, IDC_CALLINGNUM, m_strCallerNum);
    DDX_Control(pDX, IDC_PHONENUMBER, m_cbLines);
	DDX_Check(pDX, IDC_EXTERNAL, m_fExternal);
	//}}AFX_DATA_MAP
    DDX_CBStringArray(pDX, IDC_PHONENUMBER, m_arrAddr);
}// CCallDlg::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
// CCallDlg Message Map
//
BEGIN_MESSAGE_MAP(CCallDlg, CDialog)
    //{{AFX_MSG_MAP(CCallDlg)
    ON_BN_CLICKED(IDC_BTN_CLEAR, OnBtnClear)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////
// CCallDlg::OnInitDialog
//
// This handler processes the WM_INITDIALOG message and initializes our
// call dialog window.
//
BOOL CCallDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    CFont fntAnsi;
    fntAnsi.CreateStockObject(ANSI_VAR_FONT);
    CWnd* pwnd = GetWindow (GW_CHILD);
    while (pwnd && IsChild(pwnd))
    {
        pwnd->SetFont(&fntAnsi);
        pwnd = pwnd->GetWindow (GW_HWNDNEXT);
    }
    
    CenterWindow();

    // Set the first available call appearances active.
    m_cbLines.SetCurSel (0);

    // Fill the media type listbox
    FillMediaListbox();

    return TRUE;  

}// CCallDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////////
// CCallDlg::OnOK
//
// Read the combo box and determine if we have selected a line or
// typed in a new phone number.
//
void CCallDlg::OnOK()
{
    int iSel = m_cbLines.GetCurSel();
    m_iLine = m_arrIndex[iSel];
    
    if (!m_lbMediaModes.GetSelCount())
    {
        AfxMessageBox ("You must select at least one media type");
        return;
    }

    m_dwMediaTypes = 0L;

    // Build a single DWORD with all types set
    for (int i = 0 ; i < m_lbMediaModes.GetCount(); i++)
    {
        if (m_lbMediaModes.GetSel (i) > 0)
            m_dwMediaTypes |= m_lbMediaModes.GetItemData (i);
    }

    CDialog::OnOK();

}// CCallDlg::OnOK

///////////////////////////////////////////////////////////////////////////////
// CCallDlg::FillMediaListbox
//
// Fills the media types listbox and sets the default
// selection - called by OnInitDialog
//
void CCallDlg::FillMediaListbox()
{
    for (int i = 0 ; g_pszMediaTypes[i].pszText != NULL; i++)
    {
        int iPos = m_lbMediaModes.AddString (g_pszMediaTypes [i].pszText);
        m_lbMediaModes.SetItemData (iPos, g_pszMediaTypes [i].dwType);
    }
    
    // Set "Interactive Voice" as default selection
    int iPos = m_lbMediaModes.FindString (-1, g_pszMediaTypes[6].pszText);
    if (iPos != LB_ERR)
        m_lbMediaModes.SetSel (iPos, TRUE);

}// CCallDlg::FillMediaListbox

///////////////////////////////////////////////////////////////////////////////
// CCallDlg::OnBtnClear
//
// Clear all selections in the media types listbox
//
void CCallDlg::OnBtnClear()
{
    m_lbMediaModes.SetSel (-1, FALSE);

}// CCallDlg::OnBtnClear

///////////////////////////////////////////////////////////////////////////////
// CCallDlg::AddLine
//
// Add a new line to the call box
//
void  CCallDlg::AddLine(int iPos, const char *pszLine)
{ 
    m_arrAddr.Add(pszLine);
    m_arrIndex.Add (iPos);
    
}// CCallDlg::AddLine 
