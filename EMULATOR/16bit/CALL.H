//*****************************************************************************
//
// CALL.H
//
// Written 11/02/94 Mark Smith (MCS)
//
//*****************************************************************************

/////////////////////////////////////////////////////////////////////////////
// CCallDlg dialog
//
class CCallDlg : public CDialog
{
// Class data
public:
    //{{AFX_DATA(CCallDlg)
    enum { IDD = IDD_CALLNUMBER };
    CListBox        m_lbMediaModes;
    CEdit           m_edtCallingNum;
    CEdit           m_edtCallingName;
    CStatic         m_statCall2;
    CStatic         m_statCall1;
    CString         m_strCaller;
    CString         m_strCallerNum;
    CComboBox       m_cbLines;
    CStringArray    m_arrAddr;
    CUIntArray      m_arrIndex;
    DWORD           m_dwMediaTypes;
    int             m_iLine;
    //}}AFX_DATA

// Construction
public:
    CCallDlg(CWnd* pParent = NULL);  // standard constructor

// Implementation
public:               
    void AddLine(int iPos, const char* pszName);
    int   GetLineCount() { return m_arrAddr.GetSize(); }
    int   GetIncomingLine() { return m_iLine; }
    const char * GetCallerName() { return m_strCaller; }
    const char * GetCallerNum() { return m_strCallerNum; }
    DWORD GetMediaTypes() { return m_dwMediaTypes; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support
    virtual BOOL OnInitDialog();
    void  FillMediaListbox();

    // Generated message map functions
    //{{AFX_MSG(CCallDlg)
    virtual void OnOK();
    afx_msg void OnInternalcall();
    afx_msg void OnBtnClear();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

