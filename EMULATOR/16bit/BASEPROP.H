//*****************************************************************************
//
// BASEPROP.H
//
// Digital switch emulator program
//
//*****************************************************************************

#ifndef __BASEPROP_INC__
#define __BASEPROP_INC__

//////////////////////////////////////////////////////////////////////////////
// CBasePropertySheet
//
class CBasePropertySheet : public CPropertySheet
{      
// Constructor
public:       
    CBasePropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage = 0);
    virtual ~CBasePropertySheet();
    
// Messages
protected:
    // Generated message map functions
    //{{AFX_MSG(CPropertySheet)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // __BASEPROP_INC__

