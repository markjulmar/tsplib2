/*****************************************************************************/
//
// CONFGDLG.CPP - Digital Switch Service Provider Sample
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
#include "confgdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
    
///////////////////////////////////////////////////////////////////////////////
// Globals

struct
{
    const char *pszText;	// Text
    WORD wFunction;         // Button function
    
} g_Functions[] = {

    { "Call", 		BUTTONFUNCTION_CALL },
    { "Conference", BUTTONFUNCTION_CONFERENCE },
    { "Display", 	BUTTONFUNCTION_DISPLAY },
    { "Forward", 	BUTTONFUNCTION_FORWARD },
    { "Transfer", 	BUTTONFUNCTION_TRANSFER },
    { "UNUSED", 	BUTTONFUNCTION_NONE },
    { NULL, 0 }
};    

extern const char * g_pszButtonFace[100];
    
///////////////////////////////////////////////////////////////////////////////
// CConfigDlg::CConfigDlg
//
// Constructor for the configuration dialog
//
CConfigDlg::CConfigDlg() : CPropertyPage(CConfigDlg::IDD)
{
    //{{AFX_DATA_INIT(CConfigDlg)
    //}}AFX_DATA_INIT            
    for (int i = 0; i < 16; i++)
        m_iButtonInfo[i] = BUTTONFUNCTION_NONE;
    
}// CConfigDlg::CConfigDlg

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg::DoDataExchange
//
// Dialog data exchange for the configuration dialog.
//
void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfigDlg)
    DDX_Control(pDX, IDC_LIST_FUNCTIONS, m_lbFunctions);
    //}}AFX_DATA_MAP
    
}// CConfigDlg::DoDataExchange

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg message map

BEGIN_MESSAGE_MAP(CConfigDlg, CPropertyPage)
    //{{AFX_MSG_MAP(CConfigDlg)
    ON_BN_CLICKED(IDC_BUTTON0, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON1, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON2, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON3, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON4, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON5, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON6, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON7, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON8, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON9, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON10, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON11, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON12, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON13, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON14, OnButtonPress)
    ON_BN_CLICKED(IDC_BUTTON15, OnButtonPress)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg::OnInitDialog
//
// Read the settings from our .INI file
// 
BOOL CConfigDlg::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    CenterWindow();

    // Set the font on all child controls
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd * pWnd = GetWindow (GW_CHILD);
    while (pWnd != NULL && IsChild (pWnd) )
    {
        pWnd->SetFont (& fntAnsi);
        pWnd = pWnd->GetWindow (GW_HWNDNEXT);
    }

    // Add the functions to the listbox
    for (int i = 0; g_Functions[i].pszText != NULL; i++)
    {
        int iPos = m_lbFunctions.AddString (g_Functions[i].pszText);
        ASSERT (iPos != LB_ERR);
        m_lbFunctions.SetItemData (iPos, (DWORD) g_Functions[i].wFunction);
    }        
    
    // Get the current button settings from our .INI file
    for (i = 0 ; i < 16 ; i++)
    {                                    
        CString strKey, strText;
        strKey.Format("Key%d", i+FIRST_FUNCTION_INDEX);
        m_iButtonInfo[i] = AfxGetApp()->GetProfileInt ("Buttons", strKey, BUTTONFUNCTION_NONE);
        strKey = g_pszButtonFace[(int)m_iButtonInfo[i]];
        if (strKey.IsEmpty() == FALSE)
        {
	        if (m_iButtonInfo[i] == BUTTONFUNCTION_CALL)
	        {     
	            int iCount = 0;
	            for (int x = 0; x < 16; x++)
	            {
	                if (m_iButtonInfo[x] == BUTTONFUNCTION_CALL)
	                    iCount++;    
	            }          
	            strText.Format(strKey, iCount);
	        }
	        else
	        	strText = strKey;
		}
        if (strText.IsEmpty())
            strText = "UNUSED";
        GetDlgItem (IDC_BUTTON0+i)->SetWindowText(strText);
    }
    
    return TRUE;

}// CConfigDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg::OnOK
//
// Dismiss the property page
//
void CConfigDlg::OnOK()
{
    WriteButtonInfo();
    CPropertyPage::OnOK();

}// CConfigDlg::OnOK

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg::OnSetActive
//
// This tab is becoming the active page
//
BOOL CConfigDlg::OnSetActive()
{
    return CPropertyPage::OnSetActive();
    
}// CConfigDlg::OnSetActive

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg::OnKillActive
//
// This tab is being switch away from
//
BOOL CConfigDlg::OnKillActive()
{
    WriteButtonInfo();
    return CPropertyPage::OnKillActive();
    
}// CConfigDlg::OnKillActive

///////////////////////////////////////////////////////////////////////////////
// CConfigDlg::OnButtonPress
//
// Assign a button function to a button on our phone.
//
void CConfigDlg::OnButtonPress()
{
    int i = (GetFocus()->GetDlgCtrlID() - IDC_BUTTON0);
    int iCurSel = m_lbFunctions.GetCurSel();
    if (iCurSel != LB_ERR)
    {
        m_iButtonInfo[i] = (int) m_lbFunctions.GetItemData (iCurSel);
        CString strKey = g_pszButtonFace[m_iButtonInfo[i]];
        if (m_iButtonInfo[i] == BUTTONFUNCTION_CALL)
        {     
			CString strTemp;
            int iCount = 0;
            for (int i = 0; i < 16; i++)
            {
                if (m_iButtonInfo[i] == BUTTONFUNCTION_CALL)
                    iCount++;    
            }
            strTemp.Format(strKey, iCount);
			strKey = strTemp;
        }
        
        if (strKey.IsEmpty())
            strKey = "UNUSED";
        GetDlgItem (IDC_BUTTON0+i)->SetWindowText(strKey);
    }

}// CConfigDlg::OnButtonPress

////////////////////////////////////////////////////////////////////////
// CConfigDlg::WriteButtonInfo
//
// Write the button information into the .INI file
//
void CConfigDlg::WriteButtonInfo()
{                              
    for (int i = 0; i < 16; i++)
    {
        CString strKey;
        strKey.Format("Key%d", i+FIRST_FUNCTION_INDEX);
        AfxGetApp()->WriteProfileInt ("Buttons", strKey, m_iButtonInfo[i]);
    }            

}// CConfigDlg::WriteButtonInfo
