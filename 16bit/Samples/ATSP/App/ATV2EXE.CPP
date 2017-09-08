/******************************************************************************/
//                                                                        
// ATV2EXE.CPP - Service Provider thread sample application
//                                                                        
// This file contains all our low-level device code for the serial
// port ATSP sample service provider.
// 
// Copyright (C) 1994, 1995, 1996 Mark C. Smith
// All rights reserved                                                    
//                                                                        
// This service provider drives a Hayes compatible AT style modem.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
/******************************************************************************/

#include "stdafx.h"
#include "atv2exe.h"
#include <windowsx.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////
// Globals and constants

const UINT RXQUEUE = 1024;
const UINT TXQUEUE = 128;
const UINT MAX_BLOCK = 255;
const UINT IDT_TIMER = 100;

//////////////////////////////////////////////////////////////////////////////
// Main executable object
//
CATExeApp NEAR theApp;

//////////////////////////////////////////////////////////////////////////////
// CATExeApp::InitInstance
//
// This sets up the thread application and registers us with the 
// service provider.
//
BOOL CATExeApp::InitInstance()
{   
    // Create the new main window.
    m_pMainWnd = new CMainFrame();
    if (m_pMainWnd)
    {   
        // Register us with the service provider.
        DeviceNotify (RESULT_INIT, 0, (DWORD)(UINT)m_pMainWnd->GetSafeHwnd(), NULL, 0);
        
        // Setup an interval timer for the application
        m_pMainWnd->SetTimer(IDT_TIMER, 500, NULL);
        return TRUE;
    }  
    
    // Failed - exit.
    return FALSE;
    
}// CATExeApp::InitInstance

