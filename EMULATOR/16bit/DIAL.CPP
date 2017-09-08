//*****************************************************************************
//
// DIAL.CPP
//
// Dialing dialog used for DSSP Emulation program
//
// Written 11/02/94 Mark Smith (MCS)
//
//*****************************************************************************

#include "stdafx.h"
#include "objects.h"
#include "lamp.h"
#include "colorlb.h"
#include "resource.h"
#include "emulator.h"
#include "dial.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDialDlg::CDialDlg
//
// Constructor for the dialing dialog.
//
CDialDlg::CDialDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CDialDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CDialDlg)
    m_strDisplay = "";
    m_fUsePulse = FALSE;
    //}}AFX_DATA_INIT
    
}// CDialDlg::CDialDlg

/////////////////////////////////////////////////////////////////////////////
// CDialDlg::DoDataExchange
//
// Process the dialog data exchange
//
void CDialDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDialDlg)
    DDX_Text(pDX, IDC_DIALEDIT, m_strDisplay);
    DDX_Control(pDX, IDC_DIALEDIT, m_edtDisplay);
    DDX_Check(pDX, IDC_CHECK_PULSE, m_fUsePulse);
    //}}AFX_DATA_MAP

}// CDialDlg::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
// CDialDlg message map

BEGIN_MESSAGE_MAP(CDialDlg, CDialog)
    //{{AFX_MSG_MAP(CDialDlg)
    ON_BN_CLICKED(IDC_BUTTON1, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON2, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON3, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON4, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON5, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON6, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON7, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON8, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON9, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON0, OnButtonPress)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialDlg::OnInitDialog
//
// Process the WM_INITDIALOG and initialize our dialog.
//
BOOL CDialDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    CenterWindow();
    m_edtDisplay.SetFocus();
    return FALSE;

}// CDialDlg::OnInitDialog

/////////////////////////////////////////////////////////////////////////////
// CDialDlg::OnButtonPress
//
// Process the pressing of a button
//
void CDialDlg::OnButtonPress()
{
    CButton * pButton = (CButton *)GetFocus();
    int iButton = pButton->GetDlgCtrlID() - IDC_BUTTON0;
    char cNum = (char)(iButton + '0');

    UpdateData (TRUE);
    m_edtDisplay.SetRedraw (FALSE);
    GotoDlgCtrl (& m_edtDisplay);
    m_strDisplay += cNum;
    m_edtDisplay.SetRedraw (TRUE);
    UpdateData (FALSE);

}// CDialDlg::OnButtonPress

