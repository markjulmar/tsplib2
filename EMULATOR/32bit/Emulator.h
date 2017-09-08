/*****************************************************************************/
//
// EMULATOR.H - Digital Switch Service Provider Sample
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

#include "resource.h"

// Define the classes from OBJECT.H
class CEmulButton;
class CClientSocket;
class CListeningSocket;

///////////////////////////////////////////////////////////////////////////////
// CEmulatorApp - main application class
class CEmulatorApp : public CWinApp
{
public:
   virtual BOOL InitInstance();
};

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg dialog

class CEmulatorDlg : public CDialog
{
// Construction
public:
    CEmulatorDlg(CWnd* pParent = NULL);
	virtual ~CEmulatorDlg();

// Dialog Data
    //{{AFX_DATA(CEmulatorDlg)
    //}}AFX_DATA
    enum { IDD = IDD_EMULATOR };
    enum Features { None=0, Forward, Transfer, Conference, Display };
    CColorLB m_lstDebugInfo;         		// Listbox for all data to/from phone
    CMfxBitmapButton m_btnHold;             // The Hold button
    CMfxBitmapButton m_btnRelease;          // The Release button
    CMfxBitmapButton m_btnDial;             // The Dial button
    CMfxBitmapButton m_btnIncoming;         // The Incoming button
    CMfxBitmapButton m_btnBusy;             // Button representing a BUSY call
    CMfxBitmapButton m_btnAnswered;         // Button representing an ANSWERED call
    CMfxBitmapButton m_btnGenerate;         // The Generate Codes button
    CMfxBitmapButton m_btnVolUp;            // Volume up button
    CMfxBitmapButton m_btnVolDn;            // Volume down button
    CMfxBitmapButton m_btnGainUp;           // Gain up button
    CMfxBitmapButton m_btnGainDn;           // Gain down button
    CMfxBitmapButton m_btnGenerateTone;     // Tone Generation
    CString m_strStationName;       		// Name of person using this phone.
    CString  m_strDisplay;           		// Our current display contents
    CEmulButton m_Buttons[BUTTON_COUNT]; 	// Array of button objects
    int m_iCursor;              			// Current position in display buffer
    UINT m_iVolumeLevel;         			// Volume level
    UINT m_iGainLevel;           			// Gain level
    BOOL m_fHandsetHookswitch;   			// Current handset hookswitch value   
    BOOL m_fMicrophone;          			// Current microphone hookswitch value
    BOOL m_fSpeaker;             			// Current speaker hookswitch value
    HICON m_hIcon;                			// Icon used to draw minimized state.
    CObArray m_arrAppearances;       		// Array of address appearances
    CComboBox m_cbRinger;             		// Ringer style
	CProgressCtrl m_ctlVolume;				// Volume bar
	CProgressCtrl m_ctlGain;				// Gain bar
    // Switch feature information                                    
    int m_iActiveFeature;       			// Current feature active
    int m_iDisplayButton;       			// Button index of Display button pressed
    CString m_strFeatureData;       		// Data for active feature
    CPtrArray m_arrCompletions;       		// Call completions pending.
    CPtrArray m_arrParks;             		// Calls parked
	CListeningSocket* m_pSocket;			// Server socket
	CPtrList m_connectionList;				// List of connections

// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CEmulatorDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnAnswered();
	afx_msg LRESULT OnTSPICommand (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCopyData (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTSPIOpenConn(DWORD dwConnId);
	afx_msg void OnTSPICloseConn(DWORD dwConnID);
    afx_msg void OnBusy();
    afx_msg void OnMakecall();
    afx_msg void OnMsgwaiting();
    afx_msg void OnReset();
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT nIdEvent);
	afx_msg UINT OnNcHitTest(CPoint pt);
    afx_msg void OnHoldCall();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnReleaseCall();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPressButton();
    afx_msg void OnPaint();
    afx_msg void OnHookswitchChange();
    afx_msg void OnClose();
    afx_msg void OnDial();
    afx_msg void OnRingerChange();
    afx_msg void OnConfigure();
    afx_msg void OnGenerate();
    afx_msg void OnVolumeUp();
    afx_msg void OnVolumeDown();
    afx_msg void OnGainUp();
    afx_msg void OnGainDown();
    afx_msg void OnGenerateTone();                        
    afx_msg void OnHelpMe();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
	void ProcessEmulatorMessage (CClientSocket* pSocket, DWORD dwCommand, LPVOID lpData, DWORD dwSize);
    void SendNotification (WORD wResult, LPARAM lData);
    void AddDebugInfo (BYTE bType, LPCSTR lpszBuff);
    void OnButtonUpDown (int iButtonLampID);
    int DialNumber (int iAddress, LPCSTR lpBuff);
    CAddressAppearance* FindCallByState(int iState);
    CAddressAppearance* FindCallByButtonID(int iButtonID);
    CAddressAppearance * GetAddress (int iAddressID);
    void SetRingStyle (WORD wStyle);

private:
    void DestroyCallAppearances();
    void EmptyCallInfo (void);
    void EmptyCallInfo (int iPos);
    void SetupFunctionButtons();
    void OnVolumeChanged(UINT uiNewVol);
    void OnGainChanged(UINT uiNewGain);
    void LoadINISettings();
    void WriteINISettings();
    WORD CreateCompletionRequest(WORD wAddressID, WORD wType);
    void ProcessCallCompletions();
        
    CEmulButton* GetButton(int iPos);
    CEmulButton* FindButtonByFunction(int iFunction);
    void UpdateStates();
    
    void ResetDisplay (BOOL bFill);
    void ShowDisplay (CAddressAppearance * pAddrApp);
    void CopyDisplay (LPSTR lpBuff);
    void PressFunctionKey (int iFunctionKey);
    void PressCallAppearance (int iKey);
    void PressDisplay(int iKey);
    void PressForward(int iKey);
    void PressTransfer(int iKey);

    void SetHookSwitch (int iHookSwitchDev, int iState);
    int PerformDrop(int iAddress);
    int PerformHold (int iAddressID);
    int PerformUnhold (int iAddressID);
    void PerformDropConf(CAddressAppearance* pAddr);
    void OnRing();
    void DoContextHelp();

	// Socket management code
public:
	BOOL BroadcastMsg (DWORD dwResult, LPVOID lpBuff, DWORD dwSize);
	BOOL ReadMsg(CClientSocket* pSocket, LPDWORD pdwCommand, LPVOID* ppBuff, LPDWORD pdwSize);
	BOOL SendMsg(CClientSocket* pSocket, DWORD dwResult, LPVOID lpBuff, DWORD dwSize);
	BOOL SendErr(CClientSocket* pSocket, WORD wAddressID, WORD dwResult);
	void CloseSocket(CClientSocket* pSocket);
	void ProcessPendingAccept();
	void ProcessPendingRead(CClientSocket* pSocket);

private:
    // TSP intercept points
    void DRV_QueryCaps (CClientSocket* pSocket);
    void DRV_QueryVersion (CClientSocket* pSocket);
    void DRV_GetAddressInfo (CClientSocket* pSocket, WORD wAddressID);
    void DRV_PrepareAddress (CClientSocket* pSocket, WORD wAddressID);
    void DRV_Dial (CClientSocket* pSocket, LPEMADDRESSINFO lpAddrInfo);
    void DRV_DropCall (CClientSocket* pSocket, WORD wAddressID);
    void DRV_Answer (CClientSocket* pSocket, WORD wAddressID);
    void DRV_HoldCall(CClientSocket* pSocket, WORD wAddressID);
    void DRV_UnholdCall(CClientSocket* pSocket, WORD wAddressID);
    void DRV_Forward (CClientSocket* pSocket, LPEMFORWARDINFO lpForward);    
    void DRV_Flash (CClientSocket* pSocket, WORD wAddressID);
    void DRV_Transfer (CClientSocket* pSocket, LPEMTRANSFERINFO lpTransInfo);
    void DRV_SetLevel(CClientSocket* pSocket, LPEMLEVELCHANGE lpChange);
    void DRV_SetHookswitch (CClientSocket* pSocket, LPEMHOOKSWITCHCHANGE lpChange);
    void DRV_Conference(CClientSocket* pSocket, LPEMCONFERENCEINFO lpConfInfo);
    void DRV_Redirect (CClientSocket* pSocket, LPEMFORWARDINFO lpForwardInfo);
    void DRV_CompleteCall (CClientSocket* pSocket, LPEMCOMPLETECALL lpCompleteCall);
    void DRV_ParkCall (CClientSocket* pSocket, LPEMPARKINFO lpParkInfo);
    void DRV_UnparkCall (CClientSocket* pSocket, LPEMPARKINFO lpParkInfo);
};

