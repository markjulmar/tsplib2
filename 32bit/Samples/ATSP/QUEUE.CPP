/******************************************************************************/
//                                                                        
// QUEUE.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains the code for I/O queue thread.
// 
// This service provider drives a Hayes compatible AT style modem.  It
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
#include "resource.h"
#include "atsp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////
// CommThreadR
//
// Communications thread manager.
//
UINT CommThreadR (CIOQueue* pQueue)
{
	pQueue->ReaderThread();
	return FALSE;

}// CommThreadR

//////////////////////////////////////////////////////////////////////////////
// CommThreadW
//
// Communications thread manager.
//
UINT CommThreadW (CIOQueue* pQueue)
{
	pQueue->WriterThread();
	return FALSE;

}// CommThreadW

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::CIOQueue
//
// Constructor for the basic queue class
//
CIOQueue::CIOQueue () : 
	m_evtClose (FALSE, TRUE), 
	m_evtPendingInput (FALSE, TRUE),
	m_evtPendingOutput (FALSE, TRUE)
{
	m_hComm = INVALID_HANDLE_VALUE;
	m_pThreadR = NULL;
	m_pThreadW = NULL;
	m_evtPendingInput.ResetEvent();

}// CIOQueue::CIOQueue

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::~CIOQueue
//
// Destructor for the basic I/O queue class
//
CIOQueue::~CIOQueue()
{
	Terminate();

}// CIOQueue::~CIOQueue

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::Start
//
// Startup our threads for I/O to our COMM device.
//
void CIOQueue::Start (HANDLE hComm)
{
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		// Still running?
		if (hComm == m_hComm)
			return;

		// Old handle?
		Terminate();
	}

	// Reset our close event - this will be signalled when
	// the "Terminate" method is invoked.
	m_evtClose.ResetEvent();

	// Setup our array event objects based on whether any
	// information is available in the array for I/O.
	CSingleLock Key ((CSyncObject*)&m_csInput, TRUE);
	if (m_arrOutput.GetSize() > 0)
		m_evtPendingOutput.SetEvent();
	else
		m_evtPendingOutput.ResetEvent();

	if (m_arrInput.GetSize() > 0)
		m_evtPendingInput.SetEvent();
	else
		m_evtPendingInput.ResetEvent();
	Key.Unlock();

	// Save off our COMM handle
	m_hComm = hComm;

	// Start our I/O threads.
	m_pThreadW = AfxBeginThread ((AFX_THREADPROC)CommThreadW, (LPVOID)this, THREAD_PRIORITY_ABOVE_NORMAL);
	m_pThreadR = AfxBeginThread ((AFX_THREADPROC)CommThreadR, (LPVOID)this, THREAD_PRIORITY_ABOVE_NORMAL);
	ASSERT (m_pThreadR && m_pThreadW);

}// CIOQueue::Start

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::Terminate
//
// Shutdown the threads.
//
void CIOQueue::Terminate()
{
	// If it is already shutdown, then exit.
	if (m_hComm == INVALID_HANDLE_VALUE)
		return;

	m_hComm = INVALID_HANDLE_VALUE;

	// Fire our close event.
	m_evtClose.SetEvent();

	HANDLE arrHandles[2];
	arrHandles[0] = m_pThreadR->m_hThread;
	arrHandles[1] = m_pThreadW->m_hThread;

	// Wait for our threads to terminate gracefully.
	if (WaitForMultipleObjects (2, arrHandles, TRUE, 5000) == WAIT_TIMEOUT)
	{
		// They didn't terminate properly.
		TerminateThread (arrHandles[0], 0);
		TerminateThread (arrHandles[1], 0);
		delete m_pThreadR;
		delete m_pThreadW;
	}

	m_pThreadR = NULL;
	m_pThreadW = NULL;

}// CIOQueue::Terminate

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::Read
// 
// Return any pending string from this queue.  This should be called
// by whoever is reading from the I/O queue - it will block the caller
// until either a string is available -OR- the queue shuts down.
//
CString CIOQueue::Read (HANDLE hEvent)
{
	// Wait for an event to show up.
	HANDLE arrHandles[2];
	arrHandles[0] = hEvent;
	arrHandles[1] = m_evtPendingInput;
	LONG lResult = WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE);
	if (lResult == WAIT_OBJECT_0 || lResult == WAIT_ABANDONED_0)
		return _T("");

	// Get the semaphore for this queue.
	CSingleLock Key ((CSyncObject*)&m_csInput, TRUE);
	ASSERT (m_arrInput.GetSize() > 0);

	CString strBuff = m_arrInput[0];
	m_arrInput.RemoveAt(0);

	// Reset the event if no more strings pending.
	if (m_arrInput.GetSize() == 0)
		m_evtPendingInput.ResetEvent();

	Key.Unlock();

	return strBuff;

}// CIOQueue::GetString

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::Write
//
// Add a new string to the input queue and signal the event.
//
void CIOQueue::Write(LPCTSTR lpszBuff)
{
	// If the buffer is NULL, ignore it.
	if (lpszBuff == NULL || *lpszBuff == '\0')
		return;

	LPSTR lpszNewBuff = NULL;

#ifdef _UNICODE
	// Convert the buffer to multibyte from UNICODE.
	int iRequiredLen = WideCharToMultiByte (CP_ACP, 0, lpszBuff, -1, NULL, 0, NULL, NULL);
	if (iRequiredLen > 0)
	{
		lpszNewBuff = AllocMem (iRequiredLen);
		if (lpszNewBuff != NULL)
			WideCharToMultiByte (CP_ACP, 0, lpszBuff, -1, lpszNewBuff, 
								 iRequiredLen, NULL, NULL);
	}
#else
	lpszNewBuff = (LPSTR) AllocMem (strlen(lpszBuff)+1);
	strcpy (lpszNewBuff, lpszBuff);
#endif

	if (lpszNewBuff == NULL)
		return;

	// Wait on the semaphore for this queue.
	CSingleLock Key ((CSyncObject*)&m_csOutput, TRUE);
	m_arrOutput.Add (lpszNewBuff);
	m_evtPendingOutput.SetEvent();

}// CIOQueue::Write

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::ReaderThread
//
// This method runs the COMM input thread.
//
void CIOQueue::ReaderThread()
{
	DWORD dwCount, dwMask, dwErr;
	CSingleLock Key (&m_csInput);
	CEvent evtRead;
	OVERLAPPED overlappedIO = { 0, 0, 0, 0, evtRead };
	unsigned char szBuff[128];
	int iCount = 0;

	// Setup our master wait array.
	HANDLE arrHandles[2];
	arrHandles[0] = m_evtClose;
	arrHandles[1] = evtRead;

	TRACE(_T("QUEUE: Reader thread started\r\n"));

	// Wait for an event character on our port.
	while (TRUE)
	{
		WaitCommEvent (m_hComm, &dwMask, &overlappedIO);
		LONG lResult = WaitForMultipleObjects (2, arrHandles, FALSE, INFINITE);
		if (lResult == WAIT_OBJECT_0 || lResult == WAIT_ABANDONED_0)
			break;

		// Reset the read event.
		evtRead.ResetEvent();
		memset(&overlappedIO, 0, sizeof(OVERLAPPED));
		overlappedIO.hEvent = evtRead;

		// If the wait was interrupted by an error, clear it.
		if (dwMask & EV_ERR)
			ClearCommError(m_hComm, &dwErr, NULL);

		// If the wait was interrupted by a BREAK, clear it.
		if (dwMask & EV_BREAK)
			ClearCommBreak (m_hComm);

		// If there is not an event character available, go back to waiting.
		if ((dwMask & EV_RXFLAG) == 0)
			continue;

		// Grab the buffer a character at a time so we never miss any
		// event characters.  Also, there may be additional characters which
		// have been inserted since our thread waking up that we need to leave
		// in the buffer.
		DWORD dwTickCount = GetTickCount();
		while (dwTickCount + 5000L > GetTickCount())
		{
			unsigned char c;
			if (ReadFile(m_hComm, &c, 1, &dwCount, &overlappedIO))
			{
				// Data pending - receive it.
				if (!GetOverlappedResult(m_hComm, &overlappedIO, &dwCount, TRUE))
					break;

				if (dwCount == 1)
				{
					if (c == _T('\n'))
					{
						// Skip single '\r\n' response which precedes response with some modems.
						if (iCount == 1 && szBuff[0] == '\r')
							iCount = 0;
						else
							break;
					}
					else
						szBuff[iCount++] = c;
				}
			}
			else 
			{
				if (GetLastError() != ERROR_IO_PENDING)
					TRACE(_T("ReadFile(hComm) failed, rc=0x%lx\r\n"), GetLastError());
				break;
			}
		}

		// If we got a string, add it to the buffer.
		if (iCount > 2)
		{
			Key.Lock();
			szBuff[iCount] = _T('\0');
			m_arrInput.Add((LPCSTR)szBuff);
			Key.Unlock();
			m_evtPendingInput.SetEvent();
			iCount = 0;
		}
	}

	TRACE(_T("QUEUE: Reader thread ended\r\n"));

}// CIOQueue::ReaderThread

