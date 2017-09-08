/******************************************************************************/
//                                                                        
// SPTHREAD.CPP - Service Provider Thread support
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This module implements the connection to the companion application
// in the 16-bit library.
//                                                                        
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
//
/******************************************************************************/

#include "stdafx.h"
#include "spuser.h"
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::InitializeRequestThread
//
// Initialize our worker request thread.  The provider library
// always has one worker thread, this thread is our companion 
// application which calls into our service provider when it first 
// initializes.
//
VOID CServiceProvider::InitializeRequestThread(HWND hwnd)
{
    // Save off the window handle
    m_pMainWnd = new CWnd;
    m_pMainWnd->Attach(hwnd);

}// CServiceProvider::InitializeRequestThread

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::TerminateRequestThread
//
// Called to shutdown the request thread if active.
//
VOID CServiceProvider::TerminateRequestThread()
{
    if (m_pMainWnd != NULL)
    {
        ASSERT (m_pMainWnd != NULL);
        // Force the application to close and then delete our
        // handle to the window.
        if (m_pszExeName != NULL)
        {
            m_pMainWnd->SendMessage(WM_CLOSE);
            m_pMainWnd->Detach();
        }
        else
            m_pMainWnd->DestroyWindow();            

        delete m_pMainWnd;
        m_pMainWnd = NULL;
    }
    
}// CServiceProvider::TerminateRequestThread

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::OpenDevice
//
// Open a device and return a handle to the device.
//
BOOL CServiceProvider::OpenDevice (CTSPIConnection* pConn)
{   
    TRACE("Opening device 0x%lX\r\n", pConn->GetPermanentDeviceID());
    return SendThreadRequest (COMMAND_OPENCONN, (LPARAM)pConn->GetPermanentDeviceID());

}// CServiceProvider::OpenDevice

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::CloseDevice
//
// Close the device in question
//
BOOL CServiceProvider::CloseDevice (CTSPIConnection* pConn)
{
    TRACE("Closing device 0x%lX\r\n", pConn->GetPermanentDeviceID());
    return SendThreadRequest (COMMAND_CLOSECONN, (LPARAM)pConn->GetPermanentDeviceID());

}// CServiceProvider::CloseDevice

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::SendData
//
// This sends the buffer to the physical telephony device.  It is called
// by the CTSPIDevice class to send a buffer through the main class.
//
BOOL CServiceProvider::SendData(CTSPIConnection* pConn, LPCVOID lpBuff, DWORD dwSize)
{
    ASSERT(!IsBadReadPtr (lpBuff, (UINT)dwSize));
    
    // Allocate a SendData buffer to send to the device.  It MUST be allocated with
    // GlobalAlloc with DDESHARE turned on.
    LPTSPSENDDATA lpSend = (LPTSPSENDDATA) GlobalAllocPtr(GHND | GMEM_DDESHARE, sizeof(TSPSENDDATA)+dwSize);
    if (lpSend == NULL)
    {
        TRACE("*ERROR* Unable to allocate memory for Send!\r\n");
        return FALSE;
    }
    
    // Fill in the SendStr buffer.
    lpSend->lpBuff = (LPSTR)lpSend + sizeof(TSPSENDDATA);
    lpSend->dwConnId = (DWORD) pConn->GetPermanentDeviceID();
    lpSend->dwSize = dwSize;
    CopyBuffer (lpSend->lpBuff, lpBuff, dwSize);
    
    // Send it to the companion thread via COPYDATA method.
    BOOL fRC = SendThreadRequest (COMMAND_SENDDATA, (LPARAM)lpSend, TRUE, sizeof(TSPSENDDATA)+dwSize);
    GlobalFreePtr (lpSend);
    return fRC;
    
}// CServiceProvider::SendData

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::StartNextCommand
//
// Start a new command on the service provider.
//
VOID CServiceProvider::StartNextCommand(CTSPIConnection* pConn)
{                     
    // Tell the thread application to switch to our context.
    TRACE ("Forcing context switch to companion application, line device=0x%lx\r\n", pConn->GetPermanentDeviceID());
    SendThreadRequest (COMMAND_WAITINGREQ, (LPARAM)pConn->GetPermanentDeviceID());

    // Will come back here when service provider releases context.

}// CServiceProvider::StartNextCommand

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::SendThreadRequest
//
// Send a thread request to the companion application.  Provide for
// thunking if the lParam is a data pointer.
//
BOOL CServiceProvider::SendThreadRequest (WORD wCommand, LPARAM lData, BOOL fThunkData, DWORD dwSize)
{    
    ASSERT (m_pMainWnd != NULL);

    UINT nMsg = UM_TSPI_COMMAND;
    WPARAM wParam = (WPARAM) wCommand;
    LPARAM lParam = lData;
    COPYDATASTRUCT cd;
    
    if (fThunkData)
    {                      
        ASSERT (lParam != NULL);
        ASSERT (dwSize > 0);    
        
        // Move it to the companion using WM_COPYDATA to convert our pointers for us.
        // This is used just in case the companion is 32-bit (we are 16-bit).
        // If the app is 16-bit, then it will get a direct pointer to this data
        // structure.
        cd.dwData = wCommand;
        cd.cbData = dwSize;
        cd.lpData = (LPVOID)lParam;

#ifdef _DEBUG    
        DumpMem ("Sending Data:\r\n", (LPVOID)lParam, dwSize);
#endif    
        
        nMsg = WM_COPYDATA;
        wParam = 0;  
        lParam = (LPARAM)(LPCSTR)&cd;
    }        
    
    // Send the companion application the message.    
    if (m_pMainWnd != NULL)
        return (BOOL) m_pMainWnd->SendMessage (nMsg, wParam, lParam);
    return FALSE;        

}// CServiceProvider::SendThreadRequest
