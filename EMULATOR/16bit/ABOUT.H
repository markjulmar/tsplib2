// about.h : header file
//
                   
#ifndef _INC_ABOUT_
#define _INC_ABOUT_

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg2 dialog

class CAboutDlg2 : public CDialog
{
// Class data
private:
	CFont m_fntBold;
	
// Construction
public:
	CAboutDlg2(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAboutDlg2)
	enum { IDD = IDD_ABOUT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	// Generated message map functions
	//{{AFX_MSG(CAboutDlg2)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif // _INC_ABOUT_