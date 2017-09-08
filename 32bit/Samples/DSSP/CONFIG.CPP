/*****************************************************************************/
//
// CONFIG.CPP - Digital Switch Service Provider Sample
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

#include "stdafx.h"
#include "dssp.h"
#include "config.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/*****************************************************************************
** Procedure:  CDSProvider::providerInstall
**
** Arguments:  'dwPPID' - Permanent provider ID assigned by TAPI
**             'pwndOwner' - Owner window for any UI
**             'lpfnDLLCallback' - Callback for UI events
**
** Returns:    TAPI result code
**
** Description: This is called to install the provider into the system.
**              It verifies that only one copy of the provider is installed.
**
*****************************************************************************/
LONG CDSProvider::providerInstall(DWORD dwPermanentProviderID, CWnd* pwndOwner, 
								  TUISPIDLLCALLBACK lpfnDLLCallback)
{
	DWORD dwMyPPID;
	LONG lResult = IsProviderInstalled(AfxGetAppName(), &dwMyPPID);
	if (lResult == 0 &&
		CServiceProvider::providerInstall(dwPermanentProviderID, pwndOwner, lpfnDLLCallback) == 0)
		providerConfig(dwPermanentProviderID, pwndOwner, lpfnDLLCallback);
	return lResult;

}// CDSProvider::providerInstall

/**************************************************************************************
** Procedure: CDSProvider::lineConfigDialogEdit
**
** Arguments: 'pLine'			- Line object to show configuration for.
**            'pwndOwner'		- Owner window for any UI
**            'lpfnDLLCallback'	- Callback for notifications to TSP
**
** Returns:	TAPI 2.x return code
**
** Description: This function is called to run the configuration for a specific
**              line device owned by this TSP.
**
**************************************************************************************/
LONG CDSProvider::lineConfigDialogEdit(DWORD dwDeviceID, CWnd* pwndOwner, 
						CString& strDeviceClass, LPVOID const /*lpDeviceConfigIn*/, 
						DWORD /*dwSize*/, LPVARSTRING lpDeviceConfig, TUISPIDLLCALLBACK lpfnDLLCallback)
{
	// Set it to junk which we ignore.
	lpDeviceConfig->dwUsedSize = lpDeviceConfig->dwNeededSize = sizeof(VARSTRING)+1;
	lpDeviceConfig->dwStringFormat = STRINGFORMAT_BINARY;
	lpDeviceConfig->dwStringOffset = sizeof(VARSTRING);
	lpDeviceConfig->dwStringSize = 1;

	// Configure the line device.
	return lineConfigDialog (dwDeviceID, pwndOwner, strDeviceClass, lpfnDLLCallback);

}// CDSProvider::lineConfigDialogEdit

/**************************************************************************************
** Procedure: CDSProvider::lineConfigDialog
**
** Arguments: 'pLine'			- Line object to show configuration for.
**            'pwndOwner'		- Owner window for any UI
**            'lpfnDLLCallback'	- Callback for notifications to TSP
**
** Returns:	TAPI 2.x return code
**
** Description: This function is called to run the configuration for a specific
**              line device owned by this TSP.
**
**************************************************************************************/
LONG CDSProvider::lineConfigDialog(DWORD dwDeviceID, CWnd* pwndOwner, CString& /*strDeviceClass*/,
								   TUISPIDLLCALLBACK lpfnDLLCallback)
{
	// Convert the device id into a provider id.
	DWORD dwPPid;
	if (GetProviderIDFromDeviceID(lpfnDLLCallback, dwDeviceID, TRUE, &dwPPid) != 0)
		return LINEERR_OPERATIONFAILED;

	// Display the system configuration dialog, we don't qualify any configuration
	// data specific to any line supported.
    return providerConfig(dwPPid, pwndOwner, lpfnDLLCallback);

}// CDSProvider::lineConfigDialog

