/******************************************************************************/
//                                                                        
// DEVICE.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our device override code for our ATSP sample.
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

/*--------------------------------------------------------------------------------------
    INCLUDES
--------------------------------------------------------------------------------------*/
#include "stdafx.h"
#include "resource.h"
#include "atsp.h"

/*--------------------------------------------------------------------------------------
    DEBUG INFORMATION
--------------------------------------------------------------------------------------*/
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////
// InputThread
//
// Starter for the device worker thread running all input from the 
// modem to the service provider.  This thread provides the main context
// for the service provider to run within.
//
UINT InputThread (CATSPLine* pLine)
{
	pLine->InputThread();
	return FALSE;

}// InputThread

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::OpenDevice
//
// Open the communications port for this device.
//
BOOL CATSPLine::OpenDevice()
{
	// If we already have an open port, close it and re-open it.
	if (m_hComm != INVALID_HANDLE_VALUE)
		CloseDevice();

	// Reset our close event - this will be signalled when
	// the destructor or CloseDevice method is invoked.
	m_evtClose.ResetEvent();

	// Get the data for opening the port.
	CATSPProvider* pProvider = (CATSPProvider*) AfxGetApp();
	CString strCOMM = pProvider->ReadProfileString(GetPermanentDeviceID(), GetString(IDS_COMMPORT));
    CString strBaud = pProvider->ReadProfileString(GetPermanentDeviceID(), GetString(IDS_BAUD));
	int iBaud = atoi (strBaud);

	// Fail if we don't have any configuration information.
	if (strCOMM.IsEmpty() || iBaud == 0)
	{
		TRACE(_T("Line device not configured.\r\n"));
		return FALSE;
	}

	// Open the COMM port.
	TRACE(_T("Opening COMM port: %s, Baud=%d\r\n"), (LPCTSTR) strCOMM, iBaud);
	m_hComm = CreateFile (strCOMM, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | 
							   FILE_FLAG_OVERLAPPED, NULL);
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		TRACE(_T("Failed to open COMM port <%s>, rc=0x%lx\r\n"), strCOMM, GetLastError());
		return FALSE;
	}

	// Create our input thread object for this device.  This thread will wait for
	// a full string from the COMM port and pass it through to each connection object.
	m_pThread = AfxBeginThread ((AFX_THREADPROC)::InputThread, (LPVOID)this, THREAD_PRIORITY_NORMAL);
	ASSERT (m_pThread != NULL);

    // Now initialize our COMM port settings.
	SetupComm (m_hComm, 4096, 4096);

	// Set our communication parameters.
    DCB dcb;
    GetCommState(m_hComm, &dcb);
	dcb.BaudRate = iBaud;
    dcb.Parity = NOPARITY;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
	SetCommState(m_hComm, &dcb);

	// Set the DTR line so the modem knows we are ready to communicate with it.
	EscapeCommFunction (m_hComm, SETDTR);

	// Mark the new modem handle
	SetModemHandle();

	// Start the threads
	Resume();

	return TRUE;

}// CATSPLine::OpenDevice

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::CloseDevice
//
// This is called to close our device down when the line
// is closed.
//
BOOL CATSPLine::CloseDevice()
{
	// If we have no valid COMM port, exit.
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		// Suspend our COMM monitor threads
		Suspend();

		// Tell our device thread to stop running.
		m_evtClose.SetEvent();
		if (m_pThread && WaitForSingleObject (m_pThread->m_hThread, 5000) == WAIT_TIMEOUT)
		{
			TerminateThread (m_pThread->m_hThread, 0);
			delete m_pThread;
		}

		// Purge the COMM ports
		PurgeComm (m_hComm, PURGE_TXABORT | PURGE_TXCLEAR);
		PurgeComm (m_hComm, PURGE_RXABORT | PURGE_RXCLEAR);

		// Reset carrier to drop any active connection on the modem.
		EscapeCommFunction (m_hComm, CLRDTR);

		// Close the port and reset the value.
		SetModemHandle();
		CloseHandle (m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}
	return TRUE;

}// CATSPLine::CloseDevice

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::DropCarrier
//
// Drop the carrier of the call
//
void CATSPLine::DropCarrier()
{
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		EscapeCommFunction (m_hComm, CLRDTR);
		Sleep (250);
		EscapeCommFunction (m_hComm, SETDTR);
	}

}// CATSPLine::DropCarrier

