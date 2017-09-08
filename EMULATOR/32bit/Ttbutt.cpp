/******************************************************************************/
//                                                                        
// TTBUTT.CPP 
//
// ToolTip bitmap buttons and base tooltip classes.
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
/******************************************************************************/

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// Constants

const int IDT_TOOLTIP    = 100;
const int TIMER_INTERVAL = 200;
const long TAG_SHOWTIME  = 2000L;   // Total time to show tag when over button

// New color constants in Win95   
#define COLOR_3DDKSHADOW        21
#define COLOR_INFOTEXT          23
#define COLOR_INFOBK            24

// ROP codes we use.
#define ROP_PSDPxax  0x00B8074AL
#define ROP_DPo      0x00FA0089L
#define ROP_DPa      0x00A000C9L
#define ROP_DSPDxax  0x00E20746L
#define ROP_DSxn     0x00990066L

///////////////////////////////////////////////////////////////////////////
// Message map for the help tag class.

BEGIN_MESSAGE_MAP(CToolTip, CWnd)
    //{{AFX_MSG_MAP(CToolTip)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////
// CToolTip::CToolTip
//
// Constructor for our help tag.  Registers a window class (if one
// doesn't exist already)
//
CToolTip::CToolTip(const char *pszText)
{
    // Save off the text to display
    ASSERT(pszText != NULL);
    ASSERT(*pszText != '\0');
    m_strText = pszText;

}// CToolTip::CToolTip

////////////////////////////////////////////////////////////////////////////
// CToolTip::~CToolTip
//
// Destructor for help tag.
//
CToolTip::~CToolTip()
{
}// CToolTip::~CToolTip

///////////////////////////////////////////////////////////////////////////
// CToolTip::CreateHelpTag
//
// Method to create and show our help tag
//
BOOL CToolTip::CreateHelpTag(CRect& rcArea)
{
	static char g_szClassName[50];

    // Calculate the top/left coordinate of our help tag
    // window based on the coordinates of the button we
    // are representing.
    CPoint pt = CalcStartPos(rcArea);

    // Calculate the appropriate size of our help tag window based
    // on the length of the text we are about to display.
    CClientDC dc(NULL);
    dc.SelectStockObject(ANSI_VAR_FONT);
    CSize sizeText = dc.GetTextExtent(m_strText, m_strText.GetLength());

    // Adjust the rectangle to add an appropriate border onto the
    // tags.
    sizeText.cx += 8; 
    sizeText.cy += (sizeText.cy / 4);

    // See if the tag is off the screen, and if it is, move it back
    // onto the screen.
    int iScreenX = ::GetSystemMetrics(SM_CXSCREEN);
    if (pt.x + sizeText.cx > iScreenX-4)
        pt.x = iScreenX - sizeText.cx - 4;
    
    // Register a window class if we haven't already done it.
    if (g_szClassName[0] == '\0')
        strncpy(g_szClassName, ::AfxRegisterWndClass(CS_SAVEBITS,
                         ::AfxGetApp()->LoadStandardCursor(IDC_ARROW)), sizeof(g_szClassName)-1);

    // Create and show our help tag.
    if (CreateEx(0, g_szClassName, NULL, WS_POPUP, pt.x, pt.y,
                 sizeText.cx, sizeText.cy, NULL, 0) )
    {                  
            ShowWindow(SW_SHOWNOACTIVATE);
            return TRUE;
    }

    return FALSE;   
    
}// CToolTip::CreateHelpTag

/////////////////////////////////////////////////////////////////////////////
// CToolTip::CalcStartPos
//
// Calculate the starting top/left coordinate of our help tag window.
//
// This causes the help tag to be just below the end of the cursor.
//
CPoint CToolTip::CalcStartPos(CRect& rcArea)
{
    CPoint ptFinalPos, ptCur;

    // Initially assume the pointer position is
    // the mouse position.
    ::GetCursorPos(&ptFinalPos);
    ptCur = ptFinalPos;

    // Position the Y-coordinate just below the mouse pointer.
    ptFinalPos.y += ::GetSystemMetrics(SM_CYCURSOR) * 2 / 3;

    // Now make sure the tag window won't be off the bottom of the screen.
    // In this case, move the tag to the top of the button bar.
    if (ptFinalPos.y + ::GetSystemMetrics(SM_CYCURSOR) > ::GetSystemMetrics(SM_CYSCREEN))
    {
        LOGFONT lf;
        CFont Font;
        Font.CreateStockObject(ANSI_VAR_FONT);
        Font.GetObject(sizeof(LOGFONT),&lf);

        ptFinalPos = rcArea.TopLeft();
        ptFinalPos.x = ptCur.x;

        // Adjust the point to fit a frame + text height.
        ptFinalPos.y -= (lf.lfHeight + 4);
    }  
    return ptFinalPos;
    
}// CToolTip::CalcStartPos

