/*****************************************************************************/
//
// EMULATOR.CPP - Digital Switch Service Provider Sample
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
#include <mmsystem.h>

#undef SubclassWindow

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
// Globals

CEmulatorApp NEAR theApp;       // Application instance    

const char * g_pszButtonFace[] = {
{ "" }, {"Call %d"}, {"Display"}, 
{""}, {""}, {""}, {""}, {"Xfer"}, 
{"Forw"}, {""}, {""}, {"Conf"}
};

int g_iAddButtonFuncs[] = { 
    BUTTONFUNCTION_VOLUP,           // 28
    BUTTONFUNCTION_VOLDN,           // 29
    BUTTONFUNCTION_HOLD,            // 30
    BUTTONFUNCTION_RELEASE,         // 31
    BUTTONFUNCTION_MSGWAITING       // 32
};    

// These are constants from the WINUSER.H in the Win32 SDK.
#define WS_EX_CONTEXTHELP   0x00000400L
#define SC_CONTEXTHELP      0xF180

///////////////////////////////////////////////////////////////////////////
// CEmulatorApp::InitInstance
//
// This function is the first piece invoked by Windows.  It is 
// responsible for creating a dialog on the frame and starting it.
//
BOOL CEmulatorApp::InitInstance()
{
	// Initialize the common controls
	InitCommonControls();

	// Connect to WinSock
	if (!AfxSocketInit())
	{
		AfxMessageBox (IDS_NOSOCKETS);
		return FALSE;
	}

	// Create our dialog
    CEmulatorDlg Dlg;
    m_pMainWnd = &Dlg;  
    
    // Run the dialog instance.
    Dlg.DoModal();
    
    m_pMainWnd = NULL;
    return FALSE;

}// CEmulatorApp::InitInstance

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::CEmulatorDlg
//
// Constructor for the emulator dialog
//
CEmulatorDlg::CEmulatorDlg (CWnd * pParent /*=NULL*/)
   : CDialog (CEmulatorDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CEmulatorDlg)
    m_strDisplay = "";
    m_fHandsetHookswitch = FALSE;
    m_fMicrophone = FALSE;
    m_fSpeaker = FALSE;
    m_iCursor = 0;
    m_hIcon = AfxGetApp()->LoadIcon (IDR_MAINFRAME);
    m_iActiveFeature = None;
	m_pSocket = NULL;
    m_iDisplayButton = 0;
    //}}AFX_DATA_INIT

    EmptyCallInfo();

}// CEmulatorDlg::CEmulatorDlg

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::~CEmulatorDlg
//
// Destructor
//
CEmulatorDlg::~CEmulatorDlg()
{
	delete m_pSocket;

}// CEmulatorDlg::~CEmulatorDlg

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DoDataExchange
//
// Dialog data exchange
//
void CEmulatorDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_DEBUGINFO, m_lstDebugInfo);
   DDX_Text(pDX, IDC_DISPLAY, m_strDisplay);
   DDX_Check(pDX, IDC_HANDSET, m_fHandsetHookswitch);
   DDX_Check(pDX, IDC_MICROPHONE, m_fMicrophone);
   DDX_Check(pDX, IDC_SPEAKER, m_fSpeaker);
   DDX_Control(pDX, IDC_HOLD, m_btnHold);
   DDX_Control(pDX, IDC_RELEASE, m_btnRelease);
   DDX_Control(pDX, IDC_DIAL, m_btnDial);
   DDX_Control(pDX, IDC_MAKECALL, m_btnIncoming);
   DDX_Control(pDX, IDC_BUSY, m_btnBusy);
   DDX_Control(pDX, IDC_ANSWERED, m_btnAnswered);
   DDX_Control(pDX, IDC_GENERATE, m_btnGenerate);
   DDX_Control(pDX, IDC_GENERATETONE, m_btnGenerateTone);
   DDX_Control(pDX, IDC_VOLUP, m_btnVolUp);
   DDX_Control(pDX, IDC_VOLDN, m_btnVolDn);
   DDX_Control(pDX, IDC_GAINUP, m_btnGainUp);
   DDX_Control(pDX, IDC_GAINDN, m_btnGainDn);
   DDX_Control(pDX, IDC_VOLUMEBAR, m_ctlVolume);
   DDX_Control(pDX, IDC_GAINBAR, m_ctlGain);
   DDX_Control(pDX, IDC_RINGER, m_cbRinger);
   //{{AFX_DATA_MAP(CEmulatorDlg)
   //}}AFX_DATA_MAP
}// CEmulatorDlg::DoDataExchange

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg message map

BEGIN_MESSAGE_MAP(CEmulatorDlg, CDialog)
    //{{AFX_MSG_MAP(CEmulatorDlg)
    ON_WM_CLOSE()
    ON_WM_TIMER()
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_NCHITTEST()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_ANSWERED, OnAnswered)
    ON_BN_CLICKED(IDC_CONTEXT_HELP, OnHelpMe)
    ON_BN_CLICKED(IDC_BUSY, OnBusy)
    ON_BN_CLICKED(IDC_MAKECALL, OnMakecall)
    ON_BN_CLICKED(IDC_MSGWAITING, OnMsgwaiting)
    ON_BN_CLICKED(IDC_RESET, OnReset)
    ON_BN_CLICKED(IDC_HOLD, OnHoldCall)
    ON_BN_CLICKED(IDC_HANDSET, OnHookswitchChange)
    ON_BN_CLICKED(IDC_SPEAKER, OnHookswitchChange)
    ON_BN_CLICKED(IDC_MICROPHONE, OnHookswitchChange)
    ON_BN_CLICKED(IDC_RELEASE, OnReleaseCall)
    ON_BN_CLICKED(IDC_BUTTON0, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON1, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON2, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON3, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON4, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON5, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON6, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON7, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON8, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON9, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON10, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON11, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON12, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON13, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON14, OnPressButton)
    ON_BN_CLICKED(IDC_BUTTON15, OnPressButton)
    ON_BN_CLICKED(IDC_DIAL, OnDial)
    ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
    ON_BN_CLICKED(IDC_GENERATE, OnGenerate)
    ON_BN_CLICKED(IDC_GENERATETONE, OnGenerateTone)
    ON_BN_CLICKED(IDC_VOLUP, OnVolumeUp)
    ON_BN_CLICKED(IDC_VOLDN, OnVolumeDown)
    ON_BN_CLICKED(IDC_GAINUP, OnGainUp)
    ON_BN_CLICKED(IDC_GAINDN, OnGainDown)
    ON_CBN_SELCHANGE(IDC_RINGER, OnRingerChange)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

///////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnInitDialog
//
// This handler processes the WM_INITDIALOG message and initializes our
// emulator window.
//
BOOL CEmulatorDlg::OnInitDialog()
{
    // Perform the dialog data exchange stuff
    CDialog::OnInitDialog();

    // Center the window
    CenterWindow();

    // Set the font for all the controls to be ANSI variable.
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd * pwnd = GetWindow (GW_CHILD);
    while (pwnd && IsChild (pwnd))
    {
        pwnd->SetFont (& fntAnsi);
        pwnd = pwnd->GetWindow (GW_HWNDNEXT);
    }

	// Set the icon
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

    // Set the font for the display to be OEM fixed.
    // Adjust the formatting rectangle for this font so we get our
    // columns/rows
    CFont fntOem;
    fntOem.CreateStockObject (OEM_FIXED_FONT);
    CEdit * pwndEdit = (CEdit*)GetDlgItem (IDC_DISPLAY);
    pwndEdit->SetFont (& fntOem);
    CDC * pDC = pwndEdit->GetDC();
    CString strTest = CString ("W",DISPLAY_COLS);
    CSize sizReq = pDC->GetTextExtent (strTest, DISPLAY_COLS);
    pwndEdit->ReleaseDC (pDC);
    sizReq.cy *= DISPLAY_ROWS;
    sizReq.cy += 5;         
    CRect rcArea;
    pwndEdit->GetClientRect(rcArea);
    if (rcArea.Width() > sizReq.cx)
    {                   
        int iDiff = (rcArea.Width() - sizReq.cx);
        rcArea.left++;
        rcArea.right -= iDiff-1;
    }
    if (rcArea.Height() > sizReq.cy)
    {   
        int iDiff = (rcArea.Height() - sizReq.cy);
        rcArea.top += iDiff/2;
    }
    pwndEdit->SetRectNP (& rcArea);

	// Add our ABOUT box.
	CMenu* pMenu = GetSystemMenu(FALSE);
	if (pMenu != NULL)
		pMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, "About..."); 
    
    // Load the button bitmaps
    m_btnHold.LoadBitmap      (IDB_HOLD);
    m_btnRelease.LoadBitmap   (IDB_RELEASE);
    m_btnDial.LoadBitmap      (IDB_DIAL);
    m_btnIncoming.LoadBitmap  (IDB_INCOMING);
    m_btnBusy.LoadBitmap      (IDB_BUSY);
    m_btnAnswered.LoadBitmap  (IDB_ANSWER);
    m_btnGenerate.LoadBitmap  (IDB_DIGITS);
    m_btnGenerateTone.LoadBitmap (IDB_GENERATE);
    m_btnVolUp.LoadBitmap     (IDB_RIGHT);
    m_btnVolDn.LoadBitmap     (IDB_LEFT);
    m_btnGainUp.LoadBitmap    (IDB_RIGHT);
    m_btnGainDn.LoadBitmap    (IDB_LEFT);
    
    // Setup the progress bars
    m_cbRinger.SetCurSel(0);

    // Load the persistant settings
    LoadINISettings();

    // Start the lamp painting timer.
    SetTimer (TIMER_PAINT_LAMPS, 100, NULL);

	// Set the progress bars
	m_ctlVolume.SetRange(0, 100);
	m_ctlGain.SetRange(0, 100);

    // Init the enabled/disabled state of all our controls.
    OnReset();

    // Now use the proper "show" information
    ShowWindow (AfxGetApp()->m_nCmdShow);

	// Create our server socket.
	m_pSocket = new CListeningSocket(this);
	if (m_pSocket->Create(SOCKET_PORT))
	{
		if (!m_pSocket->Listen())
			AfxMessageBox (IDS_NOSERVER);
	}

    return TRUE;  

}// CEmulatorDlg::OnInitDialog

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnDestroy
//
// Housecleaning for the dialog to go away
//
void CEmulatorDlg::OnDestroy()
{
    // Delete any address appearance objects
    DestroyCallAppearances();

}// CEmulatorDlg::OnDestroy

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DestroyCallAppearances
//
// Destroy all the call appearances.
//
void CEmulatorDlg::DestroyCallAppearances()
{
    // Delete any address appearance objects
    for (int i = 0 ; i < m_arrAppearances.GetSize() ; i++)
    {
        CAddressAppearance * pAddrApp = GetAddress(i);
        delete pAddrApp;
    }
    m_arrAppearances.RemoveAll();
    
    // Delete the lamps
    for (i = 0; i < BUTTON_COUNT; i++)
    {
        CEmulButton* pButton = GetButton(i);
        pButton->Detach();
        if (pButton->m_pLampWnd)
        {
            delete pButton->m_pLampWnd;   
            pButton->m_pLampWnd = NULL;
        }
    }
    
    // Call completion information
    for (i = 0; i < m_arrCompletions.GetSize(); i++)
    {
        COMPLETIONREQ* pReq = (COMPLETIONREQ*) m_arrCompletions[i];
        delete pReq;
    }               
    m_arrCompletions.RemoveAll();
    
    // Park requests
    for (i = 0; i < m_arrParks.GetSize(); i++)
    {
        PARKREQ* pReq = (PARKREQ*) m_arrParks[i];
        delete pReq;
    }               
    m_arrParks.RemoveAll();
    
}// CEmulatorDlg::DestroyCallAppearances

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::SetHookSwitch
//
// Set the hookswitch to a known state
//
void CEmulatorDlg::SetHookSwitch (int /*iHookSwitchDev*/, int iState)
{                              
    CButton* pMaster = (CButton*) GetDlgItem (IDC_HANDSET);
    CButton* pMic = (CButton*) GetDlgItem (IDC_MICROPHONE);
    CButton* pSpkr = (CButton*) GetDlgItem (IDC_SPEAKER);

    if (iState == HSSTATE_ONHOOK)
    {
        pMaster->SetCheck(FALSE);
        pMic->SetCheck(FALSE);
        pSpkr->SetCheck(FALSE);
    }
    else if (iState == HSSTATE_OFFHOOKSPEAKER)
    {
        pMaster->SetCheck(m_fHandsetHookswitch);
        pMic->SetCheck(FALSE);
        pSpkr->SetCheck(TRUE);
    }
    else if (iState == HSSTATE_OFFHOOKMIC)
    {
        pMaster->SetCheck(m_fHandsetHookswitch);
        pMic->SetCheck(TRUE);
        pSpkr->SetCheck(FALSE);
    }
    else if (iState == HSSTATE_OFFHOOKMICSPEAKER)
    {
        pMaster->SetCheck(TRUE);
        pMic->SetCheck(TRUE);
        pSpkr->SetCheck(TRUE);
    }
    
    OnHookswitchChange();
    
}// CEmulatorDlg::SetHookSwitch
      