////////////////////////////////////////////////////////////////////////////
// CATSPLine::SendData
//
// Send data for this provider.  This forces the device to 
// write the data using the COMM write thread.
//
BOOL CATSPLine::SendData (LPCVOID lpBuff, DWORD /*dwSize*/)
{
	m_qData.Write ((LPCTSTR)lpBuff);
	return TRUE;

}// CATSPLine::SendData

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::InputThread
//
// This thread receives input from our QUEUE and passes it through
// the service provider code.  It waits on the queue for input and
// then parses the input to a token which is passed to 
// CServiceProvider::ProcessData.
//
VOID CATSPLine::InputThread()
{
	// Run until our close event is signaled from the destructor.
	// Wait on a string in the queue being filled by the CIOQueue::ReaderThread, 
	// and pass it to the line.
	while (TRUE)
	{
		// Wait for a string.  If this returns NULL, then the event was
		// signalled on us and we need to shutdown.
		CString strBuff = m_qData.Read (m_evtClose);
		if (strBuff.IsEmpty())
			break;

		TRACE(_T("Received [%s]\r\n"), (LPCTSTR) strBuff);

		// Decipher the result into a token
		DWORD dwToken = MODEM_UNKNOWN;
		if (strBuff.GetLength() >= 2)
		{
			CString strTok = strBuff.Left(2);
			strTok.MakeUpper();
			if (strTok == _T("CO") || strTok == _T("VC"))
				dwToken = MODEM_CONNECT;
			else if (strTok == _T("OK"))
				dwToken = MODEM_OK;
			else if (strTok == _T("BU"))
				dwToken = MODEM_BUSY;
			else if (strTok == _T("ER") || strTok == _T("NO"))
				dwToken = MODEM_ERR;
			else if (strTok == _T("RI"))
				dwToken = MODEM_RING;
		}

		// Pass it to the service provider class.  Our class doesn't use
		// the buffer for anything except the token so don't bother to pass
		// it down.
		ReceiveData (dwToken, NULL, 0L);
	}

}// CATSPLine::InputThread

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::Suspend
//
// Suspend monitoring of the COMM ports
//
void CATSPLine::Suspend()
{
	TRACE(_T("Suspending all COMM traffic\r\n"));

	// Turn off events.
	SetCommMask(m_hComm, 0L);

	// Turn off the event character
    DCB dcb;
    GetCommState(m_hComm, &dcb);
	dcb.EvtChar = 0x0;
    SetCommState(m_hComm, &dcb);

	m_qData.Terminate();

}// CATSPLine::Suspend

//////////////////////////////////////////////////////////////////////////////
// CATSPLine::Resume
//
// Resume monitoring of the COMM ports
//
void CATSPLine::Resume()
{
	TRACE(_T("Resuming COMM traffic\r\n"));

	// Reset our modem event character
    DCB dcb;
    GetCommState(m_hComm, &dcb);
	dcb.EvtChar = '\n';
    SetCommState(m_hComm, &dcb);

	// The CommTimeout numbers will very likely change if you are
	// coding to meet some kind of specification where you need to reply 
	// within a certain amount of time after recieving the last byte.  
	// However,  If 1/4th of a second goes by between recieving two 
	// characters, its a good indication that the transmitting end has 
	// finished, even assuming a 1200 baud modem.
	COMMTIMEOUTS CommTimeouts;
	CommTimeouts.ReadIntervalTimeout = 250;
	CommTimeouts.ReadTotalTimeoutMultiplier = CommTimeouts.ReadTotalTimeoutConstant = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = CommTimeouts.WriteTotalTimeoutConstant = 0;
	SetCommTimeouts(m_hComm, &CommTimeouts);

	SetCommMask(m_hComm, (EV_RXFLAG | EV_ERR | EV_BREAK));
	m_qData.Start(m_hComm);

}// CATSPLine::Resume

////////////////////////////////////////////////////////////////////////////
// CATSPLine::SetModemHandle
//
// This associates the open COMM port handle with our line.
//
void CATSPLine::SetModemHandle()
{
	CString strCOMM = GetSP()->ReadProfileString(GetPermanentDeviceID(), GetString(IDS_COMMPORT));
	if (!strCOMM.IsEmpty())
		AddDeviceClass (_T("comm/datamodem"), STRINGFORMAT_BINARY, (LPVOID)(LPCTSTR)strCOMM,
						strCOMM.GetLength()+sizeof(TCHAR), 
						(m_hComm == INVALID_HANDLE_VALUE) ? NULL : m_hComm);

}// CATSPLine::SetModemHandle