/////////////////////////////////////////////////////////////////////////////
// CToolTip::OnPaint
//
// Draw the window for the help tag.
//
void CToolTip::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    CRect rcClient;
    CBrush brBack;

    brBack.CreateSolidBrush (::GetSysColor (COLOR_INFOBK));

    // Get the rectangle to paint
    GetClientRect(&rcClient);
   
    // Draw the rectangle box for the tag
    CBrush* pOldBrush = (CBrush*) dc.SelectObject(&brBack);
    dc.Rectangle( &rcClient );

    // Draw the text into our tag
    dc.SelectStockObject(ANSI_VAR_FONT);
    dc.SetBkMode(TRANSPARENT);
    
    dc.SetTextColor (::GetSysColor (COLOR_INFOTEXT));

    dc.DrawText(m_strText, -1, &rcClient, 
         (DT_CENTER | DT_VCENTER | DT_SINGLELINE));

    dc.SelectObject(pOldBrush);

}// CToolTip::OnPaint

///////////////////////////////////////////////////////////////////////////
// CToolTip::OnEraseBkgnd
//
// Overriden to make sure we don't erase the background of the 
// window since all the painting is done in OnPaint.
//
BOOL CToolTip::OnEraseBkgnd(CDC* /*pDC*/)
{
   return TRUE;   
   
}// CToolTip::OnEraseBkgnd

///////////////////////////////////////////////////////////////////////////
// CToolTip::PostNcDestroy
//
// Method which handles the WM_NCDESTROY message and destroys our
// object associated with the (now) destroyed window.
//
void CToolTip::PostNcDestroy( )
{
    delete this;
    
}// CToolTip::PostNcDestroy

///////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton message map

BEGIN_MESSAGE_MAP(CMfxBitmapButton, CButton)
    //{{AFX_MSG_MAP(CMfxBitmapButton)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    ON_WM_ENABLE()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::CMfxBitmapButton
//
// Constructor for our button - sets up some globals and creates the 
// font which will be used for our help tags.
//
CMfxBitmapButton::CMfxBitmapButton()
{   
    m_fMouseButtonDown = FALSE;
    m_fTimerRunning = FALSE;
    m_fHasFocus = FALSE;
    m_pToolTip = NULL;
    m_dwTimeout = 0;
    m_wFlags = HelpTypeTool;
    m_ibmWidth = 0;
    m_ibmHeight = 0;
    m_fChecked = FALSE;
    m_fIsCheckable = FALSE;
    m_clrTransparent = 0;
    m_clrShadow = 0;
    m_pwndNotify = NULL;

}// CMfxBitmapButton::CMfxBitmapButton

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::~CMfxBitmapButton()
// 
// Destructor for bitmap button
//
CMfxBitmapButton::~CMfxBitmapButton()
{
}// CMfxBitmapButton::~CMfxBitmapButton

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnEraseBkgnd
//
// Don't bother to erase the background of the bitmap button.
//
BOOL CMfxBitmapButton::OnEraseBkgnd(CDC* /*pDC*/)
{                                
    return TRUE;
    
}// CMfxBitmapButton::OnEraseBkgnd

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::GetSuperWndProcAddr
//
// Returns the WNDPROC for our bitmap buttons.
//
WNDPROC* CMfxBitmapButton::GetSuperWndProcAddr()
{
    static WNDPROC wndProc;
    return &wndProc;

}// CMfxBitmapButton::GetSuperWndProcAddr