//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnHookswitchChange
//
// This method responds to the hookswitch change message
//
void CEmulatorDlg::OnHookswitchChange()
{                                   
    BOOL bHandset = m_fHandsetHookswitch;
    BOOL bSpeaker = m_fSpeaker;
    BOOL bMic     = m_fMicrophone;
   
    UpdateData (TRUE);

    EMHOOKSWITCHCHANGE sHSChange;
    sHSChange.wHookswitchID = HSDEVICE_HANDSET;     // our only device for now

    // If the handset hookswitch changed state   
    if (bHandset != m_fHandsetHookswitch)
    {
        if (m_fHandsetHookswitch)
        {
            m_fSpeaker = TRUE;
            m_fMicrophone = TRUE;
        }
        else
        {
            m_fSpeaker = FALSE;
            m_fMicrophone = FALSE;
        }
    }

    // Set the switchhook state
    if (! m_fSpeaker && ! m_fMicrophone)
        sHSChange.wHookswitchState = HSSTATE_ONHOOK;
    else if (m_fSpeaker && ! m_fMicrophone)
        sHSChange.wHookswitchState = HSSTATE_OFFHOOKSPEAKER;
    else if (! m_fSpeaker && m_fMicrophone)
        sHSChange.wHookswitchState = HSSTATE_OFFHOOKMIC;
    else
        sHSChange.wHookswitchState = HSSTATE_OFFHOOKMICSPEAKER;

    // Put out status message
    if (bSpeaker != m_fSpeaker)
        AddDebugInfo (1, m_fSpeaker ? "Spkr on" : "Spkr off");

    if (bMic != m_fMicrophone)
        AddDebugInfo (1, m_fMicrophone ? "Mic on" : "Mic off");

    SendNotification (EMRESULT_HSCHANGED, (LPARAM)&sHSChange);
    UpdateData (FALSE);
   
}// CEmulatorDlg::OnHookswitchChange

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnPaint
//
// Windows will not paint a minimized dialog window properly since 
// painting is done by DefWindowProc which is not called by DefDialogProc
// during the WM_PAINT message.  This code will properly paint our
// icon when minimized.
//
void CEmulatorDlg::OnPaint() 
{
    if (IsIconic() )
    {
        CRect    rect;      // Client rectangle
        CPaintDC dc (this); // device context for painting

        // Erase the background - the above BeginPaint (done by CPaintDC)
        // will not send the ICONERASE message, so send it ourselves.
        SendMessage (WM_ICONERASEBKGND, (WPARAM)dc.GetSafeHdc(), 0);

        // Center the icon within our client rectangle.
        GetClientRect (& rect);
        int cxIcon = GetSystemMetrics (SM_CXICON);
        int cyIcon = GetSystemMetrics (SM_CYICON);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon (x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
        CDC* pDC = GetDC();
        
        CRect rcBox, rcWnd;
        GetDlgItem (IDC_LAMP15)->GetWindowRect(&rcWnd);
        ScreenToClient(&rcWnd);
        rcBox.left = rcWnd.left-1;
        rcBox.top = rcWnd.top-1;
        GetDlgItem (IDC_LAMP0)->GetWindowRect(&rcWnd);
        ScreenToClient(&rcWnd);
        rcBox.right = rcWnd.right+1;
        rcBox.bottom = rcWnd.bottom+1;
		CBrush blkBrush;
		blkBrush.CreateStockObject(BLACK_BRUSH);                                          
        pDC->FrameRect(&rcBox, &blkBrush);      
	}    
        

}// CEmulatorDlg::OnPaint

/////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnQueryDragIcon
//
// The system calls this to obtain the cursor to display while the 
// user drags the minimized window.
// 
// This is also called when the ALT-TAB task-switcher revolves around
// all the icons.
//
HCURSOR CEmulatorDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;

}// CEmulatorDlg

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnSysCommand
//
// Handles SYSCOMMAND messages for our about box
//
void CEmulatorDlg::OnSysCommand (UINT nID, LPARAM lParam)
{
    switch (nID & 0xFFF0)
    {
        case SC_CONTEXTHELP:
            DoContextHelp();
            break;
                 
		case IDM_ABOUTBOX:
		{
			CAboutDlg dlg;
			dlg.DoModal();
			break;
		}                 
                 
        default:
            CDialog::OnSysCommand (nID, lParam);
            break;
    }

}// CEmulatorDlg::OnSysCommand

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnAnswered
//
// This handler processes the "Answered" button which indicates that
// a remote connection picked up the handset to a ring.
//
void CEmulatorDlg::OnAnswered()
{   
    CAddressAppearance* pAddr = FindCallByState(ADDRESSSTATE_ONLINE);
    if (pAddr != NULL)
    {                
        CString strMsg;
        strMsg.Format("Remote Answer <ID = %d>", pAddr->m_wAddressID);
        AddDebugInfo (1, (LPCSTR)strMsg);

        // Set the call state to online
        pAddr->SetState (ADDRESSSTATE_CONNECT);
    }
    else
        AddDebugInfo (1, "Remote Answer Err - no ID");
    
    // Enable/Disable the code generation button based on whether we
    // have a CONNECTED call now.
    UpdateStates();
   
}// CEmulatorDlg::OnAnswered

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnBusy
//
// This handler processes the "Busy" button which indicates that a 
// generated call is "busy".
//
void CEmulatorDlg::OnBusy()
{
    CAddressAppearance* pAddr = FindCallByState(ADDRESSSTATE_ONLINE);
    if (pAddr != NULL)
    {
        CString strMsg;
        strMsg.Format("Remote Busy <ID = %d>", pAddr->m_wAddressID);
        AddDebugInfo (1, (LPCSTR)strMsg);
        
        // Set the call state to busy
        pAddr->SetState (ADDRESSSTATE_BUSY, BUSYTYPE_NORMAL);
    }   
    else
        AddDebugInfo (1, "Remote Busy Err - no ID");

    UpdateStates();
    
}// CEmulatorDlg::OnBusy
                             
/////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnMakecall
//
// This method creates a new call appearance and starts the specified
// line as "ringing".
//
void CEmulatorDlg::OnMakecall()
{   
    CAddressAppearance* pAddr;
    CCallDlg CallDlg (this);

    // Add all the lines which can be used for an incoming call to the call dialog.
    for (int i = 0 ; i < m_arrAppearances.GetSize(); i++)
    {
        pAddr = (CAddressAppearance*)m_arrAppearances[i];
        ASSERT (pAddr != NULL);

        if (pAddr->m_Call.m_iState == ADDRESSSTATE_OFFLINE)
        {
            CString strEntry;
            strEntry.Format("Call #%d <%s>", pAddr->m_wAddressID+1, (LPCSTR)pAddr->m_strNumber);
            CallDlg.AddLine (i, strEntry);
        }
    }
    
    // If there are no lines, then give an error.
    if (CallDlg.GetLineCount() == 0)
    {
        AfxMessageBox ("No available call appearances !");
        return;
    }
    
    // Execute the dialog
    if (CallDlg.DoModal() == IDOK)
    {
        int iDest = CallDlg.GetIncomingLine();
        if (iDest >= 0)
        {
            ASSERT (iDest < m_arrAppearances.GetSize());
            pAddr = GetAddress(iDest);
            ASSERT (pAddr != NULL);
        
            // See if there is caller ID information
            CString strNum  = CallDlg.GetCallerNum();
            CString strName = CallDlg.GetCallerName();

            // Mark the call appearance as FLASHING 
            SetTimer (TIMER_RING, 3000, NULL);
            
            CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
            ASSERT (pButton != NULL);
            pButton->SetLampState(LAMPSTATE_FLASHING);

            // Update the call appearance
            pAddr->m_iRingCount = 0;
            pAddr->m_Call.m_fIncoming = TRUE;
            pAddr->m_Call.m_strDestName = strName;
            pAddr->m_Call.m_strDestNumber = strNum;
            pAddr->m_Call.m_dwMediaModes = CallDlg.GetMediaTypes();

			if (CallDlg.IsExternal())
				pAddr->SetState (ADDRESSSTATE_OFFERINGT);
			else
				pAddr->SetState (ADDRESSSTATE_OFFERING);
            
            // Update the display
            ShowDisplay (pAddr);
        }
    }              
    
    UpdateStates();

}// CEmulatorDlg::OnMakeCall

