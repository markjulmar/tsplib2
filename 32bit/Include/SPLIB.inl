/******************************************************************************/
//                                                                        
// SPLIB.INL - TAPI Service Provider C++ Library header                     
//                                             
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// The SPLIB classes provide a basis for developing MS-TAPI complient     
// Service Providers.  They provide basic handling for all of the TSPI    
// APIs and a C-based handler which routes all requests through a set of C++     
// classes.                                                                 
//              
// Version 2.00 is for 32-bit TAPI service provider development.
// This library is for use with TAPI version 2.x under Windows NT and Windows 98
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
// INLINE FUNCTIONS
//                                                           
/******************************************************************************/

#ifndef _SPLIB_INL_INC_
#define _SPLIB_INL_INC_

#ifndef _NOINLINES_
#define TSP_INLINE inline
#else
#define TSP_INLINE
#endif

/******************************************************************************/
//
// GLOBAL FUNCTIONS
//
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////
// AllocMem
//
// This function is globally used to allocate memory.  Most memory
// allocations performed within the library come through this single
// function.
//
TSP_INLINE LPVOID AllocMem (DWORD dwSize)
{   
    LPSTR lpszBuff = NULL;

	// Wrap in exception handler in case the C++ library
	// ever actually follows the specification; it should throw
	// on failure - not return NULL.
    try
    {                 
        lpszBuff = new char [dwSize];
    }
    catch (...)
    {
    }

	ASSERT(lpszBuff != NULL);
    return lpszBuff;

}// AllocMem

////////////////////////////////////////////////////////////////////////////
// FreeMem
//
// This function is globally used to free memory.  It should be matched
// to the above function to insure that the proper memory conventions.
//
TSP_INLINE VOID FreeMem (LPVOID lpBuff)
{
	try
	{
		delete [] lpBuff;
	}
	catch (...)
	{
	}

}// FreeMem

////////////////////////////////////////////////////////////////////////////
// CopyBuffer
//
// This function is used globally to transfer a block of memory from
// one area to another.
//
TSP_INLINE VOID CopyBuffer (LPVOID lpDest, LPCVOID lpSource, DWORD dwSize)
{
	ASSERT (DwordAlignPtr (lpDest) == lpDest);
    memcpy (lpDest, lpSource, (size_t) dwSize);

}// CopyBuffer

////////////////////////////////////////////////////////////////////////////
// FillBuffer
//
// Initialize a buffer with a known value.
//
TSP_INLINE VOID FillBuffer (LPVOID lpDest, BYTE bValue, DWORD dwSize)
{             
	ASSERT (DwordAlignPtr (lpDest) == lpDest);
    memset (lpDest, bValue, (size_t) dwSize);

}// FillBuffer

////////////////////////////////////////////////////////////////////////////
// DwordAlignPtr
//
// Given an input pointer, return the nearest DWORD aligned address
// moving forward if necessary.
//
#ifndef _X86_
TSP_INLINE LPVOID DwordAlignPtr (LPVOID lpBuff)
{
	DWORD dw = (DWORD) lpBuff;
	dw += 3;
	dw >>= 2;
	dw <<= 2;
	return (LPVOID) dw;

}// DwordAlignPtr
#else
#define DwordAlignPtr(p) p
#endif

////////////////////////////////////////////////////////////////////////////
// ReportError
//
// Return whether the return code is an error and output it through the
// TRACE stream.
//
TSP_INLINE BOOL ReportError (LONG lResult)
{
    return (lResult >= 0) ? FALSE : TRUE;

}// ReportError

///////////////////////////////////////////////////////////////////////////
// AddDataBlock
//
// Public function to add a string to a VARSTRING type buffer.
//
TSP_INLINE BOOL AddDataBlock (LPVOID lpVB, DWORD& dwOffset, DWORD& dwSize, LPCWSTR lpszBuff)
{
	return AddDataBlock (lpVB, dwOffset, dwSize, lpszBuff, (wcslen(lpszBuff)+1) * sizeof(wchar_t));

}// AddDataBlock


/******************************************************************************/
//
// CTSPIAddressInfo class functions
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddAsynchRequest
//
// This method adds a request to a particular connection.  It
// will add the request to the device list this connection belongs
// to.
//
TSP_INLINE CTSPIRequest* CTSPIAddressInfo::AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId, LPCVOID lpBuff, DWORD dwSize)
{
    return GetLineOwner()->AddAsynchRequest(this, NULL, wReqId, dwReqId, lpBuff, dwSize);

}// CTSPIAddressInfo::AddAsynchRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanHandleRequest
//
// Determine if our service provider is capable of handling the request.
// 
TSP_INLINE BOOL CTSPIAddressInfo::CanHandleRequest(WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest(this, wRequest, dwData);                

}// CTSPIAddressInfo::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAddressID
//
// Return the address identifier for this address.  This is always a
// numeric number from 0-numAddr on the line owner.
//
TSP_INLINE DWORD CTSPIAddressInfo::GetAddressID() const
{
    return m_dwAddressID;

}// CTSPIAddressInfo::GetAddressID

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetLineOwner
//
// Return the line owner for this address
//
TSP_INLINE CTSPILineConnection* CTSPIAddressInfo::GetLineOwner() const
{
    return m_pLine;

}// CTSPIAddressInfo::GetLineOwner

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCallCount
//
// Return the total number of allocated CTSPICallAppearance object in
// the call array.  Note that some of the objects may be idle.
//
TSP_INLINE int CTSPIAddressInfo::GetCallCount() const
{
	CEnterCode sLock(this);  // Synch access to object
	return m_lstCalls.GetCount();
	
}// CTSPIAddressInfo::GetCallCount

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetDialableAddress
//
// Return the dialable address (phone#) for this address
//
TSP_INLINE LPCTSTR CTSPIAddressInfo::GetDialableAddress() const
{ 
    return m_strAddress;

}// CTSPIAddressInfo::GetDialableAddress

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetName
//
// Return the name given to us when the address was created.
//
TSP_INLINE LPCTSTR CTSPIAddressInfo::GetName() const
{ 
    return m_strName;

}// CTSPIAddressInfo::GetName

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetName
//
// Changes the name of the address object internally.
//
TSP_INLINE VOID CTSPIAddressInfo::SetName (LPCTSTR pwszName)
{
	m_strName = pwszName;
	OnAddressCapabiltiesChanged();

}// CTSPIAddressInfo::SetName

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::SetDialableAddress
//
// Changes the address of the address object internally.
//
TSP_INLINE VOID CTSPIAddressInfo::SetDialableAddress (LPCTSTR pwszAddr)
{
	m_strAddress = pwszAddr;
	OnAddressCapabiltiesChanged();

}// CTSPIAddressInfo::SetDialableAddress

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAddressCaps
//
// Return our "partial" address caps structure.
//
TSP_INLINE LPLINEADDRESSCAPS CTSPIAddressInfo::GetAddressCaps()
{ 
    return &m_AddressCaps;

}// CTSPIAddressInfo::GetAddressCaps

//////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAddressStatus
//
// Return our "partial" address status structure.
//
TSP_INLINE LPLINEADDRESSSTATUS CTSPIAddressInfo::GetAddressStatus()
{ 
    return &m_AddressStatus;

}// CTSPIAddressInfo::GetAddressStatus

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCompletionMessageCount
//
// Return the total number of completion messages available.
//
TSP_INLINE int CTSPIAddressInfo::GetCompletionMessageCount() const
{          
#ifdef _DEBUG
	CEnterCode sLock(this);  // Synch access to object
    ASSERT (m_AddressCaps.dwNumCompletionMessages == (DWORD) m_arrCompletionMsgs.GetSize());
#endif
    return (int) m_AddressCaps.dwNumCompletionMessages;
    
}// CTSPIAddressInfo::GetCompletionMessageCount

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCompletionMessage
//
// Return a particular completion message.
//
TSP_INLINE LPCTSTR CTSPIAddressInfo::GetCompletionMessage (int iPos) const
{
	CEnterCode sLock(this);  // Synch access to object
    if (iPos >= 0 && iPos < m_arrCompletionMsgs.GetSize())
        return m_arrCompletionMsgs[iPos];
    return NULL;
                                          
}// CTSPIAddressInfo::GetCompletionMessage

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetAvailableMediaModes
//
// Return the available media modes for this address.
//
TSP_INLINE DWORD CTSPIAddressInfo::GetAvailableMediaModes () const
{
    return m_AddressCaps.dwAvailableMediaModes;

}// CTSPIAddressInfo::GetAvailableMediaModes

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanMakeCalls
//
// Return whether this address can make calls EVER
//
TSP_INLINE BOOL CTSPIAddressInfo::CanMakeCalls() const
{                                 
    return ((m_dwFlags & OutputAvail) != 0);
    
}// CTSPIAddressInfo::CanMakeCalls

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::CanAnswerCalls
//
// Return whether this address can answer calls EVER
//
TSP_INLINE BOOL CTSPIAddressInfo::CanAnswerCalls() const
{                                 
    return ((m_dwFlags & InputAvail) != 0);
    
}// CTSPIAddressInfo::CanAnswerCalls

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCurrentRate
//
// Get the current data rate
//
TSP_INLINE DWORD CTSPIAddressInfo::GetCurrentRate() const
{                                   
	CEnterCode sLock(this);
    return m_dwCurrRate;
    
}// CTSPIAddressInfo::GetCurrentRate

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCurrentBearerMode
//
// Get the current bearer mode for this address.  This cooresponds to
// the Quality of Service (QOS) for the address.
//
TSP_INLINE DWORD CTSPIAddressInfo::GetBearerMode() const
{   
	CEnterCode sLock(this);
    return m_dwBearerMode;
    
}// CTSPIAddressInfo::GetCurrentBearerMode

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetTerminalInformation
//
// Return the terminal information for the specified terminal.
//
TSP_INLINE DWORD CTSPIAddressInfo::GetTerminalInformation (int iTerminalID) const
{                                           
	CEnterCode sLock(this);  // Synch access to object
	if (iTerminalID >= 0 && iTerminalID < m_arrTerminals.GetSize())
		return m_arrTerminals[iTerminalID];
	return 0L;

}// CTSPIAddressInfo::GetTerminalInformation

