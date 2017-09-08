/*****************************************************************************/
//
// CONTROLS.H - Digital Switch Service Provider Sample
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
            
void AFXAPI DDX_CBStringArray(CDataExchange* pDX, int nIDC, CStringArray& array);
            
/*****************************************************************************/
//
// CToolTip - ToolTip window class
//
/*****************************************************************************/
class CToolTip : public CWnd
{
private:
    CString  m_strText;  // String to display (passed in on constructor)
    static CString CToolTip::m_strClassName;

public:
    CToolTip(const char *pszText);
    virtual ~CToolTip();
    BOOL CreateHelpTag(CRect& rcArea);
	CPoint CalcStartPos(CRect& rcArea);

// Implementation
protected:
    virtual void PostNcDestroy();
    // Generated message map functions
    //{{AFX_MSG(CToolTip)
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/*****************************************************************************/
//
// CMfxBitmapButton - Bitmap button with "ToolTip" support.
//
/*****************************************************************************/
class CMfxBitmapButton : public CButton
{
// Class data
public:
    enum HelpStyle { HelpTypeNone = 0, HelpTypeTool = 1 };
private:
    CToolTip* m_pToolTip;             // Tool tip which is visible (only one of these)
    DWORD     m_dwTimeout;            // Timeout value to destroy tooltip.
    COLORREF  m_clrTransparent;       // Transparent color for bitmap image
    COLORREF  m_clrShadow;            // Shadow color for disabling effect
    int       m_ibmWidth;             // Bitmap image width
    int       m_ibmHeight;            // Bitmap image height;
    CWnd*     m_pwndNotify;                       // Notification window
    CBitmap   m_bmButton;             // Button bitmap       
    CBitmap   m_bmMask;               // Mask used for button.
    WORD      m_wFlags;               // Style flags
    BOOL      m_fChecked : 1;         // Button is "checked"
    BOOL      m_fMouseButtonDown : 1; // (1=mouse button down)
    BOOL      m_fTimerRunning : 1;    // Interval timer running.
    BOOL      m_fIsCheckable : 1;     // Button can be CHECKED.
    BOOL      m_fHasFocus : 1;            // Button has focus
    
// Constructors
public:
    CMfxBitmapButton();
    virtual ~CMfxBitmapButton();
    virtual WNDPROC* GetSuperWndProcAddr();
    BOOL LoadBitmap(UINT uidResId, COLORREF clr=RGB(192,192,192), COLORREF clr2=0);
    BOOL LoadBitmap(LPCSTR lpszResID, COLORREF clr=RGB(192,192,192), COLORREF clr2=0);
    BOOL LoadBitmap(HINSTANCE hInst, LPCSTR lpszResID, COLORREF clr=RGB(192,192,192), COLORREF clr2=0);
    void SizeToContent();
    void SetAutoCheck (BOOL fAutoCheck);
    void SetCheck (int nCheck);
    int GetCheck () const;
    void SetStyle (enum CMfxBitmapButton::HelpStyle wStyle);
    void SetNotifyWindow (CWnd* pwndNotify);

// Message Map stuff
protected:
    virtual void DrawItem (LPDRAWITEMSTRUCT lpdi);
    //{{AFX_MSG(CMfxBitmapButton)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT nIdTimer);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnSetFocus (CWnd* pwndOld);
    afx_msg void OnKillFocus(CWnd* pwndOld);
    afx_msg void OnEnable(BOOL fEnable);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

// Private methods
private:
    void DestroyHelpTag();
    void DrawButtonFace (CDC* pDC, CRect& rcButton, CPoint& ptOffset, BOOL fPressed, BOOL fDither);
    BOOL ProcessBitmap (HANDLE hResource, COLORREF clrTransparent, COLORREF clrShadow);
    HBITMAP CreateBitmapMask(LPBITMAPINFOHEADER lpBI, COLORREF clrTrans, BOOL bMask=TRUE);
};

///////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::SizeToContent
//
// Autosize the button to match the bitmap + edges.
//
inline void CMfxBitmapButton::SizeToContent()
{                            
    SetWindowPos(NULL, 0, 0, m_ibmWidth + 8, m_ibmHeight + 8, SWP_NOMOVE|SWP_NOZORDER|SWP_NOREDRAW|SWP_NOACTIVATE);

}// CMfxBitmapButton::SizeToContent

            