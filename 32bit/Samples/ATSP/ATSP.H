/******************************************************************************/
//                                                                        
// ATSP.H - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our driver code for the ATSP sample.
// 
// This service provider drives a Hayes compatible AT style modem.  It
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
/******************************************************************************/

// Defaults for LINEDIALPARAMS
#define MIN_DURATION    50
#define MIN_DIALSPEED   50
#define MIN_DIALTONE    1000
#define MIN_DIALPAUSE   0
#define MIN_DATARATE    1200
#define DEF_DURATION    95
#define DEF_DIALSPEED   95
#define DEF_DIALTONE    60000
#define DEF_DIALPAUSE   2000
#define MAX_DURATION    255
#define MAX_DIALSPEED   255
#define MAX_DIALTONE    255000
#define MAX_DIALPAUSE   255000
#define MAX_DATARATE    33600

// Response Types from modem
#define MODEM_INTERVAL_TIMER  1         // Timer
#define MODEM_ERR             2         // ERROR
#define MODEM_OK              3         // OK
#define MODEM_CONNECT         4         // CONNECT
#define MODEM_BUSY            5         // BUSY
#define MODEM_RING            6         // RING
#define MODEM_UNKNOWN         7         // Unknown response from device
#define MODEM_TALKDROP_DROP   8         // Talk Drop Drop call
#define MODEM_TALKDROP_TALK   9         // Talk Drop Talk 
#define MODEM_IGNORE          10        // Ignore

// Internal timer states
#define MODEM_TIMEOUT         10        // Timeout value in seconds.

// Internal INIT states
#define INITMODEM_MSEC         0		// Timeout values (Sreg) in MSEC
#define INITMODEM_SEC          1		// Timeout values (Sreg) in SEC
#define INITMODEM_GENERIC      2		// No timeouts used

// States for processMakeCall
#define STATE_INITMODEM   (STATE_INITIAL + 1)
#define STATE_DIALMODEM   (STATE_INITIAL + 2)

// States for processDial
#define STATE_TALKDROP    (STATE_INITIAL + 4)
#define STATE_CHECKMODEM  (STATE_INITIAL + 5)
#define STATE_WAITING     (STATE_INITIAL + 6)
#define STATE_DESTROYTD   (STATE_INITIAL + 7)
#define STATE_TALKPRESSED (STATE_INITIAL + 8)
#define STATE_DROPPRESSED (STATE_INITIAL + 9)

class CTalkDropDlg;

///////////////////////////////////////////////////////////////////////////
// CATSPProvider
//
// This class defines our Hayes AT service provider
//
class CATSPProvider : public CServiceProvider
{
// Class data
protected:
	CMapPtrToPtr m_UIMap;	// Used in the UIDLL portion to map dialog instance to window.

// Constructor
public:
    CATSPProvider();

// Methods overriden from CServiceProvider
public:
    // TSPI methods
    virtual LONG providerEnumDevices(DWORD dwProviderId, LPDWORD lpNumLines, LPDWORD lpNumPhones, HPROVIDER hProvider, LINEEVENT lpfnLineCreateProc, PHONEEVENT lpfnPhoneCreateProc);
    virtual LONG providerGenericDialogData (CTSPIDevice* pDev, CTSPILineConnection* pLine, CTSPIPhoneConnection* pPhone, HDRVDIALOGINSTANCE hdDlgInstance, LPVOID lpBuff, DWORD dwSize);

    // TUISPI methods called in the context of application
    virtual LONG providerGenericDialog (HTAPIDIALOGINSTANCE htDlgInst, LPVOID lpParams, DWORD dwSize, HANDLE hEvent, TUISPIDLLCALLBACK lpfnDLLCallback);
    virtual LONG providerGenericDialogData(HTAPIDIALOGINSTANCE htDlgInst, LPVOID lpParams, DWORD dwSize);
    virtual LONG providerInstall(DWORD dwPermanentProviderID, CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback);
    virtual LONG providerConfig(DWORD dwPPID, CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback);
    virtual LONG lineConfigDialogEdit(DWORD dwDeviceID, CWnd* pwndOwner, CString& strDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize, LPVARSTRING lpDeviceConfigOut, TUISPIDLLCALLBACK lpfnDLLCallback);
    virtual LONG lineConfigDialog(DWORD dwDeviceID, CWnd* pwndOwner, CString& strDeviceClass, TUISPIDLLCALLBACK lpfnDLLCallback);
};

