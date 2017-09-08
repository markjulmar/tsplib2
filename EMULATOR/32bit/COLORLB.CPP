/*****************************************************************************/
//
// COLORLB.CPP - Digital Switch Service Provider Sample
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
#include "colorlb.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////
// CColorLB Message map

BEGIN_MESSAGE_MAP(CColorLB, CListBox)
   //{{AFX_MSG_MAP(CColorLB)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////
// CColorLB::AddString
//
// This adds a string to our colored listbox
//
int CColorLB::AddString(char c, LPCSTR lpsz)
{
   int iLen = lstrlen(lpsz);

   if (iLen > 0)
   {
      CString strTemp(c, 1);
      strTemp += lpsz;
      strTemp = strTemp.SpanExcluding("\r\n");
      iLen = CListBox::AddString(strTemp);
      if (iLen >= 0)
         SetTopIndex(iLen);
   }
   else
      iLen = LB_ERR;

   return iLen;

}// CColorLB::AddString

////////////////////////////////////////////////////////////////////////////
// CColorLB::MeasureItem
//
// This method processes the WM_MEASUREITEM message for our owner-drawn
// listbox.
//
void CColorLB::MeasureItem(LPMEASUREITEMSTRUCT lpmi)
{
   TEXTMETRIC tm;

   CDC* pDC = GetDC();
   pDC->GetTextMetrics(&tm);
   lpmi->itemHeight = tm.tmHeight + tm.tmDescent;
   ReleaseDC(pDC);

}// CColorLB::MeasureItem

////////////////////////////////////////////////////////////////////////////
// CColorLB::DrawItem
//
// This method processes the WM_DRAWITEM message for our owner-drawn
// listbox
//
void CColorLB::DrawItem(LPDRAWITEMSTRUCT lpdi)
{
   ASSERT(lpdi->CtlType == ODT_LISTBOX);
   CDC* pDC = CDC::FromHandle(lpdi->hDC);

   // If we are drawing the entire listbox entry -OR- changing
   // selection state for the item, then redraw the entry completely.

   if (lpdi->itemAction & (ODA_DRAWENTIRE | ODA_SELECT ))
   {
     COLORREF clrBack, clrFore, clrOldBack, clrOldFore;

     // Determine and set the background/foreground of the listbox
     // text based on our selection state and the system colors
     // used for listboxes.

     if (lpdi->itemState & ODS_SELECTED)
        clrBack = ::GetSysColor(COLOR_HIGHLIGHT);
     else
        clrBack = ::GetSysColor(COLOR_WINDOW);

     CString strText;
     GetText(lpdi->itemID, strText);

     switch (strText[0])
     {
         case '0':  clrFore = RGB(255,0,0); break; // Rcvd = Red
         case '1':  clrFore = RGB(0,0,255); break; // Sent = Blue
         case '2':  clrFore = RGB(0,128,0); break; // Command = Green
         default:   clrFore = RGB(0,0,0); break;
     }

     clrOldFore = pDC->SetTextColor(clrFore);
     clrOldBack = pDC->SetBkColor(clrBack);

     // Remove our key character.

     strText = strText.Mid(1);

     // Fill in the entry with the background color

     CBrush Brush(clrBack);
     pDC->FillRect(&lpdi->rcItem, &Brush);

     // If the item has focus (due to DRAWENTIRE) then draw the
     // focus rectangle.

     if ((GetStyle() & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) &&
         ( lpdi->itemState & ODS_FOCUS ))
        pDC->DrawFocusRect(&lpdi->rcItem);

     // Draw our text

     pDC->DrawText(strText, strText.GetLength(),
           (LPRECT)(const RECT FAR *)&lpdi->rcItem, 
           (DT_SINGLELINE | DT_LEFT | DT_VCENTER));

     pDC->SetTextColor(clrOldFore);
     pDC->SetBkColor(clrOldBack);
   }                              
   
}// CColorLB::DrawItem