///////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetCallTreatmentName
//
// Return the name for the specified call treatment value.
//
TSP_INLINE CString CTSPIAddressInfo::GetCallTreatmentName (DWORD dwCallTreatment) const
{
	CString strName;
	if (m_mapCallTreatment.Lookup (dwCallTreatment, strName))
		return strName;
	return _T("");

}// CTSPIAddressInfo::GetCallTreatmentName

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddDeviceClass
//
// Add a DWORD data object to our device class list
//
TSP_INLINE int CTSPIAddressInfo::AddDeviceClass (LPCTSTR pszClass, DWORD dwData)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, &dwData, sizeof(DWORD));

}// CTSPIAddressInfo::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddDeviceClass
//
// Add a DWORD data object to our device class list
//
TSP_INLINE int CTSPIAddressInfo::AddDeviceClass (LPCTSTR pszClass, LPCTSTR lpszBuff, DWORD dwType)
{
	if (dwType == -1L)
		dwType = m_pLine->m_LineCaps.dwStringFormat;
	return AddDeviceClass (pszClass, dwType, (LPVOID)lpszBuff, (_tcslen(lpszBuff)+1) * sizeof(TCHAR));

}// CTSPIAddressInfo::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddDeviceClass
//
// Add a HANDLE and BUFFER to our device class list
//
TSP_INLINE int CTSPIAddressInfo::AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPCTSTR lpszBuff)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, (LPVOID)lpszBuff, (_tcslen(lpszBuff)+1) * sizeof(TCHAR), hHandle);

}// CTSPIAddressInfo::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddDeviceClass
//
// Add a HANDLE and BUFFER to our device class list
//
TSP_INLINE int CTSPIAddressInfo::AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPVOID lpBuff, DWORD dwSize)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, lpBuff, dwSize, hHandle);

}// CTSPIAddressInfo::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::AddDeviceClass
//
// Add a DWORD data object to our device class list
//
TSP_INLINE int CTSPIAddressInfo::AddDeviceClass (LPCTSTR pszClass, DWORD dwFormat, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle)
{
	CEnterCode sLock(this);
	return GetSP()->AddDeviceClassInfo (m_arrDeviceClass, pszClass, dwFormat, lpBuff, dwSize, hHandle);

}// CTSPIAddressInfo::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::RemoveDeviceClass
//
// Remove a device class list object.
//
TSP_INLINE BOOL CTSPIAddressInfo::RemoveDeviceClass (LPCTSTR pszClass)
{
	CEnterCode sLock(this);
	return GetSP()->RemoveDeviceClassInfo (m_arrDeviceClass, pszClass);	

}// CTSPIAddressInfo::RemoveDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIAddressInfo::GetDeviceClass
//
// Return the device class information for a specified name.
//
TSP_INLINE DEVICECLASSINFO* CTSPIAddressInfo::GetDeviceClass(LPCTSTR pszClass)
{
	CEnterCode sLock(this);
	return GetSP()->FindDeviceClassInfo (m_arrDeviceClass, pszClass);

}// CTSPIAddressInfo::GetDeviceClass

/******************************************************************************/
//
// CPhoneButtonInfo
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::CPhoneButtonInfo
//
// Default constructor (protected)
//
TSP_INLINE CPhoneButtonInfo::CPhoneButtonInfo() : 
    m_dwButtonFunction(PHONEBUTTONFUNCTION_UNKNOWN), m_dwButtonMode(PHONEBUTTONMODE_DUMMY),
    m_dwLampMode(PHONELAMPMODE_DUMMY), m_dwButtonState(PHONEBUTTONSTATE_UNKNOWN),
    m_dwAvailLampModes(PHONELAMPMODE_DUMMY)
{
}// CPhoneButtonInfo::CPhoneButtonInfo

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::CPhoneButtonInfo
//
// Parameter constructor
//
TSP_INLINE CPhoneButtonInfo::CPhoneButtonInfo(DWORD dwFunction, DWORD dwMode, 
		DWORD dwAvailLamp, DWORD dwLamp, LPCTSTR lpszDesc) :
    m_dwButtonFunction(dwFunction), m_dwButtonMode(dwMode),
    m_dwLampMode(dwLamp), m_strButtonDescription(lpszDesc),
    m_dwAvailLampModes(dwAvailLamp)
{
}// CPhoneButtonInfo::CPhoneButtonInfo

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::SetButtonInfo
//
// Set the button information when it changes.
//
TSP_INLINE VOID CPhoneButtonInfo::SetButtonInfo (DWORD dwFunction, DWORD dwMode, LPCTSTR lpszDesc)
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
TSP_INLINE VOID CPhoneButtonInfo::SetButtonState (DWORD dwButtonState)
{                                   
    m_dwButtonState = dwButtonState;

}// CPhoneButtonInfo::SetButtonState

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetButtonState
//                                 
// Return the current button state
//
TSP_INLINE DWORD CPhoneButtonInfo::GetButtonState() const
{                                   
    return m_dwButtonState;

}// CPhoneButtonInfo::GetButtonState

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetDescription
//
// Return the description of the button.
//
TSP_INLINE LPCTSTR CPhoneButtonInfo::GetDescription() const
{
    return m_strButtonDescription;
    
}// CPhoneButtonInfo::GetDescription

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetFunction
//
// Return the button function
//
TSP_INLINE DWORD CPhoneButtonInfo::GetFunction() const
{
    return m_dwButtonFunction;
    
}// CPhoneButtonInfo::GetFunction

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetButtonMode
//
// Return the button mode
//
TSP_INLINE DWORD CPhoneButtonInfo::GetButtonMode() const
{
    return m_dwButtonMode;
    
}// CPhoneButtonInfo::GetButtonMode

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetLampMode
//
// Return the lamp mode (state)
//
TSP_INLINE DWORD CPhoneButtonInfo::GetLampMode() const
{
    return m_dwLampMode;
    
}// CPhoneButtonInfo::GetLampMode

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::GetAvailLampModes
//
// Return the available lamp modes for this button.
//
TSP_INLINE DWORD CPhoneButtonInfo::GetAvailLampModes() const
{                                      
    return m_dwAvailLampModes;
    
}// CPhoneButtonInfo::GetAvailLampModes

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonInfo::SetLampMode
//
// Set the current lamp state
//
TSP_INLINE VOID CPhoneButtonInfo::SetLampMode(DWORD dwLampMode)
{   
    ASSERT (m_dwAvailLampModes & dwLampMode);
    m_dwLampMode = dwLampMode;
    
}// CPhoneButtonInfo::SetLampMode

/******************************************************************************/
//
// CPhoneButtonArray
//
/******************************************************************************/

//////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::CPhoneButtonArray
//
// Constructor
//
TSP_INLINE CPhoneButtonArray::CPhoneButtonArray() : m_fDirty(FALSE)
{
}// CPhoneButtonArray::CPhoneButtonArray
 
/////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::Serialize
//
// This method handles the serialization of the phone array.
//
TSP_INLINE VOID CPhoneButtonArray::Serialize(CArchive& ar)
{
    m_fDirty = FALSE;
    CObArray::Serialize(ar);

}// CPhoneButtonArray::Serialize

/////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::Add
//
// Add a new phone button object to our array.
//
TSP_INLINE int CPhoneButtonArray::Add(CPhoneButtonInfo* pButton)
{
	return CObArray::Add ((CObject*) pButton);
    
}// CPhoneButtonArray::Add

/////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::Add
//
// Add a new phone button object to our array.
//
TSP_INLINE int CPhoneButtonArray::Add(DWORD dwButtonFunc, DWORD dwButtonMode, 
									  DWORD dwAvailLampModes, DWORD dwLampMode, LPCTSTR pszDesc)
{
    return Add (new CPhoneButtonInfo (dwButtonFunc, dwButtonMode, 
                                    dwAvailLampModes, dwLampMode, pszDesc));
    
}// CPhoneButtonArray::Add

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::operator[]
//
// Array operator for the button array.
//
TSP_INLINE CPhoneButtonInfo * CPhoneButtonArray::operator[] (int nIndex) const 
{
    return GetAt (nIndex);
    
}// CPhoneButtonArray::operator[]

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::operator[]
//
// Array operator for the button array.
//
TSP_INLINE CPhoneButtonInfo * & CPhoneButtonArray::operator [] (int nIndex)
{
    return ElementAt (nIndex);
    
}// CPhoneButtonArray::operator[]

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::ElementAt
//
// Retrieve a reference to the pointer at specified position.
//
TSP_INLINE CPhoneButtonInfo * & CPhoneButtonArray::ElementAt (int nIndex) 
{
    return (CPhoneButtonInfo* &) CObArray::ElementAt(nIndex);
    
}// CPhoneButtonArray::ElementAt

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::GetAt
//
// Return a pointer to the element at the specified position.
//
TSP_INLINE CPhoneButtonInfo * CPhoneButtonArray::GetAt(int nIndex) const
{
    return (CPhoneButtonInfo*) CObArray::GetAt(nIndex);
    
}// CPhoneButtonArray::GetAt

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::IsEmpty
//
// Return whether we have buttons or not.
//
TSP_INLINE BOOL CPhoneButtonArray::IsEmpty (VOID) const 
{
    return (GetSize() == 0);
    
}// CPhoneButtonArray::IsEmpty

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::IsValidIndex
//
// Return whether the button exists at the specified index
//
TSP_INLINE BOOL CPhoneButtonArray::IsValidIndex (int nIndex) const
{
    return (nIndex >= 0 && nIndex < GetSize() && GetAt(nIndex) != NULL);

}// CPhoneButtonArray::IsValidIndex

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::RemoveAll
//
// Remove all the entries from the phone button array
//
TSP_INLINE VOID CPhoneButtonArray::RemoveAll() 
{
    RemoveAt (0, GetSize());
    
}// CPhoneButtonArray::RemoveAll

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::IsDirty
//
// Return whether the array elements have changed since we read them
// into memory.
//
TSP_INLINE BOOL CPhoneButtonArray::IsDirty() const
{
    return m_fDirty;
   
}// CPhoneButtonArray::IsDirty

