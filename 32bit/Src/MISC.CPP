/******************************************************************************/
//                                                                        
// MISC.CPP - Source file for the misc. class functions
//                                             
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//
// This file implements all the request object constructors and
// the various support classes required in the library which are
// considered internal to the spLIB++ product.
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

IMPLEMENT_DYNCREATE (CTSPIBaseObject, CObject)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CADObArray::~CADObArray
//
// Destructor for the auto-delete object array
//
CADObArray::~CADObArray()
{                          
    for (int i = 0; i < GetSize(); i++)
    {
        CObject* pObj = GetAt(i);
        delete pObj;
    }
    
}// CADObArray::~CADObArray

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEFORWARD::~TSPILINEFORWARD
//
// Destructor for the LINEFORWARD object request
//
TSPILINEFORWARD::~TSPILINEFORWARD()
{                                    
    for (int i = 0; i < arrForwardInfo.GetSize(); i++)
    {
        TSPIFORWARDINFO* lpInfo = (TSPIFORWARDINFO*) arrForwardInfo[i];
        lpInfo->DecUsage();
    }
    FreeMem ((LPTSTR)lpCallParams);

}// TSPILINEFORWARD::~TSPILINEFORWARD

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIGENERATE::~TSPIGENERATE
//
// Constructor for the TSPIGENERATE object request
//
TSPIGENERATE::~TSPIGENERATE()
{                             
    for (int i = 0; i < arrTones.GetSize(); i++)
    {                          
        LPLINEGENERATETONE lpTone = (LPLINEGENERATETONE) arrTones[i];
        delete lpTone;
    }
    
}// TSPIGENERATE::~TSPIGENERATE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITONEMONITOR::~TSPITONEMONITOR
//
// Destructor for the tone monitor request object
//
TSPITONEMONITOR::~TSPITONEMONITOR()
{
    for (int i = 0; i < arrTones.GetSize(); i++)
    {
        LPLINEMONITORTONE lpTone = (LPLINEMONITORTONE) arrTones[i];
        delete lpTone;
    }
    arrTones.RemoveAll();
    
}// TSPITONEMONITOR::~TSPITONEMONITOR

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::~TSPIMEDIACONTROL
//
// Destructor for the TSPIMEDIACONTROL object request
//
TSPIMEDIACONTROL::~TSPIMEDIACONTROL()
{   
    for (int i = 0; i < arrDigits.GetSize(); i++)
    {
        LPLINEMEDIACONTROLDIGIT lpMedia = (LPLINEMEDIACONTROLDIGIT) arrDigits[i];
        delete lpMedia;
    }

    for (i = 0; i < arrMedia.GetSize(); i++)
    {
        LPLINEMEDIACONTROLMEDIA lpMedia = (LPLINEMEDIACONTROLMEDIA) arrMedia[i];
        delete lpMedia;
    }

    for (i = 0; i < arrTones.GetSize(); i++)
    {
        LPLINEMEDIACONTROLTONE lpMedia = (LPLINEMEDIACONTROLTONE) arrTones[i];
        delete lpMedia;
    }

    for (i = 0; i < arrCallStates.GetSize(); i++)
    {
        LPLINEMEDIACONTROLCALLSTATE lpMedia = (LPLINEMEDIACONTROLCALLSTATE) arrCallStates[i];
        delete lpMedia;
    }
                                  
}// TSPIMEDIACONTROL::~TSPIMEDIACONTROL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICONFERENCE::~TSPICONFERENCE
//
// Destructor for the TSPICONFERENCE object request
//
TSPICONFERENCE::~TSPICONFERENCE()
{                                 
	if (lpCallParams)
		FreeMem (lpCallParams);

}// TSPICONFERENCE::~TSPICONFERENCE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMAKECALL::~TSPIMAKECALL
//
// Destructor for the TSPIMAKECALL object request
//
TSPIMAKECALL::~TSPIMAKECALL()
{                             
	if (lpCallParams)
		FreeMem (lpCallParams);
    
}// TSPIMAKECALL::~TSPIMAKECALL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONESETDISPLAY::~TSPIPHONESETDISPLAY
//
// Destructor for the TSPIPHONESETDISPLAY object request
//
TSPIPHONESETDISPLAY::~TSPIPHONESETDISPLAY()
{            
	if (lpvDisplay)
		FreeMem (lpvDisplay);

}// TSPIPHONESETDISPLAY::~TSPIPHONESETDISPLAY

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONEDATA::~TSPIPHONEDATA
//
// Destructor for the TSPIPHONEDATA object request
//
TSPIPHONEDATA::~TSPIPHONEDATA()
{                                
    if (lpBuffer)
        FreeMem (lpBuffer);

}// TSPIPHONEDATA::~TSPIPHONEDATA

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICALLDATA::~TSPICALLDATA
// 
// Delete our call data
//
TSPICALLDATA::~TSPICALLDATA()
{
	if (lpvCallData)
		FreeMem (lpvCallData);

}// TSPICALLDATA::~TSPICALLDATA

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIQOS::~TSPIQOS
//
// Quality of Service object destructor
//
TSPIQOS::~TSPIQOS()
{
	if (lpvSendingFlowSpec)
		FreeMem (lpvSendingFlowSpec);
	if (lpvReceivingFlowSpec)
		FreeMem (lpvReceivingFlowSpec);

}// TSPIQOS::~TSPIQOS

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::~CFlagArray
//
// Destructor for the flag array
//
CFlagArray::~CFlagArray()
{
	m_arrValues.RemoveAll();

}// CFlagArray::~CFlagArray

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::GetAt
//
// Retrieve a bit from a specified position.
//
BOOL CFlagArray::GetAt (int nIndex) const
{    
    int nByteIndex = nIndex / 8;          
    nIndex -= (nByteIndex*8);
	
	if (nByteIndex >= m_arrValues.GetSize())
		return FALSE;

    BYTE bValue = m_arrValues[nByteIndex];
    return (BOOL) (bValue>>nIndex) & 0x1;
    
}// CFlagArray::GetAt

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::SetAt
//
// Set a bit in a particular position - add allocation bytes to the array
// if necessary.
//
void CFlagArray::SetAt (int nIndex, BOOL fFlag)
{
    int nByteIndex = nIndex / 8;          
    nIndex -= (nByteIndex*8);
    BYTE bValue = 0;
    
    if (nByteIndex < m_arrValues.GetSize())
        bValue = m_arrValues[nByteIndex];
    else
        m_arrValues.SetAtGrow (nByteIndex, 0);
        
    BYTE bMask = 0;     
    if (nIndex > 0)
        bMask |= (1 << nIndex);
    else
        bMask = 1;
    if (fFlag)
        bValue |= bMask;
    else
        bValue &= ~bMask;
    m_arrValues.SetAt (nByteIndex, bValue);                
    
}// CFlagArray::SetAt

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICECLASSINFO::DEVICECLASSINFO
//
// Constructor for the device class information structure
//
DEVICECLASSINFO::DEVICECLASSINFO (LPCTSTR pszName, DWORD dwFormat, LPVOID lpData, 
		  DWORD dwDataSize, HANDLE htHandle)
{
	strName = pszName;
	dwStringFormat = dwFormat;
	hHandle = htHandle;
	lpvData = NULL;
	dwSize = 0L;
	
	if (dwDataSize > 0)
	{
		if (dwFormat == STRINGFORMAT_ASCII)
		{
#ifdef _UNICODE
			int iSize = WideCharToMultiByte (CP_ACP, 0, (LPCWSTR)lpData, dwDataSize, NULL, 0, NULL, NULL);
			if (iSize > 0)
			{
				dwSize = iSize+1;
				lpvData = AllocMem (dwSize);
				WideCharToMultiByte (CP_ACP, 0, (LPCWSTR)lpData, dwDataSize, (LPSTR)lpvData, iSize+1, NULL, NULL);
			}
#else
			dwSize = (lstrlen((LPCTSTR)lpData)+1) * sizeof(TCHAR);
			lpvData = AllocMem (dwSize);
			_tcscpy ((LPTSTR)lpvData, (LPCTSTR)lpData);
#endif
		}
		else if (dwFormat == STRINGFORMAT_UNICODE)
		{
#ifndef _UNICODE
			int iSize = MultiByteToWideChar (CP_ACP, 0, (LPCSTR)lpData, dwDataSize, NULL, 0);
			if (iSize > 0)
			{
				dwSize = iSize*sizeof(WCHAR);
				lpvData = AllocMem (dwSize);
				MultiByteToWideChar (CP_ACP, 0, (LPCSTR)lpData, dwDataSize, (LPWSTR)lpvData, iSize);
			}
#else
			dwSize = (lstrlen((LPCTSTR)lpData)+1) * sizeof(TCHAR);
			lpvData = AllocMem (dwSize);
			_tcscpy ((LPTSTR)lpvData, (LPCTSTR)lpData);
#endif
		}
		else
		{
			lpvData = (LPVOID) AllocMem (dwDataSize);
			CopyBuffer((LPSTR)lpvData, (LPCSTR)lpData, dwDataSize);
			dwSize = dwDataSize;
		}
	}

}// DEVICECLASSINFO::DEVICECLASSINFO

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEVICECLASSINFO::~DEVICECLASSINFO
//
// Destructor for the device class information structure
//
DEVICECLASSINFO::~DEVICECLASSINFO()
{
	if (lpvData)
		FreeMem (lpvData);

}// DEVICECLASSINFO::~DEVICECLASSINFO

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CIntCriticalSection::CIntCriticalSection
//
// Constructor for the critical section object
//
CIntCriticalSection::CIntCriticalSection() : CSyncObject(NULL)
{
   m_lLockCount = -1;
   m_dwThreadId = 0;
   m_lInThreadCount = 0;

}// CIntCriticalSection::CIntCriticalSection

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CIntCriticalSection::Lock
//
// Lock method for the critical section
//
BOOL CIntCriticalSection::Lock(DWORD dwTimeout)
{
	DWORD dwThreadId = GetCurrentThreadId();

	// Increment the lock count.  If it is zero, we own it.
	if (InterlockedIncrement(&m_lLockCount) == 0)
	{
		m_dwThreadId = dwThreadId;
		m_lInThreadCount = 1;
	} 
	else
	{
	   // If we already own the critical section..
		if (m_dwThreadId == dwThreadId)
			++m_lInThreadCount;

		// Otherwise, another thread owns it, wait.
		else 
		{
			// If it times-out, error out.
			if (WaitForSingleObject(m_evtLock, dwTimeout) == WAIT_TIMEOUT)
				return FALSE;
			m_dwThreadId = dwThreadId;
			m_lInThreadCount = 1;
		}
	}
	return TRUE;

}// CIntCriticalSection::Lock

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// CIntCriticalSection::Unlock
//
// Unlock method for the critical section
//
BOOL CIntCriticalSection::Unlock()
{
	// If our thread entered the critical section more than once,
	// then require an unlock for *each* thread entry.
	if (--m_lInThreadCount > 0)
		InterlockedDecrement(&m_lLockCount);

	// Otherwise, decrement the main lock count
	else
	{
		m_dwThreadId = 0;

		// If the lock count is NOT zero, then unblock any
		// thread waiting.  The event is a PULSED event, so only
		// one thread will be tagged for execution.
		if (InterlockedDecrement(&m_lLockCount) >= 0) 
			m_evtLock.SetEvent();
	}

	return TRUE;

}// CIntCriticalSection::Unlock
