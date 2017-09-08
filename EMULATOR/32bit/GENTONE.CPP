/*****************************************************************************/
//
// GENTONE.CPP - Digital Switch Service Provider Sample
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
#include "objects.h"
#include "colorlb.h"
#include "resource.h"
#include "emulator.h"
#include "gentone.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global data
                     
struct
{
	const char * pszDesc;
	DWORD dwFreq1;
	DWORD dwFreq2;
	DWORD dwFreq3;
	
} g_ToneList[] = {
    // Do not change the frequencies, they are tied to DSSP.
	{ "Ringback", 1300, 1400, 1550 },
	{ "Busy",      500,  900,  400 },
	{ "Beep",     1250,  650, 1250 },
	{ "Billing",  1500, 1500,    0 },
	{ NULL, 0, 0, 0 }
};

/////////////////////////////////////////////////////////////////////////////
// CGenTone::CGenTone
//
// Tone Generation Dialog
//
CGenTone::CGenTone(CWnd* pParent, CAddressAppearance* pAddr)
	: CDialog(CGenTone::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGenTone)
	m_dwFreq1 = 0;
	m_dwFreq2 = 0;
	m_dwFreq3 = 0;
	//}}AFX_DATA_INIT
	m_pAddr = pAddr;

}// CGenTone::CGenTone

/////////////////////////////////////////////////////////////////////////////
// CGenTone::DoDataExchange
//
// Dialog data exchange for the tone generation dialog
//
void CGenTone::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGenTone)
	DDX_Control(pDX, IDC_TONELIST, m_lbToneList);
	DDX_Text(pDX, IDC_FREQ1, m_dwFreq1);
	DDX_Text(pDX, IDC_FREQ2, m_dwFreq2);
	DDX_Text(pDX, IDC_FREQ3, m_dwFreq3);
	//}}AFX_DATA_MAP

}// CGenTone::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
// CGenTone message map

BEGIN_MESSAGE_MAP(CGenTone, CDialog)
	//{{AFX_MSG_MAP(CGenTone)
	ON_BN_CLICKED(IDC_PREDEFINED, OnPredefined)
	ON_BN_CLICKED(IDC_USERDEFINED, OnUserdefined)
	ON_LBN_SELCHANGE(IDC_TONELIST, OnToneListSelchange)
	ON_BN_CLICKED(IDC_GENERATE, OnGenerate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGenTone::OnInitDialog
//
// Initial processing of the tone generation dialog
//
BOOL CGenTone::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    // Set the font on all child controls
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd * pWnd = GetWindow (GW_CHILD);
    while (pWnd != NULL && IsChild (pWnd) )
    {
        pWnd->SetFont (& fntAnsi);
        pWnd = pWnd->GetWindow (GW_HWNDNEXT);
    }

    CenterWindow();

	// Fill the tone list.
	for (int i = 0; g_ToneList[i].pszDesc != NULL; i++)
	{
		int iPos = m_lbToneList.AddString (g_ToneList[i].pszDesc);
		ASSERT (iPos >= 0);
		m_lbToneList.SetItemData (iPos, (DWORD) i);
	}

	// Set the initial selection to the pre-defined list.
	((CButton*)GetDlgItem (IDC_PREDEFINED))->SetCheck(TRUE);
	OnPredefined();

	return TRUE;          
	
}// CGenTone::OnInitDialog

/////////////////////////////////////////////////////////////////////////////
// CGenTone::OnPredefined
//
// Select a tone from the pre-defined list.
//
void CGenTone::OnPredefined() 
{                                          
	GetDlgItem (IDC_FREQ1)->EnableWindow (FALSE);
	GetDlgItem (IDC_FREQ2)->EnableWindow (FALSE);
	GetDlgItem (IDC_FREQ3)->EnableWindow (FALSE);
	m_lbToneList.EnableWindow (TRUE);
	m_lbToneList.SetCurSel (0);
	OnToneListSelchange();

}// CGenTone::OnPredefined

/////////////////////////////////////////////////////////////////////////////
// CGenTone::OnUserdefined
//
// Allow the user to select a tone.
//
void CGenTone::OnUserdefined() 
{                                     
	m_lbToneList.SetCurSel (-1);
	m_lbToneList.EnableWindow (FALSE);
	GetDlgItem (IDC_FREQ1)->EnableWindow (TRUE);
	GetDlgItem (IDC_FREQ2)->EnableWindow (TRUE);
	GetDlgItem (IDC_FREQ3)->EnableWindow (TRUE);
	
	m_dwFreq1 = m_dwFreq2 = m_dwFreq3 = 0L;   
	UpdateData (FALSE);

}// CGenTone::OnUserdefined

/////////////////////////////////////////////////////////////////////////////
// CGenTone::OnToneListSelchange
//
// The tone list selection has changed - update our frequency.
//
void CGenTone::OnToneListSelchange() 
{                                
	int iCurSel = m_lbToneList.GetCurSel();
	if (iCurSel >= 0)
	{
		int iIndex = (int) m_lbToneList.GetItemData (iCurSel);
		m_dwFreq1 = g_ToneList[iIndex].dwFreq1;
		m_dwFreq2 = g_ToneList[iIndex].dwFreq2;
		m_dwFreq3 = g_ToneList[iIndex].dwFreq3;
		UpdateData (FALSE);
	}

}// CGenTone::OnToneListSelchange

/////////////////////////////////////////////////////////////////////////////
// CGenTone::OnGenerate
//
// Send the tone generation to the service provider
//
void CGenTone::OnGenerate() 
{                       
    CEmulatorDlg* pwnd = (CEmulatorDlg*) AfxGetApp()->m_pMainWnd;
	EMTONEBUFF ToneBuff;
	
	UpdateData (TRUE);

	ToneBuff.wAddressID = m_pAddr->m_wAddressID;
	ToneBuff.dwFreq[0] = m_dwFreq1;
	ToneBuff.dwFreq[1] = m_dwFreq2;
	ToneBuff.dwFreq[2] = m_dwFreq3;
	
	CString strBuff;	
    strBuff.Format("Call %d <Tone 0x%lx 0x%lx 0x%lx>", m_pAddr->m_wAddressID, 
                    m_dwFreq1, m_dwFreq2, m_dwFreq3);
    pwnd->AddDebugInfo (1, strBuff);
    pwnd->SendNotification (EMRESULT_TONEDETECTED, (LPARAM)&ToneBuff);
    
    // Send silence to end tone.
	ToneBuff.dwFreq[0] = ToneBuff.dwFreq[1] = ToneBuff.dwFreq[2] = 0L;
    pwnd->SendNotification (EMRESULT_TONEDETECTED, (LPARAM)&ToneBuff);

}// CGenTone::OnGenerate
