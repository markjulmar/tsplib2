/******************************************************************************/
//
// PROGRESS.CPP
//
// Progress bar code
//
// Copyright (C) 1995, Ping Software, Inc.
// Copyright (C) 1994, 1995, Mark C. Smith
//  All rights reserved.
//
/******************************************************************************/

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////////
// CProgressCtl message map

BEGIN_MESSAGE_MAP(CProgressCtl, CWnd)
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_NCHITTEST()
    ON_WM_NCPAINT()
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::CProgressCtl
//
// Constructor for the progress bar
//
CProgressCtl::CProgressCtl(Style iStyle)
{
    m_Style = iStyle;
    m_iCurrPos = 0;
    m_iMaxPos = 100;
    m_fDraw3d = TRUE;

    // By default use the caption colors
    m_rgbBarColor  = ::GetSysColor (COLOR_ACTIVECAPTION);
    m_rgbTextColor = ::GetSysColor (COLOR_CAPTIONTEXT);

}// CProgressCtl::CProgressCtl

//////////////////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::SetPosition
//
// Set the current position of the bar
//
void CProgressCtl::SetPosition(int iPos)
{
    if (iPos < 0)
        iPos = 0;
    if (iPos > m_iMaxPos)
        iPos = m_iMaxPos;

    if (iPos != m_iCurrPos)
    {
        m_iCurrPos = iPos;
        Invalidate();
    }

}// CProgressCtl::SetPosition

//////////////////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::SetRange
//
// Set the range for the progress bar (like a scrollbar)
//
void CProgressCtl::SetRange(int iRange)
{
    if (iRange <= 0)
        iRange = 100;

    m_iMaxPos = iRange;
    m_iCurrPos = 0;

    Invalidate();

}// CProgressCtl::SetRange

/////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::GetSuperWndProcAddr
//
// Return a storage area for the original WNDPROC.
//
WNDPROC* CProgressCtl::GetSuperWndProcAddr()
{
    static WNDPROC wndProc;
    return &wndProc;

}// CProgressCtl::GetSuperWndProcAddr

//////////////////////////////////////////////////////////////////////////////
// CProgressCtl::OnEraseBkgnd
//
// Don't allow windows to redraw the background.
//
BOOL CProgressCtl::OnEraseBkgnd (CDC* /*pDC*/)
{
    return TRUE;

}// CProgressCtl::OnEraseBkgnd

///////////////////////////////////////////////////////////////////////////////
// CProgressCtl::OnPaint
//
// Paint the progress control.
//
void CProgressCtl::OnPaint()
{
    CPaintDC dc( this );

    switch (m_Style)
    {
        case Percentage:
            DrawPercentage(&dc);
            break;

        case Blocked:
            DrawBlocks(&dc);
            break;

        default:
            break;
    }

}// CProgressCtl::OnPaint

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::DrawFrame
//
// Draw the frame
//
void CProgressCtl::DrawFrame(CDC* pDC, CRect& rcArea)
{
    CBrush brBlack (RGB(0,0,0));
    if (m_fDraw3d)
    {
        CCtl3d::Draw3dBorder(pDC, rcArea);
        rcArea.top++;
        rcArea.left++;
    }

    pDC->FrameRect (rcArea, &brBlack);
    rcArea.InflateRect (-1,-1);

}// CProgressCtl::DrawFrame

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::DrawPercentage
//
// Draws a "percentage" bar.
//
void CProgressCtl::DrawPercentage (CDC* pDC)
{
    // Determine what our "area" to draw to is.
    CRect rcArea;
    GetClientRect (rcArea);
    rcArea.right--;
    rcArea.bottom--;

    DrawFrame(pDC, rcArea);

    // Draw the completed portion of the bar.
    pDC->SetTextColor (GetTextColor());
    pDC->SetBkColor (GetBarColor());

    // Format the resultant string.
    CString strBuff;
    strBuff.Format("%3d%%", (WORD)((DWORD)m_iCurrPos * 100 / m_iMaxPos));
    
    // Determine how far we have reached
    WORD wCurrPos = (WORD)((DWORD)m_iCurrPos * rcArea.Width() / m_iMaxPos);

    // Now setup two rectangles - one is the area we are going to fill
    // with our color, and the second is the area which is not yet covered.
    CRect rcDone (rcArea.left, rcArea.top, rcArea.left+wCurrPos, rcArea.bottom);
    CRect rcNotDone (rcArea.left+wCurrPos, rcArea.top, rcArea.right, rcArea.bottom);
    CSize sizText = pDC->GetTextExtent(strBuff, strBuff.GetLength());

    // Draw the completed portion of our status bar
    if (m_iCurrPos > 0)
    {
        pDC->ExtTextOut (rcArea.left + (rcArea.Width()/2-sizText.cx/2),
                         rcArea.top + (rcArea.Height()/2-sizText.cy/2),
                         ETO_OPAQUE | ETO_CLIPPED,
                         &rcDone, strBuff, strBuff.GetLength(), 
                         NULL);
    }

    // Now draw out the unfinished portion of the status bar.
    pDC->SetBkColor (GetTextColor());
    pDC->SetTextColor (GetBarColor());
    pDC->ExtTextOut (rcArea.left + (rcArea.Width()/2-sizText.cx/2),
                     rcArea.top + (rcArea.Height()/2-sizText.cy/2),
                     ETO_OPAQUE | ETO_CLIPPED,
                     &rcNotDone, strBuff, strBuff.GetLength(), 
                     NULL);

}// CProgressCtl::DrawPercentage

