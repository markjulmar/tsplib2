/*****************************************************************************/
//
// DDX.CPP - Digital Switch Service Provider Sample
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

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////
// DDX_CBStringArray
//
// This method associates a combobox with a CStringArray object
//
void AFXAPI DDX_CBStringArray(CDataExchange* pDX, int nIDC, CStringArray& array)
{
   HWND hWndCtrl = pDX->PrepareCtrl(nIDC);

   if (hWndCtrl == NULL)
      return;

   if (pDX->m_bSaveAndValidate)
   {
      array.RemoveAll();

      int nCount = (int) ::SendMessage(hWndCtrl, CB_GETCOUNT, 0, 0L);
      if (nCount != -1)
      {
         for (int x = 0; x < nCount; x++)
         {
            int nLen = (int)::SendMessage(hWndCtrl, CB_GETLBTEXTLEN, x, 0L);
            ASSERT(nLen > 0);

            char *pszItem = new char [nLen+1];
            ::SendMessage(hWndCtrl, CB_GETLBTEXT, x, (LPARAM)(LPSTR)pszItem);
            array.Add(pszItem);
            delete [] pszItem;
         }
      }
   }
   else
   {
      ::SendMessage(hWndCtrl, CB_RESETCONTENT, 0, 0L);

      for (int x = 0; x < array.GetSize(); x++)
      {
         CString str = array.GetAt(x);
         if (!str.IsEmpty())
            ::SendMessage(hWndCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)str);
      }
   }

}// DDX_CBStringArray

