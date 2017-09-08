/*****************************************************************************/
//
// SOCKET.CPP - Digital Switch Service Provider Sample
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
#include "resource.h"
#include "objects.h"
#include "baseprop.h"
#include "call.h"
#include "dial.h"
#include "confgdlg.h"
#include "socket.h"
#include "gendlg.h"
#include "addrset.h"
#include "gensetup.h"
#include "gentone.h"
#include "about.h"
#include "emulator.h"
#include <windowsx.h>

IMPLEMENT_DYNAMIC(CListeningSocket, CSocket)
IMPLEMENT_DYNAMIC(CClientSocket, CSocket)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif                                              

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::ProcessPendingAccept
//
// Process a pending socket accept
//
void CEmulatorDlg::ProcessPendingAccept()
{
	CClientSocket* pSocket = new CClientSocket(this);
	if (m_pSocket->Accept(*pSocket))
	{
		pSocket->Init();
		m_connectionList.AddTail(pSocket);

		CString strSocket, strMsg;
		UINT uiPort;
		pSocket->GetPeerName(strSocket, uiPort);
		strMsg.Format("Connected to %s", strSocket);
		AddDebugInfo (1, strMsg);
		strMsg.Format("%d active connections", m_connectionList.GetCount());
		AddDebugInfo (1, strMsg);
		UpdateStates();
	}
	else
		delete pSocket;

}// CEmulatorDlg::ProcessPendingAccept

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::ProcessPendingRead
//
// Process a pending socket data request
//
void CEmulatorDlg::ProcessPendingRead(CClientSocket* pSocket)
{
	DWORD dwCommand, dwSize;
	LPVOID lpBuff;

	do
	{
		if (!ReadMsg(pSocket, &dwCommand, &lpBuff, &dwSize) || dwCommand == EMCOMMAND_CLOSE)
		{
			CloseSocket(pSocket);
			break;
		}

		// Let the thread waiting on us release.
		if (dwCommand != EMCOMMAND_QUERYCAPS &&
			dwCommand != EMCOMMAND_GETVERSION &&
			dwCommand != EMCOMMAND_GETADDRESSINFO)
		SendMsg(pSocket, EMRESULT_RECEIVED, NULL, 0);

		// Process the message and delete the buffer.
		ProcessEmulatorMessage(pSocket, dwCommand, lpBuff, dwSize);
		if (dwSize > 0 && lpBuff != NULL)
			delete [] lpBuff;
	}
	while (!pSocket->m_pArchiveIn->IsBufferEmpty());

}// CEmulatorDlg::ProcessPendingRead

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::ReadMsg
//
// Read a single message block off the socket
//
BOOL CEmulatorDlg::ReadMsg(CClientSocket* pSocket, LPDWORD pdwCommand, LPVOID* ppBuff, LPDWORD pdwSize)
{
	TRY
	{
		pSocket->ReceiveMsg(pdwCommand, ppBuff, pdwSize);
	}
	CATCH(CFileException, e)
	{
		pSocket->Abort();
		return FALSE;
	}
	END_CATCH
	return TRUE;

}// CEmulatorDlg::ReadMsg

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::BroadcastMsg
//
// Send a message out to all sockets
//
BOOL CEmulatorDlg::BroadcastMsg (DWORD dwResult, LPVOID lpBuff, DWORD dwSize)
{
	BOOL fAllSent = TRUE;
	for (POSITION pos = m_connectionList.GetHeadPosition(); pos != NULL;)
	{
		CClientSocket* pSock = (CClientSocket*)m_connectionList.GetNext(pos);
		if (!SendMsg(pSock, dwResult, lpBuff, dwSize))
			fAllSent = FALSE;
	}

	return fAllSent;

}// CEmulatorDlg::BroadcastMsg

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::SendErr
//
// Sends an error out the socket
//
BOOL CEmulatorDlg::SendErr(CClientSocket* pSocket, WORD wAddress, WORD wResult)
{
	DWORD dwResult = MAKEERR(wAddress, wResult);
	return SendMsg(pSocket, EMRESULT_ERROR, &dwResult, sizeof(DWORD));

}// CEmulatorDlg::SendErr

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::SendMsg
//
// Send a message out the socket
//
BOOL CEmulatorDlg::SendMsg(CClientSocket* pSocket, DWORD dwResult, LPVOID lpBuff, DWORD dwSize)
{
	TRY
	{
		pSocket->SendMsg(dwResult, lpBuff, dwSize);
	}
	CATCH(CFileException, e)
	{
		pSocket->Abort();
		return FALSE;
	}
	END_CATCH
	return TRUE;

}// CEmulatorDlg::SendMsg

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::CloseSocket
//
// Close the specified socket
//
void CEmulatorDlg::CloseSocket(CClientSocket* pSocket)
{
	// Close the socket.
	pSocket->Close();

	// Locate and remove the socket
	POSITION pos,temp;
	for (pos = m_connectionList.GetHeadPosition(); pos != NULL;)
	{
		temp = pos;
		CClientSocket* pSock = (CClientSocket*)m_connectionList.GetNext(pos);
		if (pSock == pSocket)
		{
			m_connectionList.RemoveAt(temp);

			CString strSocket, strMsg;
			UINT uiPort;
			pSocket->GetPeerName(strSocket, uiPort);
			strMsg.Format("Closing %s", strSocket);
			AddDebugInfo (1, strMsg);
			strMsg.Format("%d active connections", m_connectionList.GetCount());
			AddDebugInfo (1, strMsg);

			if (m_connectionList.GetCount() == 0)
				OnReset();

			UpdateStates();
			break;
		}
	}
	delete pSocket;

}// CEmulatorDlg::CloseSocket