/////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::ShowDisplay
//
// Show the display for a particular call appearance.
//
void CEmulatorDlg::ShowDisplay (CAddressAppearance * pAddrApp)
{                            
    ASSERT (pAddrApp != NULL);
    CEmulButton* pButton = GetButton(pAddrApp->m_iButtonIndex);
    ASSERT (pButton != NULL);

    // Reset the display - don't fill it in.
    ResetDisplay (FALSE);
    
    // If the address is conferened, then show that.
    if (pAddrApp->m_Call.m_iState == ADDRESSSTATE_INCONF)
    {                          
        CString strBuff;
        m_strDisplay = "CONF: ";
        
        strBuff.Format("%d", pAddrApp->m_wAddressID);
        m_strDisplay += strBuff;
        m_strDisplay += ' ';

        for (POSITION pos = pAddrApp->m_lstConf.GetHeadPosition(); pos != NULL;)
        {
            CString strAddress = pAddrApp->m_lstConf.GetNext(pos);
            if (strAddress.Left(1) == '@')
            {
                CString strNum = strAddress.Mid(1);
                int iAddressID = atoi(strNum);
                CAddressAppearance* pAddrDst = GetAddress(iAddressID);
                ASSERT (pAddrDst != NULL);
                if (pAddrDst)
                {
                    strBuff.Format("%d", pAddrDst->m_wAddressID);
                    m_strDisplay += strBuff;
                    m_strDisplay += ' ';
                }
            }
        }
    }
    else
    {
        // Get the name of this caller
        CString strName = pAddrApp->m_Call.m_strDestName;
        if (strName.IsEmpty())
            strName = "<blank name>";
    
        // Get the number of this caller
        CString strNum = pAddrApp->m_Call.m_strDestNumber;
        if (strNum.IsEmpty())
            strNum = "<no caller-id>";

        m_strDisplay = strName + "\r\n" + strNum;
    }
            
    UpdateData(FALSE);

    // Send a "display" changed message to the provider
    EMDISPLAY Display;
    CopyDisplay((LPSTR)&Display.szDisplay);
    SendNotification (EMRESULT_DISPLAYCHANGED, (LPARAM)&Display);

}// CEmulatorDlg::ShowDisplay

/////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnMsgwaiting
//
// This routine handles the changing of the message waiting lamp.
//
void CEmulatorDlg::OnMsgwaiting()
{   
    BOOL fMsgWaiting = ((CButton*)GetDlgItem(IDC_MSGWAITING))->GetCheck();
    CEmulButton* pButton = GetButton(BTN_MSGWAITING);
    
    AddDebugInfo (1, fMsgWaiting ? "Msg waiting on" : "Msg waiting off");

    // Change the state of the lamp information in the button.
    pButton->SetLampState ((fMsgWaiting) ? LAMPSTATE_STEADY : LAMPSTATE_OFF);
    
}// CEmulatorDlg::OnMsgwaiting

////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnTimer
//
// This method handles all timers sent to our emulator.
//
void CEmulatorDlg::OnTimer (UINT nIdEvent)
{                
	if (nIdEvent == TIMER_RING)
    {   
        OnRing();
        
        // Go through and find all call appearances that are in the offering state
        for (int i = 0 ; i < m_arrAppearances.GetSize() ; i++)
        {
            CAddressAppearance * pAddrApp = GetAddress (i);
            if (pAddrApp->m_Call.m_iState == ADDRESSSTATE_OFFERING)
            {   
                CString strBuff;
                strBuff.Format ("Ring Addr %d <count=%d>", i, pAddrApp->m_iRingCount);
                AddDebugInfo (1, strBuff);

                // Send the ring indication to the SP for this address
                SendNotification(EMRESULT_RING, (LPARAM)pAddrApp->m_wAddressID);
            
                // If this is the second ring, see if there is caller ID info.
                if (++pAddrApp->m_iRingCount == 1)
                {
                    // If we have either caller name or number, send caller ID info. to SP
                    if (!pAddrApp->m_Call.m_strDestName.IsEmpty() ||
                        !pAddrApp->m_Call.m_strDestNumber.IsEmpty())
                    {
                        EMCALLERID sCallerID;
						memset(&sCallerID, 0, sizeof(EMCALLERID));
                        sCallerID.wAddressID = pAddrApp->m_wAddressID;

                        if (!pAddrApp->m_Call.m_strDestNumber.IsEmpty())
                            strncpy (sCallerID.szAddressInfo,
                                     (LPCSTR)pAddrApp->m_Call.m_strDestNumber, ADDRESS_SIZE);
                        if (!pAddrApp->m_Call.m_strDestName.IsEmpty())
                            strncpy (sCallerID.szName, (LPCSTR)pAddrApp->m_Call.m_strDestName, OWNER_SIZE);
                        SendNotification (EMRESULT_CALLERID, (LPARAM)& sCallerID);
                    }
                }
            }
        }
    }
    else if (nIdEvent == TIMER_PAINT_LAMPS)
    {
        for (int i = 0 ; i < BUTTON_COUNT; i++)
        {
            CEmulButton* pButton = GetButton(i);
            if (pButton && pButton->m_pLampWnd)
                pButton->m_pLampWnd->DoTimer();
        }  
        ProcessCallCompletions();              
    }

}// CEmulatorDlg::OnTimer