///////////////////////////////////////////////////////////////////////////
// CIOQueue
//
// Input/Output queue class with embededd COMM port support.
// Used by the device object to manage string-based input/output.
//
class CIOQueue
{
// Class data
protected:
    CEvent m_evtClose;              // I/O Threads need to shutdown.
    HANDLE  m_hComm;                // COMM port handle
    CStringArray m_arrInput;        // Input -from- the COMM port
    CPtrArray m_arrOutput;          // Output -to- the COMM port
    CEvent m_evtPendingInput;       // String available in input array
    CEvent m_evtPendingOutput;      // String available in output array
    CCriticalSection m_csInput;     // Array access.
    CCriticalSection m_csOutput;    // Array access
    CWinThread* m_pThreadR;         // COMM I/O thread
    CWinThread* m_pThreadW;         // COMM I/O thread

// Constructor
public:
    CIOQueue();
    virtual ~CIOQueue();

// Access methods
public:
    void Start (HANDLE hComm);
    void Terminate();
    CString Read (HANDLE hEvent);
    void Write(LPCTSTR lpszBuff);

// Internal methods
public:
    void ReaderThread();
    void WriterThread();
};

///////////////////////////////////////////////////////////////////////////
// CATSPLine
//
// Override of our line connection object.  Supports a single COMM
// port at the line level.
//
class CATSPLine : public CTSPILineConnection
{
    DECLARE_DYNCREATE (CATSPLine)

// Class data
protected:
    HTAPIDIALOGINSTANCE m_hdlgTalkDrop; // Handle to Talk/Drop UI dialog
	DWORD m_dwRefCnt;				// Total count of open devices.
	DWORD m_dwLastActivity;			// Last RING activity on the device
    HANDLE m_hComm;					// COMM port handle
	CEvent m_evtClose;				// Device being closed notification
	CIOQueue m_qData;				// COMM threaded queue
	CWinThread* m_pThread;			// Thread for the device

// Constructor/Destructor
public:
    CATSPLine();
	virtual ~CATSPLine();

// Overriden methods from the CTSPILineConnection.
protected:
    virtual VOID Init(CTSPIDevice* pDev, DWORD dwLineDeviceID, DWORD dwPos, DWORD dwItemData);
    virtual BOOL ReceiveData (DWORD dwData=0, const LPVOID lpBuff=NULL, DWORD dwSize=0);
    virtual LONG GetDevConfig(CString& strDeviceClass, LPVARSTRING lpDeviceConfig);
    virtual LONG SetDevConfig(CString& strDeviceClass, LPVOID const lpDevConfig, DWORD dwSize);
    virtual VOID OnTimer();
	virtual BOOL OpenDevice();
	virtual BOOL CloseDevice ();
	virtual BOOL SendData (LPCVOID lpBuff, DWORD dwSize);

// Internal methods
private:
	friend UINT InputThread (CATSPLine* pLine);
	VOID InputThread();
    void DropCarrier();
	void Suspend();
	void Resume();
	void SetModemHandle();
	BOOL ReadRegistrySettings();
    CString BuildInitString(TSPIMAKECALL* lpMakeCall, BOOL fUseSecs=FALSE);
    CString BuildDialString (CTSPICallAppearance* pCall, DIALINFO* pDialInfo, DWORD dwCountryCode);
	void processRingEvent();
	void processAnswer(CTSPIRequest* pReq, BYTE bResult);
    void HandleDial (CTSPIRequest* pReq, BYTE bResult, CADObArray* parrAddresses, DWORD dwCountryCode);
    void processMakeCall(CTSPIRequest* pReq, BYTE bCommand);
    void processDial(CTSPIRequest* pReq, BYTE bCommand);
    void processDropCall(CTSPIRequest* pReq, BYTE bCommand);
    void processSetCallParams(CTSPIRequest* pReq, BYTE bCommand);
    void CreateTalkDropDialog(CTSPIRequest* pRequest);
    void ShowTalkDropDialog ();
    void DismissTalkDropDialog();
};

////////////////////////////////////////////////////////////////////////////
// GetString
//
// Returns a character string from the resource table
// This is inline since CString will use reference counting.
//
inline CString GetString(UINT iResId)
{
   CString strBuff;
   strBuff.LoadString(iResId);
   return strBuff;

}// GetString