//////////////////////////////////////////////////////////////////////////
// CListeningSocket::CListeningSocket
//
// Run the listen thread.
//
CListeningSocket::CListeningSocket(CEmulatorDlg* pDlg)
{
	m_pDlg = pDlg;

}// CListeningSocket::CListeningSocket

///////////////////////////////////////////////////////////////////////////
// CListeningSocket::OnAccept
//
// Pending accept on our server socket
//
void CListeningSocket::OnAccept(int nErrorCode)
{
	CSocket::OnAccept(nErrorCode);
	m_pDlg->ProcessPendingAccept();

}// CListeningSocket::OnAccept

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::CClientSocket
//
// Client socket constructor
//
CClientSocket::CClientSocket(CEmulatorDlg* pDlg)
{
	m_pDlg = pDlg;
	m_pFile = NULL;
	m_pArchiveIn = NULL;
	m_pArchiveOut = NULL;

}// CClientSocket::CClientSocket

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::~ClientSocket
//
// Destructor for the socket.

CClientSocket::~CClientSocket()
{
	delete m_pArchiveOut;
	delete m_pArchiveIn;
	delete m_pFile;

}// CClientSocket::~ClientSocket

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::Init
//
// Initialize the socket
//
void CClientSocket::Init()
{
	m_pFile = new CSocketFile(this);
	m_pArchiveIn = new CArchive(m_pFile,CArchive::load);
	m_pArchiveOut = new CArchive(m_pFile,CArchive::store);

}// CClientSocket::Init

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::Abort
//
// Abort the current operation.
//
void CClientSocket::Abort()
{
	if (m_pArchiveOut != NULL)
	{
		m_pArchiveOut->Abort();
		delete m_pArchiveOut;
		m_pArchiveOut = NULL;
	}

}// CClientSocket::Abort

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::SendMsg
//
// Send a message out the socket
//
void CClientSocket::SendMsg(DWORD dwResult, LPVOID lpBuff, DWORD dwSize)
{
	if (m_pArchiveOut != NULL)
	{
		*m_pArchiveOut << dwResult;
		*m_pArchiveOut << dwSize;
		if (dwSize > 0)
			m_pArchiveOut->Write (lpBuff, dwSize);
		m_pArchiveOut->Flush();
	}

}// CClientSocket::SendMsg

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::ReceiveMsg
//
// Receive a message on this socket
//
void CClientSocket::ReceiveMsg(LPDWORD pdwCommand, LPVOID* ppBuff, LPDWORD pdwSize)
{
	*m_pArchiveIn >> *pdwCommand;
	*m_pArchiveIn >> *pdwSize;
	if (*pdwSize > 0)
	{
		char* pBuff = new char[*pdwSize];
		m_pArchiveIn->Read(pBuff, *pdwSize);
		*ppBuff = (void*) pBuff;
	}
	else
		*ppBuff = NULL;

}// CClientSocket::ReceiveMessage

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::OnReceive
//
// Callback when data arrives on the socket.
//
void CClientSocket::OnReceive(int nErrorCode)
{
	CSocket::OnReceive(nErrorCode);
	m_pDlg->ProcessPendingRead(this);

}// CClientSocket::OnReceive

/////////////////////////////////////////////////////////////////////////////
// CClientSocket::OnClose
//
// Callback when the socket is closed by the provider
//
void CClientSocket::OnClose(int nErrorCode)
{
	CSocket::OnClose(nErrorCode);
	m_pDlg->CloseSocket(this);

}// CClientSocket::OnClose


