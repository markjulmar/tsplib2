/*****************************************************************************/
//
// GENDLG.H - Digital Switch Service Provider Sample
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

/////////////////////////////////////////////////////////////////////////////
// CGenerateDlg dialog

class CGenerateDlg : public CDialog
{
// Dialog Data
protected:
    //{{AFX_DATA(CGenerateDlg)
    enum { IDD = IDD_DIALGENERATE };
    BOOL    m_fUsePulse;
    CAddressAppearance* m_pAddr;
    //}}AFX_DATA

// Construction
public:
    CGenerateDlg(CWnd* pParent, CAddressAppearance* pAddr);

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    void SendNotification (char cDigit);

    // Generated message map functions
    //{{AFX_MSG(CGenerateDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNumberPress();
    afx_msg void OnButton10();
    afx_msg void OnButton11();
    afx_msg void OnBtnA();
    afx_msg void OnBtnB();
    afx_msg void OnBtnC();
    afx_msg void OnBtnD();
    afx_msg void OnCheckPulse();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
