/******************************************************************************/
//                                                                        
// CONFIG.CPP - TAPI Service Provider for AT style modems
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

#include "stdafx.h"
#include "atsp.h"
#include "resource.h"
#include "config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::CSpConfigDlg
//
// Constructor for the ATSP Configuration dialog
//
CSpConfigDlg::CSpConfigDlg(CWnd* pParent, DWORD dwLineID, TUISPIDLLCALLBACK lpfnDLLCallback)
   : CDialog(CSpConfigDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSpConfigDlg)
	m_strPort = _T("");
    m_strSpeed = _T("");
    m_strLineAddr = _T("");
    m_strInitStr = _T("");
    m_strAddrName = _T("");
    m_fPulse = FALSE;
	m_dwLineID = dwLineID;
	m_lpfnTSP = lpfnDLLCallback;
	//}}AFX_DATA_INIT
    
}// CSpConfigDlg::CSpConfigDlg

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::DoDataExchange
//
// Dialog Data exchange function for the ATSP Configuration dialog
//
VOID CSpConfigDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSpConfigDlg)
    DDX_CBString(pDX, IDC_SPEED, m_strSpeed);
    DDX_Text(pDX, IDC_LINEADDR, m_strLineAddr);
    DDX_Text(pDX, IDC_INITSTR, m_strInitStr);
    DDX_Text(pDX, IDC_ADDRNAME, m_strAddrName);
    DDX_Check(pDX, IDC_PULSE, m_fPulse);
	DDX_CBString(pDX, IDC_PORT, m_strPort);
	DDX_Control(pDX, IDC_PORT, m_cbPort);
	//}}AFX_DATA_MAP
    
}// CSpConfigDlg::DoDataExchange

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg Message Map
//
BEGIN_MESSAGE_MAP(CSpConfigDlg, CDialog)
    //{{AFX_MSG_MAP(CSpConfigDlg)
    ON_BN_CLICKED(IDOK, OnOk)
	ON_EN_CHANGE(IDC_LINEADDR, ManageControls)
	ON_EN_CHANGE(IDC_ADDRNAME, ManageControls)
	ON_EN_CHANGE(IDC_INITSTR, ManageControls)
	ON_CBN_SELCHANGE(IDC_SPEED, ManageControls)
	ON_CBN_SELCHANGE(IDC_PORT, ManageControls)
	ON_BN_CLICKED(IDC_PULSE, ManageControls)
	ON_BN_CLICKED(IDC_USESECS, ManageControls)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::OnInitDialog
//
// Message handler for the WM_INITDIALOG (ATSP Configuration dialog).
//
BOOL CSpConfigDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // Set the font for all the controls to something
    // a little better than the default.
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd* pwndChild = GetWindow (GW_CHILD);
    while (pwndChild != NULL && IsChild (pwndChild))
    {
        pwndChild->SetFont (&fntAnsi);
        pwndChild = pwndChild->GetWindow (GW_HWNDNEXT);
    }
    
    // Center the window on the screen
    CenterWindow(GetDesktopWindow());

	// Fill the COMM port combo with available comm ports.
	FillCommPorts();

	// Gather all the configuration data from the
	// registry.
	m_strPort = ReadProfileString (GetString(IDS_COMMPORT));
    m_strSpeed = ReadProfileString(GetString(IDS_BAUD));
    m_strInitStr = ReadProfileString(GetString(IDS_INITSTR));
    m_strLineAddr = ReadProfileString(GetString(IDS_LINEADDR));
    m_strAddrName = ReadProfileString(GetString(IDS_ADDRNAME));
	CString strBuff = ReadProfileString(GetString(IDS_PULSE));
	if (!strBuff.IsEmpty())
		m_fPulse = (strBuff[0] == _T('1'));

	// Validate our COMM port.
	if (!m_strPort.IsEmpty() && 
		m_cbPort.FindStringExact(-1, m_strPort) == CB_ERR)
	{
		AfxMessageBox (_T("The COM port originally assigned to this provider is no longer available."));
		m_strPort.Empty();
	}

	UpdateData (FALSE);

    return FALSE;     // We didn't set focus anywhere
    
}// CSpConfigDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::FillCommPorts
//
// Fill the comm port combo box with all the available COMM ports
// on this computer.
//
// This works under Windows 95 (TAPI 2.1) and Windows NT.
//
void CSpConfigDlg::FillCommPorts()
{
	HKEY hkeyLocal;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialCOMM"), 0, 
				KEY_READ, &hkeyLocal) == ERROR_SUCCESS)
	{
		// DeviceMap knows about the comm ports.. use it instead of our pre-filled 
		// information in the combo box.
		m_cbPort.ResetContent();

		// Enumerate through the COMM keys in the device map.
		TCHAR szBuff[50];
		TCHAR szCOMM[50];
		DWORD dwSize = sizeof(szBuff), dwCommSize = sizeof(szCOMM);
		DWORD dwIndex = 0, dwType;
		while (RegEnumValue (hkeyLocal, dwIndex, szBuff, &dwSize, 
					NULL, &dwType, (LPBYTE)szCOMM, &dwCommSize) == ERROR_SUCCESS)
		{
			if (dwType == REG_SZ)
				m_cbPort.AddString (szCOMM);
			dwIndex++;
			dwSize = sizeof(szBuff);
			dwCommSize = sizeof(szCOMM);
		}

		RegCloseKey (hkeyLocal);
	}

}// CSpConfigDlg::FillCommPorts

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::ManageControls
//
// Enable/Disable the OK button based on whether we have all
// the information we need.
//
void CSpConfigDlg::ManageControls()
{
	UpdateData (TRUE);
	if (!m_strPort.IsEmpty() &&
		!m_strSpeed.IsEmpty() &&
		!m_strLineAddr.IsEmpty())
		GetDlgItem(IDOK)->EnableWindow (TRUE);
	else
		GetDlgItem(IDOK)->EnableWindow (FALSE);

}// CSpConfigDlg::ManageControls

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::OnOk
//
// Process the IDOK button on the configuration dialog
//
VOID CSpConfigDlg::OnOk()
{
	// Get the current values from the dialog.
    UpdateData(TRUE);

	// Write it out to our registry entries.
	WriteProfileString (GetString(IDS_COMMPORT), m_strPort);
	WriteProfileString (GetString(IDS_BAUD), m_strSpeed);
    WriteProfileString (GetString(IDS_INITSTR), m_strInitStr);
    WriteProfileString (GetString(IDS_LINEADDR), m_strLineAddr);
    WriteProfileString (GetString(IDS_ADDRNAME), m_strAddrName);
	WriteProfileString (GetString(IDS_PULSE), (m_fPulse) ? _T("1") : _T("0"));

    EndDialog(IDOK);

}// CSpConfigDlg::OnOk

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::ReadProfileString
//
// Read a string from our registry entries.
//
CString CSpConfigDlg::ReadProfileString (LPCTSTR pszKey)
{
	return GetSP()->ReadProfileString (m_dwLineID, pszKey);

}// CSpConfigDlg::ReadProfileString

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::WriteProfileString
//
// Write a string into our registry entries.
//
VOID CSpConfigDlg::WriteProfileString (LPCTSTR pszKey, LPCTSTR pszValue)
{
	GetSP()->WriteProfileString (m_dwLineID, pszKey, pszValue);

}// CSpConfigDlg::ReadProfileString
