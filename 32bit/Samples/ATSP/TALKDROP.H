/******************************************************************************/
//                                                                        
// TALKDROP.H - Talk/Drop dialog for the ATSP service provider
//                                                                        
// This file contains the dialog definitions and code for the 
// Talk/Drop dialog which is used to facilitate action from 
// the user when a call is connected using the modem.
//
// This service provider drives a Hayes compatible AT style modem.  It
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
/******************************************************************************/

enum { TALKDROP_DESTROY = 0, TALKDROP_SHOW = 1 };
                       
class CTalkDropDlg : public CDialog
{
// Dialog Data
private:
    //{{AFX_DATA(CTalkDropDlg)
    enum { IDD = IDD_TALKDROP };
    DWORD m_dwCurrTime;                     // Current time
    CString m_strElapsedTime;               // Elapsed time
    HANDLE m_hInitEvent;                    // Init EVENT
    //}}AFX_DATA

// Construction
public:
    CTalkDropDlg(CWnd* pwndOwner, HANDLE hEvent);

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    // Generated message map functions
    //{{AFX_MSG(CTalkDropDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnDrop();
    afx_msg void OnTalk();
    afx_msg void OnTimer(UINT uidEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
