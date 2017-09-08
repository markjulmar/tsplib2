/*****************************************************************************/
//
// DSSPEXE.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the executable thread for the digital switch
// sample.  The general service provider model is to use a background
// task .EXE to perform general duties involving a telephony device.
// This is especially necessary under Win3.1 where threads are not available.
// 
// Copyright (C) 1994, 1995, 1996 Mark C. Smith
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
// Change History
// ----------------------------------------------------------------
// 10/02/94     Mark Smith (MCS)
//      Initial revision.
// 04/18/96     Mark Smith (MCS)
//      Rewritten for new emulator specification 
//
/*****************************************************************************/

#include "stdafx.h"
#include "dsspexe.h"
#include <windowsx.h>
#include "..\sp\dsspint.h"
#include "..\sp\emintf.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////////////
// Globals

DSSPExeApp NEAR theApp;                 // App object
const UINT IDT_TIMER = 100;

// This array translates the messages from the service provider into
// commands for the emulator.
WORD g_wCommandTable[] = {
    EMCOMMAND_QUERYCAPS, 
    EMCOMMAND_GETADDRESSINFO,
    EMCOMMAND_PREPAREADDR,
    EMCOMMAND_DROPCALL,
    EMCOMMAND_DIAL,
    EMCOMMAND_GETVERSION,
    EMCOMMAND_ANSWER,
    EMCOMMAND_HOLDCALL,
    EMCOMMAND_UNHOLDCALL,
    EMCOMMAND_TRANSFER,
    EMCOMMAND_FLASH,
    EMCOMMAND_FORWARD,
    EMCOMMAND_SETLEVEL,
    EMCOMMAND_SETLEVEL,
    EMCOMMAND_SETHOOKSWITCH,
    EMCOMMAND_CONFERENCE,
    EMCOMMAND_REDIRECT,
    EMCOMMAND_SETRINGMODE,
    EMCOMMAND_COMPLETECALL,
    EMCOMMAND_PARKCALL,
    EMCOMMAND_UNPARKCALL
};

//////////////////////////////////////////////////////////////////////////////
// DSSPExeApp::InitInstance
//
// This sets up the thread application and registers us with the service provider.
//
BOOL DSSPExeApp::InitInstance()
{   
    // Create a new frame window to receive messages from the emulator.
    m_pMainWnd = new CMainFrame();
    ASSERT(m_pMainWnd != NULL);
    
    // Wait for the emulator to hook up to us.
    if (m_pMainWnd && ((CMainFrame*)m_pMainWnd)->WaitForEmulator())
        return TRUE;
    return FALSE;
    
}// DSSPExeApp::InitInstance

