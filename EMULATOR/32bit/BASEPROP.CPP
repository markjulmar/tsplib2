/*****************************************************************************/
//
// BASEPROP.CPP - Digital Switch Service Provider Sample
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
#include "baseprop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePropertySheet::CBasePropertySheet
//
// Constructor for the own property sheet class.
//
CBasePropertySheet::CBasePropertySheet (LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage) :
    CPropertySheet (pszCaption, pParentWnd, iSelectPage)
{   
}// CBasePropertySheet::CBasePropertySheet

/////////////////////////////////////////////////////////////////////////////
// CBasePropertySheet message map
//
BEGIN_MESSAGE_MAP(CBasePropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CPropertySheet)
    ON_WM_CREATE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBasePropertySheet::OnCreate
//
// Override to remove the APPLY/HELP buttons.
//
int CBasePropertySheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
        return -1;
    
    CWnd* pwnd = GetDlgItem (ID_APPLY_NOW);
    if (pwnd != NULL)
    {
        pwnd->ShowWindow (SW_HIDE);
        pwnd->EnableWindow (FALSE);
    }
    pwnd = GetDlgItem (ID_HELP);
    if (pwnd != NULL)
    {
        pwnd->ShowWindow (SW_HIDE);
        pwnd->EnableWindow (FALSE);
    }
    
    return 0;
    
}// CBasePropertySheet::OnCreate
