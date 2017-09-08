//*****************************************************************************
//
// ADDRSET.CPP
//
// Address configuration dialog for the DSSP Emulator
//
//*****************************************************************************

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
    CMfxString strKey;
    m_lbAddresses.ResetContent();
    int iCall = 1;

    // Fill the address listbox from our .INI file
    for (int i = 0; i < 16; i++)
    {
        strKey.Sprintf("Key%d", i+FIRST_FUNCTION_INDEX);
        int iType = AfxGetApp()->GetProfileInt ("Buttons", strKey, BUTTONFUNCTION_NONE);
        if (iType == BUTTONFUNCTION_CALL)
        {   
            strKey.Sprintf ("Address %d", iCall++);
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
        CMfxString strKey;
        strKey.Sprintf ("Call%d", iSel + 1);
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
    CMfxString strKey;
    
    // Set the text for the new selection
    int iSel = m_lbAddresses.GetCurSel();
    if (iSel != LB_ERR)
    {
        strKey.Sprintf ("Call%d", iSel + 1);
        AfxGetApp()->WriteProfileString ("Addresses", strKey, m_strAddress);
    }
    
}// CAddrSetDlg::OnSelchangeAddressList