/////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnReset
//
// This method is called to reset all the lines/calls on the
// phone.
//
void CEmulatorDlg::OnReset()
{
    KillTimer (TIMER_RING);

    m_fHandsetHookswitch = FALSE;
    m_fMicrophone = FALSE;
    m_fSpeaker = FALSE;
    m_iActiveFeature = None;
    m_strFeatureData.Empty();
    m_iVolumeLevel = 32767;
    m_iGainLevel = 32767;
    m_cbRinger.SetCurSel(0);
    
    AddDebugInfo (1, "Emulator reset\r");

    EmptyCallInfo();
    ResetDisplay (TRUE);

    // Reset the lamps
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        CEmulButton* pButton = GetButton(i);
        pButton->SetLampState(LAMPSTATE_OFF);
    }

    UpdateStates();

    m_lstDebugInfo.SetRedraw (FALSE);
    m_lstDebugInfo.ResetContent();
    m_lstDebugInfo.SetRedraw (TRUE);
    UpdateData (FALSE);

}// CEmulatorDlg::OnReset

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnHoldCall
//
// This emulates the holding pattern for the phone
//
void CEmulatorDlg::OnHoldCall()
{   
    OnButtonUpDown (BTN_HOLD);
    CAddressAppearance* pAddr = FindCallByState(ADDRESSSTATE_CONNECT);
    if (pAddr)
        PerformHold (pAddr->m_wAddressID);

}// CEmulatorDlg::OnHoldCall

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PerformHold
//
// Common code to perform a HOLD request
//
int CEmulatorDlg::PerformHold (int iAddressID)
{                            
    CAddressAppearance* pAddr = GetAddress (iAddressID);
    if (pAddr == NULL)
        return EMERROR_INVALADDRESSID;
    if (!pAddr->m_Call.IsActive() &&
         pAddr->m_Call.m_iState != ADDRESSSTATE_INCONF)
        return EMERROR_INVALADDRESSSTATE;

    // Now perform the same operations to THIS call.
    pAddr->m_ConsultationCall = pAddr->m_Call;
    pAddr->SetState (ADDRESSSTATE_ONHOLD, HOLDTYPE_NORMAL);
    CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
    pButton->SetLampState(LAMPSTATE_BLINKING);

    // Turn off our mic/speaker
    SetHookSwitch (HSDEVICE_HANDSET, HSSTATE_ONHOOK);

    // Put out a status message
    UpdateStates();
    return FALSE;
    
}// CEmulatorDlg::PerformHold

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PerformUnhold
//
// Common code to perform a UNHOLD request
//
int CEmulatorDlg::PerformUnhold (int iAddressID)
{
    // See if we have an address for the ID passed
    CAddressAppearance * pAddr = GetAddress (iAddressID);
    if (pAddr == NULL)
        return EMERROR_INVALADDRESSID;
    if (pAddr->m_Call.m_iState != ADDRESSSTATE_ONHOLD &&
        pAddr->m_ConsultationCall.m_iState != ADDRESSSTATE_ONHOLD)
        return EMERROR_INVALADDRESSSTATE;
    
    // If this is a conference-hold, then simply transition the call into CONF.
    // This happens when the consultant call is deleted, and an UNHOLD is performed
    // on a conference call.  This does NOT cancel the conference.
    if (pAddr->m_Call.m_iState == ADDRESSSTATE_ONHOLD &&
        pAddr->m_Call.m_iStateInfo == HOLDTYPE_CONFERENCE &&
        pAddr->m_lstConf.GetCount() > 1)
    {   
        pAddr->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
        pAddr->SetState (ADDRESSSTATE_INCONF);
    }
    else
    {   
        // If this is a conference being unheld with only one call, then
        // revert it to the two-party call which started it.
        if (pAddr->m_Call.m_iState == ADDRESSSTATE_ONHOLD &&
            pAddr->m_Call.m_iStateInfo == HOLDTYPE_CONFERENCE)
        {
            pAddr->SetState (ADDRESSSTATE_OFFLINE);         
            CEmulButton* pButton = FindButtonByFunction (BUTTONFUNCTION_CONFERENCE);
            if (pButton)
                pButton->SetLampState (LAMPSTATE_OFF);
        }
        else if (pAddr->m_Call.m_iState == ADDRESSSTATE_ONHOLD &&
            pAddr->m_Call.m_iStateInfo == HOLDTYPE_TRANSFER)
        {
            CEmulButton* pButton = FindButtonByFunction (BUTTONFUNCTION_TRANSFER);
            if (pButton)
                pButton->SetLampState (LAMPSTATE_OFF);
        }           
        
        // Transfer over the consultation call information.
        pAddr->m_Call = pAddr->m_ConsultationCall;
        pAddr->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
    
        // Transition to the last state of this call.  If it is a consultation
        // call going away, then re-connect the original party
        int iState = pAddr->m_Call.m_iState;
        if (iState == ADDRESSSTATE_ONHOLD)
        {   
            if (pAddr->m_Call.m_iStateInfo == HOLDTYPE_TRANSFER)
            {
                CEmulButton* pButton = FindButtonByFunction (BUTTONFUNCTION_TRANSFER);
                if (pButton)
                    pButton->SetLampState (LAMPSTATE_OFF);
            }
            pAddr->SetState(ADDRESSSTATE_OFFLINE);
            iState = ADDRESSSTATE_CONNECT;        
        }      
        else
        	pAddr->m_Call.m_iState = ADDRESSSTATE_ONHOLD;
        	
        pAddr->SetState (iState);
    }           
    
    CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
    if (pAddr->m_Call.IsActive() || 
        pAddr->m_Call.m_iState == ADDRESSSTATE_INCONF)
    {        
        // Turn on our mic/speaker
        SetHookSwitch (HSDEVICE_HANDSET, HSSTATE_OFFHOOKMICSPEAKER);
        pButton->SetLampState(LAMPSTATE_STEADY);
    }        
    else
        pButton->SetLampState(LAMPSTATE_OFF);        
    
    ShowDisplay (pAddr);
    UpdateStates();
    return FALSE;
    
}// CEmulatorDlg::PerformUnhold    

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnDial
//
// Show a dialog box to input a number for dialing.  This is used
// to simulate dialing on the local side of the call (us).
//
void CEmulatorDlg::OnDial()
{
    CGenerateDlg dlg (this, NULL);
    dlg.DoModal();

}// CEmulatorDlg::OnDial

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DialNumber
//
// This function emulates dialing a number on the phone.
//
int CEmulatorDlg::DialNumber (int iAddress, LPCSTR lpBuff)
{
    // See if we have an address for the ID passed
    CAddressAppearance * pAddr = GetAddress (iAddress);
    if (pAddr != NULL)
    {   
        // Check the state of the address.
        int iState = pAddr->m_Call.m_iState;
        if (iState == ADDRESSSTATE_OFFLINE || iState == ADDRESSSTATE_DISCONNECT)
            return EMERROR_INVALADDRESSSTATE;
        
        char szBuff [ADDRESS_SIZE];
        LPSTR lpDest = szBuff;
        while (*lpBuff)
        {
            if (isdigit (*lpBuff))
                *lpDest++ = *lpBuff;
            lpBuff++;    
        }
        *lpDest = '\0';
        
        // Move the digits to our destination number in the callinfo record
        if (iState == ADDRESSSTATE_DIALTONE || iState == ADDRESSSTATE_ONLINE)
        {
            pAddr->m_Call.m_strDestNumber += szBuff;
            if (iState == ADDRESSSTATE_DIALTONE)
            {                        
                if (pAddr->m_Call.m_iStateInfo == DIALTONETYPE_INTERNAL)
                {
                    if (szBuff[0] == '9')
                        pAddr->SetState (ADDRESSSTATE_DIALTONE, DIALTONETYPE_EXTERNAL);
                    if (szBuff[0] != '9' || strlen (szBuff) > 1)
                        pAddr->SetState (ADDRESSSTATE_ONLINE);
                }
                else
                    pAddr->SetState (ADDRESSSTATE_ONLINE);
            }
            
            ShowDisplay (pAddr);
            UpdateStates();
        }            
    }        
    else
        return EMERROR_INVALADDRESSID;
    return FALSE;

}// CEmulatorDlg::DialNumber

////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::ResetDisplay
//
// Resets our display screen and sets the current cursor position.
//
void CEmulatorDlg::ResetDisplay (BOOL bFill)
{
    m_strDisplay.Empty();
    
    // If we are to fill with the default information then add the user
    // information and date.
    if (bFill)
    { 
        if (m_iActiveFeature == Forward)
        {
            CTime Time = CTime::GetCurrentTime();
            CString strTime;
            strTime.Format ("PHONE IS FORWARDED <%s>\r\n%02d/%02d %02d:%02d:%02d", 
                         (LPCSTR)m_strFeatureData,
                         Time.GetMonth(), Time.GetDay(),
                         Time.GetHour(), Time.GetMinute(), Time.GetSecond());
            m_strDisplay = strTime;
        }
        else
        {
            CTime Time = CTime::GetCurrentTime();
            CString strTime;
            strTime.Format ("%s\r\n%02d/%02d %02d:%02d:%02d", 
                         (LPCSTR)m_strStationName,
                         Time.GetMonth(), Time.GetDay(),
                         Time.GetHour(), Time.GetMinute(), Time.GetSecond());
            m_strDisplay = strTime;
        }            
        
        // Send a "display" changed message to the provider
        EMDISPLAY Display;
        CopyDisplay((LPSTR)&Display.szDisplay);
        SendNotification (EMRESULT_DISPLAYCHANGED, (LPARAM)&Display);
    }

    UpdateData (FALSE);

}// CEmulatorDlg::ResetDisplay

/////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::CopyDisplay
//
// Copy the display into a buffer for the service provider
//
void CEmulatorDlg::CopyDisplay (LPSTR lpBuff)
{   
    int iPos = 0;
                         
    for (int y = 0; y < DISPLAY_ROWS; y++)
    {
        for (int x = 0; x < DISPLAY_COLS; x++)
        {   
            char cChar;
            if (m_strDisplay.GetLength() <= iPos)
                cChar = ' ';
            else
                cChar = m_strDisplay[iPos++];
                              
            if (cChar == '\r')
            {
                iPos++;
                for (int i = x; i < DISPLAY_COLS; i++)
                    *lpBuff++ = ' ';
                x = DISPLAY_COLS;
            }
            else
                *lpBuff++ = cChar;
        }
    }              
    
    *lpBuff = '\0';

}// CEmulatorDlg::CopyDisplay