////////////////////////////////////////////////////////////////////////////
// CProgressCtl::DrawBlocks
//
// Draw a "blocked" status bar
//
void CProgressCtl::DrawBlocks (CDC* pDC)
{
    CRect rcArea;
    GetClientRect (rcArea);
    rcArea.right--;
    rcArea.bottom--;

    CBrush brGray (m_rgbTextColor);
    CBrush brBar (m_rgbBarColor);
    CPen   penGray (PS_SOLID,1,m_rgbTextColor);

    // Draw the frame.
    DrawFrame (pDC, rcArea);

    // Now break it into TWO rectangles.  The first is the rectangle
    // which has our bars in it.  The second is the area we haven't 
    // touched yet.
    CRect rcFilled = rcArea;
    rcFilled.right = (int) ((float)rcFilled.right * (float)((float)m_iCurrPos / (float)m_iMaxPos));
    CRect rcEmpty = rcArea;
    rcEmpty.left = rcFilled.right;

    if (rcEmpty.left <= rcArea.left)
        rcEmpty.left = rcArea.left;

    // Determine what the size of our boxes will be.
    int iBoxSize = rcArea.Height();

    // Draw our boxes.
    CPen* penOld = (CPen*) pDC->SelectObject (&penGray);
    CBrush* brOld = (CBrush*) pDC->SelectObject (&brBar);

    for (int i = rcFilled.left; i < rcFilled.right; i += iBoxSize)
    {
        CRect rcBox (i, rcArea.top, i+iBoxSize, rcArea.bottom);
        if (rcBox.right > rcFilled.right)
            rcBox.right = rcFilled.right;
        pDC->Rectangle (&rcBox);
    }

    pDC->SelectObject (penOld);
    pDC->SelectObject (brOld);

    // Now fill the remainder with our background color
    pDC->FillRect (rcEmpty, &brGray);

}// CProgressCtl::DrawBlocks 

///////////////////////////////////////////////////////////////////////////////
// CProgressCtl::OnNcHitTest
//
// Non-client hittesting.
//
UINT CProgressCtl::OnNcHitTest(CPoint /*point*/)
{
    return HTNOWHERE;

}// CProgressCtl::OnNcHitTest

///////////////////////////////////////////////////////////////////////////////
// CProgressCtl::OnNcPaint
//
// Handle the non-client painting.
//
void CProgressCtl::OnNcPaint()
{
    /* Do nothing */

}// CProgressCtl::OnNcPaint