///////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::LoadBitmap
//
// Load a specific bitmap to use with this button.
//
BOOL CMfxBitmapButton::LoadBitmap(UINT uidResId, COLORREF clrTransparent, COLORREF clrShadow)
{                               
    return LoadBitmap (AfxGetResourceHandle(), MAKEINTRESOURCE(uidResId), clrTransparent, clrShadow);

}// CMfxBitmapButton::LoadBitmap

///////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::LoadBitmap
//
// Load a specific bitmap to use with this button.
//
BOOL CMfxBitmapButton::LoadBitmap(LPCSTR lpszResID, COLORREF clrTransparent, COLORREF clrShadow)
{                               
    return LoadBitmap (AfxGetResourceHandle(), lpszResID, clrTransparent, clrShadow);

}// CMfxBitmapButton::LoadBitmap

///////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::LoadBitmap
//
// Load a specific bitmap to use with this button.
//
BOOL CMfxBitmapButton::LoadBitmap(HINSTANCE hInst, LPCSTR lpszResID, 
                                  COLORREF clrTransparent, COLORREF clrShadow)
{
    // Locate and load the resource from our executable.
    return ProcessBitmap (LoadResource(hInst, FindResource(hInst, lpszResID, (LPSTR)RT_BITMAP)), 
                            clrTransparent, clrShadow);
    
}// CMfxBitmapButton::LoadBitmap

///////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::ProcessBitmap
//
// Create the mask and final color bitmap to use with this button.
//
BOOL CMfxBitmapButton::ProcessBitmap (HANDLE hResource, COLORREF clrTransparent, COLORREF clrShadow)
{       
    // Fail on bad handle.
    if (hResource == NULL)
        return FALSE;

    // Get a pointer to the bitmap info header.
    LPBITMAPINFOHEADER pBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hResource);
    if (pBitmapInfo == NULL)
        return(FALSE);

    // Delete the old bitmap images.
    m_bmButton.DeleteObject();
    m_bmMask.DeleteObject();

    // Load the bitmap in.
    HBITMAP hBitmap = CreateBitmapMask(pBitmapInfo, clrTransparent, FALSE);
    if (hBitmap == NULL)
        return FALSE;
    m_bmButton.Attach (hBitmap);
    
    // Create the mask.    
    hBitmap = CreateBitmapMask(pBitmapInfo, clrTransparent, TRUE);
    m_bmMask.Attach (hBitmap);
    
    // Save off the shadow color
    m_clrShadow = clrShadow;
    
    // Get the diminsions of this bitmap.
    m_ibmWidth = (int) pBitmapInfo->biWidth;
    m_ibmHeight = (int) pBitmapInfo->biHeight;

    // Free the resource we loaded from the executable.  
    UnlockResource(hResource);
    FreeResource(hResource);

    // Force the button to re-render itself.                     
    Invalidate (FALSE);
    return TRUE;                
    
}// CMfxBitmapButton::LoadBitmap

/////////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnSetFocus
//
// Redraws the button with focus
//
void CMfxBitmapButton::OnSetFocus (CWnd* /*pwndOld*/)
{                              
    m_fHasFocus = TRUE;
    Invalidate(FALSE);
    UpdateWindow();

}// CMfxBitmapButton::OnSetFocus                    

/////////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnKillFocus
//
// Redraws the button without focus
//
void CMfxBitmapButton::OnKillFocus(CWnd* /*pwndOld*/)
{                                
    m_fHasFocus = FALSE;
    Invalidate(FALSE);
    UpdateWindow();

}// CMfxBitmapButton::OnKillFocus
    