/////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnPressButton
//
// This handles when a button is pressed on the dialog.
//
void CEmulatorDlg::OnPressButton()
{
    int wId = GetFocus()->GetDlgCtrlID();
    ASSERT (wId >= IDC_BUTTON0 && wId <= IDC_BUTTON15);
    int iButtonID = (wId - IDC_BUTTON0 + FIRST_FUNCTION_INDEX);
    PressFunctionKey (iButtonID);
    UpdateStates();
      
}// CEmulatorDlg::OnPressButton

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PressFunctionKey
//
// This function simulates the pressing of a function key
//
void CEmulatorDlg::PressFunctionKey (int iFunctionKey)
{   
    OnButtonUpDown (iFunctionKey);
    
    // If this is a call appearance, then emulate the phone.
    switch (m_Buttons[iFunctionKey].m_iButtonFunction)
    {
        case BUTTONFUNCTION_CALL:
            PressCallAppearance (iFunctionKey);
            break;

        case BUTTONFUNCTION_DISPLAY:
            PressDisplay (iFunctionKey);
            break;

        case BUTTONFUNCTION_TRANSFER:
            PressTransfer(iFunctionKey);
            break;

        case BUTTONFUNCTION_FORWARD:
            PressForward(iFunctionKey);
            break;
        
        case BUTTONFUNCTION_CONFERENCE:
            AfxMessageBox("This is not functional from the emulator - use a TAPI program.");
            break;
        
        default:
            break;
    }

}// CEmulatorDlg::PressFunctionKey

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PressForward
//
// Either forward or unforward the phone.
//
void CEmulatorDlg::PressForward(int iFunctionKey)
{                             
    CString strBuff;
    CEmulButton* pButton = GetButton(iFunctionKey);

    if (m_iActiveFeature == Forward)
    {
        m_iActiveFeature = None;
        m_strFeatureData.Empty();
        pButton->SetLampState(LAMPSTATE_OFF);
    }
    else
    {
        // There can be no active call appearances.
        for (int i = 0; i < m_arrAppearances.GetSize(); i++)
        {
            CAddressAppearance* pAddr = GetAddress(i);
            if (pAddr->m_Call.IsActive())
            {   
                strBuff.Format("Call %d is ACTIVE", pAddr->m_wAddressID);              
                AddDebugInfo(1, strBuff);
                return;
            }
        }
        
        m_iActiveFeature = Forward;
        m_strFeatureData = "555-1212";
        pButton->SetLampState(LAMPSTATE_BLINKING);
    }

    ResetDisplay (TRUE);
    UpdateStates();
    
}// CEmulatorDlg::PressForward

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PressTransfer
//
// Transfer the current active call
//
void CEmulatorDlg::PressTransfer (int iFunctionKey)
{                             
    CString strBuff;
    CEmulButton* pTransButton = GetButton(iFunctionKey);
    pTransButton->SetLampState(LAMPSTATE_BLINKING);

    // Locate an active call appearance.
    CAddressAppearance* pAddr = FindCallByState(ADDRESSSTATE_CONNECT);
    if (pAddr != NULL)
    {   
        CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
    
        // Transition the call appearance to onHold pending transfer.
        pAddr->SetState(ADDRESSSTATE_ONHOLD, HOLDTYPE_TRANSFER);
        
        // Now transition it to OFFLINE indicating transfer.
        pAddr->SetState(ADDRESSSTATE_OFFLINE);        
        EmptyCallInfo (pAddr->m_wAddressID);
        pButton->SetLampState(LAMPSTATE_OFF);
    }
    else
        AddDebugInfo(1, "No active call");

    pTransButton->SetLampState(LAMPSTATE_OFF);
    ResetDisplay (TRUE);
    UpdateStates();
    
}// CEmulatorDlg::PressForward

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PressCallAppearance
//
// Press a button associated with a call appearance.
//
void CEmulatorDlg::PressCallAppearance (int iKey)
{
    // Activate a new indicator
    CAddressAppearance * pAddrApp = FindCallByButtonID (iKey);
    CEmulButton * pButton = GetButton (iKey);
    
    if (pAddrApp)
    {
        // See if the Display feature is active
        if (m_iActiveFeature == Display)
        {
            // Show the display info. for this call appearance if the address
            // state is other than unknown or offline
            if (pAddrApp->m_Call.m_iState != ADDRESSSTATE_OFFLINE &&
              pAddrApp->m_Call.m_iState != ADDRESSSTATE_UNKNOWN)
                ShowDisplay (pAddrApp);
            m_iActiveFeature = None;
            // Turn the Display function button flashing off
            CEmulButton * pButton = GetButton (m_iDisplayButton);
            if (pButton)
                pButton->SetLampState (LAMPSTATE_OFF);
            return;
        }

        // If there is a consultation call placed on this address, then
        // cancel it.
        if (pAddrApp->m_ConsultationCall.m_iState == ADDRESSSTATE_ONHOLD)
        {
            pAddrApp->m_Call = pAddrApp->m_ConsultationCall;
            pAddrApp->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
        }
        
        switch (pAddrApp->m_Call.m_iState)
        {                             
            case ADDRESSSTATE_OFFLINE:
                pAddrApp->SetState(ADDRESSSTATE_DIALTONE, DIALTONETYPE_INTERNAL);
                pButton->SetLampState(LAMPSTATE_STEADY);
                ShowDisplay (pAddrApp);
                break;

            case ADDRESSSTATE_ONHOLD:
                pAddrApp->SetState(ADDRESSSTATE_CONNECT);
                pButton->SetLampState(LAMPSTATE_STEADY);
                ShowDisplay (pAddrApp);
                break;
            
            case ADDRESSSTATE_OFFERING:
                UpdateStates();
                pAddrApp->SetState(ADDRESSSTATE_CONNECT);
                pButton->SetLampState(LAMPSTATE_STEADY);
                if (FindCallByState(ADDRESSSTATE_OFFERING) == NULL)
                    KillTimer (TIMER_RING);
                ShowDisplay (pAddrApp);
                break;
            
            default:
                break;        
        }
        
        // If there is a completion request pending on this address, then
        // transition to the CONNECT state and complete the request.
        for (int i = 0; i < m_arrCompletions.GetSize(); i++)
        {
            COMPLETIONREQ* pComplReq = (COMPLETIONREQ*) m_arrCompletions[i];
            if (pComplReq->wAddressID == pAddrApp->m_wAddressID)
            {
                if (pComplReq->wComplType == CALLCOMPLTYPE_CAMP)
                {
                    m_arrCompletions.RemoveAt(i--);
                    SendNotification (EMRESULT_COMPLRESULT, MAKEERR(pComplReq->wAddressID,(WORD)(LONG)pComplReq));
                    pAddrApp->SetState (ADDRESSSTATE_CONNECT);
                    delete pComplReq;
                }
            }
        }
    }

    // Kill any other active call appearances.
    for (int i = 0; i < m_arrAppearances.GetSize(); i++)
    {                                                 
        CAddressAppearance* pOldAddr = GetAddress(i);
        if (pOldAddr != pAddrApp)
        {
            if (pOldAddr->m_Call.IsActive())
            {
                pOldAddr->SetState(ADDRESSSTATE_OFFLINE);
                CEmulButton* pButton = GetButton(pOldAddr->m_iButtonIndex);
                pButton->SetLampState(LAMPSTATE_OFF);
            }                
        }
    }

    // If the current call appearance is active, then change the hookswitch
    // state if necessary.
    BOOL fActiveCalls = (pAddrApp && pAddrApp->m_Call.IsActive());
    BOOL fMic = m_fMicrophone;
    BOOL fSpkr = m_fSpeaker;

    EMHOOKSWITCHCHANGE sHSChange;
    sHSChange.wHookswitchID = HSDEVICE_HANDSET;

    // If we have killed all our lines..
    if (!fActiveCalls)
    {
        m_fMicrophone = FALSE;
        m_fSpeaker = FALSE;
        m_fHandsetHookswitch = FALSE;
    
        if (m_fMicrophone != fMic)
            AddDebugInfo (1, "Mic off\r");
        if (m_fSpeaker != fSpkr)
            AddDebugInfo (1, "Spkr off\r");
        
        if (m_fMicrophone != fMic || m_fSpeaker != fSpkr)
        {
            sHSChange.wHookswitchState = HSSTATE_ONHOOK;
            SendNotification (EMRESULT_HSCHANGED, (LPARAM)& sHSChange);
        }            
    }
    else
    {
        m_fMicrophone = TRUE;
        m_fSpeaker = TRUE;
        m_fHandsetHookswitch = TRUE;
        
        if (m_fMicrophone != fMic)
            AddDebugInfo (1, "Mic on\r");
        if (m_fSpeaker != fSpkr)
            AddDebugInfo (1, "Spkr on\r");
        
        if (m_fMicrophone != fMic || m_fSpeaker != fSpkr)
        {
            sHSChange.wHookswitchState = HSSTATE_OFFHOOKMICSPEAKER;
            SendNotification (EMRESULT_HSCHANGED, (LPARAM)& sHSChange);
        }            
    }

    UpdateData (FALSE);

}// CEmulatorDlg::PressCallAppearance

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PressDisplay
//
// Press the button associated with the display key.
//
void CEmulatorDlg::PressDisplay (int iFunctionKey)
{
    CEmulButton* pButton = GetButton (iFunctionKey);

    if (m_iActiveFeature == Display)
    {
        m_iActiveFeature = None;
        m_strFeatureData.Empty();
        pButton->SetLampState (LAMPSTATE_OFF);
    }
    else
    {
        m_iDisplayButton = iFunctionKey;
        m_iActiveFeature = Display;
        pButton->SetLampState (LAMPSTATE_BLINKING);
    }

}// CEmulatorDlg::PressDisplay

///////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnReleaseCall
//
// This function is called in response to a RELEASE
//
void CEmulatorDlg::OnReleaseCall()
{   
    OnButtonUpDown (BTN_DROP);
    
    // See if we have an active call
    for (int i = 0 ; i < m_arrAppearances.GetSize() ; i++)
    {
        CAddressAppearance * pAddrApp = GetAddress(i);
        if (pAddrApp && pAddrApp->m_Call.IsActive())
        {
            PerformDrop(i);
            break;
        }
    }
    
}// CEmulatorDlg::OnReleaseCall

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PerformDrop
//
// Common code to drop an address.
//
int CEmulatorDlg::PerformDrop(int iAddress)
{   
    CAddressAppearance* pAddr = GetAddress(iAddress);
    if (pAddr == NULL)
        return EMERROR_INVALADDRESSID;

    // Get the current call state
    int iState = pAddr->m_Call.m_iState;
    if (iState == ADDRESSSTATE_OFFLINE)
        return EMERROR_INVALADDRESSSTATE;

    // Check to see if the consultation call is on-hold pending a transfer or
    // conference event.  If so, then transition the address to that.
    if (pAddr->m_ConsultationCall.m_iState == ADDRESSSTATE_ONHOLD &&
        (pAddr->m_ConsultationCall.m_iStateInfo == HOLDTYPE_TRANSFER ||
         pAddr->m_ConsultationCall.m_iStateInfo == HOLDTYPE_CONFERENCE))
    {   
        // Move it offline first so DSSP drops the consultation call properly.
        pAddr->SetState (ADDRESSSTATE_OFFLINE);
        
        // Use the state of the original call now.
        pAddr->m_Call = pAddr->m_ConsultationCall;
        pAddr->m_Call.m_iState = ADDRESSSTATE_OFFLINE;
        pAddr->SetState(pAddr->m_ConsultationCall.m_iLastState);

		// If this was a TRANSFER, then turn off the button.
		if (pAddr->m_ConsultationCall.m_iStateInfo == HOLDTYPE_TRANSFER)
		{
    		CEmulButton* pTransButton = FindButtonByFunction(BUTTONFUNCTION_TRANSFER);
    		if (pTransButton)
	        	pTransButton->SetLampState(LAMPSTATE_OFF);
		}
        
        // Reset the state of the consultant call.
        pAddr->m_ConsultationCall.m_iState = pAddr->m_ConsultationCall.m_iLastState = ADDRESSSTATE_UNKNOWN;
        pAddr->m_ConsultationCall.m_iStateInfo = 0;
    }
    
    // Otherwise, see if the active call IS a consultation call in response to 
    // a transfer or conference.
    else if (pAddr->m_ConsultationCall.m_iState == ADDRESSSTATE_ONHOLD &&
        pAddr->m_ConsultationCall.m_iStateInfo == HOLDTYPE_NORMAL)
    {                               
        // Disconnect the active call
        if (iState == ADDRESSSTATE_CONNECT)
            pAddr->SetState(ADDRESSSTATE_DISCONNECT);
    
    	// Dropping the original call - kills BOTH calls.
        // Move it offline first so DSSP drops the consultation call properly.
        pAddr->SetState (ADDRESSSTATE_OFFLINE);
        
        // Use the state of the original call now.
        pAddr->m_Call = pAddr->m_ConsultationCall;
        pAddr->m_Call.m_iState = ADDRESSSTATE_OFFLINE;
        pAddr->SetState(pAddr->m_ConsultationCall.m_iLastState);

		// If this was a TRANSFER, then turn off the button.
		CEmulButton* pTransButton = FindButtonByFunction(BUTTONFUNCTION_TRANSFER);
		if (pTransButton)
        	pTransButton->SetLampState(LAMPSTATE_OFF);
        
        // Reset the state of the consultant call.
        pAddr->m_ConsultationCall.m_iState = pAddr->m_ConsultationCall.m_iLastState = ADDRESSSTATE_UNKNOWN;
        pAddr->m_ConsultationCall.m_iStateInfo = 0;

		// Drop consultation call too.
        pAddr->SetState (ADDRESSSTATE_OFFLINE);
        
		// Turn off the lamp associated with the button.        
        CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
        pButton->SetLampState(LAMPSTATE_OFF);
    }
     
    // Otherwise it is a simple drop
    else
    {                                
        // Disconnect the active call
        if (iState == ADDRESSSTATE_CONNECT)
            pAddr->SetState(ADDRESSSTATE_DISCONNECT);
            
		// Transition the call to the OFFLINE state.            
        pAddr->SetState(ADDRESSSTATE_OFFLINE);
        
        // If it was offering, the kill the ring timer.
        if (iState == ADDRESSSTATE_OFFERING &&
            FindCallByState(ADDRESSSTATE_OFFERING) == NULL)
            KillTimer(TIMER_RING);        

		// Turn off the lamp associated with the button.        
        CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
        pButton->SetLampState(LAMPSTATE_OFF);
    }
    
    // If the call is not offline, then update the display with the
    // correct information.
    if (pAddr->m_Call.m_iState != ADDRESSSTATE_OFFLINE)
        ShowDisplay (pAddr);
    else        
    {
        EmptyCallInfo(pAddr->m_wAddressID);
        ResetDisplay(TRUE);
        SetHookSwitch (HSDEVICE_HANDSET, HSSTATE_ONHOOK);
    }   
     
    UpdateStates();
    return FALSE;
    
}// CEmulatorDlg::PerformDrop

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::GetAddress
//                         
// Return the CAddressAppearance at the specified array position.
//
CAddressAppearance * CEmulatorDlg::GetAddress (int iAddressID)
{
    CAddressAppearance * pAddrApp = NULL;
    if (iAddressID >= 0 && iAddressID < m_arrAppearances.GetSize())
        pAddrApp = (CAddressAppearance *)m_arrAppearances.GetAt (iAddressID);
    return pAddrApp;

}// CEmulatorDlg::GetAddress

