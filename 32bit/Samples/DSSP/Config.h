/*****************************************************************************/
//
// CONFIG.H - Digital Switch Service Provider Sample
//                                                                        
// This file contains the configuration support code.
// 
// Copyright (C) 1997-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
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
/*****************************************************************************/

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog

class CConfigDlg : public CDialog
{
// Dialog Data
private:
	//{{AFX_DATA(CConfigDlg)
	enum { IDD = IDD_CONFIGURE };
	CString	m_strIPAddress;
	DWORD m_dwProviderID;
	int		m_iTimeout;
	//}}AFX_DATA

// Construction
public:
	CConfigDlg(CWnd* pParent, DWORD dwPPID);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CConfigDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnChangeIpaddress();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