/////////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::DrawButtonFace
//
// Draw the button face, return an offset for the bitmap image.
//
void CMfxBitmapButton::DrawButtonFace (CDC* pDC, CRect& rcButton, CPoint& ptOffset, BOOL fPressed, BOOL fDitherBkgnd)
{   
    CBrush brFace, brBlack;
    brFace.CreateSolidBrush (GetSysColor (COLOR_BTNFACE));
    brBlack.CreateSolidBrush (::GetSysColor(COLOR_3DDKSHADOW));
    
    CPen penLight (PS_SOLID, 1, ::GetSysColor (COLOR_BTNHIGHLIGHT));
    CPen penDark  (PS_SOLID, 1, ::GetSysColor (COLOR_BTNSHADOW));
    
    // Fill in the face of the button.
    if (fDitherBkgnd)
    {   
        // Create our "dither" brush.
        int iDither[] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
        CBitmap Bitmap;
        Bitmap.CreateBitmap (8, 8, 1, 1, &iDither);
        CBrush brHalf;
        brHalf.CreatePatternBrush (&Bitmap);
        pDC->SetTextColor (::GetSysColor (COLOR_BTNFACE));
        pDC->SetBkColor (::GetSysColor (COLOR_BTNSHADOW));
        pDC->FillRect (rcButton, &brHalf);
    }
    else
        pDC->FillRect (&rcButton, &brFace);
        
    pDC->FrameRect (&rcButton, &brBlack);
    
    // Determine where the image will go.    
    ptOffset.x = (rcButton.Width() - m_ibmWidth - 1) >> 1;
    ptOffset.y = (rcButton.Height() - m_ibmHeight) >> 1;
    rcButton.right--;
    rcButton.bottom--;
    
    // Now draw either depressed or up.
    CPen* penOld = pDC->SelectObject (&penLight);
    if (fPressed || fDitherBkgnd)
    {   
        pDC->MoveTo (rcButton.right, rcButton.top+1);
        pDC->LineTo (rcButton.right, rcButton.bottom);
        pDC->LineTo (rcButton.left, rcButton.bottom);
        pDC->SelectObject (&penDark);
        pDC->MoveTo (rcButton.left+1, rcButton.bottom-1);
        pDC->LineTo (rcButton.left+1, rcButton.top+1);
        pDC->LineTo (rcButton.right, rcButton.top+1);
        ptOffset.x++;
        ptOffset.y++;
        rcButton.left += 2;
        rcButton.top += 2;
        rcButton.right--;
        rcButton.bottom--;
    }
    else
    {
        pDC->MoveTo (rcButton.left, rcButton.bottom-1);
        pDC->LineTo (rcButton.left, rcButton.top);
        pDC->LineTo (rcButton.right, rcButton.top);
        pDC->SelectObject (&penDark);
        pDC->MoveTo (rcButton.right-1, rcButton.top+1);
        pDC->LineTo (rcButton.right-1, rcButton.bottom-1);
        pDC->LineTo (rcButton.left, rcButton.bottom-1);
        rcButton.left ++;
        rcButton.top ++;
        rcButton.right -= 2;
        rcButton.bottom -= 2;
    }   
    
    pDC->SelectObject (penOld);

}// CMfxBitmapButton::DrawButtonFace

