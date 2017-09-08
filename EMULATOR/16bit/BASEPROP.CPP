//*****************************************************************************
//
// BASEPROP.CPP
//
// Digital switch emulator program
//
//*****************************************************************************

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
// CBasePropertySheet::~CBasePropertySheet
//
// Destructor for our own property sheet
//
CBasePropertySheet::~CBasePropertySheet()
{                                          
    
}// CBasePropertySheet::~CBasePropertySheet
                    
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
