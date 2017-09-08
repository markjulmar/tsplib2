//*****************************************************************************
//
// GENSETUP.H
//
// Digital switch emulator program
//
// Written 04/08/96
//
//*****************************************************************************

/////////////////////////////////////////////////////////////////////////////
// CGenSetupDlg dialog
//
class CGenSetupDlg : public CPropertyPage
{
// Construction
public:
    CGenSetupDlg();

// Dialog Data
    //{{AFX_DATA(CGenSetupDlg)
    enum { IDD = IDD_GENERAL_SETUP };
    CString m_strName;
    CString m_strSwitchInfo;
    //}}AFX_DATA
    CEmulatorDlg *  m_pParent;

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CGenSetupDlg)
    virtual BOOL OnInitDialog();
    virtual BOOL OnKillActive();
    virtual BOOL OnSetActive();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