//////////////////////////////////////////////////////////////////////////////
// CIOQueue::WriterThread
// 
// This method runs the COMM output thread.  It will never be suspended.
//
void CIOQueue::WriterThread()
{
	DWORD dwCount;
	CSingleLock Key (&m_csOutput);
	CEvent evtWrite;
	OVERLAPPED overlappedIO = { 0, 0, 0, 0, evtWrite };

	// Setup our master wait array.
	HANDLE arrHandles[2];
	arrHandles[0] = m_evtClose;
	arrHandles[1] = m_evtPendingOutput;

	TRACE(_T("QUEUE: Writer thread started\r\n"));

	while (TRUE)
	{
		LONG lResult = WaitForMultipleObjects (2, arrHandles, FALSE, INFINITE);
		if (lResult == WAIT_OBJECT_0 || lResult == WAIT_ABANDONED_0)
			break;

		// Grab the first string from the queue.
		LPSTR lpszBuff = NULL;

		Key.Lock();
		if (m_arrOutput.GetSize() > 0)
		{
			lpszBuff = (LPSTR) m_arrOutput[0];
			m_arrOutput.RemoveAt(0);
		}
		Key.Unlock();

		// If we have a string to output, put it out the COMM port.
		if (m_hComm != INVALID_HANDLE_VALUE)
		{
#ifdef _DEBUG
			DumpMem(_T("Writing:\r\n"), lpszBuff, strlen(lpszBuff));
#endif
			WriteFile (m_hComm, lpszBuff, strlen(lpszBuff), &dwCount, &overlappedIO);
			lResult = GetLastError();
			FreeMem (lpszBuff);

			if (lResult == ERROR_INVALID_HANDLE)
			{
				m_hComm = INVALID_HANDLE_VALUE;
				break;
			}
			else if (lResult == ERROR_IO_PENDING)
			{
				HANDLE arrHandles[2];
				arrHandles[0] = m_evtClose;
				arrHandles[1] = evtWrite;
				lResult =WaitForMultipleObjects(2, arrHandles, FALSE, INFINITE);
				if (lResult == WAIT_OBJECT_0 || lResult == WAIT_ABANDONED_0)
					break;
			}
		}

		// Reset the pending event if we have no more output ready.
		Key.Lock();
		if (m_arrOutput.GetSize() == 0)
			m_evtPendingOutput.ResetEvent();
		Key.Unlock();
	}

	TRACE(_T("QUEUE: Writer thread ended\r\n"));

	// Dump any waiting data in our queue.
	Key.Lock();
	for (int i = 0; i < m_arrOutput.GetSize(); i++)
	{
		LPSTR lpszBuff = (LPSTR) m_arrOutput[i];
		FreeMem (lpszBuff);
	}
	m_arrOutput.RemoveAll();
	Key.Unlock();

}// CIOQueue::WriterThread

