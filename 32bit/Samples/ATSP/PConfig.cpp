/******************************************************************************/
//                                                                        
// PCONFIG.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our configuration code for the 
// ATSP sample provider.
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
#include "PConfig.h"
#include "config.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProvConfig::CProvConfig
//
// Constructor for the provider level configuration dialog.
//
CProvConfig::CProvConfig(CWnd* pParent, DWORD dwPPid, TUISPIDLLCALLBACK lpfnTSP)
	: CDialog(CProvConfig::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProvConfig)
	m_dwPPid = dwPPid;
	m_lpfnTSP = lpfnTSP;
	m_fNewInstall = FALSE;
	//}}AFX_DATA_INIT

}// CProvConfig::CProvConfig

///////////////////////////////////////////////////////////////////////////
// CProvConfigDlg::DoDataExchange
//
// Dialog Data exchange function for the ATSP Configuration dialog
//
void CProvConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProvConfig)
	DDX_Control(pDX, IDC_REMOVE, m_btnRemove);
	DDX_Control(pDX, IDC_CONFIG, m_btnConfig);
	DDX_Control(pDX, IDC_MODEMLIST, m_lbModems);
	//}}AFX_DATA_MAP

}// CProvConfig::DoDataExchange

///////////////////////////////////////////////////////////////////////////
// CProvConfigDlg Message Map
//
BEGIN_MESSAGE_MAP(CProvConfig, CDialog)
	//{{AFX_MSG_MAP(CProvConfig)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_CONFIG, OnConfig)
	ON_LBN_SELCHANGE(IDC_MODEMLIST, OnSelchangeModemlist)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////
// CProvConfigDlg::OnInitDialog
//
// Message handler for the WM_INITDIALOG (ATSP Configuration dialog).
//
BOOL CProvConfig::OnInitDialog() 
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

	// Fill our modem list.
	FillModemList();

    // Center the window on the screen
    CenterWindow(GetDesktopWindow());

	UpdateData (FALSE);

    return FALSE;     // We didn't set focus anywhere

}// CProvConfig::OnInitDialog

///////////////////////////////////////////////////////////////////////////
// CProvConfig::FillModemList
//
// Fills the modem list with all our available lines.
//
void CProvConfig::FillModemList()
{
	TCHAR szBuff[80];
	DWORD dwCount = GetSP()->ReadProfileDWord(0, _T("Count"), 0);
	for (int i = 0; i < (int)dwCount; i++)
	{
		wsprintf (szBuff, _T("Sample Modem #%d"), i+1);
		int iPos = m_lbModems.AddString(szBuff);

		// Ask the provider which line id this is.
		DWORD dwLineID = ((m_dwPPid << 16) + i);
		m_lbModems.SetItemData (iPos, dwLineID);
	}

	if (m_lbModems.GetCount() == 0)
		m_fNewInstall = TRUE;

	OnSelchangeModemlist();

}// CProvConfig::FillModemList

///////////////////////////////////////////////////////////////////////////
// CProvConfig::OnAdd
//
// Adds a new line to the provider.
//
void CProvConfig::OnAdd() 
{
	// Calc the permanent line id using the permanent line algorithm.
	int iCount = m_lbModems.GetCount();
	DWORD dwLineID = ((m_dwPPid << 16) + iCount);

	// Add the line.
	TCHAR szBuff[80];
	wsprintf (szBuff, _T("Sample Modem #%d"), iCount+1);
	iCount = m_lbModems.AddString(szBuff);
	m_lbModems.SetItemData(iCount, dwLineID);

	// Increase the count.
	GetSP()->WriteProfileDWord(0, _T("Count"), m_lbModems.GetCount());
	m_lbModems.SetCurSel(iCount);
	OnSelchangeModemlist();

	// Now force the user to configure the modem.
	OnConfig();

	AfxMessageBox (_T("You will have to restart your TAPI application to see the new line."));

}// CProvConfig::OnAdd

///////////////////////////////////////////////////////////////////////////
// CProvConfig::OnRemove
//
// Adds a new line to the provider.
//
void CProvConfig::OnRemove() 
{
	// Remove the line from the listbox.
	int iCurSel = m_lbModems.GetCurSel();
	if (iCurSel != LB_ERR)
	{
		// Decrease the count.
		GetSP()->WriteProfileDWord(0, _T("Count"), m_lbModems.GetCount()-1);

		// Now delete the section associated with this modem.  We will then
		// go through all the other sections and move the data.
		DWORD dwPos = m_lbModems.GetItemData(iCurSel);
		GetSP()->DeleteProfile (dwPos);

		DWORD dwCount = (DWORD) m_lbModems.GetCount();

		// Remove the line from the listbox.
		m_lbModems.DeleteString(iCurSel);

		// Rename the other profile sections.
		for (DWORD dwItem = dwPos; dwItem <= dwPos+dwCount; dwItem++)
		{
			m_lbModems.SetItemData (iCurSel++, dwItem);
			GetSP()->RenameProfile (dwItem+1, dwItem);
		}

		// Our selection changed.
		OnSelchangeModemlist();
	}

}// CProvConfig::OnRemove

///////////////////////////////////////////////////////////////////////////
// CProvConfig::OnConfig
//
// Configure a line in our provider.
//
void CProvConfig::OnConfig() 
{
	// Invoke the CONFIGURATION dialog for this line.
	int iCurSel = m_lbModems.GetCurSel();
	if (iCurSel != LB_ERR)
	{
		DWORD dwLineID = m_lbModems.GetItemData(iCurSel);
		CSpConfigDlg Config (this, dwLineID, m_lpfnTSP);
		Config.DoModal();
	}

}// CProvConfig::OnConfig

///////////////////////////////////////////////////////////////////////////
// CProvConfig::OnSelchangeModemlist
//
// The selection for the listbox has changed - update the buttons.
//
void CProvConfig::OnSelchangeModemlist() 
{
	if (m_lbModems.GetCurSel() != LB_ERR)
	{
		m_btnRemove.EnableWindow (TRUE);
		m_btnConfig.EnableWindow (TRUE);
	}
	else
	{
		m_btnRemove.EnableWindow (FALSE);
		m_btnConfig.EnableWindow (FALSE);
	}

}// CProvConfig::OnSelchangeModemlist