//////////////////////////////////////////////////////////////////////////////
// DSSPExeApp Message Map
//                                                             
static UINT NEAR WM_DSS_EMULATOR = ::RegisterWindowMessage("DSSEmulator");

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
   //{{AFX_MSG_MAP(CMainFrame)
   ON_MESSAGE(UM_TSPI_COMMAND,OnTSPICommand)
   ON_MESSAGE(WM_COPYDATA, OnCopyData)
   ON_REGISTERED_MESSAGE(WM_DSS_EMULATOR, OnEmulator)
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
    Create(NULL, "DSSP thread application", 
           (WS_OVERLAPPEDWINDOW & ~WS_VISIBLE), rectDefault, NULL);

    m_hwndEmulator = NULL;
    m_dwTapiId = -1L;
    m_iOpenCount = 0;
    
}// CMainFrame::CMainFrame

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::WaitForEmulator
//
// Wait a predetermined amount of time for the emulator to respond.
//
BOOL CMainFrame::WaitForEmulator()
{
    // Broadcast out a message for the emulator.
    ::SendMessage(HWND_BROADCAST, WM_DSS_EMULATOR, EMCOMMAND_INIT, MAKELPARAM(GetSafeHwnd(), 0));
    
    // Wait 5 seconds for a response.
    DWORD dwTick = GetTickCount();
    while (dwTick+3000L > GetTickCount())
    {
        if (m_hwndEmulator != NULL)
        {                      
            // Register us with the service provider and setup the interval timer.
            DeviceNotify (RESULT_INIT, 0, (DWORD)(UINT)GetSafeHwnd(), NULL, 0);
            SetTimer(IDT_TIMER, 500, NULL);
            return TRUE;
        }
        Yield();
    }
    return FALSE;
    
}// CMainFrame::WaitForEmulator
    
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
        DeviceNotify(RESULT_INTERVALTIMER, 0, 0, NULL, 0);

}// CMainFrame::OnTimer

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnClose
//
// This method closes the thread application
//
void CMainFrame::OnClose()
{  
    // Tell the emulator we are shutting down.
    SendEmulatorMessage (EMCOMMAND_TSPEXIT);
    DestroyWindow();

}// CMainFrame::OnClose

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnEmulator
//
// This method is called by an emulator response.
//
LRESULT CMainFrame::OnEmulator(WPARAM wParam, LPARAM lParam)
{   
    // If this is an INIT command, then manage it here within the companion
    // application.
    if (wParam == EMRESULT_INIT)
    {   
        HWND hwnd = (HWND)LOWORD(lParam);
        if (IsWindow (hwnd))
        {
            if (hwnd != GetSafeHwnd())
                m_hwndEmulator = hwnd;
        }                
    }                              
    else // Pass it onto the service provider if it has opened a connection to us.
    {   
        if (m_dwTapiId != -1L)
            DeviceNotify (RESULT_RCVDATA, m_dwTapiId, (DWORD)wParam, (LPVOID)lParam, 0L);
    }    
   
    return 1L;
   
}// CMainFrame::OnEmulator

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnTSPICommand
//
// This processes any commands from the Telebridge Service Provider.
//
LRESULT CMainFrame::OnTSPICommand (WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case COMMAND_OPENCONN:
            OnTSPIOpenConn((DWORD)lParam);
            break;
            
        case COMMAND_CLOSECONN:
            OnTSPICloseConn((DWORD)lParam);
            break;
                     
        case COMMAND_WAITINGREQ:                
            DeviceNotify (RESULT_CONTEXTSWITCH, (DWORD)lParam, 0, NULL, 0);
            break;
        
        // New 16-bit providerInit method
        case COMMAND_PERFORMINIT:
            DeviceNotify (RESULT_PERFORMINIT, (DWORD)lParam, 0, NULL, 0);
            break;
                     
        case COMMAND_GETEMULATORSETTINGS:
        case COMMAND_GETEMULATORADDRESSINFO:
        case COMMAND_EMULATORPREPARECALL:
        case COMMAND_EMULATORDROPCALL:
        case COMMAND_GETVERSIONINFO:
        case COMMAND_EMULATORANSWERCALL:
        case COMMAND_EMULATORHOLDCALL:
        case COMMAND_EMULATORUNHOLDCALL:
        case COMMAND_EMULATORFLASH:
        case COMMAND_SETRINGMODE:
            SendEmulatorMessage (g_wCommandTable[wParam-(COMMAND_END+1)], lParam);
            break;
            
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
    switch ((WORD)lpCopyData->dwData)
    {
        case COMMAND_EMULATORTRANSFER:
        case COMMAND_EMULATORFORWARD:
        case COMMAND_EMULATORCONFERENCE:
        case COMMAND_EMULATORDIAL:
        case COMMAND_EMULATORREDIRECT:
        case COMMAND_COMPLETECALL:
        case COMMAND_PARKCALL:
        case COMMAND_UNPARKCALL:
        case COMMAND_SETGAIN:                        
        case COMMAND_SETVOL:
            SendEmulatorMessage (g_wCommandTable[lpCopyData->dwData-(COMMAND_END+1)], 
                                (LPARAM)lpCopyData->lpData);
            break;    
            
        default:
            ASSERT (FALSE);
            break;
    }
                
    return TRUE;        

}// CMainFrame::OnCopyData

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::OnTSPIOpenConn
//
// This method handles an open request from the SP.
//
LRESULT CMainFrame::OnTSPIOpenConn(DWORD dwConnId)
{            
    if (++m_iOpenCount == 1)
    {
        ASSERT (m_dwTapiId == -1L);
        // Strip off the line/phone id leaving only the device (provider) id.
        // We can only service one provider at a time.
        m_dwTapiId = (dwConnId & 0xffff0000);
    }        
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
    ASSERT ((dwConnID & 0xffff0000) == m_dwTapiId);
#else
    dwConnID;
#endif

    if (--m_iOpenCount == 0)
        m_dwTapiId = -1;

}// CMainFrame::OnTSPICloseConn

//////////////////////////////////////////////////////////////////////////////
// CMainFrame::SendEmulatorMessage
//
// Send the emulator a command
//
void CMainFrame::SendEmulatorMessage (WORD wCommand, LPARAM lParam)
{                                  
    ASSERT (m_hwndEmulator);
    ASSERT (IsWindow (m_hwndEmulator));
    ::SendMessage (m_hwndEmulator, WM_DSS_EMULATOR, (WPARAM)wCommand, lParam);

}// CMainFrame::SendEmulatorMessage
