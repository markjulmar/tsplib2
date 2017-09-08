/******************************************************************************/
//                                                                        
// CONFIG.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our configuration code for the 
// ATSP sample driver.
// 
// Copyright (C) 1994, 1995, 1996 Mark C. Smith
// All rights reserved                                                    
//                                                                        
// This service provider drives a Hayes compatible AT style modem.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
// Change History
// -------------------------------------------------------------------
// 09/04/94 Mark Smith (MCS)
//      Initial revision.
// 
/******************************************************************************/

class CSpConfigDlg : public CDialog
{
// Dialog Data
private:
	//{{AFX_DATA(CSpConfigDlg)
	enum { IDD = IDD_CFGDLG };
	int m_iPort;
	CString m_strSpeed;
	CString m_strLineAddr;
	CString m_strInitStr;
	CString m_strAddrName;
	BOOL m_fPulse;
	DWORD m_dwProviderID;
	HINSTANCE m_h3dLib;
	//}}AFX_DATA

// Construction
public:
   	CSpConfigDlg(CWnd* pParent, DWORD dwPpid);   
	~CSpConfigDlg();

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support
	// Generated message map functions
	//{{AFX_MSG(CSpConfigDlg)
	virtual BOOL OnInitDialog();
	afx_msg VOID OnOk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


