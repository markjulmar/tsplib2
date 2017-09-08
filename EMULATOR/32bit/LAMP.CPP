/*****************************************************************************/
//
// LAMP.CPP - Digital Switch Service Provider Sample
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

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////
// Lamp message map

BEGIN_MESSAGE_MAP(CLampWnd, CWnd)
    //{{AFX_MSG_MAP(CLampWnd)
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_WM_ERASEBKGND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

const int MARGIN = 4;

///////////////////////////////////////////////////////////////////////////
// CLampWnd::CLampWnd
//
// Constructor
//
CLampWnd::CLampWnd()
{
    m_iState = LAMPSTATE_OFF;
    m_Direction  = Left;
    m_iCounter = 0;
    m_iDrawState = 0;
    m_fDrawn = FALSE;

}// CLampWnd::CLampWnd

///////////////////////////////////////////////////////////////////////////
// CLampWnd::~CLampWnd
//
// Destructor
//
CLampWnd::~CLampWnd()
{   
    if (m_hWnd != NULL)
    {
        WNDPROC* pwndProc = GetSuperWndProcAddr();
        ::SetWindowLong (m_hWnd, GWL_WNDPROC, (LONG) *pwndProc);
        Detach();
    }
    
}// CLampWnd::CLampWnd

///////////////////////////////////////////////////////////////////////////
// CLampWnd::OnPaint
//
// Paint method for the lamp indicators
//
void CLampWnd::OnPaint()
{
    CPaintDC dc (this);
    CRect rcArea;
    CDC memDC;
   	CBrush Brush, *pOldBrush;
                 
    // Set our state to a fixed value if on/off.
    if (m_iState == LAMPSTATE_OFF)
        m_fDrawn = FALSE;
    else if (m_iState == LAMPSTATE_STEADY)
        m_fDrawn = TRUE;

    Brush.CreateStockObject (LTGRAY_BRUSH);

    // Create a mem DC compatible with this device.
	GetClientRect(rcArea);
    CBitmap Bitmap, *pOldBitmap;
    Bitmap.CreateCompatibleBitmap (&dc, rcArea.Width(), rcArea.Height());
    memDC.CreateCompatibleDC (&dc);
    pOldBitmap = (CBitmap*) memDC.SelectObject (&Bitmap);
  
    // Erase the current area
    memDC.FillRect (&rcArea, &Brush);

    if (m_fDrawn)
    {
        // Now draw our actual object.
        POINT ptPoint[3];

        if (m_Direction == Left)
        {
            ptPoint[0].x = MARGIN;
            ptPoint[0].y = rcArea.Height() / 2;
            ptPoint[1].x = rcArea.Width() - MARGIN;
            ptPoint[1].y = MARGIN;
            ptPoint[2].x = rcArea.Width() - MARGIN;
            ptPoint[2].y = rcArea.Height() - MARGIN;
        }
        else
        {
            ptPoint[0].x = MARGIN;
            ptPoint[0].y = rcArea.Height() - MARGIN;
            ptPoint[1].x = MARGIN;
            ptPoint[1].y = MARGIN;
            ptPoint[2].x = rcArea.Width() - MARGIN;
            ptPoint[2].y = rcArea.Height() / 2;
        }

        // Use the color black as the filled interior.
        // Leave the pen as default (BLACK)
        Brush.DeleteObject();
        Brush.CreateSolidBrush (RGB(0,0,0));
        pOldBrush = (CBrush*) memDC.SelectObject (&Brush);
        memDC.Polygon (ptPoint, 3);
    }

    // Blt the whole thing to the original DC.
    dc.BitBlt(0, 0, rcArea.Width(), rcArea.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject (pOldBitmap);

}// CLampWnd::OnPaint
  
////////////////////////////////////////////////////////////////
// CLampWnd::OnTimer
// 
// Processes the WM_TIMER for the lamp
//  
void CLampWnd::OnTimer(UINT /*nIDEvent*/)
{
    static DWORD FlashTable[] = { 6, 0 };
    static DWORD WinkTable[] = { 1, 0 };
   
    DWORD * pdwTable = NULL;
    if (m_iState == LAMPSTATE_FLASHING)
        pdwTable = FlashTable;
    else if (m_iState == LAMPSTATE_BLINKING)
        pdwTable = WinkTable;
    else
        return;
              
    if (*(pdwTable + m_iDrawState) <= (DWORD) ++m_iCounter)
    {
        m_iDrawState++;
        m_iCounter = 0;
        if (*(pdwTable + m_iDrawState) == 0)
            m_iDrawState = 0;
        m_fDrawn = !m_fDrawn;

		Invalidate(FALSE);
		UpdateWindow();
    }

}// CLampWnd::OnTimer

////////////////////////////////////////////////////////////////
// CLampWnd::SetState
//
// Set the state of the lamp.
//
void CLampWnd::SetState (int iState)
{   
    ASSERT (iState >= LAMPSTATE_NONE && iState <= LAMPSTATE_FLASHING);
    m_iState = iState;
    if (GetSafeHwnd() != NULL)
    {
    	CRect rcBox;
    	GetWindowRect(&rcBox);
    	CWnd* pWnd = GetParent();
    	pWnd->ScreenToClient (&rcBox);
    	pWnd->InvalidateRect(&rcBox, TRUE);
        pWnd->UpdateWindow();
    }
    
}// CLampWnd::SetState

////////////////////////////////////////////////////////////////
// CLampWnd::OnEraseBkgnd
//
// Don't erase the background
//
BOOL CLampWnd::OnEraseBkgnd(CDC* /*pDC*/)
{                         
	return TRUE;

}// CLampWnd::OnEraseBkgnd

////////////////////////////////////////////////////////////////
// CLampWnd::GetSuperWndProcAddr
//
// Overriden WNDPROC address
//
WNDPROC* CLampWnd::GetSuperWndProcAddr() 
{   
    static WNDPROC wndProc;
    return &wndProc;
    
}// CLampWnd::GetSuperWndProcAddr
