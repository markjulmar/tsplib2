// gendlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGenerateDlg dialog

class CGenerateDlg : public CDialog
{
// Dialog Data
protected:
    //{{AFX_DATA(CGenerateDlg)
    enum { IDD = IDD_DIALGENERATE };
    BOOL    m_fUsePulse;
    CAddressAppearance* m_pAddr;
    //}}AFX_DATA

// Construction
public:
    CGenerateDlg(CWnd* pParent, CAddressAppearance* pAddr);

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    void SendNotification (char cDigit);

    // Generated message map functions
    //{{AFX_MSG(CGenerateDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNumberPress();
    afx_msg void OnButton10();
    afx_msg void OnButton11();
    afx_msg void OnBtnA();
    afx_msg void OnBtnB();
    afx_msg void OnBtnC();
    afx_msg void OnBtnD();
    afx_msg void OnCheckPulse();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
