/******************************************************************************/
//                                                                        
// TALKDROP.CPP - Talk/Drop dialog for the ATSP service provider
//                                                                        
// This file contains the dialog definitions and code for the Talk/Drop
// dialog which is used to facilitate action from the user when a
// call is connected using the modem.
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
// Change History
// -------------------------------------------------------------------
// 09/04/94 Mark Smith (MCS)
//      Initial revision.
//
/******************************************************************************/
                       
class CTalkDropDlg : public CDialog
{
// Dialog Data
public:
    enum ReasonCodes { None=1, TalkPressed=2, DropPressed=3 };
private:
    //{{AFX_DATA(CTalkDropDlg)
    enum { IDD = IDD_TALKDROP };
    CTSPIRequest* m_pCurrentRequest;        // Request which cause Talk/Drop
    enum ReasonCodes m_eReason;             // Reason for dismissal
    DWORD m_dwCurrTime;                     // Current time
    CString m_strElapsedTime;               // Elapsed time
    //}}AFX_DATA

// Construction
public:
    CTalkDropDlg(CTSPIRequest* pRequest);

// Implementation
public:
    int GetReasonCode() const;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    // Generated message map functions
    //{{AFX_MSG(CTalkDropDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnDrop();
    afx_msg void OnTalk();
    afx_msg void OnTimer(UINT uidEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