//////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::AddDebugInfo
//
// Add a string to the listbox indicating response/command
//
void CEmulatorDlg::AddDebugInfo (BYTE bType, LPCSTR lpszBuff)
{
    m_lstDebugInfo.AddString ((char)('0' + bType), lpszBuff);

}// CEmulatorDlg::AddDebugInfo

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnConfigure
//
// Bring up the configuration dialog
//
void CEmulatorDlg::OnConfigure()
{
    // Instantiate a property sheet for the configuration dialogs
    CBasePropertySheet dlgProp ("Emulator Configuration", this);

    // Instantiate a general setup dialog and add it to the property sheet
    CGenSetupDlg dlgGenSetup;
    CConfigDlg dlgConfig;
    CAddrSetDlg dlgAddrSetup;

    dlgProp.AddPage (&dlgGenSetup);
    dlgProp.AddPage (&dlgConfig);
    dlgProp.AddPage (&dlgAddrSetup);

    if (dlgProp.DoModal() == IDOK)
    {
        // Delete any address appearance objects
        DestroyCallAppearances();
        
        // Reload the button information.
        LoadINISettings();
    }
    else // Reset settings
        WriteINISettings();

}// CEmulatorDlg::OnConfigure

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnGenerate
//
// Bring up the generate code dialog
//
void CEmulatorDlg::OnGenerate()
{                
    CAddressAppearance* pAddr = FindCallByState(ADDRESSSTATE_CONNECT);
    if (pAddr == NULL)
    {
        AddDebugInfo(1, "No active call");
        return;
    }

    CGenerateDlg dlg (this, pAddr);
    dlg.DoModal();

}// CEmulatorDlg::OnGenerate

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnGenerateTone
//
// Bring up the generate tone dialog
//
void CEmulatorDlg::OnGenerateTone()
{                
    CAddressAppearance* pAddr = FindCallByState(ADDRESSSTATE_CONNECT);
    if (pAddr == NULL)
        pAddr = FindCallByState(ADDRESSSTATE_ONLINE);
        
    if (pAddr == NULL)
    {
        AddDebugInfo(1, "No active call");
        return;
    }

    CGenTone dlg (this, pAddr);
    dlg.DoModal();

}// CEmulatorDlg::OnGenerateTone

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::FindCallByButtonID
//
// Return the call appearance object pointer for the button ID passed in.
//
CAddressAppearance* CEmulatorDlg::FindCallByButtonID (int iButtonID)
{
    for (int i = 0 ; i < m_arrAppearances.GetSize() ; i++)
    {
        CAddressAppearance* pAddrApp = GetAddress(i);
        if (pAddrApp->m_iButtonIndex == iButtonID)
            return pAddrApp;
    }
    return NULL;

}// CEmulatorDlg::FindCallByButtonID

