/*****************************************************************************/
//
// BUTTINFO.CPP - Source code for the CPhoneButtonInfo and CPhoneButtonArray
//                class objects.
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the functions for manipulating the phone        
// button information class which is managed by the CTSPIPhoneConnection
// object.  These two classes are internal to the library and cannot be
// overriden by the derived service provider.
//
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
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
// CPhoneButtonInfo::CPhoneButtonInfo
//
// Default constructor (protected)
//
CPhoneButtonInfo::CPhoneButtonInfo()
{
    m_dwButtonFunction = PHONEBUTTONFUNCTION_UNKNOWN;
    m_dwButtonMode = PHONEBUTTONMODE_DUMMY;
    m_dwLampMode = PHONELAMPMODE_DUMMY;
    m_dwButtonState = PHONEBUTTONSTATE_UNKNOWN;
    m_dwAvailLampModes = PHONELAMPMODE_DUMMY;
    m_strButtonDescription = "";

}// CPhoneButtonInfo::CPhoneButtonInfo

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::CPhoneButtonInfo
//
// Parameter constructor
//
CPhoneButtonInfo::CPhoneButtonInfo(DWORD dwFunction, DWORD dwMode, DWORD dwAvailLamp, 
                                   DWORD dwLamp, LPCSTR lpszDesc)
{
    m_dwButtonFunction = dwFunction;
    m_dwButtonMode = dwMode;
    m_dwLampMode = dwLamp;         
    m_dwAvailLampModes = dwAvailLamp;
    m_strButtonDescription = lpszDesc;
    
}// CPhoneButtonInfo::CPhoneButtonInfo

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::SetButtonInfo
//
// Set the button information when it changes.
//
VOID CPhoneButtonInfo::SetButtonInfo (DWORD dwFunction, DWORD dwMode, LPCSTR lpszDesc)
{
    m_dwButtonFunction = dwFunction;
    m_dwButtonMode = dwMode;
    m_strButtonDescription = lpszDesc;

}// CPhoneButtonInfo::SetButtonInfo

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::SetButtonState
//
// Set the pressed state of the button.
//
VOID CPhoneButtonInfo::SetButtonState (DWORD dwButtonState)
{                                   
    m_dwButtonState = dwButtonState;

}// CPhoneButtonInfo::SetButtonState

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetButtonState
//                                 
// Return the current button state
//
DWORD CPhoneButtonInfo::GetButtonState() const
{                                   
    return m_dwButtonState;

}// CPhoneButtonInfo::GetButtonState

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
        m_strButtonDescription = (LPCSTR)lpPhoneInfo + lpPhoneInfo->dwButtonTextOffset;
    else
        m_strButtonDescription.Empty();        
   
   return *this;

}// CPhoneButtonInfo::operator=

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetDescription
//
// Return the description of the button.
//
const char * CPhoneButtonInfo::GetDescription() const
{
    return m_strButtonDescription;
    
}// CPhoneButtonInfo::GetDescription

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetFunction
//
// Return the button function
//
DWORD CPhoneButtonInfo::GetFunction() const
{
    return m_dwButtonFunction;
    
}// CPhoneButtonInfo::GetFunction

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetButtonMode
//
// Return the button mode
//
DWORD CPhoneButtonInfo::GetButtonMode() const
{
    return m_dwButtonMode;
    
}// CPhoneButtonInfo::GetButtonMode

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetLampMode
//
// Return the lamp mode (state)
//
DWORD CPhoneButtonInfo::GetLampMode() const
{
    return m_dwLampMode;
    
}// CPhoneButtonInfo::GetLampMode

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetAvailLampModes
//
// Return the available lamp modes for this button.
//
DWORD CPhoneButtonInfo::GetAvailLampModes() const
{                                      
    return m_dwAvailLampModes;
    
}// CPhoneButtonInfo::GetAvailLampModes

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::SetLampMode
//
// Set the current lamp state
//
VOID CPhoneButtonInfo::SetLampMode(DWORD dwLampMode)
{   
    ASSERT (m_dwAvailLampModes & dwLampMode);
    m_dwLampMode = dwLampMode;
    
}// CPhoneButtonInfo::SetLampMode

