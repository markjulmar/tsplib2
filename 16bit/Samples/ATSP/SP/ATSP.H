/******************************************************************************/
//                                                                        
// ATSP.H - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all our driver code for the ATSP sample.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.
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
#define MAX_DATARATE    28800

// This is the EXE to use as a companion application.  It should be a 16-bit windows app.
#define STR_EXENAME "ATV2EXE.EXE"

// States for processMakeCall
#define STATE_INITMODEM (STATE_INITIAL + 1)
#define STATE_DIALMODEM (STATE_INITIAL + 2)

// States for processDial
#define STATE_TALKDROP   (STATE_INITIAL + 3)
#define STATE_CHECKMODEM (STATE_INITIAL + 4)
#define STATE_WAITING    (STATE_INITIAL + 5)
#define STATE_DESTROYTD  (STATE_INITIAL + 6)

// States for processHangup
#define STATE_CHECKFORAT (STATE_INITIAL + 7)
#define STATE_HANGUP     (STATE_INITIAL + 8)

// New request packets
#define REQUEST_HANGUPMODEM (REQUEST_END + 1)

class CTalkDropDlg;
                           
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
	DWORD m_dwLastActivity;			// Last RING activity on the device
    CTalkDropDlg* m_pdlgTalkDrop; 	// Talk/drop dialog which is active

// Constructor/Destructor
public:
    CATSPLine();

// Overriden methods from the CTSPILineConnection.
protected:
    virtual VOID Init(CTSPIDevice* pDev, DWORD dwLineDeviceID, DWORD dwPos);
    virtual BOOL ReceiveData (DWORD dwData=0, const LPVOID lpBuff=NULL, DWORD dwSize=0);
    virtual VOID OnTimer();
	virtual BOOL OpenDevice();
	virtual BOOL CloseDevice ();

// Internal methods
private:
    friend class CTalkDropDlg;
	BOOL CreateAddresses();
    CString BuildInitString(TSPIMAKECALL* lpMakeCall, BOOL fUseSecs);
    CString BuildDialString (CTSPICallAppearance* pCall, DIALINFO* pDialInfo, DWORD dwCountryCode);
	void processRingEvent();
	void processAnswer(CTSPIRequest* pReq, BYTE bResult);
    void processMakeCall(CTSPIRequest* pReq, BYTE bCommand);
    void processDial(CTSPIRequest* pReq, BYTE bCommand);
    void processDropCall(CTSPIRequest* pReq, BYTE bCommand);
    void processSetCallParams(CTSPIRequest* pReq, BYTE bCommand);
    void ShowTalkDropDialog (CTSPIRequest* pRequest);
    int DismissTalkDropDialog();
    void HandleDial (CTSPIRequest* pReq, BYTE bResult, CADObArray* parrAddresses, DWORD dwCountryCode);
};
                           
///////////////////////////////////////////////////////////////////////////
// CATSPProvider
//
// This class defines our Hayes AT service provider
//
class CATSPProvider : public CServiceProvider
{
// Constructor
public:
	friend class CATSPLine;
    CATSPProvider();
    
// TSPI Methods
public:
	virtual LONG providerEnumDevices(DWORD dwProviderId, LPDWORD lpNumLines, LPDWORD lpNumPhones, HPROVIDER hProvider, LINEEVENT lpfnLineCreateProc, PHONEEVENT lpfnPhoneCreateProc);
    virtual LONG providerConfig(DWORD dwPPID, CWnd* pwndOwner);
    virtual LONG providerInstall(DWORD dwPermanentProviderID, CWnd* pwndOwner);
	virtual BOOL OpenDevice (CTSPIConnection* pConn);

    // Line members
    virtual LONG lineGetID(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, CString& strDevClass, LPVARSTRING lpDeviceID);
    virtual LONG lineConfigDialog(CTSPILineConnection* pConn, CWnd* pwndOwner, CString& strDeviceClass);
};

/////////////////////////////////////////////////////////////////////////
// GetString
//
// Function to return a string from the resource table.
//
const char * GetString(UINT iResId);
