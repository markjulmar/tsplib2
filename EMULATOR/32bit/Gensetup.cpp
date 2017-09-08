/*****************************************************************************/
//
// GENSETUP.CPP - Digital Switch Service Provider Sample
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
#include "gensetup.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenSetupDlg::CGenSetupDlg
//                           
// Constructor for the general setup tab
//
CGenSetupDlg::CGenSetupDlg() : CPropertyPage(CGenSetupDlg::IDD)
{
    //{{AFX_DATA_INIT(CGenSetupDlg)
    m_strName = "";
    m_strSwitchInfo = "";
    //}}AFX_DATA_INIT
    
}// CGenSetupDlg::CGenSetupDlg

/////////////////////////////////////////////////////////////////////////////
// CGenSetupDlg::DoDataExchange
//
// Dialog data exchange for the general setup dialog
//
void CGenSetupDlg::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGenSetupDlg)
    DDX_Text(pDX, IDC_NAME, m_strName);
    DDX_Text(pDX, IDC_SWINFO, m_strSwitchInfo);
    //}}AFX_DATA_MAP
    
}// CGenSetupDlg::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
// CGenSetupDlg message map

BEGIN_MESSAGE_MAP(CGenSetupDlg, CPropertyPage)
    //{{AFX_MSG_MAP(CGenSetupDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// CGenSetupDlg::OnInitDialog
//
// Initialize the settings of the dialog
// 
BOOL CGenSetupDlg::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    
    // Set the font on all child controls
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd * pWnd = GetWindow (GW_CHILD);
    while (pWnd != NULL && IsChild (pWnd) )
    {
        pWnd->SetFont (& fntAnsi);
        pWnd = pWnd->GetWindow (GW_HWNDNEXT);
    }

    ((CEdit*)GetDlgItem(IDC_NAME))->LimitText(80);
    ((CEdit*)GetDlgItem(IDC_SWINFO))->LimitText(254);
    
    return TRUE;

}// CGenSetupDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////////
// CGenSetupDlg::OnSetActive
//
// This tab is becoming the active page
//
BOOL CGenSetupDlg::OnSetActive()
{
    // Retrieve the current settings from the INI file
    m_strName = AfxGetApp()->GetProfileString ("General", "Name", "");
    m_strSwitchInfo = AfxGetApp()->GetProfileString ("General", "SwInfo");
	UpdateData(FALSE);

    return CPropertyPage::OnSetActive();
    
}// CGenSetupDlg::OnSetActive

///////////////////////////////////////////////////////////////////////////////
// CGenSetupDlg::OnKillActive
//
// This tab is being switch away from
//
BOOL CGenSetupDlg::OnKillActive()
{
    UpdateData (TRUE);
    AfxGetApp()->WriteProfileString ("General", "Name", m_strName);
    AfxGetApp()->WriteProfileString ("General", "SwInfo", m_strSwitchInfo);

    return CPropertyPage::OnKillActive();
    
}// CGenSetupDlg::OnKillActive