/*****************************************************************************
** Procedure:  CDSProvider::providerConfig
**
** Arguments:  'dwPPID' - Permanent provider ID
**             'pwndOwner' - UI dialog owner
**             'lpfnDLLCallback' - UI event callback.
**
** Returns:    TAPI result code
**
** Description:  This is used to configure the provider.
**
*****************************************************************************/
LONG CDSProvider::providerConfig(DWORD dwPPID, CWnd* pwndOwner, TUISPIDLLCALLBACK /*lpfnDLLCallback*/)
{
	CConfigDlg dlg(pwndOwner, dwPPID);
	dlg.DoModal();
	return 0;

}// CDSProvider::providerConfig

/*****************************************************************************
** Procedure:  CConfigDlg::CConfigDlg
**
** Arguments:  void
**
** Returns:    void
**
** Description: Constructor for the configuration dialgo.
**
*****************************************************************************/
CConfigDlg::CConfigDlg(CWnd* pParent, DWORD dwProviderID) : CDialog(CConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigDlg)
	m_strIPAddress = _T("");
	m_dwProviderID = dwProviderID;
	m_iTimeout = 0;
	//}}AFX_DATA_INIT

}// CConfigDlg::CConfigDlg

/*****************************************************************************
** Procedure:  CConfigDlg::DoDataExchange
**
** Arguments:  'pDX' - Dialog data exchange
**
** Returns:    void
**
** Description: Connects dialog variables up to the class objects.
**
*****************************************************************************/
void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDlg)
	DDX_Text(pDX, IDC_IPADDRESS, m_strIPAddress);
	DDX_Text(pDX, IDC_TIMEOUT, m_iTimeout);
	DDV_MinMaxInt(pDX, m_iTimeout, 3, 30);
	//}}AFX_DATA_MAP

}// CConfigDlg::DoDataExchange

/*****************************************************************************
** MFC MESSAGE MAP
*****************************************************************************/
BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	ON_EN_CHANGE(IDC_IPADDRESS, OnChangeIpaddress)
	ON_EN_CHANGE(IDC_TIMEOUT, OnChangeIpaddress)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*****************************************************************************
** Procedure:  CConfigDlg::OnInitDialog
**
** Arguments:  void
**
** Returns:    TRUE/FALSE whether focus management should be done by Windows.
**
** Description: This is called when the dialog is first created.
**
*****************************************************************************/
BOOL CConfigDlg::OnInitDialog() 
{
	// Write the data out
	m_strIPAddress = GetSP()->ReadProfileString (m_dwProviderID, _T("IPServer"), _T("127.0.0.1"));
	m_iTimeout = GetSP()->ReadProfileDWord (m_dwProviderID, _T("IPTimeout"), 5);

	// Connect up the elements to the dialgo.
	CDialog::OnInitDialog();
	OnChangeIpaddress();

	// Center the window
	CenterWindow();
	
	// Let Windows process focus.
	return TRUE;

}// CConfigDlg::OnInitDialog

/*****************************************************************************
** Procedure:  CConfigDlg::OnOK
**
** Arguments:  void
**
** Returns:    void
**
** Description: This is called when the user presses the OK button.
**
*****************************************************************************/
void CConfigDlg::OnOK() 
{
	if (UpdateData(TRUE))
	{
		// Quick validation of the data.
		CString strCheck = m_strIPAddress.SpanIncluding(_T("0123456789."));
		if (strCheck != m_strIPAddress)
		{
			AfxMessageBox (IDS_BADIPADDRESS);
			GotoDlgCtrl(GetDlgItem(IDC_IPADDRESS));
			return;
		}

		// Write the data out
		GetSP()->WriteProfileString (m_dwProviderID, _T("IPServer"), m_strIPAddress);

		// Dismiss the dialog.
		CDialog::OnOK();
	}

}// CConfigDlg::OnOK

/*****************************************************************************
** Procedure:  CConfigDlg::OnChangeIpaddress
**
** Arguments:  void
**
** Returns:    void
**
** Description: This is called when the user changes the contents of the
**              edit control.
**
*****************************************************************************/
void CConfigDlg::OnChangeIpaddress() 
{
	UpdateData(TRUE);
	GetDlgItem(IDOK)->EnableWindow(m_strIPAddress.IsEmpty() == FALSE && m_iTimeout > 0);

}// CConfigDlg::OnChangeIpaddress