///////////////////////////////////////////////////////////////////////////
// CPhoneButtonArray::SetDirtyFlag
//
// Set/Unset the dirty flag for the array
//
TSP_INLINE VOID CPhoneButtonArray::SetDirtyFlag(BOOL fDirty) 
{
    m_fDirty = fDirty;
 
}// CPhoneButtonArray::SetDirtyFlag

/******************************************************************************/
//
// CTSPICallAppearance
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::IncRefCount
//
// Increment the reference count
//
TSP_INLINE void CTSPICallAppearance::IncRefCount()
{
	InterlockedIncrement(&m_lRefCount);

}// CTSPICallAppearance::IncRefCount

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DecRefCount
//
// Decrement the reference count
//
TSP_INLINE void CTSPICallAppearance::DecRefCount()
{
	if (InterlockedDecrement(&m_lRefCount) == 0)
	{
		ASSERT ((m_dwFlags & IsDeleted) == IsDeleted);
		DTRACE(TRC_MIN, _T("Deleting call appearance 0x%lx\r\n"), (DWORD)this);
		delete this;
	}

}// CTSPICallAppearance::DecRefCount

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallType
//
// Return a unique call type which can identify conference and
// consultant calls
//
TSP_INLINE int CTSPICallAppearance::GetCallType() const
{
    return m_iCallType;

}// CTSPICallAppearance::GetCallType

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetCallType
//
// Set the call type for this call.
//
TSP_INLINE VOID CTSPICallAppearance::SetCallType(int iCallType)
{
    m_iCallType = iCallType;

}// CTSPICallAppearance::SetCallType

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::CanHandleRequest
//
// Determine dynamically what the service provider can support
//
TSP_INLINE BOOL CTSPICallAppearance::CanHandleRequest (WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest (this, wRequest, dwData);

}// CTSPICallAppearance::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AddAsynchRequest
//
// This method adds a request to a particular connection.  It
// will add the request to the device list this connection belongs
// to.
//
TSP_INLINE CTSPIRequest* CTSPICallAppearance::AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId, LPCVOID lpBuff, DWORD dwSize)
{
    return GetLineOwner()->AddAsynchRequest(this, wReqId, dwReqId, lpBuff, dwSize);

}// CTSPICallAppearance::AddAsynchRequest

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetLineOwner
//
// This returns the line connection information for this call
// appearance.
//
TSP_INLINE CTSPILineConnection* CTSPICallAppearance::GetLineOwner() const
{
    return GetAddressOwner()->GetLineOwner();

}// CTSPICallAppearance::GetLineOwner

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetAddressOwner
//
// Return the address information for this call appearance.
//
TSP_INLINE CTSPIAddressInfo* CTSPICallAppearance::GetAddressOwner() const
{
    ASSERT (m_pAddr != NULL);
    return m_pAddr;

}// CTSPICallAppearance::GetAddressOwner

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallHandle
//
// Return the TAPI call handle for this call appearance.
//
TSP_INLINE HTAPICALL CTSPICallAppearance::GetCallHandle() const
{
    return m_htCall;

}// CTSPICallAppearance::GetCallHandle

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallState
//
// Return the call state of the call appearance from our LINECALLSTATE
// structure.
//
TSP_INLINE DWORD CTSPICallAppearance::GetCallState() const
{ 
    return m_CallStatus.dwCallState;

}// CTSPICallAppearance::GetCallState

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallInfo
//
// Return a pointer to the LINECALLINFO record
//
TSP_INLINE LPLINECALLINFO CTSPICallAppearance::GetCallInfo()
{                                     
    return &m_CallInfo;
    
}// CTSPICallAppearance::GetCallInfo

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetCallStatus
//                                   
// Return a pointer to the LINECALLSTATUS record
//
TSP_INLINE LPLINECALLSTATUS CTSPICallAppearance::GetCallStatus()
{   
    return &m_CallStatus;
    
}// CTSPICallAppearance::GetCallStatus

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetConsultationCall
//                                   
// Sets the consultation call for this call
//
TSP_INLINE void CTSPICallAppearance::SetConsultationCall(CTSPICallAppearance* pConsultCall)
{
	if (pConsultCall != NULL)
	{
	    pConsultCall->SetCallType(CALLTYPE_CONSULTANT);
		AttachCall(pConsultCall);
	    pConsultCall->AttachCall(this);
	}
	else
	{
		if (m_pConsult != NULL)
			m_pConsult->DetachCall();
		DetachCall();
	}

}// CTSPICallAppearance::SetConsultationCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetConsultationCall
//
// Return the attached call appearance
//
TSP_INLINE CTSPICallAppearance* CTSPICallAppearance::GetConsultationCall() const
{
    return m_pConsult;
    
}// CTSPICallAppearance::GetConsultationCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AttachCall
//
// Attach a call to this call appearance
//
TSP_INLINE void CTSPICallAppearance::AttachCall (CTSPICallAppearance* pCall)
{                                  
	if (pCall == NULL)
		DetachCall();
	else
	{
		m_pConsult = pCall;
		SetRelatedCallID(pCall->GetCallInfo()->dwCallID);
	}
    
}// CTSPICallAppearance::AttachCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::SetConferenceOwner
//
// Set the attached conference owner
//
TSP_INLINE void CTSPICallAppearance::SetConferenceOwner(CTSPIConferenceCall* pConf)
{
	m_pConf = pConf;

}// CTSPICallAppearance::SetConferenceOwner

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetAttachedCall
//
// Return the attached call appearance
//
TSP_INLINE CTSPICallAppearance* CTSPICallAppearance::GetAttachedCall() const
{
    return m_pConsult;
    
}// CTSPICallAppearance::GetAttachedCall

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetConferenceOwner
//
// Return the attached conference call appearance
//
TSP_INLINE CTSPIConferenceCall* CTSPICallAppearance::GetConferenceOwner() const
{
    return m_pConf;
    
}// CTSPICallAppearance::GetConferenceOwner

///////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::DetachCall
//
// Detach a call from this call appearance
//
TSP_INLINE void CTSPICallAppearance::DetachCall()
{                                  
    m_pConsult = NULL;
	SetRelatedCallID(0);

}// CTSPICallAppearance::DetachCall

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AddDeviceClass
//
// Add a DWORD data object to our device class list
//
TSP_INLINE int CTSPICallAppearance::AddDeviceClass (LPCTSTR pszClass, DWORD dwData)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, &dwData, sizeof(DWORD));

}// CTSPICallAppearance::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AddDeviceClass
//
// Add a STRING data object to our device class list
//
TSP_INLINE int CTSPICallAppearance::AddDeviceClass (LPCTSTR pszClass, LPCTSTR lpszBuff, DWORD dwType)
{
	if (dwType == -1L)
		dwType = m_pAddr->m_pLine->m_LineCaps.dwStringFormat;
	return AddDeviceClass (pszClass, dwType, (LPVOID)lpszBuff, (_tcslen(lpszBuff)+1) * sizeof(TCHAR));

}// CTSPICallAppearance::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AddDeviceClass
//
// Add a HANDLE and BUFFER to our device class list
//
TSP_INLINE int CTSPICallAppearance::AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPCTSTR lpszBuff)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, (LPVOID)lpszBuff, (_tcslen(lpszBuff)+1) * sizeof(TCHAR), hHandle);

}// CTSPICallAppearance::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AddDeviceClass
//
// Add a HANDLE and BUFFER to our device class list
//
TSP_INLINE int CTSPICallAppearance::AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPVOID lpBuff, DWORD dwSize)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, lpBuff, dwSize, hHandle);

}// CTSPICallAppearance::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::AddDeviceClass
//
// Add a DWORD data object to our device class list
//
TSP_INLINE int CTSPICallAppearance::AddDeviceClass (LPCTSTR pszClass, DWORD dwFormat, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle)
{
	CEnterCode sLock(this);
	return GetSP()->AddDeviceClassInfo (m_arrDeviceClass, pszClass, dwFormat, lpBuff, dwSize, hHandle);

}// CTSPICallAppearance::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::RemoveDeviceClass
//
// Remove a device class list object.
//
TSP_INLINE BOOL CTSPICallAppearance::RemoveDeviceClass (LPCTSTR pszClass)
{
	CEnterCode sLock(this);
	return GetSP()->RemoveDeviceClassInfo (m_arrDeviceClass, pszClass);	

}// CTSPICallAppearance::RemoveDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPICallAppearance::GetDeviceClass
//
// Return the device class information for a specified name.
//
TSP_INLINE DEVICECLASSINFO* CTSPICallAppearance::GetDeviceClass(LPCTSTR pszClass)
{
	CEnterCode sLock(this);
	return GetSP()->FindDeviceClassInfo (m_arrDeviceClass, pszClass);

}// CTSPICallAppearance::GetDeviceClass

/******************************************************************************/
//
// CTSPIConferenceCall
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::CTSPIConferenceCall
//
// Conference call constructor
//
TSP_INLINE CTSPIConferenceCall::CTSPIConferenceCall()
{
	m_iCallType = CALLTYPE_CONFERENCE;

}// CTSPIConferenceCall::CTSPIConferenceCall

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::GetConferenceCount
//
// Return the count of call appearances in our conference.
//
TSP_INLINE int CTSPIConferenceCall::GetConferenceCount() const
{                                          
	CEnterCode sLock(this);  // Synch access to object
    return m_arrConference.GetSize();

}// CTSPIConferenceCall::GetConferenceCount

///////////////////////////////////////////////////////////////////////////
// CTSPIConferenceCall::GetConferenceCall
//
// Return the call at the index specified
//
TSP_INLINE CTSPICallAppearance* CTSPIConferenceCall::GetConferenceCall(int iPos)
{   
	CEnterCode sLock(this);  // Synch access to object
    if (iPos >= 0 && iPos < GetConferenceCount())                                      
        return (CTSPICallAppearance*) m_arrConference.GetAt(iPos);
    return NULL;        

}// CTSPIConferenceCall::GetConferenceCall