//////////////////////////////////////////////////////////////////////////////
// CATExeApp Message Map

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
   //{{AFX_MSG_MAP(CMainFrame)
   ON_MESSAGE(UM_TSPI_COMMAND, OnTSPICommand)
   ON_MESSAGE(WM_COMMNOTIFY, OnCommNotify)
   ON_MESSAGE(WM_COPYDATA, OnCopyData)
   ON_WM_CLOSE()
   ON_WM_TIMER()
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::CMainFrame
//
// This is the frame class constructor.
//
CMainFrame::CMainFrame()
{   
    // Initialize any variables.
    m_iOpenCount = 0;
    m_idComDev = -1;

    // Create the main window.
    Create(NULL, "", (WS_OVERLAPPEDWINDOW & ~WS_VISIBLE), 
           rectDefault, NULL);
    
}// CMainFrame::CMainFrame

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnTimer
//
// This generates the interval timer which the service provider
// relies upon for time-out conditions.
//
void CMainFrame::OnTimer(UINT nIdEvent)
{
    // Send TSP an interval message
    if (nIdEvent == IDT_TIMER)
        DeviceNotify (RESULT_INTERVALTIMER, 0, 0, NULL, 0);

}// CMainFrame::OnTimer

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnClose
//
// This method closes the thread application
//
void CMainFrame::OnClose()
{   
    if (m_idComDev >= 0)        
    {   
        FlushComm (m_idComDev, 0);
        FlushComm (m_idComDev, 1);
        CloseComm (m_idComDev);
        m_idComDev = -1;
    }        
    DestroyWindow();

}// CMainFrame::OnClose

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnTSPICommand
//
// This processes any commands from the service provider.
//
LRESULT CMainFrame::OnTSPICommand (WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    	case COMMAND_OPENSERIAL:
        	return OnTSPIOpenConn((LPSERIALINFO)lParam); 

        case COMMAND_CLOSECONN: 
            OnTSPICloseConn((DWORD)lParam); 
            break;
            
        case COMMAND_WAITINGREQ:                
            DeviceNotify (RESULT_CONTEXTSWITCH, (DWORD)lParam, 0, NULL, 0);
            break;

        case COMMAND_SETDTR:
            ManageDTR(TRUE);
            break;
            
        case COMMAND_CLEARDTR:
            ManageDTR(FALSE);
            break;

        case COMMAND_GETCOMMHANDLE:
            return (LRESULT) m_idComDev;

	    case COMMAND_SENDDATA:
        	OnTSPISendStr((LPTSPSENDDATA)lParam);
            
        default: 
            break;
    }
    return 1L;
   
}// CMainFrame::OnTSPICommand

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnCopyData
//
// This method is called when the service provider is sending a block of
// data to this executable.  The library uses the WM_COPYDATA message since
// it will thunk the parameters under Windows 95, Windows NT, and Win32s
// if we go from a 16-bit context to a 32-bit context.
//
LRESULT CMainFrame::OnCopyData (WPARAM /*wParam*/, LPARAM lParam)
{                         
    LPCOPYDATASTRUCT lpCopyData = (LPCOPYDATASTRUCT) lParam;
    return OnTSPICommand ((WPARAM)lpCopyData->dwData, (LPARAM)lpCopyData->lpData);

}// CMainFrame::OnCopyData

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnTSPIOpenConn
//
// This method handles an open request from the SP.
//
BOOL CMainFrame::OnTSPIOpenConn(LPSERIALINFO pSI)
{   
    TRACE ("Open request by <0x%lx> (%d)\r\n", pSI->dwConnId, m_iOpenCount);
	TRACE ("pSI->iPort=%d, pSI->dwBaud=%ld, pSI->dwConnId=%ld\r\n", pSI->iPort, pSI->dwBaud, pSI->dwConnId);    

    // Increment our open count.
    if (++m_iOpenCount > 1)
    {
        ASSERT (m_dwConnId == pSI->dwConnId);
        return TRUE;                    
    }
    
    m_dwConnId = pSI->dwConnId;

    // Open the COMM port using the specified parameters.
    char szBuff[30];
    wsprintf (szBuff, "COM%d", pSI->iPort);
    
    m_idComDev = OpenComm (szBuff, RXQUEUE, TXQUEUE);
    if (m_idComDev < 0)
    {
        m_iOpenCount = 0;
        TRACE ("Failed to open %s, rc=%d\r\n", (LPCSTR)szBuff, m_idComDev);
        return FALSE;
    }        

    // Now setup the comm port according to what the SP asked for.
    DCB dcb;
    GetCommState(m_idComDev, &dcb);
    
    switch (pSI->dwBaud)
    {
        case 300:    dcb.BaudRate = CBR_300; break;
        case 1200:   dcb.BaudRate = CBR_1200; break;
        case 2400:   dcb.BaudRate = CBR_2400; break;
        case 9600:   dcb.BaudRate = CBR_9600; break;
        case 19200:  dcb.BaudRate = CBR_19200; break;
        case 38400:  dcb.BaudRate = CBR_38400; break;
		case 57600:  dcb.BaudRate = CBR_56000; break;
		case 115200: dcb.BaudRate = CBR_128000; break;
        default:     dcb.BaudRate = CBR_9600; break;
    }
    
    dcb.Parity = NOPARITY;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;

    // Setup hardware flow control
    dcb.fOutxDsrFlow = FALSE;
    dcb.DsrTimeout = 0;
    dcb.fOutxCtsFlow = dcb.fRtsflow = TRUE;
    dcb.CtsTimeout = 30;

    // Setup software flow control
    dcb.fInX = dcb.fOutX = FALSE;
    dcb.XonChar = 0x17;
    dcb.XoffChar = 0x13;
    dcb.XonLim = 100;
    dcb.XoffLim = 100;

    // Other various settings
    dcb.fBinary = TRUE;
    dcb.fParity = TRUE;
    dcb.fRtsDisable = FALSE;
    dcb.fDtrDisable = FALSE;
    
    // Set the COMM state.
    SetCommState(&dcb);
    
    // Manage COMM notify when we receive characters.
    SetCommEventMask(m_idComDev, EV_RXCHAR);
    EnableCommNotification(m_idComDev, GetSafeHwnd(), -1, -1);
    
    return TRUE;
    
}// CMainFrame::OnTSPIOpenConn

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnTSPICloseConn
//
// This method handles a request to close a SP.
//
void CMainFrame::OnTSPICloseConn(DWORD dwConnID)
{
#ifdef _DEBUG
    TRACE ("Close request by <0x%lx> (%d)\r\n", dwConnID, m_iOpenCount);
    ASSERT (dwConnID == m_dwConnId);
#else
    dwConnID;
#endif            

    if (--m_iOpenCount > 0)
        return;
        
    if (m_idComDev > 0)
    {
        FlushComm (m_idComDev, 1);
        VERIFY (EscapeCommFunction(m_idComDev, CLRDTR) == 0);
        CloseComm (m_idComDev);
        m_idComDev = -1;
        m_iOpenCount = 0;
    }        

}// CMainFrame::OnTSPICloseConn

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnTSPISendStr
//
// This method processes a request to send a string to the modem.
// We assume that the buffer is NULL terminated.
//   
void CMainFrame::OnTSPISendStr(LPTSPSENDDATA sendPtr)
{   
    ASSERT (m_idComDev > 0);    
    ASSERT (sendPtr->dwSize < 32767);
    if (m_idComDev > 0)
        WriteComm (m_idComDev, sendPtr->lpBuff, (int)(sendPtr->dwSize-1));

}// CMainFrame::OnTSPISendStr

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::ManageDTR
//
// Manage the Data Terminal Ready line on the modem.
//
void CMainFrame::ManageDTR(BOOL fSet)
{                        
    if (m_idComDev > 0)
        VERIFY (EscapeCommFunction(m_idComDev, (fSet) ? SETDTR : CLRDTR) == 0);

}// CMainFrame::ManageDTR

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnCommNotify
//
// This method processes COMM port responses from the modem
//
LRESULT CMainFrame::OnCommNotify (WPARAM /*wParam*/, LPARAM lParam)
{   
    // Buffer we store our current line we are building into - service
    // provider expects full lines.
    char Buffer[MAX_BLOCK+1];
    MSG msg;
    
    // If this isn't for us, then don't process it.
    if (LOWORD(lParam) & CN_EVENT != CN_EVENT || m_idComDev < 0)
        return FALSE;
    
    // Reset the event mask.
    GetCommEventMask(m_idComDev, EV_RXCHAR);

    // Now loop - this is necessary at higher baud rates (>= 9600)
    // to insure that we always see every charakter.
    BOOL fDone = FALSE;
    do
    {   
        // If we found a CR/LF pair, then send back the token to the SP.
        if (ProcessCommBlock(Buffer, MAX_BLOCK, &fDone))
        {
            // Determine what the result code is.
            BYTE bToken = DecipherReply (Buffer);
            if (bToken != MODEM_UNKNOWN && bToken != MODEM_IGNORE)
                DeviceNotify (RESULT_RCVDATA, m_dwConnId, (DWORD) bToken, NULL, 0);
            memset (Buffer, '\0', sizeof (Buffer));
        }
    }
    while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) || !fDone);
    
    return TRUE;

}// CMainFrame::OnCommNotify

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::ProcessCommBlock
//
// Process the characters waiting at the COMM port.  Wait for a CR/LF, and
// then send the resulting token back to the service provider.
//
BOOL CMainFrame::ProcessCommBlock (LPSTR lpszBuff, UINT uiMaxLen, BOOL* pfDone)
{  
    static CString strLastRead;
    static char szBuffer [MAX_BLOCK*2+1];
    
    // Read the new data from the port.
    memset (lpszBuff, '\0', uiMaxLen);
    int nLength = ReadComm(m_idComDev, lpszBuff, uiMaxLen);
    if  (nLength < 0)
    {
        nLength *= -1 ;
        int nError;
        while ((nError = GetCommError(m_idComDev, NULL)) > 0)
        {                
            char szCurrRead[80];
            wsprintf (szCurrRead, "COMM error: %02x", nError);
            AfxMessageBox (szCurrRead);
        }
        return FALSE;
    }
    
    // Save off whether we read any characters.
    *pfDone = (nLength == 0);
    
    // No data to process?  Tell the caller.
    if (*lpszBuff == '\0' && strLastRead.IsEmpty())
        return FALSE;
        
    // Reset our building buffer
    memset (szBuffer, '\0', MAX_BLOCK+1);
    
    // Move any data left over from the LAST read which
    // we terminated at a CR/LF.
    if (!strLastRead.IsEmpty())
        strcpy(szBuffer, (LPCSTR)strLastRead);
    strcat(szBuffer, lpszBuff);

    // Kill any preceding CR/LF chars we see so we don't
    // send back a NULL string - otherwise we might never
    // process the remainder if we have TWO sets here.
    while (*szBuffer == '\r' || *szBuffer == '\n')
    {
        memmove(szBuffer, szBuffer+1, MAX_BLOCK);
        szBuffer[MAX_BLOCK] = '\0';
    }        

    // Now run through the complete string and see if we
    // have a terminator (CR or LF).
    for (int i = 0; *(szBuffer+i) != '\0'; i++)
    {
        if (*(szBuffer+i) == '\r' || *(szBuffer+i) == '\n')
        {
            // Found a CR or LF.. end the buffer here, and copy the
            // remainder to the storage area.
            *(szBuffer+i) = '\0';
            i++;
            if (*(szBuffer+i) == '\r' || *(szBuffer+i) == '\n')
                i++;
            
            // Save off the remainder of the buffer
            strLastRead = (LPSTR) (szBuffer+i);

            // Copy our return string to the original buffer and return
            // a TRUE result indicating we got a response.
            strcpy(lpszBuff, szBuffer);
            return TRUE;
        }
    } 
    
    // Otherwise, save the entire buffer back
    strLastRead = (LPSTR) szBuffer;
    return FALSE;

}// CMainFrame::ProcessCommBlock