/////////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnPaint
//
// Draw the button and appropriate tile image.
//
void CMfxBitmapButton::OnPaint()
{
    ASSERT (m_bmButton.GetSafeHandle() != NULL);

    // Get a DC to paint with, create two additional DCs for the 
    // temporary memory images.
    CPaintDC dc(this);
    CDC dcImage, dcFinal;
    
    // Get the area for the button.
    CRect rcButton;
    GetClientRect(&rcButton);
    
    // Create the intermediate DC which we will draw to - this will be 
    // drawn to the final DC in order to look "smooth".  Also, create a 
    // DC to load the bitmap image into.
    dcFinal.CreateCompatibleDC (&dc);   
    dcImage.CreateCompatibleDC (&dc);

    // Now, create an intermediate bitmap which we can draw to, and load
    // it into our image buffer.  Load the image bitmap into the image dc.
    CBitmap bmFinal;
    bmFinal.CreateCompatibleBitmap (&dc, rcButton.Width(), rcButton.Height());
    
    CBitmap* pbmFinal = (CBitmap*) dcFinal.SelectObject (&bmFinal);
    CBitmap* pbmImage = (CBitmap*) dcImage.SelectObject (&m_bmButton);
    
    // Draw the button face.                     
    CPoint ptOffset;
    DrawButtonFace (&dcFinal, rcButton, ptOffset, m_fMouseButtonDown,
                    m_fChecked && IsWindowEnabled());
    
    // Load our mask
    CDC dcMask;
    dcMask.CreateCompatibleDC (&dc);
    CBitmap *pbmMask = dcMask.SelectObject (&m_bmMask);
    
    // Put the image onto the button.
    if (IsWindowEnabled())
    {
        dcFinal.SetTextColor (RGB(0,0,0));
        dcFinal.SetBkColor (RGB(255,255,255));   
        dcFinal.BitBlt (ptOffset.x, ptOffset.y,
                        m_ibmWidth, m_ibmHeight, &dcMask, 0, 0, SRCAND);
        dcFinal.BitBlt (ptOffset.x, ptOffset.y,                  
                        m_ibmWidth, m_ibmHeight, &dcImage, 0, 0, SRCPAINT);
    }        
    else // if window is disabled
    {
        // Create a new mask with an additional highlight attribute.
        CDC dcDisable;
        dcDisable.CreateCompatibleDC (&dc);
        CBitmap bmDisable;
        bmDisable.CreateBitmap (m_ibmWidth, m_ibmHeight, 1, 1, NULL);
        CBitmap* poldBM = (CBitmap*) dcDisable.SelectObject (&bmDisable);
        dcDisable.BitBlt (0, 0, m_ibmWidth, m_ibmHeight, &dcMask, 0, 0, SRCCOPY);
        dcImage.SetBkColor (m_clrShadow);
        dcDisable.BitBlt(0, 0, m_ibmWidth, m_ibmHeight, &dcImage, 0, 0, SRCPAINT);
        dcDisable.BitBlt(0, 0, m_ibmWidth, m_ibmHeight, &dcMask, 0, 0, ROP_DSxn);
 
        // First put the highlight color down.
        CBrush brHighlight (::GetSysColor (COLOR_BTNHIGHLIGHT));
        CBrush* pbrOld = (CBrush*) dcFinal.SelectObject (&brHighlight);
        dcFinal.BitBlt (ptOffset.x+1, ptOffset.y+1, m_ibmWidth, m_ibmHeight, 
                        &dcDisable, 0, 0, ROP_PSDPxax);
        
        // Now the shadow color.
        CBrush brShadow (::GetSysColor (COLOR_BTNSHADOW));
        dcFinal.SelectObject (&brShadow);
        dcFinal.BitBlt (ptOffset.x, ptOffset.y, m_ibmWidth, m_ibmHeight, 
                        &dcDisable, 0, 0, ROP_PSDPxax);
        dcFinal.SelectObject (pbrOld);
        dcDisable.SelectObject (poldBM);
    }
    
    // Put the bitmap out to the screen DC.
    GetClientRect(&rcButton);
    dc.BitBlt (rcButton.left, rcButton.top, rcButton.Width(), rcButton.Height(),
                 &dcFinal, 0, 0, SRCCOPY);

    // If it has a focus rectangle, draw it in.
    if (m_fHasFocus != 0)
    {
        rcButton.InflateRect (-3,-3);
        dc.DrawFocusRect (&rcButton);
    }
    
    // Deselect all our objects and exit.
    dcFinal.SelectObject (pbmFinal);
    dcImage.SelectObject (pbmImage);
    dcMask.SelectObject (pbmMask);
        
}// CMfxBitmapButton::DrawItem

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::SetCheck
//
// Set the "checked" state for this button.
//
void CMfxBitmapButton::SetCheck (int iCheck)
{                            
    ASSERT (iCheck == 0 || iCheck == 1);
    if (GetCheck() != iCheck)
    {
        m_fChecked = (iCheck != 0);
        Invalidate(FALSE);
        UpdateWindow();
    }

}// CMfxBitmapButton::SetCheck

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::GetCheck
//
// Retrieve the check state for the button.
//
int CMfxBitmapButton::GetCheck() const
{                            
    return (m_fChecked != 0);

}// CMfxBitmapButton::GetCheck

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::SetAutoCheck
//
// Turn on "auto check" ability.
//
void CMfxBitmapButton::SetAutoCheck (BOOL fAutoCheck)
{                                
    m_fIsCheckable = (fAutoCheck != 0);

}// CMfxBitmapButton::SetAutoCheck

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnLButtonDown
//
// This method watches for a mouse button down and destroys the help
// tag window if it is visible.
//
void CMfxBitmapButton::OnLButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{   
    // Destroy the helptag
    DestroyHelpTag();
    if (m_fTimerRunning)
    {
        m_fTimerRunning = FALSE;
        KillTimer(IDT_TOOLTIP);
    }

    // Down-press the bitmap.
    m_fMouseButtonDown = TRUE;
    Invalidate(FALSE);
    UpdateWindow();
    
    SetCapture();

}// CMfxBitmapButton::OnLButtonDown

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnLButtonUp
//
// This method resets the mouse down condition and allows the help
// tags to be displayed.
//
void CMfxBitmapButton::OnLButtonUp(UINT /*nFlags*/, CPoint point)
{
    DestroyHelpTag();
    ReleaseCapture();

    m_fMouseButtonDown = FALSE;
    Invalidate(FALSE);
    UpdateWindow();

    if (m_fIsCheckable != 0 && IsWindowEnabled())
        SetCheck (!GetCheck());

    // If we are within our button area, send a click message.
    CRect rcClient;
    GetClientRect(&rcClient);
    
    if (rcClient.PtInRect(point))
    {
        CWnd* pwndParent = m_pwndNotify;
        if (pwndParent == NULL)
        {
            pwndParent = GetWindow (GW_OWNER);
            if (pwndParent == NULL)
                pwndParent = GetParent();
        }               
        if (pwndParent)
            pwndParent->SendMessage (WM_COMMAND, ::GetWindowLong(GetSafeHwnd(), GWL_ID), 
                                     MAKELPARAM(GetSafeHwnd(), BN_CLICKED));
    }

}// CMfxBitmapButton::OnLButtonUp

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnMouseMove
//
// Process the mouse movement messages.
//
void CMfxBitmapButton::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
    if (GetCapture() == this)
    {
        CRect rcClient;
        GetClientRect(&rcClient);
        
        if (rcClient.PtInRect(point))
        {
            if (m_fMouseButtonDown == FALSE)
            {
                m_fMouseButtonDown = TRUE;
                Invalidate(FALSE);
                UpdateWindow();
            }
        }
        else
        {
            if (m_fMouseButtonDown != FALSE)
            {
                m_fMouseButtonDown = FALSE;
                Invalidate(FALSE);
                UpdateWindow();
            }
        }
    }       
    else
    {
        if (!m_fTimerRunning)
        {
             m_fTimerRunning = TRUE;
             SetTimer(IDT_TOOLTIP, TIMER_INTERVAL, NULL);
        }
    }

}// CMfxBitmapButton::OnMouseMove
    