/******************************************************************************/
//
// CTSPIConnection
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::CTSPIConnection
//
// Constructor
//
TSP_INLINE CTSPIConnection::CTSPIConnection() :
	m_pDevice(NULL), m_dwDeviceID(0), m_dwFlags(0)
{
	m_dwNegotiatedVersion = GetSP()->GetSupportedVersion();

}// CTSPIConnection::CTSPIConnection

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetDeviceInfo
//
// Return the device owner for this connection
//
TSP_INLINE CTSPIDevice* CTSPIConnection::GetDeviceInfo() const
{
    return m_pDevice;

}// CTSPIConnection::GetDeviceInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetName
//
// Retrieve the name of the device
//
TSP_INLINE LPCTSTR CTSPIConnection::GetName() const
{
    return m_strName;

}// CTSPIConnection::GetName

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetDeviceID
//
// Return the device identifier
//
TSP_INLINE DWORD CTSPIConnection::GetDeviceID() const 
{ 
    return m_dwDeviceID;

}// CTSPIConnection::GetDeviceID

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetFlags
//
// Returns the connection flags
//
TSP_INLINE DWORD CTSPIConnection::GetFlags() const
{
	return m_dwFlags;
	
}// CTSPIConnection::GetFlags

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::HasBeenDeleted
//
// Return whether or not this device has been REMOVED from the system.
//
TSP_INLINE BOOL CTSPIConnection::HasBeenDeleted() const
{
    return ((GetFlags() & _IsDeleted) == _IsDeleted);

}// CTSPIConnection::HasBeenDeleted

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::SetDeviceID
//
// This function is called in response to a LINE_CREATE message (and
// TAPIs subsequent call to providerCreateLineDevice) to reset our new
// device identifier.
//
TSP_INLINE VOID CTSPIConnection::SetDeviceID(DWORD dwID)
{
    TRACE (_T("Connection <0x%lx> changing device id to %ld\r\n"), (DWORD)this, dwID);
    m_dwDeviceID = dwID;

}// CTSPIConnection::SetDeviceID

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::SetName
//
// Set the connection name.  This is not a required piece of data,
// but if supplied, will be placed into the LINE/PHONE capabilities
// structure.
//
TSP_INLINE VOID CTSPIConnection::SetName(LPCTSTR lpszName)
{
    m_strName = lpszName;
    
}// CTSPIConnection::SetName

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetConnInfo
//
// Return the switch/phone information for this connection
//
TSP_INLINE LPCTSTR CTSPIConnection::GetConnInfo() const
{                               
    return m_strDevInfo;

}// CTSPIConnection::GetConnInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::SetConnInfo
//
// Set the switch/phone information for this connection - should
// be called during providerINIT.
//
TSP_INLINE VOID CTSPIConnection::SetConnInfo(LPCTSTR pszInfo)
{                               
    m_strDevInfo = pszInfo;

}// CTSPIConnection::SetConnInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetNegotiatedVersion
//
// Return the negotiated TAPI version for this connection
//
TSP_INLINE DWORD CTSPIConnection::GetNegotiatedVersion() const
{   
    return m_dwNegotiatedVersion;
    
}// CTSPIConnection::GetNegotiatedVersion

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddAsynchRequest
//
// This method adds a request to a particular connection.  It
// will add the request to the device list this connection belongs
// to.
//
TSP_INLINE CTSPIRequest* CTSPIConnection::AddAsynchRequest(CTSPICallAppearance* pCall, WORD wReqId, 
                            DRV_REQUESTID dwReqId, LPCVOID lpBuff, DWORD dwSize)
{                                  
    CTSPIAddressInfo* pAddr = (pCall) ? pCall->GetAddressOwner() : NULL;
    return AddAsynchRequest (pAddr, pCall, wReqId, dwReqId, lpBuff, dwSize);

}// CTSPIConnection::AddAsynchRequest

///////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetRequestCount
//
// Return the total pending requests.
//
TSP_INLINE int CTSPIConnection::GetRequestCount() const
{
	CEnterCode sLock(this);  // Synch access to object
    return m_oblAsynchRequests.GetCount();

}// CTSPIConnection::GetRequestCount

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::SendString
//
// Send data out the device.  This is called whenever data needs to 
// go out to this specific device.  The data must be NULL terminated.
//
TSP_INLINE BOOL CTSPIConnection::SendString (LPCTSTR lpszBuff)
{
    return SendData ((LPCVOID)lpszBuff, (_tcslen(lpszBuff)+1)*sizeof(TCHAR));

}// CTSPIConnection::SendString

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::IsLineDevice
//
// Return whether this connection is a CTSPILineConnection
//
TSP_INLINE BOOL CTSPIConnection::IsLineDevice() const
{
    WORD wIndex = (WORD)(GetPermanentDeviceID() & 0xffff);
    return ((wIndex & 0x8000) == 0);

}// CTSPIConnection::IsLineDevice

///////////////////////////////////////////////////////////////////////////
// CTSPIConnection::IsPhoneDevice
//
// Return whether this connection is a CTSPIPhoneConnection
//
TSP_INLINE BOOL CTSPIConnection::IsPhoneDevice() const
{
    WORD wIndex = (WORD)(GetPermanentDeviceID() & 0xffff);
    return ((wIndex & 0x8000) > 0);

}// CTSPIConnection::IsPhoneDevice

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddDeviceClass
//
// Add a DWORD data object to our device class list
//
TSP_INLINE int CTSPIConnection::AddDeviceClass (LPCTSTR pszClass, DWORD dwData)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, &dwData, sizeof(DWORD));

}// CTSPIConnection::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddDeviceClass
//
// Add a HANDLE and BUFFER to our device class list
//
TSP_INLINE int CTSPIConnection::AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPCTSTR lpszBuff)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, (LPVOID)lpszBuff, (_tcslen(lpszBuff)+1) * sizeof(TCHAR), hHandle);

}// CTSPIConnection::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddDeviceClass
//
// Add a HANDLE and BUFFER to our device class list
//
TSP_INLINE int CTSPIConnection::AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPVOID lpBuff, DWORD dwSize)
{
	return AddDeviceClass (pszClass, STRINGFORMAT_BINARY, lpBuff, dwSize, hHandle);

}// CTSPIConnection::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::AddDeviceClass
//
// Add a DWORD data object to our device class list
//
TSP_INLINE int CTSPIConnection::AddDeviceClass (LPCTSTR pszClass, DWORD dwFormat, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle)
{
	CEnterCode sLock(this);
	return GetSP()->AddDeviceClassInfo (m_arrDeviceClass, pszClass, dwFormat, lpBuff, dwSize, hHandle);

}// CTSPIConnection::AddDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::RemoveDeviceClass
//
// Remove a device class list object.
//
TSP_INLINE BOOL CTSPIConnection::RemoveDeviceClass (LPCTSTR pszClass)
{
	CEnterCode sLock(this);
	return GetSP()->RemoveDeviceClassInfo (m_arrDeviceClass, pszClass);	

}// CTSPIConnection::RemoveDeviceClass

////////////////////////////////////////////////////////////////////////////
// CTSPIConnection::GetDeviceClass
//
// Return the device class information for a specified name.
//
TSP_INLINE DEVICECLASSINFO* CTSPIConnection::GetDeviceClass(LPCTSTR pszClass)
{
	CEnterCode sLock(this);
	return GetSP()->FindDeviceClassInfo (m_arrDeviceClass, pszClass);

}// CTSPIConnection::GetDeviceClass

/******************************************************************************/
//
// CTSPIDevice
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::CTSPIDevice
//
// Constructor
//
TSP_INLINE CTSPIDevice::CTSPIDevice() :
	m_dwProviderId(0), m_lpfnCompletionProc(NULL), m_hProvider(NULL), 
	m_dwIntervalTimeout(0), m_pIntervalTimer(0), m_evtDeviceShutdown(FALSE, TRUE)
{
}// CTSPIDevice::CTSPIDevice

///////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetPermanentDeviceID
//
// Return a permanent device id which identifies this device but
// no line or phone.
//
TSP_INLINE DWORD CTSPIDevice::GetPermanentDeviceID() const
{
    return ((m_dwProviderId << 16) & 0xffff0000);

}// CTSPIDevice::GetPermanentDeviceID

////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetProviderHandle
//
// Returns the handle supplied to TSPI_providerEnumDevices.
//
TSP_INLINE HPROVIDER CTSPIDevice::GetProviderHandle() const
{
	return m_hProvider;
	
}// CTSPIDevice::GetProviderHandle

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetLineCount
//
// Return the count of lines available on this device.
//
TSP_INLINE int CTSPIDevice::GetLineCount() const
{
	CEnterCode sLock(this);  // Synch access to object
    return m_arrayLines.GetSize();

}// CTSPIDevice::GetLineCount

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetPhoneCount
//
// Return the count of phones available on this device.
//
TSP_INLINE int CTSPIDevice::GetPhoneCount() const
{
	CEnterCode sLock(this);  // Synch access to object
    return m_arrayPhones.GetSize();

}// CTSPIDevice::GetPhoneCount

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::AssociateLineWithPhone
//
// Associate a line/phone together the "tapi" way.
//
TSP_INLINE void CTSPIDevice::AssociateLineWithPhone(int iLine, int iPhone)
{
	CTSPILineConnection* pLine = GetLineConnectionInfo(iLine);
	CTSPIPhoneConnection* pPhone = GetPhoneConnectionInfo(iPhone);
	if (pLine && pPhone)
	{
		pLine->AddDeviceClass(_T("tapi/phone"), pPhone->GetDeviceID());
		pPhone->AddDeviceClass(_T("tapi/line"), pLine->GetDeviceID());
	}

}// CTSPIDevice::AssociateLineWithPhone

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::AddLineConnectionInfo
//
// Add a new line connection object to our device list.
//
TSP_INLINE WORD CTSPIDevice::AddLineConnectionInfo(CTSPILineConnection* pConn)
{
	CEnterCode sLock(this);  // Synch access to object
    return (WORD) m_arrayLines.Add((CObject*) pConn);

}// CTSPIDevice::AddLineConnectionInfo

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::AddPhoneConnectionInfo
//
// Add a new line connection object to our device list.
//
TSP_INLINE WORD CTSPIDevice::AddPhoneConnectionInfo(CTSPIPhoneConnection* pConn)
{
	CEnterCode sLock(this);  // Synch access to object
    return (WORD) m_arrayPhones.Add((CObject*) pConn);

}// CTSPIDevice::AddPhoneConnectionInfo

