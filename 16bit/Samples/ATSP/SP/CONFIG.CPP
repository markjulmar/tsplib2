/******************************************************************************/
//                                                                        
// CONFIG.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our configuration code for the 
// ATSP sample driver.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider drives a Hayes compatible AT style modem.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
/******************************************************************************/

#include "stdafx.h"
#include "atsp.h"
#include "resource.h"
#include "config.h"
#include "ctl3d.h"

/////////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::CSpConfigDlg
//
// Constructor for the ATSP Configuration dialog
//
CSpConfigDlg::CSpConfigDlg(CWnd* pParent /*=NULL*/, DWORD dwProviderID)
   : CDialog(CSpConfigDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSpConfigDlg)
    m_strSpeed = "";
    m_iPort = 0;
    m_strLineAddr = "";
    m_strInitStr = "";
    m_strAddrName = "";
    m_fPulse = FALSE;
    m_dwProviderID = dwProviderID;
    m_h3dLib = NULL;
    //}}AFX_DATA_INIT
    
}// CSpConfigDlg::CSpConfigDlg
                    
/////////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::~CSpConfigDlg
//
// Destructor for the ATSP Configuration dialog
//
CSpConfigDlg::~CSpConfigDlg()
{
    if (m_h3dLib)
    {
		typedef int (CALLBACK* EXITPROC)(HINSTANCE);
		EXITPROC pfnExit = (EXITPROC) GetProcAddress (m_h3dLib, "CtlUnregister");
		if (pfnExit)
			pfnExit (AfxGetInstanceHandle());
    	::FreeLibrary (m_h3dLib);
    }
    
}// CSpConfigDlg::~CSpConfigDlg
                    
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
    DDX_CBIndex(pDX, IDC_PORT, m_iPort);
    DDX_Text(pDX, IDC_LINEADDR, m_strLineAddr);
    DDX_Text(pDX, IDC_INITSTR, m_strInitStr);
    DDX_Text(pDX, IDC_ADDRNAME, m_strAddrName);
    DDX_Check(pDX, IDC_PULSE, m_fPulse);
    //}}AFX_DATA_MAP
    
}// CSpConfigDlg::DoDataExchange

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg Message Map
//
BEGIN_MESSAGE_MAP(CSpConfigDlg, CDialog)
    //{{AFX_MSG_MAP(CSpConfigDlg)
    ON_BN_CLICKED(IDOK, OnOk)
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
    
    // Center the window on the screen and force it to "popup".
    // TAPI 2.1 under Win95 has some odd problems in this area since the parent window
    // is a 32-bit window.
    CenterWindow(GetParent());
    SetWindowPos (&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
   
    // Get the settings from the .INI file.      
    m_strSpeed 		= GetSP()->ReadProfileString(m_dwProviderID, GetString(IDS_BAUD));
    m_strInitStr 	= GetSP()->ReadProfileString(m_dwProviderID, GetString(IDS_INITSTR));
    m_strLineAddr 	= GetSP()->ReadProfileString(m_dwProviderID, GetString(IDS_LINEADDR));
    m_strAddrName 	= GetSP()->ReadProfileString(m_dwProviderID, GetString(IDS_ADDRNAME));
    m_iPort 		= (int) (GetSP()->ReadProfileDWord(m_dwProviderID, GetString(IDS_PORT), 1) - 1);
    m_fPulse 		= (BOOL) GetSP()->ReadProfileDWord(m_dwProviderID, GetString(IDS_PULSE), FALSE);
    
    // If we don't have a baud rate, then we didn't have a section in our registry.
    // Force the user to configure the device properly.
    if (m_strSpeed.IsEmpty())
    {
    	GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
        m_strSpeed = "9600";                      
    }

    UpdateData(FALSE);         

    return FALSE;     // We didn't set focus anywhere
    
}// CSpConfigDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////
// CSpConfigDlg::OnOk
//
// Process the IDOK button on the configuration dialog
//
VOID CSpConfigDlg::OnOk()
{
    if (UpdateData(TRUE))
    {
    	// Make sure we have a dialable address.
    	if (m_strLineAddr.IsEmpty())
    	{
    		AfxMessageBox(IDS_ERR_NOLINEADDR);
    		GotoDlgCtrl(GetDlgItem(IDC_LINEADDR));
    		return;
    	}
    
    	// Update our profile
	    GetSP()->WriteProfileString(m_dwProviderID, GetString(IDS_BAUD),m_strSpeed);      
	    GetSP()->WriteProfileString(m_dwProviderID, GetString(IDS_INITSTR),m_strInitStr);      
	    GetSP()->WriteProfileString(m_dwProviderID, GetString(IDS_LINEADDR),m_strLineAddr);   
	    GetSP()->WriteProfileString(m_dwProviderID, GetString(IDS_ADDRNAME),m_strAddrName);   
	    GetSP()->WriteProfileDWord(m_dwProviderID, GetString(IDS_PORT), (DWORD) m_iPort+1); 
	    GetSP()->WriteProfileDWord(m_dwProviderID, GetString(IDS_PULSE),(DWORD) m_fPulse);
	
    	EndDialog(IDOK);
    }

}// CSpConfigDlg::OnOk