/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnTimer
//
// Process the timer interval.  If we are over our button, show a tool
// tip.
//
void CMfxBitmapButton::OnTimer(UINT /*nIdTimer*/)
{
    // If our timer has been turned off - ignore this event.
    if (m_fTimerRunning == 0 || GetCapture() == this)
        return;

    // If the mouse is STILL over our button, show a tooltip.
    CPoint ptCursor;
    GetCursorPos(&ptCursor);

    CRect rcArea;
    GetWindowRect(&rcArea);

    if (rcArea.PtInRect(ptCursor))
    {
        // If style is set to no help, just return
        if (m_wFlags & HelpTypeNone)
            return;

        // If the style is set to the default tool tip style of help
        if (m_wFlags & HelpTypeTool)
        {
            if (m_pToolTip == NULL)
            {
                m_dwTimeout = GetTickCount() + TAG_SHOWTIME;

                // Get the string for the button.  If we don't have one,
                // then don't show a help tag.
                CString strBuff;
                strBuff.LoadString((UINT)GetDlgCtrlID());
                if (strBuff.IsEmpty())
                {
                    m_fTimerRunning = FALSE;
                    KillTimer(IDT_TOOLTIP);
                    return;
                }    

                m_pToolTip = new CToolTip (strBuff);
                if (m_pToolTip)
                    m_pToolTip->CreateHelpTag (rcArea);
            }
            else
            {
                // If enough time has elapsed with the tag visible, 
                // automatically destroy it ala Windows 95.
                if (GetTickCount() > m_dwTimeout)
                    m_pToolTip->ShowWindow (SW_HIDE);
            }
        }
    }
    else
    {
        m_fTimerRunning = FALSE;
        KillTimer(IDT_TOOLTIP);
        DestroyHelpTag();
    }

}// CMfxBitmapButton::OnTimer

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::DestroyHelpTag
//
// Destroys the help tag and resets the timer so we have to wait the
// whole interval before another help tag is displayed
//
void CMfxBitmapButton::DestroyHelpTag()
{
    if (m_wFlags & HelpTypeNone)
        return;

    if ((m_wFlags & HelpTypeTool) && m_pToolTip)
    {
        m_pToolTip->DestroyWindow();
        m_pToolTip = NULL;
    }

}// CMfxBitmapButton::DestroyHelpTag

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::SetStyle
//
// Set the style of the helptag.
//
void CMfxBitmapButton::SetStyle (enum CMfxBitmapButton::HelpStyle wStyle)
{
    m_wFlags = static_cast<WORD>(wStyle);

}// CMfxBitmapButton::SetStyle

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::SetNotifyWindow
//
// Set the notication window for this button
//
void CMfxBitmapButton::SetNotifyWindow (CWnd* pwndNotify)
{
    m_pwndNotify = pwndNotify;

}// CMfxBitmapButton::SetNotifyWindow

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::OnEnable
//
// This enables/disables the button.
//
void CMfxBitmapButton::OnEnable(BOOL /*fEnable*/)
{
    Invalidate(FALSE);
    UpdateWindow();

}// CMfxBitmapButton::OnEnable

