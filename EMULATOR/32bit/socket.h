/*****************************************************************************/
//
// SOCKET.H - Digital Switch Service Provider Sample
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

class CMsg;
class CEmulatorDlg;

/////////////////////////////////////////////////////////////////////////////
// CListeningSocket server socket

class CListeningSocket : public CSocket
{
	DECLARE_DYNAMIC(CListeningSocket);

// Attributes
public:
	CEmulatorDlg* m_pDlg;

// Construction
public:
	CListeningSocket(CEmulatorDlg* pDlg);

// Overridable callbacks
protected:
	virtual void OnAccept(int nErrorCode);

// Noop functions
private:
	CListeningSocket(const CListeningSocket& rSrc);
	void operator=(const CListeningSocket& rSrc);
};

/////////////////////////////////////////////////////////////////////////////
// CClientSocket client socket

class CClientSocket : public CSocket
{
	DECLARE_DYNAMIC(CClientSocket);

// Attributes
public:
	CSocketFile* m_pFile;
	CArchive* m_pArchiveIn;
	CArchive* m_pArchiveOut;
	CEmulatorDlg* m_pDlg;

// Construction
public:
	CClientSocket(CEmulatorDlg* pDlg);
	virtual ~CClientSocket();

// Operations
public:
	void Init();
	void Abort();
	void SendMsg(DWORD dwCommand, LPVOID lpBuff, DWORD dwSize);
	void ReceiveMsg(LPDWORD pdwCommand, LPVOID* ppBuff, LPDWORD pdwSize);
	BOOL IsAborted() { return m_pArchiveOut == NULL; }

// Overridable callbacks
protected:
	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode);

// Noop functions
private:
	CClientSocket(const CClientSocket& rSrc);
	void operator=(const CClientSocket& rSrc);
};
