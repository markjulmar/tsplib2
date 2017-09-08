//*****************************************************************************
//
// CONFGDLG.H
//
//*****************************************************************************

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CPropertyPage
{
// Dialog Data
protected:
    //{{AFX_DATA(CConfigDlg)
    enum { IDD = IDD_CONFIGURE };
    CListBox    m_lbFunctions;
    //}}AFX_DATA
    int     m_iButtonInfo[16];     // Our button function array

// Construction
public:
    CConfigDlg();
    
// Public members
public:
    void WriteButtonInfo();
    
// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    // Generated message map functions
    //{{AFX_MSG(CConfigDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual BOOL OnKillActive();
    virtual BOOL OnSetActive();
    afx_msg void OnButtonPress();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
