/******************************************************************************/
//
// DIALOG.H - Dialog to run the installation sample
//
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// Copyright (C) 1997 Mark C. Smith
// All rights reserved.
//
// This source code is only intended as a supplement to the
// documentation provided with the TSPI C++ class library.
// 
// Change History
// -----------------------------------------------------------------
// 05/01/1997	Mark Smith (MCS@JulMar)	Initial revision
//
/******************************************************************************/

#include "resource.h"		// main symbols

class CMainApp : public CWinApp
{
// Overrides
public:
	virtual BOOL InitInstance();
};

class CInstallDlg : public CDialog
{
// Dialog Data
protected:
	//{{AFX_DATA(CInstallDlg)
	enum { IDD = IDD_INSTALL_DIALOG };
	BOOL	m_fATSP;
	BOOL	m_fDSSP;
	BOOL	m_foATSP;
	BOOL	m_foDSSP;
	CString	m_strATSPOk;
	CString	m_strDSSPOk;
	CString m_strPath;
	CString m_strSystemDir;
	//}}AFX_DATA

// Construction
public:
	CInstallDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CInstallDlg();
	static BOOL TAPI_Installed();

// Implementation
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInstallDlg)
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(CInstallDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
// Internal methods
private:
	static CString GetString(int iResID);
	BOOL CheckForFile(int iResID);
	DWORD TAPI_addProvider(int iResID);
	BOOL TAPI_removeProvider(int iResID);
	BOOL TAPI_checkForDevice(int iResID);
	BOOL TAPI_runConfig (DWORD dwPpid, int iResID);
};
