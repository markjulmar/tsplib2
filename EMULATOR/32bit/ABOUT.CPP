/*****************************************************************************/
//
// ABOUT.CPP - Digital Switch Service Provider Sample
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
#include "colorlb.h"
#include "resource.h"
#include "objects.h"
#include "baseprop.h"
#include "emulator.h"
#include "about.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog

CAboutDlg::CAboutDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAboutDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Walk through the dialog and set our ansi font.
	CFont fntAnsi;
	fntAnsi.CreateStockObject(ANSI_VAR_FONT);
	
	CWnd* pwnd = GetWindow (GW_CHILD);
	while (pwnd != NULL && IsChild(pwnd))
	{
		if (pwnd->GetDlgCtrlID() == IDC_STATIC)
			pwnd->SetFont (&fntAnsi);
		pwnd = pwnd->GetWindow (GW_HWNDNEXT);
	}
	
	// Now create our BOLD font for our title.
	LOGFONT lf;
	fntAnsi.GetObject(sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_fntBold.CreateFontIndirect(&lf);
	GetDlgItem (IDC_TITLE)->SetFont(&m_fntBold);

	// Center the dialog
	CenterWindow();
	
	return TRUE;
}
