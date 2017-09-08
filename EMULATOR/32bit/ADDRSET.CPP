/*****************************************************************************/
//
// ADDRSET.CPP - Digital Switch Service Provider Sample
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
#include "addrset.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg::CAddrSetDlg
//
// Constructor for the address configuration dialog.
//
CAddrSetDlg::CAddrSetDlg() : CPropertyPage(CAddrSetDlg::IDD)
{
    //{{AFX_DATA_INIT(CAddrSetDlg)
    m_strAddress = "";
    //}}AFX_DATA_INIT
    
}// CAddrSetDlg::CAddrSetDlg

/////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg::DoDataExchange
//
// Dialog data exchange for the address dialog
//
void CAddrSetDlg::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAddrSetDlg)
    DDX_Control(pDX, IDC_LINELIST, m_lbAddresses);
    DDX_Text(pDX, IDC_ADDRESS, m_strAddress);
    //}}AFX_DATA_MAP
    
}// CAddrSetDlg::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg message map

BEGIN_MESSAGE_MAP(CAddrSetDlg, CPropertyPage)
    //{{AFX_MSG_MAP(CAddrSetDlg)
    ON_LBN_SELCHANGE(IDC_LINELIST, OnSelchangeAddressList)
    ON_EN_CHANGE (IDC_ADDRESS, OnAddressChange)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg::OnInitDialog
//
// Initialize the dialog
// 
BOOL CAddrSetDlg::OnInitDialog()
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

    return TRUE;

}// CAddrSetDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg::OnSetActive
//
// This tab is becoming the active page
//
BOOL CAddrSetDlg::OnSetActive()
{
    CPropertyPage::OnSetActive();
    CString strKey;
    m_lbAddresses.ResetContent();
    int iCall = 1;

    // Fill the address listbox from our .INI file
    for (int i = 0; i < 16; i++)
    {
        strKey.Format ("Key%d", i+FIRST_FUNCTION_INDEX);
        int iType = AfxGetApp()->GetProfileInt ("Buttons", strKey, BUTTONFUNCTION_NONE);
        if (iType == BUTTONFUNCTION_CALL)
        {   
            strKey.Format("Address %d", iCall++);
            m_lbAddresses.AddString (strKey);
        }
    }
    
    // If we have any call addresses, select the first one
    if (m_lbAddresses.GetCount())
    {
        m_lbAddresses.SetCurSel (0);
        OnSelchangeAddressList();
    }

    return TRUE;
    
}// CAddrSetDlg::OnSetActive

///////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg::OnKillActive
//
// This tab is being switch away from
//
BOOL CAddrSetDlg::OnKillActive()
{
    return CPropertyPage::OnKillActive();
    
}// CAddrSetDlg::OnKillActive

///////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg::OnSelchangeAddressList
//
// The address has been changed in the listbox
// 
void CAddrSetDlg::OnSelchangeAddressList()
{
    // Set the text for the new selection
    int iSel = m_lbAddresses.GetCurSel();
    if (iSel != LB_ERR)
    {
        CString strKey;
        strKey.Format ("Call%d", iSel + 1);
        m_strAddress = AfxGetApp()->GetProfileString ("Addresses", strKey, "");
        UpdateData (FALSE);
    }
    
    GetDlgItem (IDC_ADDRESS)->SetFocus();
    
}// CAddrSetDlg::OnSelchangeAddressList

///////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg::OnAddressChange
//
// The address has been changed in the edit
// 
void CAddrSetDlg::OnAddressChange()
{
    UpdateData (TRUE);
    
    // Set the text for the new selection
    int iSel = m_lbAddresses.GetCurSel();
    if (iSel != LB_ERR)
    {
    	CString strKey;
    	strKey.Format ("Call%d", iSel + 1);
        AfxGetApp()->WriteProfileString ("Addresses", strKey, m_strAddress);
    }
    
}// CAddrSetDlg::OnSelchangeAddressList
