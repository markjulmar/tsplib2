// about.cpp : implementation file
//

#include "stdafx.h"
#include "pingint.h"
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
// CAboutDlg2 dialog


CAboutDlg2::CAboutDlg2(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutDlg2::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAboutDlg2)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CAboutDlg2::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg2)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg2, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg2 message handlers

BOOL CAboutDlg2::OnInitDialog()
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
