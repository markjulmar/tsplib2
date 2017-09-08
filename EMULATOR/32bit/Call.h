/*****************************************************************************/
//
// CALL.H - Digital Switch Service Provider Sample
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
// CCallDlg dialog
//
class CCallDlg : public CDialog
{
// Class data
public:
    //{{AFX_DATA(CCallDlg)
	enum { IDD = IDD_CALLNUMBER };
    CListBox m_lbMediaModes;
    CEdit m_edtCallingNum;
    CEdit m_edtCallingName;
    CStatic m_statCall1;
    CStatic m_statCall2;
    CString m_strCaller;
    CString m_strCallerNum;
    CComboBox m_cbLines;
    CStringArray m_arrAddr;
    CUIntArray m_arrIndex;
    DWORD m_dwMediaTypes;
    int m_iLine;
	BOOL	m_fExternal;
	//}}AFX_DATA

// Construction
public:
    CCallDlg(CWnd* pParent = NULL);  // standard constructor

// Implementation
public:               
    void AddLine(int iPos, const char* pszName);
    int GetLineCount() const { return m_arrAddr.GetSize(); }
    int GetIncomingLine() { return m_iLine; }
    const char * GetCallerName() const { return m_strCaller; }
    const char * GetCallerNum() const { return m_strCallerNum; }
    DWORD GetMediaTypes() const { return m_dwMediaTypes; }
	BOOL IsExternal() const { return m_fExternal; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support
    virtual BOOL OnInitDialog();
    void  FillMediaListbox();

    // Generated message map functions
    //{{AFX_MSG(CCallDlg)
    virtual void OnOK();
    afx_msg void OnInternalcall();
    afx_msg void OnBtnClear();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

