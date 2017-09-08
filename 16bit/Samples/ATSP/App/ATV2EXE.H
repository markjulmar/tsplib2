/******************************************************************************/
//
// ATV2EXE.H - Service Provider thread sample application
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

#include "resource.h"
#include "..\sp\atspint.h"

//////////////////////////////////////////////////////////////////////////////
// Executable object
//
class CATExeApp : public CWinApp
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
    DWORD m_dwConnId;       // Connection id which opened port
    int m_iOpenCount;       // Open count
    int m_idComDev;         // Comm device handle

// Constructor
public:
    CMainFrame();
   
// Operations
protected:   
    BOOL ProcessCommBlock (LPSTR lpszBuff, UINT uiMaxLen, BOOL* pfDone);
    BOOL OnTSPIOpenConn(LPSERIALINFO pSI);
    void OnTSPICloseConn(DWORD dwConnID);
    void OnTSPISendStr(LPTSPSENDDATA sendPtr);
    BYTE DecipherReply(LPCSTR lpszData) const;
    void ManageDTR(BOOL fSet);

// Message map
protected:   
    //{{AFX_MSG(CMainFrame)
    afx_msg LRESULT OnTSPICommand (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCommNotify (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCopyData (WPARAM wParam, LPARAM lParam);
    afx_msg void OnTimer(UINT nIdEvent);
    afx_msg void OnClose();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