//////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::CPhoneButtonArray
//
// Constructor
//
CPhoneButtonArray::CPhoneButtonArray() 
{
    m_fDirty = FALSE; 

}// CPhoneButtonArray::CPhoneButtonArray
 
//////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::~CPhoneButtonArray
//
// Destructor
//
CPhoneButtonArray::~CPhoneButtonArray()
{
    // Delete all the buttons.
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
// CPhoneButtonArray::Serialize
//
// This method handles the serialization of the phone array.
//
VOID CPhoneButtonArray::Serialize(CArchive& ar)
{
    CObArray::Serialize(ar);
    m_fDirty = FALSE;

}// CPhoneButtonArray::Serialize

/////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::Add
//
// Add a new phone button object to our array.
//
int CPhoneButtonArray::Add(CPhoneButtonInfo* pButton)
{
    return CObArray::Add ((CObject*) pButton);
    
}// CPhoneButtonArray::Add

/////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::Add
//
// Add a new phone button object to our array.
//
int CPhoneButtonArray::Add(DWORD dwButtonFunc, DWORD dwButtonMode, DWORD dwAvailLampModes, 
                           DWORD dwLampMode, const char *pszDesc)
{
    return Add (new CPhoneButtonInfo (dwButtonFunc, dwButtonMode, 
                                    dwAvailLampModes, dwLampMode, pszDesc));
    
}// CPhoneButtonArray::Add

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

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::operator[]
//
// Array operator for the button array.
//
CPhoneButtonInfo * CPhoneButtonArray::operator[] (int nIndex) const 
{
    return GetAt (nIndex);
    
}// CPhoneButtonArray::operator[]

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::operator[]
//
// Array operator for the button array.
//
CPhoneButtonInfo * & CPhoneButtonArray::operator [] (int nIndex)
{
    return ElementAt (nIndex);
    
}// CPhoneButtonArray::operator[]

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::ElementAt
//
// Retrieve a reference to the pointer at specified position.
//
CPhoneButtonInfo * & CPhoneButtonArray::ElementAt (int nIndex) 
{
    return (CPhoneButtonInfo* &) CObArray::ElementAt(nIndex);
    
}// CPhoneButtonArray::ElementAt

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::GetAt
//
// Return a pointer to the element at the specified position.
//
CPhoneButtonInfo * CPhoneButtonArray::GetAt(int nIndex) const
{
    return (CPhoneButtonInfo*) CObArray::GetAt(nIndex);
    
}// CPhoneButtonArray::GetAt

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::IsEmpty
//
// Return whether we have buttons or not.
//
BOOL CPhoneButtonArray::IsEmpty (VOID) const 
{
    return (GetSize() == 0);
    
}// CPhoneButtonArray::IsEmpty

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::IsValidIndex
//
// Return whether the button exists at the specified index
//
BOOL CPhoneButtonArray::IsValidIndex (int nIndex) const
{
    return (nIndex >= 0 && nIndex < GetSize() && GetAt(nIndex) != NULL);

}// CPhoneButtonArray::IsValidIndex

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::RemoveAll
//
// Remove all the entries from the phone button array
//
VOID CPhoneButtonArray::RemoveAll() 
{
    RemoveAt (0, GetSize());
    
}// CPhoneButtonArray::RemoveAll

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::IsDirty
//
// Return whether the array elements have changed since we read them
// into memory.
//
BOOL CPhoneButtonArray::IsDirty() const
{
    return m_fDirty;
   
}// CPhoneButtonArray::IsDirty

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::SetDirtyFlag
//
// Set/Unset the dirty flag for the array
//
VOID CPhoneButtonArray::SetDirtyFlag(BOOL fDirty) 
{
    m_fDirty = fDirty;
 
}// CPhoneButtonArray::SetDirtyFlag