////////////////////////////////////////////////////////////////////////////
// CMainFrame::DecipherReply
//
// Evaluates the string passed from the modem to determine the result
// of our last command.
//
BYTE CMainFrame::DecipherReply(LPCSTR lpszData) const
{
    BYTE nReply = MODEM_UNKNOWN;
    WORD wResult = 0;
 
    TRACE("ATV2EXE: <%s>\r\n", (LPCSTR)lpszData);
  
    if (*lpszData)
        wResult = (((WORD)toupper(*lpszData)<<8) + (toupper(*(lpszData+1))));

    switch (wResult)
    {
        case 'AT':                 // ATxxx
            nReply = MODEM_IGNORE;
            break;
        case 'ER':                 // ERROR
        case 'NO':                 // NO DIALTONE/CARRER.
            nReply = MODEM_ERR; 
            break;
        case 'OK':                 // OK
        case 'VC':                 // VCON
            nReply = MODEM_OK;
            break;
        case 'CO':                 // CONNECT
            nReply = MODEM_CONNECT;
            break;
        case 'CA':
            nReply = MODEM_CARRIER;
            break;    
        case 'BU':                 // BUSY
            nReply = MODEM_BUSY;
            break;
        case 'RI':                 // RING
            nReply = MODEM_RING;
            break;
        default:
            break;;
    }             
    return nReply;

}// CMainFrame::DecipherReply

