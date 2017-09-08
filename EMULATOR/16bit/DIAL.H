/////////////////////////////////////////////////////////////////////////////
// CDialDlg dialog

class CDialDlg : public CDialog
{
// Construction
public:
    CDialDlg(CWnd* pParent = NULL); // standard constructor

// Dialog Data
    //{{AFX_DATA(CDialDlg)
    enum { IDD = IDD_DIALDLG };
    CString m_strDisplay;
    CEdit   m_edtDisplay;
    BOOL    m_fUsePulse;
    //}}AFX_DATA
    CEmulatorDlg *  m_pParent;

// Implementation
public:
    const char *GetDialStr() { return m_strDisplay; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CDialDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonPress();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