///////////////////////////////////////////////////////////////////////////////
// CTSPIDevice::GetProviderId
//
// The provider id is assigned by TAPISRV.EXE when a service provider
// is installed through TSPI_providerInstall.  It generally starts
// with zero (the first one) and gets incremented as each new TSP is
// installed.
//
TSP_INLINE DWORD CTSPIDevice::GetProviderID() const
{
    return m_dwProviderId;

}// CTSPIDevice::GetProviderId

/******************************************************************************/
//
// CTSPIDisplay
//
/******************************************************************************/

//////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::CPhoneDisplay
//
// Display constructor
//
TSP_INLINE CPhoneDisplay::CPhoneDisplay() :
	m_lpsDisplay(NULL), m_sizDisplay(CSize(0,0)), 
	m_ptCursor(CPoint(0,0)), m_cLF(_T('\n'))
{
}// CPhoneDisplay::CPhoneDisplay

//////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::~CPhoneDisplay
//
// Display Destructor
//
TSP_INLINE CPhoneDisplay::~CPhoneDisplay()
{
    FreeMem(m_lpsDisplay);

}// CPhoneDisplay::~CPhoneDisplay

///////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::GetCursorPosition
//
// Returns the maximum number of columns on display
//
TSP_INLINE CPoint CPhoneDisplay::GetCursorPosition() const
{
    return m_ptCursor;

}// CPhoneDisplay::GetCursorPosition

///////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::GetDisplaySize
//
// Returns the size of our display.
//
TSP_INLINE CSize CPhoneDisplay::GetDisplaySize() const
{
    return m_sizDisplay;

}// CPhoneDisplay::GetDisplaySize

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::GetTextBuffer
//
// Return the contents of the screen buffer
//
TSP_INLINE LPCTSTR CPhoneDisplay::GetTextBuffer() const
{
    m_lpsDisplay[m_sizDisplay.cx*m_sizDisplay.cy] = _T('\0');
    return m_lpsDisplay;

}// CPhoneDisplay::GetTextBuffer

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::AddString
//
// Add a string to the display.
//
TSP_INLINE void CPhoneDisplay::AddString(LPCTSTR pszText)
{
    while (*pszText != _T('\0'))
        AddCharacter (*pszText++);

}// CPhoneDisplay::AddString

/******************************************************************************/
//
// CTSPILineConnection
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::CanHandleRequest
//
// Determine if our service provider is capable of handling the request.
// 
TSP_INLINE BOOL CTSPILineConnection::CanHandleRequest(WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest(this, wRequest, dwData);                

}// CTSPILineConnection::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddress
//
// Return an address based on an address ID.
//
TSP_INLINE CTSPIAddressInfo* CTSPILineConnection::GetAddress (int iAddressID) const
{
    return GetAddress((DWORD)iAddressID);

}// CTSPILineConnection::GetAddress

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddress
//
// Return an address based on an address ID.
//
TSP_INLINE CTSPIAddressInfo* CTSPILineConnection::GetAddress (DWORD dwAddressID) const
{
	CEnterCode sLock(this);  // Synch access to object
    return (dwAddressID >= (DWORD)m_arrAddresses.GetSize()) ? 
			NULL : (CTSPIAddressInfo*) m_arrAddresses[(int)dwAddressID];

}// CTSPILineConnection::GetAddress

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAddressCount
//
// Return the number of addresses available on this line.
//
TSP_INLINE int CTSPILineConnection::GetAddressCount() const
{
	CEnterCode sLock(this);  // Synch access to object
	return m_arrAddresses.GetSize();

}// CTSPILineConnection::GetAddressCount

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetLineHandle
//
// Return the line handle for this line device.
//
TSP_INLINE HTAPILINE CTSPILineConnection::GetLineHandle() const
{
    return m_htLine;

}// CTSPILineConnection::GetLineHandle

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::ForceClose
//
// This method may be used by the service provider to forcibly close the
// line object.
//
TSP_INLINE void CTSPILineConnection::ForceClose()
{
	Send_TAPI_Event (NULL, LINE_CLOSE);

}// CTSPILineConnection::ForceClose

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetLineDevCaps
//
// Return the line device capabilities.  Keep in mind that the capabilities
// are not complete here - the optional fields are not filled out until
// an actual call to GetCapabilities is made, and they are not stored in
// our structure (only the callers).
//
TSP_INLINE LPLINEDEVCAPS CTSPILineConnection::GetLineDevCaps()
{
    return &m_LineCaps;

}// CTSPILineConnection::GetLineDevCaps

///////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetLineDevStatus
//
// Return the line device status.  Keep in mind that the status information
// are not complete here - the optional fields are not filled out until
// an actual call to GatherStatus is made, and they are not stored in
// our structure (only the callers).
//
TSP_INLINE LPLINEDEVSTATUS CTSPILineConnection::GetLineDevStatus()
{
    return &m_LineStatus;

}// CTSPILineConnection::GetLineDevStatus;

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetTerminalCount
//
// Return the count of terminals on this line.
//
TSP_INLINE int CTSPILineConnection::GetTerminalCount() const
{
	CEnterCode sLock(this);  // Synch access to object
	return m_arrTerminalInfo.GetSize();
	
}// CTSPILineConnection::GetTerminalCount

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetDefaultMediaDetection
//
// Return the current set of media modes being detected by the SP.
// This is used when a new call is created to setup the initial monitoring
// of media modes.
//
TSP_INLINE DWORD CTSPILineConnection::GetDefaultMediaDetection() const
{                                                
    return m_dwLineMediaModes;
    
}// CTSPILineConnection::GetDefaultMediaDetection

///////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::GetAssociatedPhone
//
// Return the associated phone device with this line.
//
TSP_INLINE CTSPIPhoneConnection* CTSPILineConnection::GetAssociatedPhone() const
{
	DEVICECLASSINFO* pDevInfo = ((CTSPILineConnection*)this)->GetDeviceClass(_T("tapi/phone"));
	if (pDevInfo != NULL)
	{
		DWORD dwPhoneDeviceID = *((LPDWORD)pDevInfo->lpvData);
		return GetDeviceInfo()->FindPhoneConnectionByDeviceID(dwPhoneDeviceID);
	}
	return NULL;

}// CTSPILineConnection::GetAssociatedPhone

////////////////////////////////////////////////////////////////////////////
// CTSPILineConnection::SendDialogInstanceData
//
// Causes TAPI to call the TUISPI_providerGenericDialogData
// function of the UI DLL associated with the htDlgInstance.
//
TSP_INLINE VOID CTSPILineConnection::SendDialogInstanceData (HTAPIDIALOGINSTANCE htDlgInstance, 
					LPVOID lpParams, DWORD dwSize)
{
	Send_TAPI_Event (NULL, LINE_SENDDIALOGINSTANCEDATA, (DWORD)lpParams, 
					 dwSize, (DWORD)htDlgInstance);

}// CTSPILineConnection::SendDialogInstanceData

