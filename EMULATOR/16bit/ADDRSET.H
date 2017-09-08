//*****************************************************************************
//
// ADDRSET.H
//
// Address configuration dialog for the DSSP Emulator
//
//*****************************************************************************

/////////////////////////////////////////////////////////////////////////////
// CAddrSetDlg dialog
//
class CAddrSetDlg : public CPropertyPage
{
// Construction
public:
    CAddrSetDlg();

// Dialog Data
    //{{AFX_DATA(CAddrSetDlg)
    enum { IDD = IDD_ADDRESS_SETUP };
    CListBox    m_lbAddresses;
    CString     m_strAddress;
    //}}AFX_DATA
    CEmulatorDlg *  m_pParent;

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CAddrSetDlg)
    virtual BOOL OnInitDialog();
    virtual BOOL OnKillActive();
    virtual BOOL OnSetActive();
    afx_msg void OnSelchangeAddressList();
    afx_msg void OnAddressChange();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
