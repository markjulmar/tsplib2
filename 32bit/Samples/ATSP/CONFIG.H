/******************************************************************************/
//                                                                        
// CONFIG.H - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our configuration code for the 
// ATSP sample driver.
// 
// This service provider drives a Hayes compatible AT style modem.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// Original Copyright © 1994-2004 JulMar Entertainment Technology, Inc. All rights reserved.
//
// "This program is free software; you can redistribute it and/or modify it under the terms of 
// the GNU General Public License as published by the Free Software Foundation; version 2 of the License.
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without 
// even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General 
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program; if not, write 
// to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 
// Or, contact: JulMar Technology, Inc. at: info@julmar.com." 
//                                                           
/******************************************************************************/

class CSpConfigDlg : public CDialog
{
// Dialog Data
private:
	//{{AFX_DATA(CSpConfigDlg)
	enum { IDD = IDD_CFGDLG };
	CString m_strSpeed;
	CString m_strLineAddr;
	CString m_strInitStr;
	CString m_strAddrName;
	BOOL m_fPulse;
	DWORD m_dwLineID;
	CString	m_strPort;
	CComboBox m_cbPort;
	TUISPIDLLCALLBACK m_lpfnTSP;
	//}}AFX_DATA

// Construction
public:
	CSpConfigDlg(CWnd* pParent, DWORD dwLineID, TUISPIDLLCALLBACK lpfnDLLCallback);

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support
	// Generated message map functions
	//{{AFX_MSG(CSpConfigDlg)
	virtual BOOL OnInitDialog();
	afx_msg void ManageControls();
	afx_msg VOID OnOk();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Internal functions
private:
	CString ReadProfileString (LPCTSTR pszKey);
	VOID WriteProfileString (LPCTSTR pszKey, LPCTSTR pszValue);
	void FillCommPorts();
};