/******************************************************************************/
//
// CMapDWordToString
//
/******************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::HashKey
//
// Internal helper to create a hash key for a value - uses a
// default identity hash which works for most primitive values.
//
TSP_INLINE UINT CMapDWordToString::HashKey(DWORD key) const
{
	return ((UINT)(void*)key) >> 4;

}// CMapDWordToString::HashKey

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::GetCount
//
// Return the count of elements in our map
//
TSP_INLINE int CMapDWordToString::GetCount() const
{
	return m_nCount;

}// CMapDWordToString::GetCount

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::IsEmpty
//
// Return whether the map is empty (has no elements)
//
TSP_INLINE BOOL CMapDWordToString::IsEmpty() const
{
	return (GetCount() == 0);

}// CMapDWordToString::IsEmpty

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::SetAt
//
// Set a new element into our map
//
TSP_INLINE void CMapDWordToString::SetAt(DWORD key, LPCTSTR newValue)
{
	(*this) [key] = newValue;

}// CMapDWordToString::SetAt

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::GetHashTableSize
//
// Return the hash table size
//
TSP_INLINE UINT CMapDWordToString::GetHashTableSize() const
{
	return m_nHashTableSize;

}// CMapDWordToString::GetHashTableSize

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::GetStartPosition
//
// Find the first element in our list.
//
TSP_INLINE POSITION CMapDWordToString::GetStartPosition() const
{
	if (IsEmpty())
		return NULL;
	return BEFORE_START_POSITION;

}// CMapDWordToString::GetStartPosition

/******************************************************************************/
//
// CTSPIPhoneConnection
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetPhoneHandle
//
// Return the TAPI phone handle associated with this object
//
TSP_INLINE HTAPIPHONE CTSPIPhoneConnection::GetPhoneHandle() const
{                                       
    return m_htPhone;

}// CTSPIPhoneConnection::GetPhoneHandle

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::CanHandleRequest
//
// Determine if our service provider is capable of handling the request.
// 
TSP_INLINE BOOL CTSPIPhoneConnection::CanHandleRequest(WORD wRequest, DWORD dwData)
{
    return GetSP()->CanHandleRequest(this, wRequest, dwData);                

}// CTSPIPhoneConnection::CanHandleRequest

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetButtonCount
//
// Returns the number of button/lamp pairs in our array
//
TSP_INLINE int CTSPIPhoneConnection::GetButtonCount() const
{                      
	CEnterCode sLock(this);  // Synch access to object
    ASSERT (m_PhoneCaps.dwNumButtonLamps == (DWORD) m_arrButtonInfo.GetSize());
    return m_arrButtonInfo.GetSize();

}// CTSPIPhoneConnection::GetButtonCount

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetButtonInfo
//
// Return the button object for the specified button.
//
TSP_INLINE const CPhoneButtonInfo* CTSPIPhoneConnection::GetButtonInfo(int iButtonID) const
{   
	CEnterCode sLock(this);  // Synch access to object
    if (m_arrButtonInfo.IsValidIndex (iButtonID))
        return m_arrButtonInfo.GetAt (iButtonID);
    return NULL;

}// CTSPIPhoneConnection::GetButtonInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetPhoneCaps
//
// Return a pointer to the phone capabilities
//
TSP_INLINE LPPHONECAPS CTSPIPhoneConnection::GetPhoneCaps()
{
    return &m_PhoneCaps;
    
}// CTSPIPhoneConnection::GetPhoneCaps

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetPhoneStatus
//
// Return a pointer to the phone status
//
TSP_INLINE LPPHONESTATUS CTSPIPhoneConnection::GetPhoneStatus()
{                                       
    return &m_PhoneStatus;

}// CTSPIPhoneConnection::GetPhoneStatus

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetRing
//
// Return the ring value for the phone device.
//
TSP_INLINE LONG CTSPIPhoneConnection::GetRing (LPDWORD lpdwRingMode, LPDWORD lpdwVolume) 
{                               
	if ((GetPhoneStatus()->dwPhoneFeatures & PHONEFEATURE_GETRING) == 0)
		return PHONEERR_OPERATIONUNAVAIL;
	
    *lpdwRingMode = m_PhoneStatus.dwRingMode;
    *lpdwVolume = m_PhoneStatus.dwRingVolume;
    return FALSE;

}// CTSPIPhoneConnection::GetRing

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetDisplayBuffer
//
// Return the current display buffer as a string.  Each row is
// spaced out to the full size for our display.
//
TSP_INLINE CString CTSPIPhoneConnection::GetDisplayBuffer() const
{                                         
	CEnterCode sLock(this);  // Synch access to object
    return m_Display.GetTextBuffer();

}// CTSPIPhoneConnection::GetDisplayBuffer

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetCursorPos
//
// Return the cursor position of the display.
//
TSP_INLINE CPoint CTSPIPhoneConnection::GetCursorPos() const
{
	CEnterCode sLock(this);  // Synch access to object
    return m_Display.GetCursorPosition();

}// CTSPIPhoneConnection::GetCursorPos

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddDisplayChar
//
// Add a single character to our display at the specified position.
//
TSP_INLINE VOID CTSPIPhoneConnection::AddDisplayChar (TCHAR cChar)
{                                       
	CEnterCode sLock(this);  // Synch access to object
    m_Display.AddCharacter (cChar);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::AddDisplayChar

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetDisplayChar
//
// Set a character at a specified position in the buffer to a vlue.
//
TSP_INLINE VOID CTSPIPhoneConnection::SetDisplayChar (int iColumn, int iRow, TCHAR cChar)
{                                       
	CEnterCode sLock(this);  // Synch access to object
    m_Display.SetCharacterAtPosition (iColumn, iRow, cChar);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::SetDisplayChar

////////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::ForceClose
//
// This method may be used by the service provider to forcibly close the
// phone object.
//
TSP_INLINE void CTSPIPhoneConnection::ForceClose()
{
	Send_TAPI_Event (PHONE_CLOSE);

}// CTSPIPhoneConnection::ForceClose

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::ResetDisplay
//
// Reset the display (cursor, buffer)
//
TSP_INLINE VOID CTSPIPhoneConnection::ResetDisplay()
{                                       
	CEnterCode sLock(this);  // Synch access to object
    m_Display.Reset();
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::ResetDisplay()

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::SetDisplayCursorPos
//
// Set the current cursor position for the display.
//
TSP_INLINE VOID CTSPIPhoneConnection::SetDisplayCursorPos (int iColumn, int iRow)
{                                            
	CEnterCode sLock(this);  // Synch access to object
    m_Display.SetCursorPosition (iColumn, iRow);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::SetDisplayCursorPos

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::ClearDisplayLine
//
// Clear a single display line
//
TSP_INLINE VOID CTSPIPhoneConnection::ClearDisplayLine (int iRow)
{                                         
	CEnterCode sLock(this);  // Synch access to object
    m_Display.ClearRow (iRow);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);

}// CTSPIPhoneConnection::ClearDisplayLine

///////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::AddDisplayString 
//
// Add a string to the display.
//
TSP_INLINE VOID CTSPIPhoneConnection::AddDisplayString (LPCTSTR lpszText)
{                                          
	CEnterCode sLock(this);  // Synch access to object
    m_Display.AddString (lpszText);
    OnPhoneStatusChange (PHONESTATE_DISPLAY);
    
}// CTSPIPhoneConnection::AddDisplayString 

///////////////////////////////////////////////////////////////////////////////
// CTSPIPhoneConnection::GetAssociatedLine
//
// Return the associated line device with this phone.
//
TSP_INLINE CTSPILineConnection* CTSPIPhoneConnection::GetAssociatedLine() const
{
	DEVICECLASSINFO* pDevInfo = ((CTSPIPhoneConnection*)this)->GetDeviceClass(_T("tapi/line"));
	if (pDevInfo != NULL)
	{
		DWORD dwLineDeviceID = *((LPDWORD)pDevInfo->lpvData);
		return GetDeviceInfo()->FindLineConnectionByDeviceID(dwLineDeviceID);
	}
	return NULL;

}// CTSPIPhoneConnection::GetAssociatedLine

/******************************************************************************/
//
// CTSPIRequest
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetCommand
//
// Set the command for this request object.  This is useful when
// a packet is transitioned through two commands (like MakeCall to Dial).
//
TSP_INLINE VOID CTSPIRequest::SetCommand(WORD wCommand)
{
    m_wReqType = wCommand;
    SetState (STATE_INITIAL);

}// CTSPIRequest::SetCommand

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetCommand
//
// Return the command request for this packet
//
TSP_INLINE WORD CTSPIRequest::GetCommand() const
{
    return m_wReqType;

}// CTSPIRequest::GetCommand

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetAsynchRequestId
//
// Return the TAPI asynchronous request id associated with this
// command.
//
TSP_INLINE DRV_REQUESTID CTSPIRequest::GetAsynchRequestId() const 
{
    return m_dwRequestId;

}// CTSPIRequest::GetAsynchRequestId

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetDataPtr
//
// Return the optional data associated with this request.
//
TSP_INLINE LPVOID CTSPIRequest::GetDataPtr() const
{ 
    return m_lpData;

}// CTSPIRequest::GetDataPtr

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetDataSize
//
// Return the size of the data block in our data pointer
//
TSP_INLINE DWORD CTSPIRequest::GetDataSize() const 
{
    return m_dwSize;

}// CTSPIRequest::GetDataSize

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetState
//
// Set the current state of this request.
//
TSP_INLINE VOID CTSPIRequest::SetState(int iState) 
{ 
    m_iReqState = iState;

}// CTSPIRequest::SetState

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetState
//
// Get the state of this request
//
TSP_INLINE int CTSPIRequest::GetState() const 
{
    return m_iReqState;

}// CTSPIRequest::GetState

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetStateData
//
// Set the state data for this request
//
TSP_INLINE VOID CTSPIRequest::SetStateData(DWORD dwData)
{
    m_dwAppData = dwData;

}// CTSPIRequest::SetStateData

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetDataPtr
//
// Set the state data pointer for this request
//
TSP_INLINE VOID CTSPIRequest::SetDataPtr(LPVOID lpBuff)
{
	m_lpData = lpBuff;

}// CTSPIRequest::SetDataPtr

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetDataSize
//
// Set the state data size for this request
//
TSP_INLINE VOID CTSPIRequest::SetDataSize(DWORD dwSize)
{
	m_dwSize = dwSize;

}// CTSPIRequest::SetDataSize

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetStateData
//
// Return the state data for this request (USER-DEFINED)
//
TSP_INLINE DWORD CTSPIRequest::GetStateData() const
{
    return m_dwAppData;

}// CTSPIRequest::GetStateData

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::HaveSentResponse
//
// Returns BOOL indicating whether a response has been sent to TAPI
// about this request.  If this has no request id, then always return
// as if we sent a response.
//
TSP_INLINE BOOL CTSPIRequest::HaveSentResponse() const 
{ 
    return (GetAsynchRequestId() > 0) ? m_fResponseSent : TRUE;

}// CTSPIRequest::HaveSentResponse

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetConnectionInfo
//
// Return the CTSPIConnection pointer this packet relates to
//
TSP_INLINE CTSPIConnection* CTSPIRequest::GetConnectionInfo() const
{ 
    return m_pConnOwner;

}// CTSPIRequest::GetConnectionInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetAddressInfo
//
// Return the address information for a request packet.
//
TSP_INLINE CTSPIAddressInfo* CTSPIRequest::GetAddressInfo() const
{                               
    return m_pAddress;
    
}// CTSPIRequest::GetAddressInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::GetCallInfo
//
// Return the CTSPICallAppearance associated with this request.
//
TSP_INLINE CTSPICallAppearance* CTSPIRequest::GetCallInfo() const
{ 
    return m_pCall;

}// CTSPIRequest::GetCallInfo

///////////////////////////////////////////////////////////////////////////
// CTSPIRequest::SetCallInfo
//
// Return the CTSPICallAppearance associated with this request.
//
TSP_INLINE void CTSPIRequest::SetCallInfo(CTSPICallAppearance* pCall)
{ 
	// Decrement the previous call.
	if (m_pCall != pCall)
	{
		if (m_pCall != NULL)
			m_pCall->DecRefCount();

		m_pCall = pCall;
		if (pCall != NULL)
		{
			m_pCall->IncRefCount();
			m_pAddress = pCall->GetAddressOwner();
			m_pConnOwner = pCall->GetLineOwner();
		}
	}

}// CTSPIRequest::SetCallInfo

/******************************************************************************/
//
// CServiceProvider object
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetSupportedVersion
//
// Return the highest level of TAPI support this service provider
// has decided to conform to.
//
TSP_INLINE DWORD CServiceProvider::GetSupportedVersion() const
{                               
    return m_dwTapiVerSupported;

}// CServiceProvider::GetSupportedVersion

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetSystemVersion
//
// Return the version which TAPI.DLL is at on this computer.
//
TSP_INLINE DWORD CServiceProvider::GetSystemVersion() const
{                                     
    return m_dwTAPIVersionFound;

}// CServiceProvider::GetSystemVersion

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetCurrentLocation
//
// Return the current location set by TAPI and adjusted by the 
// dial property page.
//
TSP_INLINE DWORD CServiceProvider::GetCurrentLocation() const
{
    return m_dwCurrentLocation;
    
}// CServiceProvider::GetCurrentLocation

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetProviderName
//
// Return string name (passed to CWinApp constructor)
//
TSP_INLINE LPCTSTR CServiceProvider::GetProviderName() const
{                                              
    return m_pszAppName;

}// CServiceProvider::GetProviderName
                     
/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetProviderInfo
//
// Return the provider information
//
TSP_INLINE LPCTSTR CServiceProvider::GetProviderInfo() const
{
    return m_pszProviderInfo;

}// CServiceProvider::GetProviderInfo

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIRequestObj
//
// Return the TSPI request object dynamic creation descriptor
//
TSP_INLINE CRuntimeClass* CServiceProvider::GetTSPIRequestObj() const
{
    ASSERT (m_pRequestObj != NULL);
    return m_pRequestObj;

}// CServiceProvider::GetTSPIRequestObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPILineObj
//
// Return the TSPI line object dynamic creation descriptor
//
TSP_INLINE CRuntimeClass* CServiceProvider::GetTSPILineObj() const
{
    ASSERT (m_pLineObj != NULL);
    return m_pLineObj;

}// CServiceProvider::GetTSPILineObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIPhoneObj
//
// Return the TSPI phone object dynamic creation descriptor
//
TSP_INLINE CRuntimeClass* CServiceProvider::GetTSPIPhoneObj() const
{
    ASSERT (m_pPhoneObj != NULL);
    return m_pPhoneObj;

}// CServiceProvider::GetTSPIPhoneObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIDeviceObj
//
// Return the TSPI device object dynamic creation descriptor
//
TSP_INLINE CRuntimeClass* CServiceProvider::GetTSPIDeviceObj() const
{
    ASSERT (m_pDeviceObj != NULL);
    return m_pDeviceObj;

}// CServiceProvider::GetTSPIDeviceObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPICallObj
//
// Return the TSPI call object dynamic creation descriptor
//
TSP_INLINE CRuntimeClass* CServiceProvider::GetTSPICallObj() const
{
    ASSERT (m_pCallObj != NULL);
    return m_pCallObj;

}// CServiceProvider::GetTSPICallObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIConferenceCallObj
//
// Return the TSPI conference call object dynamic creation descriptor
//
TSP_INLINE CRuntimeClass* CServiceProvider::GetTSPIConferenceCallObj() const
{
    ASSERT (m_pConfCallObj != NULL);
    return m_pConfCallObj;

}// CServiceProvider::GetTSPIConferenceCallObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTSPIAddressObj
//
// Return the TSPI address object dynamic creation descriptor
//
TSP_INLINE CRuntimeClass* CServiceProvider::GetTSPIAddressObj() const
{
    ASSERT (m_pAddrObj != NULL);
    return m_pAddrObj;

}// CServiceProvider::GetTSPIAddressObj

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetLineCreateProc
//
// Return the line creation procedure if available.
//
// WARNING: This may be NULL!
//
TSP_INLINE LINEEVENT CServiceProvider::GetLineCreateProc() const
{
    return m_lpfnLineCreateProc;

}// CServiceProvider::GetLineCreateProc

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetPhoneCreateProc
//
// Return the phone creation procedure if available.
//
// WARNING: This may be NULL!
//
TSP_INLINE PHONEEVENT CServiceProvider::GetPhoneCreateProc() const
{
    return m_lpfnPhoneCreateProc;

}// CServiceProvider::GetPhoneCreateProc

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetTimeout
//
// Return the standard timeout value used for the service provider.
//
TSP_INLINE LONG CServiceProvider::GetTimeout() const
{
    return m_lTimeout;

}// CServiceProvider::GetTimeout

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::SetTimeout
//
// This method sets the default timeout used throughout the library.
//
TSP_INLINE VOID CServiceProvider::SetTimeout (LONG lTimeout)
{
    // We should always wait at LEAST 1/2 sec!
    ASSERT (lTimeout > 500);
    m_lTimeout = lTimeout;

}// CServiceProvider::SetTimeout

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetConnInfoFromLineDeviceID
//
// Return the CTSPIConnection object from a line device id.
//
TSP_INLINE CTSPILineConnection* CServiceProvider::GetConnInfoFromLineDeviceID (DWORD dwDevId)
{
    return (CTSPILineConnection*) SearchForConnInfo(dwDevId, 0);

}// CServiceProvider::GetConnInfoFromLineDeviceID

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetConnInfoFromPhoneDeviceID
//
// Return the CTSPIConnection object from a phone device id.
//
TSP_INLINE CTSPIPhoneConnection* CServiceProvider::GetConnInfoFromPhoneDeviceID (DWORD dwDevId) 
{
    return (CTSPIPhoneConnection*) SearchForConnInfo(dwDevId, 1);

}// CServiceProvider::GetConnInfoFromPhoneDeviceID

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetDeviceCount
//
// Return the count of devices.
//
TSP_INLINE DWORD CServiceProvider::GetDeviceCount() const
{
    return m_arrDevices.GetSize();

}// CServiceProvider::GetDeviceCount

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ProcessCallParameters
//
// This function is used to determine if the passed call parameters
// are valid for the device they are being used for.
//
TSP_INLINE LONG CServiceProvider::ProcessCallParameters (CTSPIAddressInfo* pAddr, LPLINECALLPARAMS lpCallParams)
{
    return ProcessCallParameters (pAddr->GetLineOwner(), lpCallParams);

}// CServiceProvider::ProcessCallParameters

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::ProcessCallParameters
//
// This function is used to determine if the passed call parameters
// are valid for the device they are being used for.
//
TSP_INLINE LONG CServiceProvider::ProcessCallParameters (CTSPICallAppearance* pCall, LPLINECALLPARAMS lpCallParams)
{   
    return ProcessCallParameters (pCall->GetLineOwner(), lpCallParams);

}// CServiceProvider::ProcessCallParameters

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::CanHandleRequest
//
// This function is used to dynamically determine what the capabilities
// of our service provider really is.
//
TSP_INLINE BOOL CServiceProvider::CanHandleRequest (CTSPIConnection* pConn, WORD wRequest, DWORD dwData)
{
    return CanHandleRequest (pConn, (CTSPIAddressInfo*)NULL, 
                             (CTSPICallAppearance*)NULL, wRequest, dwData);

}// CServiceProvider::CanHandleRequest

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::CanHandleRequest
//
// This function is used to dynamically determine what the capabilities
// of our service provider really is.
//
TSP_INLINE BOOL CServiceProvider::CanHandleRequest (CTSPIAddressInfo* pAddr, WORD wRequest, DWORD dwData)
{
    return CanHandleRequest (pAddr->GetLineOwner(), pAddr, (CTSPICallAppearance*)NULL, wRequest, dwData);

}// CServiceProvider::CanHandleRequest

////////////////////////////////////////////////////////////////////////////
// CServiceProvider::CanHandleRequest
//
// This function is used to dynamically determine what the capabilities
// of our service provider really is.
//
TSP_INLINE BOOL CServiceProvider::CanHandleRequest (CTSPICallAppearance* pCall, WORD wRequest, DWORD dwData)
{
    return CanHandleRequest (pCall->GetLineOwner(),
                             pCall->GetAddressOwner(), pCall, wRequest, dwData);

}// CServiceProvider::CanHandleRequest

/******************************************************************************/
//
// MISC. objects
//
/******************************************************************************/

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::CTSPIBaseObject
//
// Constructor
//
TSP_INLINE CTSPIBaseObject::CTSPIBaseObject() :
	m_dwItemData(0)
{
}// CTSPIBaseObject::GetSyncObject

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::GetSyncObject
//
// Return the synchronization object
//
TSP_INLINE CIntCriticalSection* CTSPIBaseObject::GetSyncObject() const
{
	CTSPIBaseObject* pThis = (CTSPIBaseObject*)this;
	return &pThis->m_csSection;

}// CTSPIBaseObject::GetSyncObject

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::GetItemData
//
// Return the DWORD item data setup by the developer
//
TSP_INLINE DWORD CTSPIBaseObject::GetItemData() const
{
	return m_dwItemData;

}// CTSPIBaseObject::GetItemData

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::GetItemDataPtr
//
// Return the DWORD item data setup by the developer
//
TSP_INLINE void* CTSPIBaseObject::GetItemDataPtr() const
{
	return (void*) m_dwItemData;

}// CTSPIBaseObject::GetItemDataPtr

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::SetItemData
//
// Set the DWORD item data 
//
TSP_INLINE void CTSPIBaseObject::SetItemData(DWORD dwItem)
{
	m_dwItemData = dwItem;

}// CTSPIBaseObject::SetItemData

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::SetItemDataPtr
//
// Set the DWORD item data
//
TSP_INLINE void CTSPIBaseObject::SetItemDataPtr(void* pItem)
{
	m_dwItemData = (DWORD) pItem;

}// CTSPIBaseObject::SetItemDataPtr

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// USERUSERINFO
// 
// User to User information structure
// 
TSP_INLINE USERUSERINFO::USERUSERINFO(LPVOID lpBuff, DWORD dwInSize) :
	dwSize(dwInSize)
{
	lpvBuff = AllocMem(dwSize);
	CopyBuffer(lpvBuff, lpBuff, dwSize);

}// USERUSERINFO::USERUSERINFO

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// ~USERUSERINFO
// 
// User to User information structure
// 
TSP_INLINE USERUSERINFO::~USERUSERINFO()
{
	FreeMem(lpvBuff);

}// USERUSERINFO::USERUSERINFO

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEFORWARD::TSPILINEFORWARD
//
// Constructor for the LINEFORWARD object request
//
TSP_INLINE TSPILINEFORWARD::TSPILINEFORWARD() :
	dwNumRings(0), pCall(NULL), lpCallParams(NULL)
{                                   
}// TSPILINEFORWARD::TSPILINEFORWARD

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIFORWARDINFO::TSPIFORWARDINFO
//
// Constructor for the TSPIFORWARDINFO object request
//
TSP_INLINE TSPIFORWARDINFO::TSPIFORWARDINFO() :
	dwForwardMode(0), dwDestCountry(0), dwRefCount(1)
{                                   
}// TSPIFORWARDINFO::TSPIFORWARDINFO

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIFORWARDINFO::IncUsage
//
// Increment the reference counter
//
TSP_INLINE VOID TSPIFORWARDINFO::IncUsage()
{                               
    InterlockedIncrement((LONG*)&dwRefCount);
    
}// TSPIFORWARDINFO::IncUsage

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIFORWARDINFO::DecUsage
//
// Increment the reference counter
//
TSP_INLINE VOID TSPIFORWARDINFO::DecUsage()
{                               
    if (InterlockedDecrement((LONG*)&dwRefCount) <= 0)
        delete this;
    
}// TSPIFORWARDINFO::DecUsage

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICALLPARAMS::TSPICALLPARAMS
//
// Constructor for the TSPICALLPARAMS object request
//
TSP_INLINE TSPICALLPARAMS::TSPICALLPARAMS() :
	dwBearerMode(0), dwMinRate(0), dwMaxRate(0)
{                                 
    FillBuffer (&DialParams, 0, sizeof(LINEDIALPARAMS));

}// TSPICALLPARAMS::TSPICALLPARAMS

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEPARK::TSPILINEPARK
//
// Constructor for the TSPILINEPARK object request
//
TSP_INLINE TSPILINEPARK::TSPILINEPARK() :
	dwParkMode(0), lpNonDirAddress(NULL)
{                             
}// TSPILINEPARK::TSPILINEPARK              

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEPICKUP::TSPILINEPICKUP
//
// Constructor for the TSPILINEPICKUP object request
//
TSP_INLINE TSPILINEPICKUP::TSPILINEPICKUP()
{                                 
}// TSPILINEPICKUP::TSPILINEPICKUP              

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITRANSFER::TSPITRANSFER
//
// Constructor for the TSPITRANSFER object request
//
TSP_INLINE TSPITRANSFER::TSPITRANSFER() :
	pCall(NULL), pConsult(NULL), pConf(NULL),
	dwTransferMode(0), lpCallParams(NULL)
{                             
}// TSPITRANSFER::TSPITRANSFER

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITRANSFER::~TSPITRANSFER
//
// Destructor for the TSPITRANSFER object request
//
TSP_INLINE TSPITRANSFER::~TSPITRANSFER()
{                             
	if (lpCallParams)
		FreeMem (lpCallParams);

}// TSPITRANSFER::~TSPITRANSFER

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICONFERENCE::TSPICONFERENCE
//
// Constructor for the TSPICONFERENCE object request
//
TSP_INLINE TSPICONFERENCE::TSPICONFERENCE() :
	pConfCall(NULL), pCall(NULL), pConsult(NULL),
	dwPartyCount(0), lpCallParams(NULL)
{                                 
}// TSPICONFERENCE::TSPICONFERENCE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIDIGITGATHER::TSPIDIGITGATHER
//
// Constructor for the TSPIDIGITGATHER object request
//
TSP_INLINE TSPIDIGITGATHER::TSPIDIGITGATHER() :
	dwEndToEndID(0), dwDigitModes(0), lpBuffer(NULL),
	dwSize(0), dwCount(0), dwFirstDigitTimeout(0),
	dwInterDigitTimeout(0), dwLastTime(0)
{                                   
}// TSPIDIGITGATHER::TSPIDIGITGATHER

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIGENERATE::TSPIGENERATE
//
// Constructor for the TSPIGENERATE object request
//
TSP_INLINE TSPIGENERATE::TSPIGENERATE() :
	dwEndToEndID(0), dwMode(0), dwDuration(0)
{                             
}// TSPIGENERATE::TSPIGENERATE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICOMPLETECALL::TSPICOMPLETECALL
//
// Constructor for the TSPICOMPLETECALL object request
//
TSP_INLINE TSPICOMPLETECALL::TSPICOMPLETECALL() :
	dwCompletionMode(0), dwMessageId(0)
{                                     
}// TSPICOMPLETECALL::TSPICOMPLETECALL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICOMPLETECALL::TSPICOMPLETECALL
// 
// Copy constructor
//
TSP_INLINE TSPICOMPLETECALL::TSPICOMPLETECALL(const TSPICOMPLETECALL* pComplete)
{
	dwCompletionID = pComplete->dwCompletionID;
    dwCompletionMode = pComplete->dwCompletionMode;
    dwMessageId = pComplete->dwMessageId;
    pCall = pComplete->pCall;
    dwSwitchInfo = pComplete->dwSwitchInfo;
    strSwitchInfo = pComplete->strSwitchInfo;
    
}// TSPICOMPLETECALL::TSPICOMPLETECALL
   
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMAKECALL::TSPIMAKECALL
//
// Constructor for the TSPIMAKECALL object request
//
TSP_INLINE TSPIMAKECALL::TSPIMAKECALL() :
	dwCountryCode(0), lpCallParams(NULL)
{                             
}// TSPIMAKECALL::TSPIMAKECALL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITONEMONITOR::TSPITONEMONITOR
//
// Constructor for the tone monitor request object
//
TSP_INLINE TSPITONEMONITOR::TSPITONEMONITOR() :
	dwToneListID(0)
{
}// TSPITONEMONITOR::TSPITONEMONITOR

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINESETTERMINAL::TSPILINESETTERMINAL
//
// Constructor for the TSPILINESETTERMINAL object request
//
TSP_INLINE TSPILINESETTERMINAL::TSPILINESETTERMINAL() :
	pLine(NULL), pAddress(NULL), pCall(NULL), dwTerminalModes(0),
	dwTerminalID(0), bEnable(FALSE)
{                                           
}// TSPILINESETTERMINAL::TSPILINESETTERMINAL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::TSPIMEDIACONTROL
//
// Constructor for the TSPIMEDIACONTROL object request
//
TSP_INLINE TSPIMEDIACONTROL::TSPIMEDIACONTROL() :
	dwRefCount(0)
{                                     
}// TSPIMEDIACONTROL::TSPIMEDIACONTROL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::IncUsage
//
// Increment our usage count
//
TSP_INLINE VOID TSPIMEDIACONTROL::IncUsage()
{                             
    InterlockedIncrement((LONG*)&dwRefCount);

}// TSPIMEDIACONTROL::IncUsage

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::DecUsage
//
// Decrement our usage count and delete when it hits zero
//
// WARNING: THIS OBJECT MUST BE ALLOCATED WITH "new"!!!
//
TSP_INLINE VOID TSPIMEDIACONTROL::DecUsage()
{                             
    if (InterlockedDecrement((LONG*)&dwRefCount))
        delete this;

}// TSPIMEDIACONTROL::DecUsage
  
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIHOOKSWITCHPARAM::TSPIHOOKSWITCHPARAM
//
// Constructor for the TSPIHOOKSWITCHPARAM object request
//
TSP_INLINE TSPIHOOKSWITCHPARAM::TSPIHOOKSWITCHPARAM() :
	dwHookSwitchDevice(0), dwParam(0)
{                                           
}// TSPIHOOKSWITCHPARAM::TSPIHOOKSWITCHPARAM

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIRINGPATTERN::TSPIRINGPATTERN
//
// Constructor for the TSPIRINGPATTERN object request
//
TSP_INLINE TSPIRINGPATTERN::TSPIRINGPATTERN() :
	dwRingMode(0), dwVolume(0)
{                                   
}// TSPIRINGPATTERN::TSPIRINGPATTERN

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPISETBUTTONINFO::TSPISETBUTTONINFO
//
// Constructor for the TSPISETBUTTONINFO object request
//
TSP_INLINE TSPISETBUTTONINFO::TSPISETBUTTONINFO() :
	dwButtonLampId(0), dwFunction(0), dwMode(0)
{   
}// TSPISETBUTTONINFO::TSPISETBUTTONINFO

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONESETDISPLAY::TSPIPHONESETDISPLAY
//
// Constructor for the TSPIPHONESETDISPLAY object request
//
TSP_INLINE TSPIPHONESETDISPLAY::TSPIPHONESETDISPLAY() :
	dwRow(0), dwColumn(0), lpvDisplay(NULL), dwSize(0)
{                                           
}// TSPIPHONESETDISPLAY::TSPIPHONESETDISPLAY
                      
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONEDATA::TSPIPHONEDATA
//
// Constructor for the TSPIPHONEDATA object request
//
TSP_INLINE TSPIPHONEDATA::TSPIPHONEDATA() :
	dwDataID(0), lpBuffer(NULL), dwSize(0)
{                               
}// TSPIPHONEDATA::TSPIPHONEDATA

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::CFlagArray
//
// Flag array constructor
//
TSP_INLINE CFlagArray::CFlagArray()
{ 
    /* Do nothing*/
    
}// CFlagArray::CFlagArray

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::operator[]
//
// Retrieve a bit from a specified position.
//
TSP_INLINE BOOL CFlagArray::operator[] (int nIndex) const
{
    return GetAt (nIndex);
    
}// CFlagArray::operator[]

#endif // _SPLIB_INL_INC_