/*****************************************************************************/
//
// BUTTINFO.CPP - Source code for the CPhoneButtonInfo and CPhoneButtonArray
//                class objects.
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the functions for manipulating the phone        
// button information class which is managed by the CTSPIPhoneConnection
// object.  These two classes are internal to the library and cannot be
// overriden by the derived service provider.
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
#endif

///////////////////////////////////////////////////////////////////////////
// Run-Time class information 

IMPLEMENT_SERIAL( CPhoneButtonInfo, CObject, 1 )
IMPLEMENT_SERIAL( CPhoneButtonArray, CObArray, 1 )

///////////////////////////////////////////////////////////////////////////
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::Serialize
//
// Function to serialize data to/from persistant storage.
//
VOID CPhoneButtonInfo::Serialize(CArchive& ar)
{
    CObject::Serialize(ar);
    if (ar.IsStoring())
    {
        ar << m_dwButtonMode;
        ar << m_dwButtonFunction;
        ar << m_dwLampMode;
        ar << m_strButtonDescription;
    }
    else
    {
        ar >> m_dwButtonMode;
        ar >> m_dwButtonFunction;
        ar >> m_dwLampMode;
        ar >> m_strButtonDescription;
    }

}// CPhoneButtonInfo::Serialize

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::operator=
//
// This is an assignment operator from a TAPI PHONEBUTTONINFO structure
//
const CPhoneButtonInfo& CPhoneButtonInfo::operator=(LPPHONEBUTTONINFO lpPhoneInfo)
{
    m_dwButtonMode = lpPhoneInfo->dwButtonMode;
    m_dwButtonFunction = lpPhoneInfo->dwButtonFunction;
    m_dwLampMode = 0L;

    if (lpPhoneInfo->dwButtonTextSize > 0)   
        m_strButtonDescription = (LPCTSTR)lpPhoneInfo + lpPhoneInfo->dwButtonTextOffset;
    else
        m_strButtonDescription.Empty();        
   
   return *this;

}// CPhoneButtonInfo::operator=

//////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::~CPhoneButtonArray
//
// Destructor
//
CPhoneButtonArray::~CPhoneButtonArray()
{
    RemoveAt (0, GetSize());
        
}// CPhoneButtonArray::~CPhoneButtonArray

//////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::Remove
//
// Removes one or more elements from the button array and deletes
// the object pointers.
//
VOID CPhoneButtonArray::RemoveAt(int nIndex, int nCount/*=1*/)
{
    for (int i = nIndex; i < (nIndex + nCount); i++)
    {
        CPhoneButtonInfo* pButton = GetAt(i);
        delete pButton;
    }
    CObArray::RemoveAt (nIndex, nCount);

}// CPhoneButtonArray

/////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::SetAt
//
// Sets the given CPhoneButtonInfo object at the array position
// indicated.  The array is grown if necessary, and any object
// previously there is removed.
//
VOID CPhoneButtonArray::SetAt(int nIndex, CPhoneButtonInfo* pButt)
{
    m_fDirty = TRUE;
    if (GetSize() > nIndex)
        RemoveAt(nIndex);
    CObArray::SetAtGrow(nIndex, pButt);

}// CPhoneButtonArray::SetAt

/////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::FindButton
//
// Locate a button index based on some criteria passed in about the
// function, type, and how many.
//
int CPhoneButtonArray::FindButton (DWORD dwButtonFunction, DWORD dwButtonMode, int iCount)
{
    int iFound=0;
    for (int i = 0; i < GetSize(); i++)
    {
        CPhoneButtonInfo* pButton = GetAt(i);
        if (pButton && pButton->GetFunction() == dwButtonFunction &&
            (dwButtonMode == PHONEBUTTONMODE_DUMMY || pButton->GetButtonMode() == dwButtonMode))
        {
            if (++iFound == iCount)
                return i;
        }
    }
    
    return -1;
   
}// CPhoneButtonArray::FindButton

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::operator=
//
// This assigns a complete phone button array, transfering all elements
// from one array into another.
//
const CPhoneButtonArray& CPhoneButtonArray::operator=(CPhoneButtonArray& arr)
{
    m_fDirty = arr.m_fDirty;
    for (int i = 0; i < arr.GetSize(); i++)
    {
        CPhoneButtonInfo* pSource = arr.GetAt(i);
        CPhoneButtonInfo* pButton = NULL;
        if (pSource)
            pButton = new CPhoneButtonInfo(pSource->GetFunction(),
                                pSource->GetButtonMode(), 
                                pSource->GetAvailLampModes(),
                                pSource->GetLampMode(),
                                pSource->GetDescription());
        SetAtGrow(i, pButton);
    }
    return *this;

}// CPhoneButtonArray::operator=
