/******************************************************************************/
//                                                                        
// PCONFIG.H - TAPI Service Provider for AT style modems
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

class CProvConfig : public CDialog
{
// Dialog Data
protected:
	//{{AFX_DATA(CProvConfig)
	enum { IDD = IDD_PROVCFG };
	CButton	m_btnRemove;
	CButton	m_btnConfig;
	CListBox m_lbModems;
	TUISPIDLLCALLBACK m_lpfnTSP;
	DWORD m_dwPPid;
	BOOL m_fNewInstall;
	//}}AFX_DATA

// Construction
public:
	CProvConfig(CWnd* pParent, DWORD dwPPid, TUISPIDLLCALLBACK lpfnTSP);

// Overrides
protected:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProvConfig)
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CProvConfig)
	virtual BOOL OnInitDialog();
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	afx_msg void OnConfig();
	afx_msg void OnSelchangeModemlist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Internal methods
private:
	void FillModemList();
};