///////////////////////////////////////////////////////////////////////////////
// CProgressCtl::OnSetFocus
//
// Ignore focus
//
void CProgressCtl::OnSetFocus(CWnd* /*pOldWnd*/)
{
    /* Do nothing */

}// CProgressCtl::OnSetFocus

///////////////////////////////////////////////////////////////////////////////
// CProgressCtl::OnKillFocus
//
// Ignore focus changes
//
void CProgressCtl::OnKillFocus(CWnd* /*pNewWnd*/)
{
    /* Do nothing */

}// CProgressCtl::OnKillFocus

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::GetPosition
//
// Return the position of the control
//
int CProgressCtl::GetPosition() const
{
    return m_iCurrPos;
    
}// CProgressCtl::GetPosition    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::GetRange
//
// Return the range of the control (the max value)
//
int CProgressCtl::GetRange() const
{ 
    return m_iMaxPos;
    
}// CProgressCtl::GetRange    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator int
//
// Return the position of the control.
//
CProgressCtl::operator int()
{ 
    return GetPosition();
    
}// CProgressCtl::operator int    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator=
//
// Set the position of the control, and return the new position.
//
int CProgressCtl::operator=(int iPos)
{ 
    SetPosition(iPos); 
    return GetPosition();
    
}// CProgressCtl::operator=    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator++
//
// Post Increment the position of the control by one.
//
int CProgressCtl::operator++(int)
{ 
    int i = GetPosition(); 
    SetPosition(GetPosition()+1); 
    return i;
    
}// CProgressCtl::operator++    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator++
//
// Pre increment the position of the control by one.
//
int CProgressCtl::operator++()
{ 
    SetPosition(GetPosition()+1); 
    return GetPosition();
    
}// CProgressCtl::operator++    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator--
//
// Post decrement the position of the control by one
//
int CProgressCtl::operator--(int)
{ 
    int i = GetPosition(); 
    SetPosition(GetPosition()-1); 
    return i;
    
}// CProgressCtl::operator--    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator--
//
// Pre decrement the position of the control by one.
//
int CProgressCtl::operator--()
{ 
    SetPosition(GetPosition()-1); 
    return GetPosition();
    
}// CProgressCtl::operator--    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator+=
//
// Additive operator.
//
int CProgressCtl::operator+=(int iAdd)
{ 
    SetPosition(GetPosition()+iAdd); 
    return GetPosition();
    
}// CProgressCtl::operator+=    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::operator-=
//
// Subtractive operator
//
int CProgressCtl::operator-=(int iSub)
{ 
    SetPosition(GetPosition()+iSub); 
    return GetPosition();
    
}// CProgressCtl::operator-=    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::SetStyle
//
// Set the style of the progress control
//
void CProgressCtl::SetStyle (enum CProgressCtl::Style iStyle, BOOL fDraw3d)
{ 
    m_Style = iStyle; 
    m_fDraw3d = fDraw3d;
    
}// CProgressCtl::SetStyle    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::GetBarColor
//
// Return the color of the bar
//
COLORREF CProgressCtl::GetBarColor() const
{ 
    return m_rgbBarColor;
    
}// CProgressCtl::GetBarColor    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::SetBarColor
//
// Set the color of the bar
//
void CProgressCtl::SetBarColor(COLORREF rgb)
{ 
    m_rgbBarColor = rgb; 
    if (GetSafeHwnd()) 
        Invalidate();
        
}// CProgressCtl::SetBarColor        

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::GetTextColor
//
// Return the text color of the bar
//
COLORREF CProgressCtl::GetTextColor() const
{ 
    return m_rgbTextColor;
    
}// CProgressCtl::GetTextColor    

////////////////////////////////////////////////////////////////////////////////
// CProgressCtl::SetTextColor
//
// Set the text color of the bar
//
void CProgressCtl::SetTextColor(COLORREF rgb)
{ 
    m_rgbTextColor = rgb; 
    if (GetSafeHwnd()) 
        Invalidate();
    
}// CProgressCtl::SetTextColor    


