//*****************************************************************************
//
// EMULATOR.H
//
// Digital switch emulator program
//
// Written 11/02/94 Mark Smith (MCS)
//
//*****************************************************************************

#include "resource.h"

// Define the classes from OBJECT.H
class CEmulButton;

///////////////////////////////////////////////////////////////////////////////
// CEmulatorApp - main application class
class CEmulatorApp : public CWinApp
{
// Class data
private:
   CCtl3d   m_ctl3d;

public:
   virtual BOOL InitInstance();
};

/////////////////////////////////////////////////////////////////////////////
// CEmulatorDlg dialog

class CEmulatorDlg : public CDialog
{
// Construction
public:
    CEmulatorDlg(CWnd* pParent = NULL); // standard constructor

// Dialog Data
    //{{AFX_DATA(CEmulatorDlg)
    enum { IDD = IDD_EMULATOR };
    enum Features { None=0, Forward, Transfer, Conference, Display };
    CColorLB        m_lstDebugInfo;         // Listbox for all data to/from phone
    CString         m_strDisplay;           // Our current display contents
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
    CProgressCtl     m_ctlVolume;           // Volume control
    CProgressCtl     m_ctlGain;             // Gain control 
    //}}AFX_DATA
    CString         m_strStationName;       // Name of person using this phone.
    CEmulButton     m_Buttons[BUTTON_COUNT]; // Array of button objects
    int             m_iCursor;              // Current position in display buffer
    UINT            m_iVolumeLevel;         // Volume level
    UINT            m_iGainLevel;           // Gain level
    BOOL            m_fHandsetHookswitch;   // Current handset hookswitch value   
    BOOL            m_fMicrophone;          // Current microphone hookswitch value
    BOOL            m_fSpeaker;             // Current speaker hookswitch value
    HWND            m_hwndTExe;             // HWND of the thread .EXE
    HICON           m_hIcon;                // Icon used to draw minimized state.
    CObArray        m_arrAppearances;       // Array of address appearances
    CComboBox       m_cbRinger;             // Ringer style
    // Switch feature information                                    
    int             m_iActiveFeature;       // Current feature active
    int             m_iDisplayButton;       // Button index of Display button pressed
    CString         m_strFeatureData;       // Data for active feature
    CPtrArray       m_arrCompletions;       // Call completions pending.
    CPtrArray       m_arrParks;             // Calls parked
// Implementation
protected:
    virtual void DoDataExchange(CDataExchange* pDX);   // DDX/DDV support

    // Generated message map functions
    //{{AFX_MSG(CEmulatorDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnAnswered();
    afx_msg void OnBusy();
    afx_msg void OnMakecall();
    afx_msg void OnMsgwaiting();
    afx_msg void OnReset();
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT nIdEvent);
    afx_msg LRESULT OnEmulator(WPARAM wParam, LPARAM lParam);
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
    afx_msg void OnExitApp();
    afx_msg void OnVolumeUp();
    afx_msg void OnVolumeDown();
    afx_msg void OnGainUp();
    afx_msg void OnGainDown();
    afx_msg void OnGenerateTone();                        
    afx_msg void OnHelpMe();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
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
    void ReleaseTSPThread ();
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
    
    // TSP intercept points
    void DRV_QueryCaps (LPEMSETTINGS lpSettings);
    void DRV_QueryVersion (LPEMVERSIONINFO lpEMVersion);
    void DRV_GetAddressInfo (LPEMADDRESSINFO lpAddrInfo);
    void DRV_PrepareAddress (int iAddressID);
    void DRV_Dial (LPEMADDRESSINFO lpAddrInfo);
    void DRV_DropCall (int iAddressID);
    void DRV_Answer (int iAddressID);
    void DRV_HoldCall(int iAddressID);
    void DRV_UnholdCall(int iAddressID);
    void DRV_Forward (LPEMFORWARDINFO lpForward);    
    void DRV_Flash (int iAddressID);
    void DRV_Transfer (LPEMTRANSFERINFO lpTransInfo);
    void DRV_SetLevel(LPEMLEVELCHANGE lpChange);
    void DRV_SetHookswitch (LPEMHOOKSWITCHCHANGE lpChange);
    void DRV_Conference(LPEMCONFERENCEINFO lpConfInfo);
    void DRV_Redirect (LPEMFORWARDINFO lpForwardInfo);
    void DRV_CompleteCall (LPEMCOMPLETECALL lpCompleteCall);
    void DRV_ParkCall (LPEMPARKINFO lpParkInfo);
    void DRV_UnparkCall (LPEMPARKINFO lpParkInfo);
};

