/*****************************************************************************/
//
// DSSPEXE.H - Digital Switch Service Provider Sample
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

#include "resource.h"

//////////////////////////////////////////////////////////////////////////////
// Executable object
//
class DSSPExeApp : public CWinApp
{
public:
    virtual BOOL InitInstance();
};

//////////////////////////////////////////////////////////////////////////////
// Window frame object
//
class CMainFrame : public CFrameWnd
{
// Class data
private:
    DWORD    m_dwTapiId;       // TAPI identifier
    HWND     m_hwndEmulator;   // Emulator window handle
    int      m_iOpenCount;     // Open count for device

// Constructor
public:
    CMainFrame();
    BOOL WaitForEmulator();
   
// Operations
protected:   
    LRESULT OnTSPIOpenConn(DWORD dwConnID);
    void OnTSPICloseConn(DWORD dwConnID);
    void SendEmulatorMessage (WORD wMessage, LPARAM lParam=0L);

// Message map
protected:   
   //{{AFX_MSG(CMainFrame)
    afx_msg LRESULT OnTSPICommand (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnEmulator (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCopyData (WPARAM wParam, LPARAM lParam);
    afx_msg void OnTimer(UINT nIdEvent);
    afx_msg void OnClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