///////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::FindCallByState
//
// Locate a call in a particular state
//
CAddressAppearance* CEmulatorDlg::FindCallByState(int iState)
{                               
    // Go through all the addresses and find a call which is active.
    for (int i = 0; i < m_arrAppearances.GetSize(); i++)
    {
        CAddressAppearance* pAddr = GetAddress(i);
        if (pAddr->m_Call.m_iState == iState)
            return pAddr;
    }
    return NULL;

}// CEmulatorDlg::FindCallByState

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::GetButton
//
// Return a button index
//
CEmulButton* CEmulatorDlg::GetButton(int iPos)
{                          
    ASSERT (iPos >= 0 && iPos < BUTTON_COUNT);
    return &m_Buttons[iPos];
    
}// CEmulatorDlg::GetButton

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::EmptyCallInfo
//
// Public function to clear a single or all call appearance objects
//
void CEmulatorDlg::EmptyCallInfo ()
{
    for (int i = 0 ; i < m_arrAppearances.GetSize() ; i++)
        EmptyCallInfo(i);

}// CEmulatorDlg::EmptyCallInfo

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::EmptyCallInfo
//
// Empty the call information for a single call
//
void CEmulatorDlg::EmptyCallInfo (int iIndex)
{                              
    if (iIndex >= 0)
    {
        CAddressAppearance * pAddrApp = GetAddress(iIndex);
        if (pAddrApp)
        {
            pAddrApp->m_iRingCount = 0;
            pAddrApp->m_Call.m_fIncoming = FALSE;
            pAddrApp->m_Call.m_strDestName.Empty();
            pAddrApp->m_Call.m_strDestNumber.Empty();
            pAddrApp->m_Call.m_iState = ADDRESSSTATE_OFFLINE;
            pAddrApp->m_Call.m_iStateInfo = 0;
            pAddrApp->m_Call.m_dwMediaModes = MEDIAMODE_INTERACTIVEVOICE;
            pAddrApp->m_ConsultationCall.m_iState = ADDRESSSTATE_UNKNOWN;
        }
    }

}// CEmulatorDlg::EmptyCallInfo

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::WriteINISettings
//
// Reset the INI settings from our data when configuration is canceled.
//
void CEmulatorDlg::WriteINISettings()
{
    // Set the station name from the INI file
    AfxGetApp()->WriteProfileString ("General", "Name", m_strStationName);

    // Set all the buttons    
    for (int i = 0; i < 16; i++)
    {
        CEmulButton* pButton = GetButton(i+FIRST_FUNCTION_INDEX);
        CString strKey;
        strKey.Format ("Key%d", i+FIRST_FUNCTION_INDEX);
        AfxGetApp()->WriteProfileInt ("Buttons", strKey, pButton->m_iButtonFunction);
        
        CAddressAppearance * pAddr = FindCallByButtonID(i+FIRST_FUNCTION_INDEX);
        if (pAddr)
        {
            strKey.Format("Call%d", pAddr->m_wAddressID+1);
            AfxGetApp()->WriteProfileString ("Addresses", strKey, pAddr->m_strNumber);
        }
    }

}// CEmulatorDlg::WriteINISettings

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::LoadINISettings
//
// Setup the persistant settings.
//
void CEmulatorDlg::LoadINISettings()
{ 
    int iCall = 1;

    // Setup all the buttons    
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        CEmulButton* pButton = GetButton(i);
        pButton->m_iLampButtonID = i;
        
        // If this is the keypad information...
        if (i < FIRST_FUNCTION_INDEX)
        {
            pButton->m_iButtonFunction = BUTTONFUNCTION_DIGIT;
            pButton->m_pLampWnd = NULL;
        }   
        
        // If this is a FUNCTION button, then load it from our .INI file
        // and connect up the lamp.
        else if (i >= FIRST_FUNCTION_INDEX && i <= LAST_FUNCTION_INDEX)
        {
            CString strKey;
            strKey.Format ("Key%d", i);
            int iType = AfxGetApp()->GetProfileInt ("Buttons", strKey, BUTTONFUNCTION_NONE);
            if (iType < BUTTONFUNCTION_NONE || iType > BUTTONFUNCTION_CONFERENCE)
            {
                ASSERT (FALSE);
                iType = BUTTONFUNCTION_NONE;
            }
        
            pButton->m_iButtonFunction = iType;
        
            CString strText = g_pszButtonFace[iType];
            if (iType == BUTTONFUNCTION_CALL)
            {
                // Instantiate a call appearance object for this call button
                CAddressAppearance * pAddrApp = new CAddressAppearance;
                if (pAddrApp)
                {                           
                    pAddrApp->m_wAddressID = (WORD)iCall - 1;
                    strKey.Format ("Call%d", iCall);
                    pAddrApp->m_strNumber = AfxGetApp()->GetProfileString ("Addresses", strKey, "");
                    pAddrApp->m_iButtonIndex = i;
                    pAddrApp->m_Call.m_iState = ADDRESSSTATE_OFFLINE;

                    // Add this call appearance to the array
                    m_arrAppearances.Add (pAddrApp);
                }
                strText.Format(g_pszButtonFace[iType], iCall++);
            }

            // Attach the button to the function itself
            pButton->Attach(GetDlgItem(IDC_BUTTON0+i-FIRST_FUNCTION_INDEX)->GetSafeHwnd());
        
            // Set the text of our button.
            pButton->SetWindowText(strText);
            pButton->EnableWindow (!strText.IsEmpty());
        
            // Setup the lamp for this button
            pButton->m_pLampWnd = new CLampWnd;
            if (i-FIRST_FUNCTION_INDEX < 8)
                pButton->m_pLampWnd->m_Direction = CLampWnd::Right;

            pButton->m_pLampWnd->SubclassWindow (GetDlgItem(IDC_LAMP0+i-FIRST_FUNCTION_INDEX)->GetSafeHwnd());
            pButton->m_pLampWnd->Invalidate(TRUE);
        }
        else // (i > LAST_FUNCTION_INDEX)
        {
            pButton->m_iButtonFunction = g_iAddButtonFuncs[i-LAST_FUNCTION_INDEX-1];
            if (pButton->m_iButtonFunction == BUTTONFUNCTION_MSGWAITING)
                pButton->m_pLampWnd = new CLampWnd;
        }
    }
   
    // Get the station name from the INI file
    m_strStationName = AfxGetApp()->GetProfileString ("General", "Name", "");
    ResetDisplay (TRUE);

}// CEmulatorDlg::LoadINISettings

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::UpdateStates
//
// Update the enabled/disable states of all our buttons.
//
void CEmulatorDlg::UpdateStates()
{                             
    BOOL fActive = FALSE;
    BOOL fConnection = FALSE;
    BOOL fOffering = FALSE;
    BOOL fAnswer = FALSE;
    BOOL fDial = FALSE;
    
    for (int i = 0; i < m_arrAppearances.GetSize(); i++)
    {                                                 
        CAddressAppearance* pAddr = GetAddress(i);
        if (pAddr->m_Call.IsActive())
            fActive = TRUE;
        if (pAddr->m_Call.m_iState == ADDRESSSTATE_CONNECT)
            fConnection = TRUE;
        if (pAddr->m_Call.m_iState == ADDRESSSTATE_OFFERING)
            fOffering = TRUE;
        if (pAddr->m_Call.m_iState == ADDRESSSTATE_ONLINE)
            fAnswer = TRUE;            
        if (pAddr->m_Call.m_iState == ADDRESSSTATE_DIALTONE ||            
            pAddr->m_Call.m_iState == ADDRESSSTATE_ONLINE)
            fDial = TRUE;            
    }

    if (m_iActiveFeature == Forward)
    {
        m_btnHold.EnableWindow (FALSE);
        m_btnRelease.EnableWindow (FALSE);
        m_btnDial.EnableWindow (FALSE);
        m_btnBusy.EnableWindow (FALSE);
        m_btnAnswered.EnableWindow (FALSE);
        m_btnGenerate.EnableWindow (FALSE);
        m_btnIncoming.EnableWindow (FALSE);
        
        for (int i = 0; i < m_arrAppearances.GetSize(); i++)
        {                                                 
            CAddressAppearance* pAddr = GetAddress(i);
            CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
            pButton->EnableWindow (FALSE);
        }            
    }
    else
    {
        m_btnHold.EnableWindow (fConnection);
        m_btnRelease.EnableWindow (fActive);
        m_btnDial.EnableWindow (fConnection || fDial);
        m_btnBusy.EnableWindow (fAnswer);
        m_btnAnswered.EnableWindow (fAnswer);
        m_btnGenerate.EnableWindow (fConnection);
        m_btnGenerateTone.EnableWindow (fConnection || fAnswer);
        m_btnIncoming.EnableWindow (TRUE);
        
        for (int i = 0; i < m_arrAppearances.GetSize(); i++)
        {                                                 
            CAddressAppearance* pAddr = GetAddress(i);
            CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
            pButton->EnableWindow (TRUE);
        }            
    }        

    GetDlgItem(IDC_CONFIGURE)->EnableWindow (m_connectionList.IsEmpty());

    // Set the volume/gain to the proper positions.
    m_ctlGain.SetPos(((DWORD)m_iGainLevel*100 / 0xffffL));
    m_ctlVolume.SetPos(((DWORD)m_iVolumeLevel*100 / 0xffffL));
    
}// CEmulatorDlg::UpdateStates

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnButtonUpDown
//
// Send a BUTTON STATE up/down notification to the service provider.
//
void CEmulatorDlg::OnButtonUpDown (int iButtonLampID)
{                               
    CString strBuff;
    strBuff.Format("Button %d <DOWN/UP>", iButtonLampID);
    AddDebugInfo (1, strBuff);
        
    EMBUTTONCHANGE ButtonChange;
    ButtonChange.wButtonLampID = (WORD) iButtonLampID;
    ButtonChange.wButtonState = BUTTONSTATE_DOWN;
    SendNotification (EMRESULT_BUTTONCHANGED, (LPARAM)&ButtonChange);
    ButtonChange.wButtonState = BUTTONSTATE_UP;
    SendNotification (EMRESULT_BUTTONCHANGED, (LPARAM)&ButtonChange);

}// CEmulatorDlg::OnButtonUpDown

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnVolumeUp
//
// Volume has changed, set new volume
//
void CEmulatorDlg::OnVolumeUp()
{   
    OnButtonUpDown (BTN_VOLUP);
    int iScale = min (6500, 0xffff-m_iVolumeLevel);
    OnVolumeChanged (m_iVolumeLevel+iScale);

}// CEmulatorDlg::OnVolumeUp

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnVolumeDown
//
// Volume has changed, set new volume
//
void CEmulatorDlg::OnVolumeDown()
{
    OnButtonUpDown (BTN_VOLDOWN);
    int iScale = min (6500, m_iVolumeLevel);
    OnVolumeChanged (m_iVolumeLevel-iScale);

}// CEmulatorDlg::OnVolumeDown

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnGainUp
//
// Gain has changed, set new gain
//
void CEmulatorDlg::OnGainUp()
{
    int iScale = min (6500, 0xffff-m_iGainLevel);
    OnGainChanged (m_iGainLevel+iScale);
    
}// CEmulatorDlg::OnGainUp

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnGainDown
//
// Gain has changed, set new gain
//                         
void CEmulatorDlg::OnGainDown()
{
    int iScale = min (6500, m_iGainLevel);
    OnGainChanged (m_iGainLevel-iScale);

}// CEmulatorDlg::OnGainDown

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnGainChanged
//
// Gain has changed, set new gain
//
void CEmulatorDlg::OnGainChanged(UINT uiNewGain)
{
    if (uiNewGain != m_iGainLevel)
    {
        CString strBuff;
        strBuff.Format("Gain <0x%x>", uiNewGain);
        AddDebugInfo (1, strBuff);
    
        m_iGainLevel = uiNewGain;
        
        m_btnGainDn.EnableWindow (m_iGainLevel > 0);
        m_btnGainUp.EnableWindow (m_iGainLevel < 0xffff);
        
        UpdateStates();
        EMLEVELCHANGE emChange;
        emChange.wLevelType = LEVELTYPE_MIC;
        emChange.wLevel = (WORD) uiNewGain;
        SendNotification (EMRESULT_LEVELCHANGED, (LPARAM)&emChange);
    }

}// CEmulatorDlg::OnGainChanged

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnVolumeChanged
//
// Volume has changed, set new volume
//
void CEmulatorDlg::OnVolumeChanged(UINT uiNewVol)
{
    if (uiNewVol != m_iVolumeLevel)
    {
        CString strBuff;
        strBuff.Format("Volume <0x%x>", uiNewVol);
        AddDebugInfo (1, strBuff);

        m_iVolumeLevel = uiNewVol;
        m_btnVolDn.EnableWindow (m_iVolumeLevel > 0);
        m_btnVolUp.EnableWindow (m_iVolumeLevel < 0xffff);

        UpdateStates();
        EMLEVELCHANGE emChange;
        emChange.wLevelType = LEVELTYPE_SPEAKER;
        emChange.wLevel = (WORD) uiNewVol;
        SendNotification (EMRESULT_LEVELCHANGED, (LPARAM)&emChange);
    }

}// CEmulatorDlg::OnVolumeChanged

//////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::PerformDropConf
//
// Drop a conference and reset the conference buttons.
//
void CEmulatorDlg::PerformDropConf(CAddressAppearance* pAddr)
{            
    CEmulButton* pButton;                    

    // Destroy the conference on this address. All calls are removed.
    if (pAddr->m_Call.m_iState == ADDRESSSTATE_INCONF)
    {
        for (POSITION pos = pAddr->m_lstConf.GetHeadPosition(); pos != NULL;)
        {
            CString strAddress = pAddr->m_lstConf.GetNext(pos);
            if (strAddress.Left(1) == '@')
            {
                CString strNum = strAddress.Mid(1);
                int iAddressID = atoi(strNum);
                CAddressAppearance* pAddrDst = GetAddress(iAddressID);
                ASSERT (pAddrDst != NULL);
                if (pAddrDst)
                {
                    pAddrDst->m_lstConf.RemoveAll();
                    pAddrDst->SetState (ADDRESSSTATE_OFFLINE);
                    EmptyCallInfo (pAddrDst->m_wAddressID);
                    pButton = GetButton(pAddrDst->m_iButtonIndex);
                    pButton->SetLampState(LAMPSTATE_OFF);
                }
            }
        }
    
        pAddr->m_lstConf.RemoveAll();
        pAddr->SetState(ADDRESSSTATE_OFFLINE);
        EmptyCallInfo(pAddr->m_wAddressID);
        pButton = GetButton(pAddr->m_iButtonIndex);
        pButton->SetLampState(LAMPSTATE_OFF);
    }        

    pButton = FindButtonByFunction(BUTTONFUNCTION_CONFERENCE);
    if (pButton != NULL)
        pButton->SetLampState(LAMPSTATE_OFF);

}// CEmulatorDlg::PerformDropConf

//////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::FindButtonByFunction
//
// Locate a button in our button array.
//
CEmulButton* CEmulatorDlg::FindButtonByFunction(int iFunction)
{
    for (int i = 0; i < BUTTON_COUNT; i++)
    {   
        CEmulButton* pButton = GetButton(i);
        if (pButton->m_iButtonFunction == iFunction)
            return pButton;
    }
    return NULL;
    
}// CEmulatorDlg::FindButtonByFunction

//////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::SetRingStyle
//
// Set the current ringer style
//
void CEmulatorDlg::SetRingStyle (WORD wStyle)
{                             
    CString strBuff;
    strBuff.Format("SP RingMode <0x%x>", wStyle);
    AddDebugInfo (0, strBuff);

    if (wStyle <= RINGER_NONE)
        m_cbRinger.SetCurSel (wStyle);

}// CEmulatorDlg::SetRingStyle