/////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::DrawItem
//
// Override for the drawitem notification.
//
void CMfxBitmapButton::DrawItem (LPDRAWITEMSTRUCT /*lpdi*/)
{                             
    /* Do nothing */
    
}// CMfxBitmapButton::DrawItem

////////////////////////////////////////////////////////////////////////////
// CMfxBitmapButton::CreateBitmapMask
//
//  This function creates a mask bitmap and returns two bitmap handles.    
//  The first is the color DIB with the background masked out.  The second 
//  is a monochrome bitmap mask which is used to mask off the area where   
//  the bitmap will be placed.  This allows us to have dithered bitmap     
//  backgrounds since BitBlt forces all colors in the bitmap to be pure.   
//                                                                         
HBITMAP CMfxBitmapButton::CreateBitmapMask(LPBITMAPINFOHEADER pBitmapInfo, COLORREF clrTransparent, BOOL bWhich)
{
    // Move past the header into the color table imbedded in the DIB.
    COLORREF* pColorTable = (COLORREF*)((LPSTR)pBitmapInfo+pBitmapInfo->biSize);
    LPSTR pBits = (LPSTR)pColorTable;
    
    // Swap the R/B values for HI/LO word ordering.
    clrTransparent = RGB (GetBValue(clrTransparent), GetGValue(clrTransparent), GetRValue(clrTransparent));

    // Search for the colors we are going to modify.  Note this is based
    // on the colors used when the bitmap is created.
    ASSERT(pBitmapInfo->biPlanes == 1);                                           
    int NumColors = (1 << pBitmapInfo->biBitCount);                     
    if (NumColors == 0 || NumColors > 256)
    {
        ASSERT (FALSE);    
        return NULL;   
    }

    // Save off the color table so we don't modify it.
    COLORREF* lpSaveColors = new COLORREF [NumColors];
    memcpy (lpSaveColors, pColorTable, sizeof(COLORREF)*NumColors);
                        
    for (int i = 0; i < NumColors; i++, pColorTable++)
    {
        if (*pColorTable == clrTransparent)
            *pColorTable = (bWhich) ? RGB(255,255,255) : 0L;
        else if (bWhich)
            *pColorTable = 0L;
    }

    // Get a pointer to the bitmap portion of this DIB..
    pBits += (NumColors * sizeof(RGBQUAD));

    // Create a new color bitmap that is compatible with the current display
    // device.
    HDC hDC = ::GetDC(NULL);
    HBITMAP hBitmap = ::CreateDIBitmap(hDC, pBitmapInfo, CBM_INIT, pBits,
                           (LPBITMAPINFO)pBitmapInfo, 0);
    ::ReleaseDC(NULL, hDC);

    // Reset the color table
    pColorTable = (LPDWORD)((LPSTR)(pBitmapInfo)+pBitmapInfo->biSize);
    memcpy (pColorTable, lpSaveColors, sizeof(COLORREF)*NumColors);
    delete [] lpSaveColors;

    // Return the bitmap created
    return(hBitmap);

}// CMfxBitmapButton::CreateBitmapMask