//////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnRingerChange
//
// The current ring style has changed
//
void CEmulatorDlg::OnRingerChange()
{                               
    CString strBuff;
    strBuff.Format("RingMode <0x%x>", m_cbRinger.GetCurSel());
    AddDebugInfo (1, strBuff);

    SendNotification (EMRESULT_RINGCHANGE, (LPARAM)m_cbRinger.GetCurSel());

}// CEmulatorDlg::OnRingerChange

//////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnRing
//
// Play our wav ring
//
void CEmulatorDlg::OnRing()
{
    int iRing = m_cbRinger.GetCurSel();
    if (iRing == RINGER_NONE)
        return;

    UINT uidRes = IDW_RINGSOUND1 + iRing;
    HRSRC hResInfo = FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(uidRes), "WAVE");
    if (hResInfo == NULL)
        return;

    HANDLE hRes = LoadResource(AfxGetInstanceHandle(), hResInfo);
    if (hRes != NULL)
    {
        LPVOID lpRes = LockResource(hRes);
        if (lpRes != NULL)
        {
            sndPlaySound((LPCSTR)lpRes, SND_MEMORY | SND_SYNC | SND_NODEFAULT);
            UnlockResource(hRes);
        }           
        FreeResource(hRes);
    }       

}// CEmulatorDlg::OnRing

//////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::CreateCompletionRequest
//
// Create a completion request
//
WORD CEmulatorDlg::CreateCompletionRequest(WORD wAddressID, WORD wComplType)
{   
    CAddressAppearance* pAddr = GetAddress(wAddressID);
                                         
    COMPLETIONREQ* pComplReq = new COMPLETIONREQ;
    pComplReq->wAddressID = wAddressID;
    pComplReq->wComplType = wComplType;
    strncpy (pComplReq->szAddress, pAddr->m_Call.m_strDestNumber, ADDRESS_SIZE);
    
    if (wComplType == CALLCOMPLTYPE_CALLBACK)
        pComplReq->dwTimeout = GetTickCount() + 5000L;
    else
        pComplReq->dwTimeout = 0L;

    // Insert it into our call completion request list
    m_arrCompletions.Add (pComplReq);   
    return (WORD) (LONG)pComplReq;

}// CEmulatorDlg::CreateCompletionRequest

//////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::ProcessCallCompletions
//
// Manage the call completion list
//
void CEmulatorDlg::ProcessCallCompletions()
{                                       
    static BOOL fInCallComplete = FALSE;
    
    if (fInCallComplete)
        return;
        
    fInCallComplete = TRUE;
    
    for (int i = 0; i < m_arrCompletions.GetSize(); i++)
    {
        COMPLETIONREQ* pComplReq = (COMPLETIONREQ*) m_arrCompletions[i];
        if (pComplReq != NULL)
        {
            if (pComplReq->dwTimeout == 0 || pComplReq->dwTimeout < GetTickCount())
            {   
                switch (pComplReq->wComplType)
                {
                    case CALLCOMPLTYPE_INTRUDE:
                        {   
                            CAddressAppearance* pAddr = GetAddress(pComplReq->wAddressID);
                            // Send a notification about completion finishing.
                            SendNotification (EMRESULT_COMPLRESULT, MAKEERR(pComplReq->wAddressID,(WORD)(LONG)pComplReq));
                            pAddr->SetState(ADDRESSSTATE_CONNECT);
                            m_arrCompletions.RemoveAt(i--);
                            delete pComplReq;
                        }
                        break;
                            
                    case CALLCOMPLTYPE_MESSAGE:
                        SendNotification (EMRESULT_COMPLRESULT, MAKEERR(pComplReq->wAddressID,(WORD)(LONG)pComplReq));
                        m_arrCompletions.RemoveAt(i--);
                        delete pComplReq;
                        break;
                        
                    case CALLCOMPLTYPE_CALLBACK:
                        {
							SendNotification (EMRESULT_COMPLRESULT, MAKEERR(pComplReq->wAddressID,(WORD)(LONG)pComplReq));

                            // Attempt to use the original address first, if that doesn't
                            // work, then use the first available.
                            CAddressAppearance* pAddr = GetAddress(pComplReq->wAddressID);
                            if (pAddr->m_Call.m_iState != ADDRESSSTATE_OFFLINE)
                            {
                                for (int x = 0 ; x < m_arrAppearances.GetSize(); x++)
                                {
                                    pAddr = (CAddressAppearance*)m_arrAppearances[x];
                                    if (pAddr->m_Call.m_iState == ADDRESSSTATE_OFFLINE)
                                        break;
                                }
                            }
                            
                            if (pAddr->m_Call.m_iState == ADDRESSSTATE_OFFLINE)
                            {   
                                // Found one, remove the completion request.
                                m_arrCompletions.RemoveAt(i--);
                                SetTimer (TIMER_RING, 3000, NULL);
                                CEmulButton* pButton = GetButton(pAddr->m_iButtonIndex);
                                pButton->SetLampState(LAMPSTATE_FLASHING);
                                // Reset the address
                                pAddr->m_iRingCount = 0;
                                pAddr->m_Call.m_fIncoming = TRUE;
                                pAddr->m_Call.m_strDestName = "";
                                pAddr->m_Call.m_strDestNumber = pComplReq->szAddress;
                                pAddr->m_Call.m_dwMediaModes = MEDIAMODE_INTERACTIVEVOICE;
                                pAddr->SetState (ADDRESSSTATE_OFFERING);

                                delete pComplReq;
								
                                // Update the display
                                ShowDisplay (pAddr);
                            }
                        }
                        break;
                }        
            }
        }
    }
    
    fInCallComplete = FALSE;
    
}// CEmulatorDlg::ProcessCallCompletions

////////////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnHelpMe
//
// Non-Windows 95 help system
//
void CEmulatorDlg::OnHelpMe()
{   
	CWnd* pwnd = GetDlgItem(IDC_CONTEXT_HELP);
    
    if (pwnd->IsWindowVisible())
    	pwnd->EnableWindow (FALSE);
    	
    DoContextHelp();            
    
    if (pwnd->IsWindowVisible())
    	pwnd->EnableWindow (TRUE);
    
}// CEmulatorDlg::OnHelpMe

////////////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnNcHitTest
//
// Hittest logic for the emulator
//
UINT CEmulatorDlg::OnNcHitTest(CPoint pt)
{                            
	UINT nHTCode = CDialog::OnNcHitTest(pt);
	if (nHTCode >= HTLEFT && nHTCode < HTBORDER)
		nHTCode = HTNOWHERE;
		
	return nHTCode;

}// CEmulatorDlg::OnNcHitTest

////////////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::DoContextHelp
//
// Function called to do context sensitive help for the dialog.
//
void CEmulatorDlg::DoContextHelp()
{
    CWnd * pwnd = (CWnd *)this;

    HCURSOR hHelpCursor = AfxGetApp()->LoadCursor (AFX_IDC_CONTEXTHELP);
    HCURSOR hNoCursor   = AfxGetApp()->LoadCursor (AFX_IDC_NODROPCRSR);
    ASSERT (hHelpCursor != NULL && hNoCursor != NULL);
    HCURSOR hOldCursor = SetCursor (hHelpCursor);
    
    CRect rcClient;
    pwnd->GetWindowRect (rcClient);
    pwnd->SetCapture();
    
    // Run a message loop until a LBUTTONUP event.
    BOOL fDone = FALSE, fIsHelp = TRUE;
    CToolTip* pHelpTag = NULL;
    CWnd* pLastWnd = NULL;
    while (!fDone)
    {
        MSG msg;
        if (::PeekMessage (&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) ||
            ::PeekMessage (&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE) ||
            ::PeekMessage (&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
        {
            if (msg.message == WM_NCLBUTTONUP || msg.message == WM_LBUTTONUP ||
                msg.message == WM_KEYUP)
                fDone = TRUE;
            else if (msg.message == WM_PAINT)
            {
                DispatchMessage (&msg);
            }    
            else if (msg.message == WM_MOUSEMOVE)
            {
                CPoint ptCursor;
                GetCursorPos(&ptCursor);
                
                // If we have moved outside our page, don't show the help cursor.
                if (!rcClient.PtInRect (ptCursor))
                {   
                    if (fIsHelp)
                    {
                        fIsHelp = FALSE;
                        SetCursor (hNoCursor);
                    }
                }
                else
                {   
                    if (!fIsHelp)
                    {
                        SetCursor (hHelpCursor);
                        fIsHelp = TRUE;
                    }
                    
                    CWnd * pwndNext = pwnd->GetWindow (GW_CHILD);
                    while (pwndNext && pwndNext != pwnd->GetWindow (GW_HWNDNEXT))
                    {
                        CRect rcWindow;
                        pwndNext->GetWindowRect (rcWindow);
                        if (rcWindow.PtInRect(ptCursor))
                        {
                            if (pwndNext->IsWindowVisible())
                            {
                                if (pwndNext == pLastWnd)
                                    break;
                                    
                                CString strTag;
                                if (strTag.LoadString (pwndNext->GetDlgCtrlID()+5000))
                                {
                                    pLastWnd = pwndNext;
                                    delete pHelpTag;
                                    pHelpTag = NULL;
                                    pHelpTag = new CToolTip (strTag);
                                    CRect rcBox (ptCursor.x, ptCursor.y, ptCursor.x + 50, ptCursor.y+50);
                                    pHelpTag->CreateHelpTag (rcBox);
                                    break;
                                }
                            } 
                        }    
                        pwndNext = pwndNext->GetWindow(GW_HWNDNEXT);
                    }      
                    
                    // Delete the help tag when no window was found.
                    if (pwndNext == NULL || 
                        pwndNext == pwnd->GetWindow(GW_HWNDNEXT))
                    {
                        pLastWnd = NULL;
                        delete pHelpTag;
                        pHelpTag = NULL;
                    }
                }
            }
        }            
    }
    
    delete pHelpTag;
    SetCursor (hOldCursor);
    ReleaseCapture();
    
}// CEmulatorDlg::DoContextHelp

////////////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg::OnClose
//
// Overrides the WM_CLOSE message
//
void CEmulatorDlg::OnClose()
{                        
    CDialog::EndDialog (IDOK);

}// CEmulatorDlg::OnClose

