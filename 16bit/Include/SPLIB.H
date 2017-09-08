/******************************************************************************/
//                                                                        
// SPLIB.H - TAPI Service Provider C++ Library header                     
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// The SPLIB classes provide a basis for developing MS-TAPI complient     
// Service Providers.  They provide basic handling for all of the TSPI    
// APIs and a C-based handler which routes all requests through a set of C++     
// classes.                                                                 
//              
// This library is for use with TAPI version 1.3 or 1.4 under Windows 3.1 
// and Windows 95.
//                                                           
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
//                                                           
/******************************************************************************/

#ifndef _SPLIB_INC_
#define _SPLIB_INC_

#ifndef RC_INVOKED
#pragma pack(1)         // Assume byte packing
#endif 

// Force TAPI 1.4 constants if TAPI 2.x headers are used.
#define TAPI_CURRENT_VERSION 0x00010004

// New const void* ptr.
typedef const void FAR* LPCVOID;

// Total # of standard buttons for TAPI (0-9, *, #).
#define TOTAL_STD_BUTTONS  12

// Define known versions of TAPI
#define TAPIVER_13 (0x00010003)
#define TAPIVER_14 (0x00010004)

// Wait timeout used for various things in the library.  We assume that
// things that we are forced to wait for (synchronous) will complete within
// this timeframe.  This can be adjusted through the CServiceProvider::SetTimeout
// method.
#define MAX_WAIT_TIMEOUT (10000L)

#ifdef DEBUG
   #define _DEBUG
#endif

///////////////////////////////////////////////////////////////////////////
// Pre-define our classes.

class CServiceProvider;
   class CTSPIDevice;
      class CTSPIConnection;
         class CTSPILineConnection;
            class CTSPIAddressInfo;
                class CTSPICallAppearance;
                class CTSPIConferenceCall;
         class CTSPIPhoneConnection;

/******************************************************************************/
//
// CADObArray
//
// Our own private object array which deletes any objects within
// it at destruction time.  The inserted objects MUST be derived from
// CObject.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class CADObArray : public CObArray
{
// Destructor
public:
    virtual ~CADObArray();
};

/******************************************************************************/
//
// CFlagArray
//
// This array stores bit field flag definitions.  It is designed to hold
// bit field arrays over 32-bits in length, and automatically allocates
// on a byte-per-byte basis as needed.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class CFlagArray
{ 
// Class data
protected:
    CByteArray  m_arrValues;
    
// Constructor
public:
    CFlagArray();
    
// Methods
public:
    BOOL GetAt(int nIndex) const;
    void SetAt(int nIndex, BOOL fFlag);
    BOOL operator[](int nIndex) const;
};

/******************************************************************************/
//
// TSPILINEFORWARD structure
//
// This is passed as the data structure for a REQUEST_FORWARD command.
//
/******************************************************************************/
class TSPILINEFORWARD : public CObject
{
public:                
    TSPILINEFORWARD();              // Constructor
    virtual ~TSPILINEFORWARD();     // Destructor
    DWORD dwNumRings;               // Number of rings before "no answer"
    CTSPICallAppearance* pCall;     // Call appearance created (consultation)
    LPLINECALLPARAMS lpCallParams;  // Call parameters for new call
    CObArray arrForwardInfo;        // A list of forward information structures
};

/******************************************************************************/
// 
// TSPIFORWARDINFO
//
// Single forwarding information structure.  This is placed into 
// the "ptrForwardInfo" array for each forward entry found in the
// forward list.
//
// The caller and destination addresses are stored in both input and
// our dialable form so that depending on what is required at the switch
// level is avialable to the derived class worker code.
//
/******************************************************************************/
class TSPIFORWARDINFO : public CObject
{                
public:
    TSPIFORWARDINFO();              // Constructor    
    int iRefCount;                  // Reference Count
    DWORD dwForwardMode;            // Forwarding mode (LINEFORWARDMODE_xxx)
    DWORD dwDestCountry;            // Destination country for addresses
    CADObArray arrCallerAddress;    // Input caller addresses (DIALINFO)
    CADObArray arrDestAddress;      // Destination addresses (DIALINFO)
    VOID IncUsage();
    VOID DecUsage();
};
    
/******************************************************************************/
// 
// TSPICALLPARAMS structure
//
// This is passed as the data structure for a REQUEST_SETCALLPARAMS command.
//
/******************************************************************************/
class TSPICALLPARAMS : public CObject
{
public:
    TSPICALLPARAMS();          // Constructor
    DWORD   dwBearerMode;      // New bearer mode for call
    DWORD   dwMinRate;         // Low bound for call data rate
    DWORD   dwMaxRate;         // Hi bound for call data rate
    LINEDIALPARAMS DialParams; // New dial parameters
};

/******************************************************************************/
//
// TSPILINEPARK structure
//
// This structure is passed as the data associated with a REQUEST_PARK command.
//
/******************************************************************************/
class TSPILINEPARK : public CObject
{
public:
    TSPILINEPARK();               // Constructor
    DWORD dwParkMode;             // LINEPARKMODE_xxxx value
    CADObArray arrAddresses;      // Park directed address(s)
    // WARNING: The derived class must verify this parameters BEFORE 
    // accessing it to make sure it is still valid!
    LPVARSTRING lpNonDirAddress;  // Return buffer for non-directed park
};

/******************************************************************************/
//
// TSPILINEPICKUP structure
//
// This structure is passed as the data associated with a REQUEST_PICKUP command.
//
/******************************************************************************/
class TSPILINEPICKUP : public CObject
{
public:             
    TSPILINEPICKUP();               // Constructor
    CADObArray arrAddresses;        // Pickup address(s)
    CString strGroupID;             // Group id to which alerting station belongs.
};

/******************************************************************************/
// 
// TSPITRANSFER
//
// This structure is passed to manage the different consultation transfer
// events.  It goes with REQUEST_SETUPXFER and REQUEST_COMPLETEXFER
//
/******************************************************************************/
class TSPITRANSFER : public CObject
{
public:
    TSPITRANSFER();                 // Constructor
    virtual ~TSPITRANSFER();        // Destructor
    CTSPICallAppearance* pCall;     // Original call (inbound)
    CTSPICallAppearance* pConsult;  // New consultation call (outbound)
    CTSPIConferenceCall* pConf;     // New conference call (for complete)
    DWORD dwTransferMode;           // Transfer mode (for complete)
    LPLINECALLPARAMS lpCallParams;  // Caller parameters for Setup
};

/******************************************************************************/
// 
// TSPICONFERENCE
//
// This structure is passed to manage the different conference events
// It goes with: REQUEST_ADDTOCONF, REQUEST_REMOVEFROMCONF, REQUEST_SETUPCONF
//
/******************************************************************************/
class TSPICONFERENCE : public CObject
{
public:
    TSPICONFERENCE();                   // Constructor
    virtual ~TSPICONFERENCE();          // Destructor
    CTSPIConferenceCall* pConfCall;     // Conference call we are working with
    CTSPICallAppearance* pCall;         // Call appearance to work with.
    CTSPICallAppearance* pConsult;      // Call appearance created as consultation
    DWORD dwPartyCount;                 // Filled on SetupConf.
    LPLINECALLPARAMS lpCallParams;      // Filled on SetupConf. 
};

/******************************************************************************/
// 
// TSPITONEMONITOR                                                                 
//
// This structure is used for tone monitoring on a call appearance.
//
/******************************************************************************/
class TSPITONEMONITOR : public CObject
{
public:                                                         
    TSPITONEMONITOR();                  // Constructor for the object
    virtual ~TSPITONEMONITOR();         // Destructor to destroy array.
    DWORD dwToneListID;                 // Tone Identifier passed on LINEMONITORTONE msg.
    CPtrArray arrTones;                 // Tones to monitor for.
};    

/******************************************************************************/
//
// TSPIDIGITGATHER
//
// This structure is passed to manage digit gathering.  It goes with
// the REQUEST_GATHERDIGITS function.
//
/******************************************************************************/
class TSPIDIGITGATHER : public CObject
{
public:           
    TSPIDIGITGATHER();                  // Constructor
    DWORD dwEndToEndID;                 // Unique identifier for this request to TAPI.
    DWORD dwDigitModes;                 // LINEDIGITMODE_xxx 
    LPSTR lpBuffer;                     // Buffer for the call to place collected digits
    DWORD dwSize;                       // Number of digits before finished
    DWORD dwCount;                      // Count of digits we have placed in the buffer.
    CString strTerminationDigits;       // Digits which will terminate gathering.
    DWORD dwFirstDigitTimeout;          // mSec timeout for first digit
    DWORD dwInterDigitTimeout;          // mSec timeout between any digits
    DWORD dwLastTime;                   // Last mSec when digit seen.
};

/******************************************************************************/
// 
// TSPIGENERATE
//
// This structure is passed to manage digit and tone generation.
// It is used with the REQUEST_GENERATEDIGIT and REQUEST_GENERATETONE
// requests.
//
/******************************************************************************/
class TSPIGENERATE : public CObject
{
public:                     
    TSPIGENERATE();                     // Constructor
    virtual ~TSPIGENERATE();            // Destructor to delete our ptr array entries
    DWORD dwEndToEndID;                 // Unique identifier for this request to TAPI.
    DWORD dwMode;                       // Digit or Tone mode to generate.
    DWORD dwDuration;                   // Digit or Tone duration.
    CString strDigits;                  // Digits to generate
    CPtrArray arrTones;                 // Array of LINEGENERATETONE structures for custom tones.
};

/******************************************************************************/
// 
// TSPICOMPLETECALL
//
// This structure is passed to manage the lineCompleteCall API.
// It goes with the REQUEST_COMPLETECALL function.
//
/******************************************************************************/
class TSPICOMPLETECALL : public CObject
{ 
public:               
    TSPICOMPLETECALL();                 // Constructor
    TSPICOMPLETECALL(const TSPICOMPLETECALL* pCall);
    DWORD dwCompletionID;               // Completion ID
    DWORD dwCompletionMode;             // Completion mode requested
    DWORD dwMessageId;                  // Message id to forward to the station
    CTSPICallAppearance* pCall;         // Original call appearance for completion for CAMP.

    // The following is to be filled in by the worker code when the
    // complete call request is sent to the PBX.  In general this would be
    // an extension which will appear on the display, or an id number generated
    // by the switch, etc.  Something to positively identify an incoming call
    // as a call completion request.
    DWORD dwSwitchInfo;
    CString strSwitchInfo;              
};

/******************************************************************************/
// 
// TSPIMAKECALL
//
// This structure is passed as the data associated with a REQUEST_MAKECALL
// command.
//
/******************************************************************************/
class TSPIMAKECALL : public CObject
{
public:
    TSPIMAKECALL();                     // Constructor
    virtual ~TSPIMAKECALL();            // Destructor
    CADObArray arrAddresses;            // Destination addresses to dial
    DWORD dwCountryCode;                // Country code    
    LPLINECALLPARAMS lpCallParams;      // Call parameters
};    

/******************************************************************************/
// 
// TSPILINESETTERMINAL structure
//
// This structure is passed as the data associated with a REQUEST_SETTERMINAL
// command.
//
/******************************************************************************/
class TSPILINESETTERMINAL : public CObject
{
public:               
    TSPILINESETTERMINAL();          // Constructor
    CTSPILineConnection* pLine;     // Line (may be NULL)
    CTSPIAddressInfo* pAddress;     // Address (may be NULL)
    CTSPICallAppearance* pCall;     // Call appearance (may be NULL)
    DWORD dwTerminalModes;          // Terminal mode
    DWORD dwTerminalID;             // Terminal to move to
    BOOL  bEnable;                  // Whether to enable or disable terminal.
};

/******************************************************************************/
// 
// TSPIMEDIACONTROL
//
// This structure is passed to control media actions on a particular
// media stream.  It is used in response to the REQUEST_SETMEDIACONTROL
// command.
//
/******************************************************************************/
class TSPIMEDIACONTROL : public CObject
{                  
private:
    int m_iRefCount;                // Reference count for shared structures
public:            
    TSPIMEDIACONTROL();             // Constructor
    virtual ~TSPIMEDIACONTROL();    // Destructor for the media control
    CPtrArray arrDigits;            // Array of digit monitoring media structures
    CPtrArray arrMedia;             // Array of media monitoring media structures
    CPtrArray arrTones;             // Array of tone monitoring media structures
    CPtrArray arrCallStates;        // Array of callstate monitoring media structures
    VOID DecUsage();                // Auto-delete mechanism after no more usage
    VOID IncUsage();                // Incremenent usage count
};

/******************************************************************************/
// 
// TSPIHOOKSWITCHPARAM
//
// This structure is passed in response to changes in our hookswitch
// device.  It is passed on a REQUEST_SETHOOKSWITCHVOL and 
// REQUEST_SETHOOKSWITCHGAIN
//
/******************************************************************************/
class TSPIHOOKSWITCHPARAM : public CObject
{ 
public:
    TSPIHOOKSWITCHPARAM();              // Constructor
    DWORD dwHookSwitchDevice;           // Hookswitch device to change
    DWORD dwParam;                      // Volume, gain
};

/******************************************************************************/
// 
// TSPIRINGPATTERN
//
// This structure is passed in response to setting changes on our
// ringer device.  It is used in a REQUEST_SETRING request.
//
/******************************************************************************/
class TSPIRINGPATTERN : public CObject
{
public:
    TSPIRINGPATTERN();                  // Constructor
    DWORD dwRingMode;                   // Ring mode (0-dwNumRingModes)
    DWORD dwVolume;                     // Volume (0-0xffff)
};

/******************************************************************************/
// 
// TSPISETBUTTONINFO structure
//
// This structure is passed as the data associated with a REQUEST_SETBUTTONINFO
//
/******************************************************************************/
class TSPISETBUTTONINFO : public CObject
{
public:             
    TSPISETBUTTONINFO();                // Constructor
    DWORD dwButtonLampId;               // Button id to change
    DWORD dwFunction;                   // Function to set button to
    DWORD dwMode;                       // Mode for function
    CString strText;                    // Text for button.
};

/******************************************************************************/
// 
// TSPIPHONESETDISPLAY structure
//
// This structure is passed as the data associated with a REQUEST_SETDISPLAY
// command.
//
/******************************************************************************/
class TSPIPHONESETDISPLAY : public CObject
{
public:
    TSPIPHONESETDISPLAY();        // Constructor
    virtual ~TSPIPHONESETDISPLAY(); // Destructor
    DWORD  dwRow;                 // Row in display to modify
    DWORD  dwColumn;              // Column in display to modify
    LPVOID lpvDisplay;            // Display changes
    DWORD  dwSize;                // Size of above.
};

/******************************************************************************/
//
// TSPIPHONEDATA
//
// This structure is used to set buffers into downloadable areas on a
// phone.  It is passed as the parameter to a REQUEST_SETPHONEDATA and
// REQUEST_GETPHONEDATA.
//
/******************************************************************************/
class TSPIPHONEDATA : public CObject
{
public:         
    TSPIPHONEDATA();              // Constructor
    virtual ~TSPIPHONEDATA();     // Destructor to delete additional memory
    DWORD dwDataID;               // Data buffer to set
    LPVOID lpBuffer;              // Buffer to set (Allocated with AllocMem)
    DWORD dwSize;                 // Size of the buffer.
};    

/******************************************************************************/
// 
// DIALINFO
//
// This structure is used to store dialable number information for
// a destination.  The information is broken out by the method
// "CheckDialableAddress" and returned in a set of structure(s) for
// each address found in the passed string.
//
// This is passed in an array for several asynchronous requests.
//
/******************************************************************************/
class DIALINFO : public CObject
{ 
// Class data
public:
    BOOL fIsPartialAddress; // Address is "partial"
    CString strNumber;      // Final number to dial, includes "!PTW@$;"
    CString strName;        // Name pulled out of dial string (may be NULL)
    CString strSubAddress;  // Sub address information (ISDN) pulled out of dial string)
};

/******************************************************************************/
//
// TIMEREVENT
//
// This is used by the call appearance to manage duration cases for
// different events.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class TIMEREVENT : public CObject
{                        
public:
    enum EventType { 
    	MediaControlMedia = 1, 
    	MediaControlTone, 
    	ToneDetect
    };
    
    DWORD dwEndTime;                     // Time this event expires (TickCount)
    int iEventType;                      // Event type (EventType or user-defined)
    DWORD dwData1;                       // Data dependant upon event type.
    DWORD dwData2;                       // Data dependant upon event type.
};

/******************************************************************************/
//
// CALLIDENTIFIER
//
// This structure defines a call which is connected to one of our call 
// appearances as a desintation.  It is used to provider CALLERID information
// to the CALLINFO structure.
//       
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class CALLIDENTIFIER : public CObject
{
public:
    CString strPartyId;             // Party id number information
    CString strPartyName;           // Name of party
};

/******************************************************************************/
// 
// TERMINALINFO
//
// This structure defines a terminal to our line connection.  Each added
// terminal will have a structure of this type added to the 'arrTerminals'.
// list in the line connection.  Each address/call will have a DWORD list
// describing the mode of the terminal info (superceding the dwMode here).
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class TERMINALINFO : public CObject
{
public:
    CString         strName;        // Name of the terminal
    LINETERMCAPS    Capabilities;   // Capabilities of the terminal
    DWORD           dwMode;         // Base line terminal mode
};

/******************************************************************************/
//
// USERUSERINFO
//
// This structure is used to store the internal reference to USERUSER
// information received on an ISDN line.
//
/******************************************************************************/
class USERUSERINFO : public CObject
{
public:
	LPVOID lpvBuff;
	DWORD dwSize;
	USERUSERINFO(LPVOID lpvBuff, DWORD dwSize);
	virtual ~USERUSERINFO();
};

// Define common states used in most state machines
#define STATE_UNINITIALIZED  -1
#define STATE_INITIAL        0
#define STATE_IGNORE         0xff

// Define our call types (additional types may be added for derived classes)
#define CALLTYPE_NORMAL      1
#define CALLTYPE_CONFERENCE  2
#define CALLTYPE_CONSULTANT  3

/******************************************************************************/
//
// CTSPIRequest class
//
// This class defines a request to the TAPI device.  Each request
// is built and stored in the asynchnous request list maintained by the
// device class.  As a request completes, the request id will
// be sent back via the ASYNCH callback.
//
// The state and state data fields are usable by the CServiceProvider class
// to manage the status of the request.  Since the service provider model is
// generally managed as a state machine, these fields allow the request to
// be used without overriding in most cases.
//
/******************************************************************************/
class CTSPIRequest : public CObject
{
    DECLARE_DYNCREATE( CTSPIRequest )   // Allows dynamic creation of object

// Class data
protected:
    CTSPIConnection* m_pConnOwner;      // Connection for this request
    CTSPIAddressInfo* m_pAddress;       // Address for request
    CTSPICallAppearance* m_pCall;       // Call connection.
    DRV_REQUESTID m_dwRequestId;        // Asynch request id for this request
    LPVOID m_lpData;                    // Ptr to data for request (may be object)
    DWORD m_dwSize;                     // Size of above data block if not str.
    DWORD m_dwAppData;                  // State-specific data
    HANDLE m_hEvent;                    // This is an event handle used when waiting on the request.
    LONG m_lResult;                     // This is the final result for this request.
    BOOL m_fResponseSent;               // Flag indicating we have already responded to TAPI.
    WORD m_wReqType;                    // Type of request (COMMAND)
    int m_iReqState;                    // Current state (set by processor)

// Constructor
protected:
    CTSPIRequest();
public:
    virtual ~CTSPIRequest();

// Methods
public:
    // These are the QUERYxxx functions                             
    CTSPIConnection* GetConnectionInfo() const;
    CTSPICallAppearance* GetCallInfo() const;
    CTSPIAddressInfo* GetAddressInfo() const;
    WORD GetCommand() const;
    DRV_REQUESTID GetAsynchRequestId() const;
    LPVOID GetDataPtr() const;
    DWORD GetDataSize() const;
    int GetState() const;
    DWORD GetStateData() const;
    BOOL HaveSentResponse() const;

    // These are the SETxxx functions
    VOID SetCommand(WORD wCommand);
    VOID SetState(int iState);
    VOID SetStateData(DWORD dwStateData);
	VOID SetCallInfo(CTSPICallAppearance* pCall);
	VOID SetDataPtr(LPVOID lpBuff);
	VOID SetDataSize(DWORD dwSize);

    static BOOL IsAddressOk(LPVOID lpBuff, DWORD dwSize);

// Overridable methods
public:
    // This function pauses a thread when it wants to wait for the request to complete.
    virtual LONG WaitForCompletion (DWORD dwMSecs=INFINITE);

#ifdef _DEBUG
    virtual VOID Dump(CDumpContext& de) const;
	LPCSTR GetRequestTypeName() const;
#endif

// Internal functions
protected:
    friend class CTSPIConnection;
    friend class CTSPILineConnection;
    friend class CTSPIAddressInfo;

    virtual VOID Init(CTSPIConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, WORD wRequest, DRV_REQUESTID dwRequestId, LPVOID lpBuff, DWORD dwSize);
    virtual VOID Complete (LONG lResult=0L, BOOL fSentTapiNotification=TRUE);
};

// This is the token sent to ReceiveData() when the library is initiating
// a new request due to it being added to the asynch request list.
#define STARTING_COMMAND        0x0000

// Define all the asynchronous request types supported directly
// by the base class.  For each TAPI function which passes a 
// DRV_REQUESTID, there should be a request.

// TSPI_line requests
#define REQUEST_ACCEPT           0x0001      // TSPI_lineAccept
#define REQUEST_ADDCONF          0x0002      // TSPI_lineAddToConference
#define REQUEST_ANSWER           0x0003      // TSPI_lineAnswer
#define REQUEST_BLINDXFER        0x0004      // TSPI_lineBlindTransfer
#define REQUEST_COMPLETECALL     0x0005      // TSPI_lineCompleteCall
#define REQUEST_COMPLETEXFER     0x0006      // TSPI_lineCompleteTransfer
#define REQUEST_DIAL             0x0007      // TSPI_lineDial
#define REQUEST_DROPCALL         0x0008      // TSPI_lineDropCall
#define REQUEST_FORWARD          0x0009      // TSPI_lineForward
#define REQUEST_HOLD             0x000A      // TSPI_lineHold
#define REQUEST_MAKECALL         0x000B      // TSPI_lineMakeCall
#define REQUEST_PARK             0x000C      // TSPI_linePark
#define REQUEST_PICKUP           0x000D      // TSPI_linePickup
#define REQUEST_REDIRECT         0x000E      // TSPI_lineRedirect
#define REQUEST_REMOVEFROMCONF   0x000F      // TSPI_lineRemoveFromConference
#define REQUEST_SECURECALL       0x0010      // TSPI_lineSecureCall
#define REQUEST_SENDUSERINFO     0x0011      // TSPI_lineSendUserToUser
#define REQUEST_SETCALLPARAMS    0x0012      // TSPI_lineSetCallParams
#define REQUEST_SETTERMINAL      0x0013      // TSPI_lineSetTerminal
#define REQUEST_SETUPCONF        0x0014      // TSPI_lineSetupConference
#define REQUEST_SETUPXFER        0x0015      // TSPI_lineSetupTransfer
#define REQUEST_SWAPHOLD         0x0016      // TSPI_lineSwapHold
#define REQUEST_UNCOMPLETECALL   0x0017      // TSPI_lineUncompleteCall
#define REQUEST_UNHOLD           0x0018      // TSPI_lineUnhold
#define REQUEST_UNPARK           0x0019      // TSPI_lineUnpark
#define REQUEST_MEDIACONTROL     0x001A      // TSPI_lineSetMediaControl (when event is seen)
#define REQUEST_MONITORDIGITS    0x001B      // TSPI_lineMonitorDigits
#define REQUEST_MONITORMEDIA     0x001C      // TSPI_lineMonitorMedia
#define REQUEST_MONITORTONES     0x001D      // TSPI_lineMonitorTones
#define REQUEST_PREPAREADDCONF   0x001E      // TSPI_linePrepareAddToConference
#define REQUEST_GATHERDIGITS     0x001F      // TSPI_lineGatherDigits
#define REQUEST_GENERATEDIGITS   0x0020      // TSPI_lineGenerateDigits
#define REQUEST_GENERATETONE     0x0021      // TSPI_lineGenerateTones
#define REQUEST_DROPCALLONCLOSE  0x0022      // TSPI_lineDropOnClose   (v1.4)
#define REQUEST_DROPNOOWNER      0x0023      // TSPI_lineDropNoOwner   (v1.4)
#define REQUEST_RELEASEUSERINFO  0x0024      // TSPI_lineReleaseUserUserInfo (v1.4)

// TSPI_phone requests
#define REQUEST_SETBUTTONINFO    0x0030      // TSPI_phoneSetButtonInfo
#define REQUEST_SETDISPLAY       0x0031      // TSPI_phoneSetDisplay
#define REQUEST_SETHOOKSWITCHGAIN 0x0032     // TSPI_phoneSetGain
#define REQUEST_SETHOOKSWITCH    0x0033      // TSPI_phoneSetHookswitch
#define REQUEST_SETLAMP          0x0034      // TSPI_phoneSetLamp
#define REQUEST_SETRING          0x0035      // TSPI_phoneSetRing
#define REQUEST_SETHOOKSWITCHVOL 0x0036      // TSPI_phoneSetVolume
#define REQUEST_SETPHONEDATA     0x0037      // TSPI_phoneSetData
#define REQUEST_GETPHONEDATA     0x0038      // TSPI_phoneGetData

// All derived request types should follow this entry.  These would
// include specialized request commands for TSPI_lineDevSpecific,
// TSPI_lineDevSpecificFeature, and TSPI_phoneSetData processing.  
// Since these must be handled completely by the derived class, no 
// command exists for them, this way, the derived class could have 
// SEVERAL commands depending on the parameters which are passed.

#define REQUEST_END              0x1000

// This special flag is used to clear out the asynchronous queue when
// a device is closed.

#define REQUEST_ALL              0xffff      

/******************************************************************************/
// 
// CTSPIBaseObject
//
// Basic object used for synchronization of classes under Win32.
//
/******************************************************************************/
class CTSPIBaseObject : public CObject
{
	DECLARE_DYNCREATE (CTSPIBaseObject)
// Class data
protected:
	DWORD m_dwItemData;					// Item data for developer use.
// Members
public:
	CTSPIBaseObject();
	DWORD GetItemData() const;
	void* GetItemDataPtr() const;
	void SetItemData(DWORD dwItem);
	void SetItemDataPtr(void* pItem);
};

/******************************************************************************/
//
// CTSPIConnection class
//
// This class defines a connection to our TSP.  This base class is
// derived from for a phone or line device.
//
// This class supports multiple requests by keeping each request in 
// an object list.  As a request is fielded by the service provider,
// the next request for this line is pulled from the front of the
// list and activated.
//
/******************************************************************************/
class CTSPIConnection : public CTSPIBaseObject
{
    DECLARE_DYNAMIC( CTSPIConnection )  // Don't allow direct replacement

// Class data.
protected:
    CTSPIDevice* m_pDevice;			// Pointer to our device association
    CString m_strName;				// Line/Phone name "MyPhone"
    CString m_strDevInfo;			// Device information for this line or phone.
    DWORD m_dwDeviceID;				// TAPI line/phone device identifier.
    DWORD m_dwNegotiatedVersion;	// Negotiated TAPI version for this connection.
    CObList m_oblAsynchRequests;	// Asynchronous requests list for this connection.

// Constructors
protected:
    CTSPIConnection();      // Should only be created by the CreateObject() method.
public:
    virtual ~CTSPIConnection();

// Methods
public:
    // These are the QUERYxxx functions
    CTSPIDevice* GetDeviceInfo() const;
    const char * GetName() const;
    const char * GetConnInfo() const;
    DWORD GetDeviceID() const;
    DWORD GetNegotiatedVersion() const;
    BOOL IsLineDevice() const;
    BOOL IsPhoneDevice() const;
    
    // These are the SETxxx functions
    VOID SetName(LPCSTR lpszName);   
    VOID SetConnInfo(LPCSTR lpszInfo);

	// This function calls SendData().
    BOOL SendString (LPCSTR lpszBuff);

    // These functions manage our asynchronous request list.
    CTSPIRequest* AddAsynchRequest(CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
    CTSPIRequest* AddAsynchRequest(CTSPICallAppearance* pCall, WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
    BOOL RemoveRequest(CTSPIRequest* pReq, BOOL fDelete=FALSE);
    int GetRequestCount() const;
    CTSPIRequest* GetRequest(int iPos) const;
    VOID RemovePendingRequests(CTSPICallAppearance* pCall = NULL, WORD wReqType = REQUEST_ALL, LONG lResult=TAPIERR_REQUESTCANCELLED, BOOL fOnlyUnstarted=FALSE);
    VOID CompleteRequest (CTSPIRequest* pReq, LONG lResult = 0, BOOL fTellTAPI = TRUE, BOOL fRemove = TRUE);
    BOOL CompleteCurrentRequest(LONG lResult = 0, BOOL fTellTAPI = TRUE, BOOL fRemove = TRUE);
    CTSPIRequest* FindRequest(CTSPICallAppearance* pCall, WORD wReqType);
    VOID WaitForAllRequests(CTSPICallAppearance* pCall=NULL, WORD wRequest=REQUEST_ALL);

// Overridable functions
public:
    // This is the init method required to initialize our connection object.
    virtual VOID Init(CTSPIDevice* pDeviceOwner, DWORD dwDeviceID);

	// Required overrides by derived classes
    virtual DWORD GetPermanentDeviceID() const = 0;

	// Overridable functions managing request list	
	virtual LONG WaitForRequest(DWORD dwTimeout = 0L, CTSPIRequest* pReq = NULL);
    virtual CTSPIRequest* GetCurrentRequest() const;

    // These functions will call back to the service provider if not supplied.
    virtual BOOL OpenDevice();
    virtual BOOL CloseDevice();
    virtual BOOL SendData (LPCVOID lpBuff, DWORD dwSize);
    virtual BOOL ReceiveData (DWORD dwData=0, const LPVOID lpBuff=NULL, DWORD dwSize=0);

#ifdef _DEBUG
    virtual VOID Dump(CDumpContext& de) const;
#endif

// Internal methods
protected:
    friend class CTSPIRequest;
    friend class CServiceProvider;
    friend class CTSPIDevice;
	virtual BOOL AddAsynchRequest (CTSPIRequest* pReq, int iPos);
	virtual BOOL OnNewRequest (CTSPIRequest* pReq, int* piPos);
	virtual VOID OnCancelRequest (CTSPIRequest* pReq);
    virtual VOID OnRequestComplete (CTSPIRequest* pReq, LONG lResult) = 0;
    virtual VOID OnTimer();
    virtual BOOL IsMatchingRequest (CTSPIRequest* pReq, CTSPICallAppearance* pCall, WORD wRequest, BOOL fOnlyUnstarted);
    void SetDeviceID(DWORD dwId);
};

/******************************************************************************/
//
// CTSPIDevice
//
// This class defines a specific device controlled through the TSPI
// interface.
//
// This class handles multiple lines and phones by placing each
// connection object into a line or phone array.
//
/******************************************************************************/
class CTSPIDevice : public CTSPIBaseObject
{
    DECLARE_DYNCREATE( CTSPIDevice ) // For dynamic object creation

// Class data
protected:
    DWORD       m_dwProviderId;     // Our permanent provider id assigned by TAPI.
    CObArray    m_arrayLines;       // List of CTSPIConnection line structures
    CObArray    m_arrayPhones;      // List of CTSPIConnection phone structures
    ASYNC_COMPLETION m_lpfnCompletionProc;   // Our asynchronous completion callback

// Constructors
protected:
    CTSPIDevice();  // Should only be created through the CreateObject() method.
public:
    virtual ~CTSPIDevice();

// Methods
public:
    // These retrieves the provider information
    DWORD GetProviderID() const;
    DWORD GetPermanentDeviceID() const;
    int GetLineCount() const;
    int GetPhoneCount() const;

    // These support Plug&Play for the devices
    int CreateLine();
    int CreatePhone();

    // This set of methods accesses the connection arrays to retrieve connection info structures.
    CTSPILineConnection* GetLineConnectionInfo(int nIndex) const;
    CTSPIPhoneConnection* GetPhoneConnectionInfo(int nIndex) const;
    CTSPILineConnection* FindLineConnectionByDeviceID(DWORD dwDeviceId) const;
    CTSPIPhoneConnection* FindPhoneConnectionByDeviceID(DWORD dwDeviceId) const;

// Overridable methods
public:
    // This distributes a received data buffer to all or a particular connection.
    virtual VOID ReceiveData (DWORD dwConnID, DWORD dwData, const LPVOID lpBuff, DWORD dwSize);

	// This is called by the connection object to open the device.  Default behavior is
	// to pass it through to the service provider object.
	virtual BOOL OpenDevice (CTSPIConnection* pConn);

	// This is called by the connection object to close the device.  Default behavior is
	// to pass it through to the service provider object.
	virtual BOOL CloseDevice (CTSPIConnection* pConn);

	// This is called by the connections to send data - default behavior is to pass through
	// to the CServiceProvider class
	virtual BOOL SendData (CTSPIConnection* pConn, LPCVOID lpBuff, DWORD dwSize);

// Internal methods
protected:
    friend class CTSPILineConnection;
    friend class CTSPIPhoneConnection;
    friend class CTSPICallAppearance;
    friend class CServiceProvider;
    friend class CTSPIConnection;

    // This function is called directly after the constructor to actually initialize the
    // device object.  Once this completes, the device should be ready to be queried by TAPI.
    virtual VOID Init(DWORD dwProviderId, DWORD dwBaseLine, DWORD dwBasePhone, DWORD dwLines, DWORD dwPhones, ASYNC_COMPLETION lpfnCompletion);
    
    // The asynchronous callback is used when a TAPI request has finished
    // and we used the asynchronous handle given to us by TAPI.
    virtual VOID OnAsynchRequestComplete(LONG lResult = 0L, CTSPIRequest* pReq = NULL);

	// This function is called peridically to process the interval timer.
    virtual VOID OnTimer();

	// This is called when a new request is inserted by the line or phone connection.
	// It passes the notification onto the CServiceProvider object by default.
	virtual BOOL OnNewRequest (CTSPIConnection* pConn, CTSPIRequest* pReq, int* piPos);

    // This method is called when a request is canceled and it has already
    // been started on the device.  It is called by the connection object when
	// the request is being deleted.  Default behavior is to pass through to the
	// CServiceProvider class.
    virtual VOID OnCancelRequest (CTSPIRequest* pReq);

    // These are called to add internal connection information to arrays.
    WORD AddLineConnectionInfo(CTSPILineConnection* pConn);
    WORD AddPhoneConnectionInfo(CTSPIPhoneConnection* pConn);
};

/******************************************************************************/
//
// CTSPILineConnection class
//
// This class describes a line connection for TAPI.  It is based
// off the above CTSPIConnection class but contains data and
// methods specific to controlling a line device.
//
// This class in turn holds an array of calls, one of which may be
// active on the line.  Each call can have an address, a call state,
// and report activity to TAPI.
//
/******************************************************************************/
class CTSPILineConnection : public CTSPIConnection
{
    DECLARE_DYNCREATE( CTSPILineConnection ) // Allow dynamic creation

// Class data
protected:
    HTAPILINE m_htLine;             // TAPI opaque line handle
    LINEDEVCAPS m_LineCaps;         // Line device capabilities
    LINEDEVSTATUS m_LineStatus;     // Line device status
    LINEEVENT m_lpfnEventProc;      // TAPI event callback for line events
    DWORD m_dwLineMediaModes;       // Current media modes of interest to TAPI.
    DWORD m_dwLineStates;           // Which status messages need to be sent to TAPI.DLL?
	DWORD m_dwConnectedCallCount;	// Total count of CONNECTED cals on line	
    CObArray m_arrAddresses;        // Addresses on this line (fixed)
    CObArray m_arrTerminalInfo;     // Terminal information
    CUIntArray m_arrRingCounts;     // Ring counts for each of the ring modes.
    CObList m_lstCompletions;       // Current outstanding call completions

// Constructor
protected:
    CTSPILineConnection();          // Should only be created by the CreateObject method.
public:
    virtual ~CTSPILineConnection();

// Methods
public:
    // The TAPI line handle defines the line we are connected to for TAPI.
    // It is passed as the first parameter for the callbacks.
    HTAPILINE GetLineHandle() const;
    
    // Function which must be called by the derived class to initialize all the
    // addresses available on this line.  Each address must be added in order to
    // have call appearances appear for this line.
    DWORD CreateAddress (LPCSTR lpszDialableAddr=NULL, LPCSTR lpszAddrName=NULL, 
                         BOOL fAllowIncoming=TRUE, BOOL fAllowOutgoing=NULL,
                         DWORD dwAvailMediaModes=LINEMEDIAMODE_UNKNOWN,
                         DWORD dwBearerMode=LINEBEARERMODE_VOICE,
                         DWORD dwMinRate=0L, DWORD dwMaxRate=0L, LPLINEDIALPARAMS lpDialParams=NULL,
                         DWORD dwMaxNumActiveCalls=1, DWORD dwMaxNumOnHoldCalls=0, 
                         DWORD dwMaxNumOnHoldPendCalls=0, DWORD dwMaxNumConference=0, 
                         DWORD dwMaxNumTransConf=0);
                         
    DWORD GetAddressCount() const;
	CTSPIAddressInfo* GetAddress (int iAddress) const;
    CTSPIAddressInfo* GetAddress (DWORD dwAddressID) const;
    CTSPIAddressInfo* GetAddress (LPCSTR lpszDialableAddr) const;

    // This function will run through all our addresses and see if any support
    // the specified media modes.  It will return success if any do.    
    DWORD GetDefaultMediaDetection() const;

    // Get a pointer to the line device capabilities
    LPLINEDEVCAPS GetLineDevCaps();
    LPLINEDEVSTATUS GetLineDevStatus();

	// Call location methods
    CTSPICallAppearance* FindCallByState(DWORD dwCallState) const;
    
    // Call completion support
    TSPICOMPLETECALL* FindCallCompletionRequest (DWORD dwSwitchInfo, const char * pszSwitchInfo);
    TSPICOMPLETECALL* FindCallCompletionRequest(CTSPICallAppearance* pCall);
    TSPICOMPLETECALL* FindCallCompletionRequest(DWORD dwCompletionID);
    VOID RemoveCallCompletionRequest(DWORD dwCompletionID, BOOL fNotifyTAPI=FALSE);

    // Terminal support functions.  A terminal is a notification device of a line.  The various
    // types are defined by the LINETERMMODE_xxx bits.  To support terminals, simply add each terminal
    // at any time (TAPI is notified).
    int AddTerminal (LPCSTR lpszName, LINETERMCAPS& Caps, DWORD dwModes=0L);
    VOID RemoveTerminal (int iTerminalId);
    int GetTerminalCount() const;
    DWORD GetTerminalInformation (int iTerminalID) const;

	// This is used to send a notification to TAPI for this line device.
    VOID Send_TAPI_Event(CTSPICallAppearance* pCall, DWORD dwMsg, DWORD dwP1 = 0L, DWORD dwP2 = 0L, DWORD dwP3 = 0L);

    // Members which set status information for this line.
    VOID SetBatteryLevel (DWORD dwBattery);
    VOID SetSignalLevel (DWORD dwSignal);
    VOID SetRoamMode (DWORD dwRoamMode);
    VOID SetDeviceStatusFlags (DWORD dwStatus);
    VOID SetRingMode (DWORD dwRingMode);
    VOID SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal);
	VOID SetLineFeatures (DWORD dwFeatures);

	// Force the line to close
	void ForceClose();

    // Method which should be called when a ring is detected on this line.
    VOID OnRingDetected (DWORD dwRingMode, BOOL fStopRinging=FALSE);

// Overridable methods
public:
    // Unique id giving device and line.
    virtual DWORD GetPermanentDeviceID() const;

    // Method which is called to verify CallParameters supported
    virtual LONG CanSupportCall (const LPLINECALLPARAMS lpCallParams) const;

    // Method which validates a media control list for this line (called by lineSetMediaControl).
    virtual LONG ValidateMediaControlList(TSPIMEDIACONTROL* lpMediaControl) const;

	// Function which locates the address to use for a new call against a line object.
    virtual CTSPIAddressInfo* FindAvailableAddress (const LPLINECALLPARAMS lpCallParams, DWORD dwFeature=0) const;

    // TAPI events
    virtual LONG Open(HTAPILINE htLine, LINEEVENT lpfnEventProc, DWORD dwTSPIVersion);
    virtual LONG Close();
    virtual LONG GetAddressID(LPDWORD lpdwAddressId, DWORD dwAddressMode, LPCSTR lpszAddress, DWORD dwSize);
    virtual LONG SetTerminal (DRV_REQUESTID dwReqID, TSPILINESETTERMINAL* lpLine);
    virtual LONG Forward (DRV_REQUESTID dwRequestId, CTSPIAddressInfo* pAddr, TSPILINEFORWARD* lpForwardInfo, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall);
    virtual LONG MakeCall (DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, TSPIMAKECALL* lpMakeCall);
    virtual LONG SetDefaultMediaDetection (DWORD dwMediaModes);
    virtual LONG SetStatusMessages(DWORD dwLineStates, DWORD dwAddressStates);
    virtual LONG ConditionalMediaDetection(DWORD dwMediaModes, LPLINECALLPARAMS const lpCallParams);
    virtual LONG SetMediaControl (TSPIMEDIACONTROL* lpMediaControl);
    virtual LONG GatherCapabilities (DWORD dwTSPIVersion, DWORD dwExtVer, LPLINEDEVCAPS lpLineCaps);
    virtual LONG GatherStatus (LPLINEDEVSTATUS);
    virtual LONG UncompleteCall (DRV_REQUESTID dwRequestID, DWORD dwCompletionID);
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID);
    virtual LONG GetDevConfig(CString& strDeviceClass, LPVARSTRING lpDeviceConfig);
    virtual LONG SetDevConfig(CString& strDeviceClass, LPVOID const lpDevConfig, DWORD dwSize);
    virtual LONG DevSpecificFeature(DWORD dwFeature, DRV_REQUESTID dwRequestId, LPVOID lpParams, DWORD dwSize);
    virtual LONG GetIcon (CString& strDevClass, LPHICON lphIcon);
    virtual LONG ConfigDialog (CWnd* pwndOwner, CString& strDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize, LPVARSTRING lpDeviceConfigOut);
    
	// These can be used to notify TAPI about changes to LINEDEVCAPS or LINEDEVSTATUS.
    virtual VOID OnLineCapabiltiesChanged();
    virtual VOID OnLineStatusChange (DWORD dwState, DWORD dwP2=0L, DWORD dwP3=0L);
	virtual VOID OnMediaConfigChanged();

// Internal methods
protected:
    friend class CTSPIAddressInfo;
    friend class CTSPICallAppearance;
    friend class CServiceProvider;
    friend class CTSPIDevice;                                               
    
	// Overridable functions and notifications
    virtual VOID Init(CTSPIDevice* pDev, DWORD dwLineDeviceID, DWORD dwPos);
	virtual DWORD OnAddressFeaturesChanged (CTSPIAddressInfo* pAddr, DWORD dwFeatures);
    virtual VOID OnCallDeleted(CTSPICallAppearance* pCall);
	virtual DWORD OnCallFeaturesChanged(CTSPICallAppearance* pCall, DWORD dwCallFeatures);
    virtual VOID OnCallStateChange (CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, DWORD dwNewState, DWORD dwOldState);
	virtual DWORD OnLineFeaturesChanged(DWORD dwLineFeatures);
    virtual VOID OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
    virtual VOID OnTimer();
	virtual VOID OnConnectedCallCountChange(CTSPIAddressInfo* pInfo, int iDelta);

	// Callable functions from within the line object
    BOOL CanHandleRequest(WORD wRequest, DWORD dwData=0);
    BOOL IsConferenceAvailable(CTSPICallAppearance* pCall);
    BOOL IsTransferConsultAvailable(CTSPICallAppearance* pCall);
};

/******************************************************************************/
// 
// CTSPIAddressInfo
//
// This class defines the address information for a single address on 
// a line.  The address may contain one or more call appearances (although 
// generally only one is active at a time).
//
// This structure is new for version 1.21 and it allows for multiple call
// appearances on an address which was a restriction in 1.20
// 
/******************************************************************************/
class CTSPIAddressInfo : public CTSPIBaseObject
{
    DECLARE_DYNCREATE (CTSPIAddressInfo) // Allow dynamic creation

// Class data
protected:
    CTSPILineConnection* m_pLine;		// Line owner
    LINEADDRESSCAPS m_AddressCaps;		// Basic address capabilities
    LINEADDRESSSTATUS m_AddressStatus;	// Current available status on this address
    DWORD m_dwAddressID;				// Address identifier (0-numAddr).
    DWORD m_dwAddressStates;			// Which address state change messages need to be sent to TAPI.DLL?
    DWORD m_dwAvailMediaModes;			// Available media modes on this address
    DWORD m_dwBearerMode;				// Available bearer mode for this address
    DWORD m_dwMinRateAvail;				// Minimum data rate on data stream (0 if not supported)
    DWORD m_dwMaxRateAvail;				// Maximum data rate on data stream (0 if not supported)
    DWORD m_dwCurrRate;					// Current data rate on address
	DWORD m_dwConnectedCallCount;		// Total count of CONNECTED cals on address
    CString m_strAddress;				// Dialable address (phone#).
    CString m_strName;					// Name of owner for this address (for callerid on outgoing calls)
    CObList m_lstCalls;					// Call appearances on this address (dynamic).
    CDWordArray m_arrTerminals;			// Terminal array
    CObArray m_arrForwardInfo;			// Forwarding information if available.
    int m_fInputAvail  : 1;				// Address may answer calls
    int m_fOutputAvail : 1;				// Address may place calls
    CStringArray m_arrCompletionMsgs;	// Completion message information
    LINEDIALPARAMS m_DialParams;		// Dialing parameters supported on address
    TSPIMEDIACONTROL* m_lpMediaControl; // Current MEDIACONTROL in effect (NULL if none).

// Constructor
protected:
    CTSPIAddressInfo();         // Should only be created by the CreateObject method.
public:
    virtual ~CTSPIAddressInfo();

// Access methods
public:
    // The following are the QUERYxxx functions
    DWORD GetAddressID() const;
    LPLINEADDRESSCAPS GetAddressCaps();
    LPLINEADDRESSSTATUS GetAddressStatus();
    CTSPILineConnection* GetLineOwner() const;
    BOOL CanAnswerCalls() const;
    BOOL CanMakeCalls() const;
    DWORD GetBearerMode() const;
    DWORD GetCurrentRate() const;

	// These allow changing the name/address of the object if it cannot be determined
	// at INIT time (such as configuring a newly added address.
    LPCSTR GetDialableAddress() const;
	VOID SetDialableAddress(LPCSTR pszAddress);
    LPCTSTR GetName() const;
	VOID SetName (LPCTSTR pwszName);

    // Media mode support
    DWORD GetAvailableMediaModes () const;

	// This function creates a new call on the address.  This would be used for incoming/outgoing calls.
    CTSPICallAppearance* CreateCallAppearance(HTAPICALL hCall=NULL, DWORD dwCallParamFlags=0, DWORD dwOrigin=LINECALLORIGIN_UNKNOWN,
                                    DWORD dwReason=LINECALLREASON_UNKNOWN, DWORD dwTrunk=0xffff, DWORD dwCompletionID=0);

	// This function creates a new CONFERENCE object to manage a conference call on the address.
    CTSPIConferenceCall* CreateConferenceCall(HTAPICALL hCall);

	// This function removes a call object (either normal or conference).  The call is deleted.
    VOID RemoveCallAppearance(CTSPICallAppearance* pCall);

    // Methods to access/manipulate the call list.
	int GetCallCount() const;
    CTSPICallAppearance* GetCallInfo(int iPos) const;
    CTSPICallAppearance* FindCallByState(DWORD dwCallState) const;
    CTSPICallAppearance* FindCallByHandle (HTAPICALL hCall) const;
    CTSPICallAppearance* FindCallByCallID (DWORD dwCallID) const;
	CTSPICallAppearance* FindAttachedCall (CTSPICallAppearance* pSCall) const;

    // Forwarding information - for existing forward information to be added.  For any forwards
    // performed by service provider, the information will automatically be added to the
    // array when the forward request completes and returns success.
    int AddForwardEntry (DWORD dwForwardMode, const char *pszCaller, const char *pszDestination, DWORD dwDestCountry);

    // Completion message support
    int AddCompletionMessage (const char *pszBuff);
    int GetCompletionMessageCount() const;
    const char * GetCompletionMessage (int iPos) const;

    // The following are the SETxxx functions
    VOID SetNumRingsNoAnswer (DWORD dwNumRings);
    VOID SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal);
	VOID SetCurrentRate (DWORD dwRate);
	VOID SetAddressFeatures(DWORD dwFeatures);

// Overridable functions
public:
    // Method which can be overriden to affect call selection during lineMakeCall and
    // verification of a CallParams structure.
    virtual LONG CanSupportCall (const LPLINECALLPARAMS lpCallParams) const;

    // Method which can be overriden to check forwarding information for this address.
    virtual LONG CanForward(TSPILINEFORWARD* lpForwardInfo, int iCount);
                 
    // This is called to determine if the address can support a particular set of media modes.
    virtual BOOL CanSupportMediaModes (DWORD dwMediaModes) const;

    // TAPI methods
    virtual LONG Unpark (DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, CADObArray* parrAddresses);
    virtual LONG Pickup (DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, TSPILINEPICKUP* lpPickup);
    virtual LONG SetTerminal (DRV_REQUESTID dwReqID, TSPILINESETTERMINAL* lpLine);
    virtual LONG SetupTransfer(DRV_REQUESTID dwRequestID, TSPITRANSFER* lpTransfer, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall);
    virtual LONG CompleteTransfer (DRV_REQUESTID dwRequestId, TSPITRANSFER* lpTransfer, HTAPICALL htConfCall, LPHDRVCALL lphdConfCall);
    virtual LONG SetupConference (DRV_REQUESTID dwRequestID, TSPICONFERENCE* lpConf, HTAPICALL htConfCall, LPHDRVCALL lphdConfCall, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall);
    virtual LONG Forward (DRV_REQUESTID dwRequestId, TSPILINEFORWARD* lpForwardInfo, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall);
    virtual LONG SetMediaControl (TSPIMEDIACONTROL* lpMediaControl);
    virtual LONG GatherCapabilities (DWORD dwTSPIVersion,DWORD dwExtVersion,LPLINEADDRESSCAPS lpAddressCaps);
    virtual LONG GatherStatusInformation (LPLINEADDRESSSTATUS lpAddressStatus);
    virtual VOID SetStatusMessages(DWORD dwStates);
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID);

#ifdef _DEBUG
	virtual void Dump (CDumpContext& dc) const;
#endif

    // The following is called when any of the ADDRESSSTATE entries have changed.  It
    // is used internally, but should also be called if the derived service provider
    // changes address state information itself.
    virtual VOID OnAddressStateChange (DWORD dwAddressState);

    // The following should be called if any of the address capabilities change
    // during the life of the service provider.
    virtual VOID OnAddressCapabiltiesChanged();

// Internal methods
protected:
    friend class CTSPILineConnection;
    friend class CTSPIRequest;
    friend class CTSPICallAppearance;

    // This method is called when the address connection is created directly after
    // the constructor, it is called by the CTSPILineConnection object
    virtual VOID Init (CTSPILineConnection* pLine, DWORD dwAddressID, LPCSTR lpszAddress, LPCSTR lpszName,
                       BOOL fIncoming, BOOL fOutgoing, DWORD dwAvailMediaModes, 
                       DWORD dwAvailBearerModes, DWORD dwMinRate, DWORD dwMaxRate,
                       DWORD dwMaxNumActiveCalls, DWORD dwMaxNumOnHoldCalls, 
                       DWORD dwMaxNumOnHoldPendCalls, DWORD dwMaxNumConference, 
                       DWORD dwMaxNumTransConf);

    // Notification for a new call appearance created
    virtual VOID OnCreateCall (CTSPICallAppearance* pCall);

    // This method is called by the call appearances when the call state changes.
    virtual VOID OnCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD dwOldState);

    // This method is called whenever the terminal line count changes.              
    virtual VOID OnTerminalCountChanged (BOOL fAdded, int iPos, DWORD dwMode=0L);

    // This is called by the perioid timer to do cleanup and event timing.
    virtual VOID OnTimer();    
    
	// Called when a request completes on this address
    virtual VOID OnRequestComplete (CTSPIRequest* pReq, LONG lResult);

	// The following is called by the call appearance when the call features for the
	// call have changed.  The return value is the adjusted call features.
	virtual DWORD OnCallFeaturesChanged(CTSPICallAppearance* pCall, DWORD dwCallFeatures);
	virtual DWORD OnAddressFeaturesChanged (DWORD dwFeatures);

    // Various support methods
    CTSPIRequest* AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
    BOOL CanHandleRequest(WORD wRequest, DWORD dwData=0);
    VOID DeleteForwardingInfo();
    DWORD GetTerminalInformation (int iTerminalID) const;
};

/******************************************************************************/
//
// CTSPICallAppearance
//
// This class defines a specific call on a line device.  Each line
// can have one or more available calls on it.  Each call has a 
// particular state on the line, and is associated to a specific address.
//
/******************************************************************************/
class CTSPICallAppearance : public CTSPIBaseObject
{
   DECLARE_DYNCREATE( CTSPICallAppearance ) // Allow dynamic creation

// Class data
protected:
	enum Flags { 
		IsNewCall	= 0x0001,			// Bit is set if this is a call created by US vs. TAPI.
		IsDeleted	= 0x0002,			// Bit is set when call is no longer active.
		IsDropped	= 0x0004,			// Bit is set when call is being dropped.
		InitNotify  = 0x0008			// Bit is set when TAPI is told about call 1st time.
	};

    int m_iCallType;                // Call type (CALLTYPE_xxxx)
    CTSPIAddressInfo* m_pAddr;      // Address identifier for this call.
	DWORD m_dwFlags;				// Call flags
	DWORD m_dwRefCount;				// Reference count
    LINECALLINFO m_CallInfo;        // Current call information for this object
    LINECALLSTATUS m_CallStatus;    // Current call status
    HTAPICALL m_htCall;             // TAPI opaque call handle
    CALLIDENTIFIER m_CallerID;      // Caller ID information
    CALLIDENTIFIER m_CalledID;      // Called ID information
    CALLIDENTIFIER m_ConnectedID;   // Connected ID information
    CALLIDENTIFIER m_RedirectionID; // Redirection ID information
    CALLIDENTIFIER m_RedirectingID; // Redirecting ID information
    CDWordArray m_arrTerminals;     // Terminal array
    TSPIDIGITGATHER* m_lpGather;    // Current DIGITGATHER in effect (NULL if none).
    TSPIMEDIACONTROL* m_lpMediaControl; // Current MEDIACONTROL in effect (NULL if none).
    CObArray m_arrMonitorTones;     // Current tones being monitored for.
    CObArray m_arrEvents;           // Pending timer events for MEDIACONTROL and TONE DETECT.
    CTSPICallAppearance* m_pConsult;	// Attached consultant call (NULL if none)
	CTSPIConferenceCall* m_pConf;		// Attached conference call (NULL if none)
	CADObArray m_arrUserUserInfo;		// Array of UserUserInfo structures.
    
// Constructor
protected:
    CTSPICallAppearance();
public:
    virtual ~CTSPICallAppearance();

// Public methods
public:
    // The TAPI call handle defines the current call we are working on.
    // There will always be a line handle when connected, but there will
    // only be a call handle when something specific is being performed
    // on a line device through TAPI.
    HTAPICALL GetCallHandle() const;

    // The following are the QUERYxxx functions for the state information of the call.      
    CTSPILineConnection* GetLineConnectionInfo() const;
    CTSPIAddressInfo* GetAddressInfo() const;
    DWORD GetCallState() const;
    LPLINECALLINFO GetCallInfo();
    LPLINECALLSTATUS GetCallStatus();
    CTSPICallAppearance* GetAttachedCall();
	CTSPIConferenceCall* GetConferenceOwner() const;

	// This allows query/set of the call type (consultant/conference/normal)
    int GetCallType() const;
    VOID SetCallType (int iCallType);
    
	// Reference count functions for request management
	// Call will NOT be deleted until final reference count removal.
	void IncRefCount();
	void DecRefCount();
    
    // Internal methods for consultant call management.
    void AttachCall(CTSPICallAppearance* pCall);
	void SetConferenceOwner(CTSPIConferenceCall* pCall);
    void DetachCall();

#ifdef _DEBUG
    const char * GetCallStateName (DWORD dwState=0L) const;
#endif
    // Return whether the supplied callstate is ACTIVE according to TAPI rules.
    static BOOL IsActiveCallState(DWORD dwState);
	static BOOL IsConnectedCallState(DWORD dwState);

// Overridable methods
public:
    // This method should be called whenever a digit is detected.  It manages any monitor
    // or gathering being performed on the call.
    VOID OnDigit (DWORD dwType, char cDigit);
            
    // This method should be called whenever a tone generation is detected.  It manages
    // any monitor or gathering being performed on the call.                
    VOID OnTone (DWORD dwFreq1, DWORD dwFreq2=0, DWORD dwFreq3=0);

    // This method is called during the initial call setup, or when a new media type
    // begins playing over the media stream.  It is automatically called during call state
    // changes if the media mode is adjusted by the SetCallState method.
    VOID OnDetectedNewMediaModes (DWORD dwMediaModes);
    
    // This method should be called if user-user information is received from
    // the underlying network.  The data is COPIED into an internal buffer and
    // may be deleted after the call.
    VOID OnReceivedUserUserInfo (LPVOID lpBuff, DWORD dwSize);
    
    // TAPI methods called by the CServiceProvider class.
    virtual LONG Close();
    virtual LONG Drop(DRV_REQUESTID dwRequestId=0, LPCSTR lpszUserUserInfo=NULL, DWORD dwSize=0);
    virtual LONG DropOnClose();
    virtual LONG DropNoOwner();
    virtual LONG Accept(DRV_REQUESTID dwRequestID, LPCSTR lpszUserUserInfo, DWORD dwSize);
    virtual LONG Answer(DRV_REQUESTID dwReq, LPCSTR lpszUserUserInfo, DWORD dwSize);
    virtual LONG BlindTransfer(DRV_REQUESTID dwRequestId, CADObArray* parrDestAddr, DWORD dwCountryCode);
    virtual LONG Dial (DRV_REQUESTID dwRequestID, CADObArray* parrAddresses, DWORD dwCountryCode);
    virtual LONG Hold (DRV_REQUESTID dwRequestID);
    virtual LONG SwapHold(DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall);
    virtual LONG Unhold (DRV_REQUESTID dwRequestID);
    virtual LONG Secure (DRV_REQUESTID dwRequestID);
    virtual LONG SendUserUserInfo (DRV_REQUESTID dwRequestID, LPCSTR lpszUserUserInfo, DWORD dwSize);
    virtual LONG Park (DRV_REQUESTID dwRequestID, TSPILINEPARK* lpPark);
    virtual LONG Unpark (DRV_REQUESTID dwRequestID, CADObArray* parrAddresses);
    virtual LONG Pickup (DRV_REQUESTID dwRequestID, TSPILINEPICKUP* lpPickup);
    virtual LONG Redirect (DRV_REQUESTID dwRequestID, CADObArray* parrAddresses, DWORD dwCountryCode);
    virtual LONG SetCallParams (DRV_REQUESTID dwRequestID, TSPICALLPARAMS* lpCallParams);
    virtual LONG SetTerminal (DRV_REQUESTID dwReqID, TSPILINESETTERMINAL* lpTermCaps);
    virtual LONG MakeCall (DRV_REQUESTID dwRequestID, TSPIMAKECALL* lpMakeCall);
    virtual LONG GatherDigits (TSPIDIGITGATHER* lpGather);
    virtual LONG GenerateDigits (TSPIGENERATE* lpGenerate);
    virtual LONG GenerateTone (TSPIGENERATE* lpGenerate);
    virtual LONG SetMediaMode (DWORD dwMediaMode); 
    virtual LONG MonitorDigits (DWORD dwDigitModes);
    virtual LONG MonitorMedia (DWORD dwMediaModes);
    virtual LONG MonitorTones (TSPITONEMONITOR* lpMon);
    virtual LONG SetMediaControl (TSPIMEDIACONTROL* lpMediaControl);
    virtual LONG CompleteCall (DRV_REQUESTID dwRequestId, LPDWORD lpdwCompletionID, TSPICOMPLETECALL* lpCompCall);
    virtual LONG GatherStatusInformation(LPLINECALLSTATUS lpCallStatus);
    virtual LONG GatherCallInformation (LPLINECALLINFO lpCallInfo);
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID);
    virtual LONG ReleaseUserUserInfo(DRV_REQUESTID dwRequest);

    // The following are the SETxxx functions for the CALLINFO of the call appearance.
    // They will cause the appropriate LINECALLSTATE message to be generated.
    VOID SetBearerMode(DWORD dwBearerMode);
    VOID SetDataRate(DWORD dwDataRate);
    VOID SetAppSpecificData(DWORD dwAppSpecific);
    VOID SetCallID (DWORD dwCallID);
    VOID SetRelatedCallID (DWORD dwCallID);
    VOID SetCallParameterFlags (DWORD dwFlags);
    VOID SetDialParameters (LINEDIALPARAMS& dp);
    VOID SetCallOrigin(DWORD dwOrigin);
    VOID SetCallReason(DWORD dwReason);
    VOID SetDestinationCountry (DWORD dwCountryCode);
    VOID SetTrunkID (DWORD dwTrunkID);
    VOID SetCallerIDInformation (DWORD dwFlags, LPCSTR lpszPartyID=NULL, LPCSTR lpszName=NULL, DWORD dwCountryCode=0);
    VOID SetCalledIDInformation (DWORD dwFlags, LPCSTR lpszPartyID=NULL, LPCSTR lpszName=NULL, DWORD dwCountryCode=0);
    VOID SetConnectedIDInformation (DWORD dwFlags, LPCSTR lpszPartyID=NULL, LPCSTR lpszName=NULL, DWORD dwCountryCode=0);
    VOID SetRedirectionIDInformation (DWORD dwFlags, LPCSTR lpszPartyID=NULL, LPCSTR lpszName=NULL, DWORD dwCountryCode=0);
    VOID SetRedirectingIDInformation (DWORD dwFlags, LPCSTR lpszPartyID=NULL, LPCSTR lpszName=NULL, DWORD dwCountryCode=0);
    VOID SetCallState(DWORD dwState, DWORD dwMode=0L, DWORD dwMediaMode=0L, BOOL fTellTapi=TRUE);
    VOID SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal);
    VOID SetDigitMonitor(DWORD dwDigitModes);
    VOID SetMediaMonitor(DWORD dwModes);
	VOID SetCallFeatures (DWORD dwFeatures);
    
// Internal methods
protected:
    friend class CTSPIAddressInfo;
    friend class CTSPIConferenceCall;
    friend class CTSPIRequest;
    
    VOID CompleteDigitGather (DWORD dwReason);
    VOID DeleteToneMonitorList();
    BOOL CanHandleRequest (WORD wRequest, DWORD dwData=0);
    CTSPIRequest* AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
    LONG PerformDropCall (WORD wRequest, DRV_REQUESTID dwRequestId=0, LPCSTR lpsUserUserInfo=NULL, DWORD dwSize=0);

    // This is the INIT function which is called directly after the constructor
    virtual VOID Init (CTSPIAddressInfo* pAddr, HTAPICALL hCall, DWORD dwBearerMode = LINEBEARERMODE_VOICE,
                       DWORD dwRate=0, DWORD dwCallParamFlags=0, DWORD dwOrigin=LINECALLORIGIN_UNKNOWN,
                       DWORD dwReason=LINECALLREASON_UNKNOWN, DWORD dwTrunk=0xffffff, 
                       DWORD dwCompletionID=0, BOOL fNewCall=FALSE);

    // This method is called when information in our CALL status record has changed.
    // If the derived class changes data directly in the LINECALLSTATUS record, it
    // should invoke this method to tell TAPI. (Generally this won't happen).
    virtual VOID OnCallStatusChange (DWORD dwCallState, DWORD dwCallInfo, DWORD dwMediaModes);

    // This method is called when a Media Control event is detected.
    virtual VOID OnMediaControl (DWORD dwMediaControl);

    // This method is called when a TONE being monitored for is detected.
    virtual VOID OnToneMonitorDetect (DWORD dwToneListID, DWORD dwAppSpecific);

    // This method is called whenever the line terminal count is changed.
    virtual VOID OnTerminalCountChanged (BOOL fAdded, int iPos, DWORD dwMode=0L);

    // This method is called whenever information in our CALL information record
    // has changed.  If the derived class changes data directly in the LINECALLINFO
    // record, it should invoke this method to tell TAPI.
    virtual VOID OnCallInfoChange (DWORD dwCallInfo);

    // This method is called whenever a call which is related to this call changes
    // state.  The call relationship is made through the CALLINFO dwRelatedCallID field
    // and is used by conference and consultation calls to relate them to a call appearance.
    virtual VOID OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD dwOldState);

    // This method is called internally during the periodic interval timer.  It is used
    // to cancel digit gathering on timeouts.
    virtual VOID OnTimer();

	// Called when a request completes on this call
    virtual VOID OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
};

/******************************************************************************/
// 
// CTSPIConferenceCall
//
// This class describes a conference call.  It is derived from a basic
// call appearance, but is a special type of call which maintains a list
// of calls which are part of the conference.  Anything performed to the
// conference call could potentially effect all the calls within the 
// conference itself.
//
// It can be identified with a CallType of CALLTYPE_CONFERENCE.
//
/******************************************************************************/
class CTSPIConferenceCall : public CTSPICallAppearance
{
   DECLARE_DYNCREATE( CTSPIConferenceCall ) // Allow dynamic creation

// Class data
protected:
    CObArray m_arrConference;    // Array of CTSPICallAppearance ptrs    

// Constructor
protected:
    CTSPIConferenceCall();
public:
    virtual ~CTSPIConferenceCall();
    
// Methods specific for conferencing.   
public:                                                                                                           
    virtual LONG PrepareAddToConference(DRV_REQUESTID dwRequestID, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall, TSPICONFERENCE* lpConf);
    virtual LONG AddToConference (DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall, TSPICONFERENCE* lpConf);
    virtual LONG RemoveFromConference(DRV_REQUESTID dwRequestID, CTSPICallAppearance* pCall, TSPICONFERENCE* lpConf);
    
    // Methods to manipulate the conference call list.
    int GetConferenceCount() const;
    CTSPICallAppearance* GetConferenceCall(int iPos);
	void AddToConference(CTSPICallAppearance* pCall);
    void RemoveConferenceCall(CTSPICallAppearance* pCall);

// Internal methods
protected:
    friend CTSPICallAppearance;
    BOOL IsCallInConference(CTSPICallAppearance* pCall) const;
    BOOL CanRemoveFromConference(CTSPICallAppearance* pCall) const;
    virtual VOID OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD dwOldState);
    virtual VOID OnCallStatusChange (DWORD dwState, DWORD dwMode, DWORD dwMediaMode);
    virtual VOID OnConsultantCallIdle();
    virtual VOID OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
};

/******************************************************************************/
//
// CPhoneButtonInfo
//
// This class contains all the elements from the PHONEBUTTONINFO
// structure, but allows the object to be stored in an object list
// and serialized.
//
/******************************************************************************/
class CPhoneButtonInfo : public CTSPIBaseObject
{
   DECLARE_SERIAL( CPhoneButtonInfo )

// Class data
protected:
    DWORD m_dwButtonMode;			// Button mode (PHONEBUTTONMODE_xxx)
    DWORD m_dwButtonFunction;		// Button function (PHONEBUTTONFUNCTION_xxx)
    DWORD m_dwLampMode;				// Current lamp mode (PHONELAMPMODE_xxx)
    DWORD m_dwAvailLampModes;		// Available lamp modes
    DWORD m_dwButtonState;			// Current button state (PHONEBUTTONSTATE_xxx)
    CString m_strButtonDescription; // Button description

// Constructor
public:      
    CPhoneButtonInfo();
    CPhoneButtonInfo(DWORD dwButtonFunction, DWORD dwButtonMode, DWORD dwAvailLamp, DWORD dwLampMode, LPCSTR lpszDesc);

// Access Methods
public:
    const char *GetDescription() const;
    DWORD GetFunction() const;
    DWORD GetButtonMode() const;
    DWORD GetButtonState() const;
    DWORD GetLampMode() const;
    DWORD GetAvailLampModes() const;
    VOID SetLampMode(DWORD dwLampMode);
    const CPhoneButtonInfo& operator=(LPPHONEBUTTONINFO lpPhoneInfo);
    VOID SetButtonInfo (DWORD dwFunction, DWORD dwMode, LPCSTR lpszDesc);
    VOID SetButtonState (DWORD dwState);
    VOID  Serialize(CArchive& ar);
};

/******************************************************************************/
//
// CPhoneButtonArray
//
// This superclass of the CObArray provides support directly for
// CPhoneButtonInfo objects.
//
// THIS IS INTENDED TO BE AN INTERNAL CLASS!
//
/******************************************************************************/
class CPhoneButtonArray : public CObArray
{
    DECLARE_SERIAL( CPhoneButtonArray )
// Class data
protected:
    BOOL m_fDirty;           // Dirty flag (needs to be written).

// Construction
public:
    CPhoneButtonArray();
    virtual ~CPhoneButtonArray();

// Operators
public:
    CPhoneButtonInfo * operator [] (int nIndex) const;
    CPhoneButtonInfo * & operator [] (int nIndex);

// Methods
public:
    BOOL IsDirty() const;
    VOID SetDirtyFlag(BOOL fDirty=TRUE);
    int  FindButton(DWORD dwFunction, DWORD dwMode=PHONEBUTTONMODE_DUMMY, int iCount=1);
    const CPhoneButtonArray& operator=(CPhoneButtonArray& a);
    int Add(CPhoneButtonInfo* pObj);
    int Add(DWORD dwMode, DWORD dwFunc, DWORD dwAvailLampModes, DWORD dwLamp, const char *pszDesc);
    CPhoneButtonInfo * & ElementAt (int nIndex);
    CPhoneButtonInfo * GetAt(int nIndex) const;
    BOOL IsEmpty () const;
    BOOL IsValidIndex (int nIndex) const;
    VOID RemoveAll();
    VOID RemoveAt (int nIndex, int nCount = 1);
    VOID SetAt(int nIndex, CPhoneButtonInfo* pObj);
    VOID Serialize(CArchive& ar);
};

/******************************************************************************/
//
// CPhoneDisplay class
//
// This class manages a virtual "display" with cursor positioning, and
// linefeed interpretation.
//
// THIS IS INTENDED TO BE AN INTERNAL CLASS!
//
/******************************************************************************/
class CPhoneDisplay
{
// Class data
protected:
    LPSTR m_lpsDisplay;     // Buffer for our display
    CSize m_sizDisplay;     // Buffer size
    CPoint m_ptCursor;      // Cursor position
    BYTE m_cLF;             // Line feed character
    
// Methods
public:
    CPhoneDisplay();
    ~CPhoneDisplay();

// Access methods
public:
    VOID Init(int iCols, int iRows, char cLF='\n');
    
    // These query different capabilities of the device.
    LPCSTR GetTextBuffer() const;
    CPoint GetCursorPosition() const;
    CSize GetDisplaySize() const;
    
    // These modify the display buffer
    VOID AddCharacter(char c);
    VOID AddString(LPCSTR lpszText);
    VOID SetCharacterAtPosition(int iCol=-1, int iRow=-1, char c = 0);
    VOID SetCursorPosition(int iCol=-1, int iRow=-1);                  
    VOID Reset();
    VOID ClearRow(int iRow);
};

/******************************************************************************/
//
// CTSPIPhoneConnection class
//
// This class describes a phone connection for TAPI.  It is based
// off the above CTSPIConnection class but contains data and methods
// specific to controlling a phone device.
//
/******************************************************************************/
class CTSPIPhoneConnection : public CTSPIConnection
{
   DECLARE_DYNCREATE( CTSPIPhoneConnection ) // Allow dynamic creation
// Class data
protected:
    HTAPIPHONE m_htPhone;               // TAPI opaque phone handle
    PHONECAPS m_PhoneCaps;              // Phone capabilities
    PHONESTATUS m_PhoneStatus;          // Phone status
    PHONEEVENT m_lpfnEventProc;         // TAPI event callback for phone events
    DWORD m_dwPhoneStates;              // Notify states for phone
    DWORD m_dwButtonModes;              // Notify modes for all buttons
    DWORD m_dwButtonStates;             // Notify states for all buttons      
    CPhoneDisplay m_Display;            // Phone display
    CPhoneButtonArray m_arrButtonInfo;  // Button Information array
    CDWordArray m_arrUploadBuffers;     // Data buffers in upload area on phone
    CDWordArray m_arrDownloadBuffers;   // Sizes of each of the download areas available on phone.

// Constructor
protected:
    CTSPIPhoneConnection();
public:
    virtual ~CTSPIPhoneConnection();

// Methods
public:
    // The TAPI phone handle defines the phone device we are connected
    // to.  It is passed as the first parameter for a phone event callback.
    HTAPIPHONE GetPhoneHandle() const;

    // These functions should be called during intial setup (providerInit)
    // to setup the phone device with the correct count of upload/download buffers
    // display dimensions, and buttons.
    int AddUploadBuffer (DWORD dwSizeOfBuffer);
    int AddDownloadBuffer (DWORD dwSizeOfBuffer);
    VOID SetupDisplay (int iColumns, int iRows, char cLineFeed='\n');
    int AddButton (DWORD dwFunction, DWORD dwMode, DWORD dwAvailLampStates, DWORD dwLampState, LPCSTR lpszText);
    VOID AddHookSwitchDevice (DWORD dwHookSwitchDev, DWORD dwAvailModes, DWORD dwCurrMode, DWORD dwVolume=-1L, DWORD dwGain=-1L);

    // These methods manage the button/lamp pairs for the phone.
    int GetButtonCount() const;
    const CPhoneButtonInfo* GetButtonInfo(int iButtonID) const;
    
    // Misc. functions
    LPPHONECAPS GetPhoneCaps();
    LPPHONESTATUS GetPhoneStatus();
    DWORD GetLampMode (int iButtonId);

    // These methods modify the display
    CString GetDisplayBuffer() const;
    CPoint GetCursorPos() const;
    VOID AddDisplayChar (char cChar);
    VOID SetDisplayChar (int iColumn, int iRow, char cChar);
    VOID ResetDisplay();        
    VOID SetDisplay(const char * pszDisplayBuff);
    VOID SetDisplayCursorPos (int iColumn, int iRow);
    VOID ClearDisplayLine (int iRow);
    VOID AddDisplayString (LPCSTR lpszText);    
    
// Overridable methods
public:
    // Unique id giving device and phone.
    virtual DWORD GetPermanentDeviceID() const;

    // TAPI requests (not all of these are asynchronous).
    virtual LONG Open (HTAPIPHONE htPhone, PHONEEVENT lpfnEventProc, DWORD dwTSPIVersion);
    virtual LONG Close ();
    virtual LONG GetButtonInfo (DWORD dwButtonId, LPPHONEBUTTONINFO lpButtonInfo);
    virtual LONG SetButtonInfo (DRV_REQUESTID dwRequestID, TSPISETBUTTONINFO* lpButtInfo);
    virtual LONG SetLamp (DRV_REQUESTID dwRequestID, TSPISETBUTTONINFO* lpButtInfo);
    virtual LONG GetDisplay (LPVARSTRING lpVarString);
    virtual LONG GetGain (DWORD dwHookSwitchDevice, LPDWORD lpdwGain);
    virtual LONG GetHookSwitch (LPDWORD lpdwHookSwitch);
    virtual LONG GetLamp (DWORD dwButtonId, LPDWORD lpdwLampMode);
    virtual LONG GetVolume (DWORD dwHookSwitchDev, LPDWORD lpdwVolume);
    virtual LONG SetGain (DRV_REQUESTID dwRequestId, TSPIHOOKSWITCHPARAM* pHSParam);
    virtual LONG SetVolume (DRV_REQUESTID dwRequestId, TSPIHOOKSWITCHPARAM* pHSParam);
    virtual LONG SetHookSwitch (DRV_REQUESTID dwRequestId, TSPIHOOKSWITCHPARAM* pHSParam);
    virtual LONG SetRing (DRV_REQUESTID dwRequestID, TSPIRINGPATTERN* pRingPattern);
    virtual LONG SetDisplay (DRV_REQUESTID dwRequestID, TSPIPHONESETDISPLAY* lpDisplay);
    virtual LONG GetData (TSPIPHONEDATA* pPhoneData);
    virtual LONG GetRing (LPDWORD lpdwRingMode, LPDWORD lpdwVolume);
    virtual LONG SetData (DRV_REQUESTID dwRequestID, TSPIPHONEDATA* pPhoneData);
    virtual LONG SetStatusMessages(DWORD dwPhoneStates, DWORD dwButtonModes, DWORD dwButtonStates);
    virtual LONG GatherCapabilities(DWORD dwTSPIVersion, DWORD dwExtVersion, LPPHONECAPS lpPhoneCaps);
    virtual LONG GatherStatus (LPPHONESTATUS lpPhoneStatus);
    virtual LONG DevSpecificFeature(DRV_REQUESTID dwRequestId, LPVOID lpParams, DWORD dwSize);
    virtual LONG GetIcon (CString& strDevClass, LPHICON lphIcon);
    virtual LONG ConfigDialog (CWnd* pwndOwner, CString& strDeviceClass);
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID);
    
    // These are the SETxxx functions which notify TAPI.  They should only
    // be called by the worker code (not by TAPI).
    VOID SetButtonInfo (int iButtonID, DWORD dwFunction, DWORD dwMode, LPCSTR pszName);
    DWORD SetLampState (int iButtonID, DWORD dwLampState);
    DWORD SetButtonState (int iButtonId, DWORD dwButtonState);
    DWORD SetStatusFlags (DWORD dwStatus);
    VOID SetRingMode (DWORD dwRingMode);
    VOID SetRingVolume (DWORD dwRingVolume);
    VOID SetHookSwitch (DWORD dwHookSwitchDev, DWORD dwMode);
    VOID SetVolume (DWORD dwHookSwitchDev, DWORD dwVolume);
    VOID SetGain (DWORD dwHookSwitchDev, DWORD dwGain);

	// Force the phone to close
	void ForceClose();

	// Functions which notify TAPI about changes within data structures.
    virtual VOID OnPhoneCapabiltiesChanged();
    virtual VOID OnPhoneStatusChange(DWORD dwState, DWORD dwParam = 0);

    // The event procedure is used as a callback into TAPISRV.EXE when
    // some event (phone state change, etc.) happens.  It will
    // be initialized when the phone is opened.
    VOID Send_TAPI_Event(DWORD dwMsg, DWORD dwP1 = 0L, DWORD dwP2 = 0L, DWORD dwP3 = 0L);

// Internal methods
protected:
    friend class CTSPIDevice;

    // This is called directly after the constructor to INIT the phone device.
    virtual VOID Init(CTSPIDevice* pDevice, DWORD dwPhoneId, DWORD dwPos);

    // Method to send phonestate notifications
    virtual VOID OnButtonStateChange (DWORD dwButtonID, DWORD dwMode, DWORD dwState);
    virtual VOID OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
    BOOL CanHandleRequest(WORD wRequest, DWORD dwData=0L);
};

/******************************************************************************/
//
// CServiceProvider class
//
// This class is used to field all the calls for the service provider.
// It is based on the CWinApp class and provides the hookups for the
// Microsoft Foundation classes to properly work.
//
// This class manages multiple provider devices, and can have multiple
// DLL instances each with a seperate permanent provider id (PPID).
// They are mapped into a WordToObj map from the provider id (which
// is assumed to never exceed 65535 since it is essentially the
// count of service providers present in the system).
//
/******************************************************************************/
class CServiceProvider : public CWinApp
{
// Class data
protected:
    int m_iRefCount;                 // Load reference count
    CMapWordToOb m_mapDevices;       // List of CTSPIConnection structures
    CRuntimeClass*  m_pRequestObj;   // dynamic creation of CTSPIRequest
    CRuntimeClass*  m_pLineObj;      // dynamic creation of CTSPILineConnection   
    CRuntimeClass*  m_pPhoneObj;     // dynamic creation of CTSPIPhoneConnection
    CRuntimeClass*  m_pDeviceObj;    // dynamic creation of CTSPIDevice
    CRuntimeClass*  m_pCallObj;      // dynamic creation of CTSPICallAppearance
    CRuntimeClass*  m_pConfCallObj;  // dynamic creation of CTSPIConferenceCall
    CRuntimeClass*  m_pAddrObj;      // dynamic creation of CTSPIAddressInfo
    LINEEVENT m_lpfnLineCreateProc;  // Support Plug&Play dynamic line creation
    PHONEEVENT m_lpfnPhoneCreateProc; // Support Plug&Play dynamic phone creation
    LONG m_lTimeout;                 // Timeout value used throughout the library.
    DWORD m_dwCurrentLocation;       // Current location according to TAPI (v1.4)
    LPCSTR m_pszExeName;             // EXE to start
    LPCSTR m_pszProviderInfo;        // Provider information
    DWORD m_dwTapiVerSupported;      // TAPI versions supported
    DWORD m_dwTAPIVersionFound;      // TAPI version on this system.
    CFlagArray m_arrProviderCaps;    // Provider capabilities

// CWinApp specific stuff
public:
    CServiceProvider(const char* pszAppName, const char* pszExeName, DWORD dwTapiVer = TAPIVER_13, const char* pszProviderInfo=NULL);
    virtual BOOL InitInstance(); // Initialization
    virtual int ExitInstance();  // Termination 
       
// Companion thread interaction methods       
public:
    // Internal thread start/stop methods
    VOID InitializeRequestThread(HWND hwnd);
    VOID TerminateRequestThread();
    virtual VOID UnknownDeviceNotify (WORD wCommand, DWORD dwConnId, DWORD dwResult, LPVOID lpvData, DWORD dwSize);
    BOOL SendThreadRequest (WORD wCommand, LPARAM lParam, BOOL flParamIsPointer=FALSE, DWORD dwSize=0);

// Retrieval Methods
public:
    // This method retreives specific device connection objects
    CTSPIDevice* GetDevice(DWORD dwId) const;

    // Return what TAPI says the current location of this computer is.
    // This will only be valid under TAPI 1.4+
    DWORD GetCurrentLocation() const;

    // Return string name (passed to CWinApp constructor)
    const char * GetProviderName() const;

    // Return provider string (passed to CServiceProvider constructor)
    const char * GetProviderInfo() const;

	// .INI file manipulation methods which map to TELEPHON.INI
	CString ReadProfileString (DWORD dwPPid, LPCSTR pszEntry, LPCSTR pszDefault=NULL);
	DWORD ReadProfileDWord (DWORD dwPPid, LPCSTR pszEntry, DWORD dwDefault=0);
	BOOL WriteProfileString (DWORD dwPPid, LPCSTR pszEntry, LPCSTR pszValue);
	BOOL WriteProfileDWord (DWORD dwPPid, LPCSTR pszEntry, DWORD dwValue);
	BOOL DeleteProfile (DWORD dwDeviceID);

    // Returns the support TAPI version (passed to CServiceProvider constructor)
    DWORD GetSupportedVersion() const;
    DWORD GetSystemVersion() const;

    // These methods search all our connections for a specific
    // connection entry.
    CTSPILineConnection* GetConnInfoFromLineDeviceID(DWORD dwDevId);
    CTSPIPhoneConnection* GetConnInfoFromPhoneDeviceID(DWORD dwDevId);

// Misc. overridable functions
public:
    // This method checks the call parameters based on the line and specific address.  
    // It runs through all the addresses depending on the parameters passed.
    virtual LONG ProcessCallParameters(CTSPILineConnection* pLine, LPLINECALLPARAMS lpCallParams);

    // This method checks the number to determine if we support the
    // dialable address.  The final form address is returned in the buffer specified.
    virtual LONG CheckDialableNumber(CTSPILineConnection* pLine, CTSPIAddressInfo* pAddr, LPCSTR lpszDigits, CObArray* parrEntries, DWORD dwCountry, LPCSTR pszValidChars=NULL);
    virtual CString GetDialableNumber (const char *pszNumber, const char *pszAllowChar=NULL) const;
    virtual CString ConvertDialableToCanonical (const char *pszNumber, DWORD dwCountry=0);

    // These functions are routed back to the SP class by the device class.  The device
    // may be overriden in order to handle them there, but we maintain compatibility with
    // previous versions of the library through the reroute here.
    virtual BOOL OpenDevice (CTSPIConnection* pConn);
    virtual BOOL CloseDevice (CTSPIConnection* pConn);
    virtual BOOL SendData(CTSPIConnection* pConn, LPCVOID lpBuff, DWORD dwSize);
    virtual VOID OnTimer(CTSPIConnection* pConn);

    // This function is called when matching a tone against one seen on the media stream.
    virtual BOOL MatchTones (DWORD dwSFreq1, DWORD dwSFreq2, DWORD dwSFreq3, DWORD dwTFreq1, DWORD dwTFreq2, DWORD dwTFreq3);

// Set methods for the derived class to use
protected:
    // Set the C++ objects to use for each basic telephony object in the system.  This must be
	// done during the constructor of the service provider.
    VOID SetRuntimeObjects(CRuntimeClass* pDevObj, CRuntimeClass* pReqObj = NULL, CRuntimeClass* pLineObj = NULL, 
						   CRuntimeClass* pAddrObj = NULL, CRuntimeClass* pCallObj = NULL, 
						   CRuntimeClass* pConfCall = NULL, CRuntimeClass* pPhoneObj = NULL);

    // This changes the timeout value used during WaitForRequest() when waiting for an
	// asynchronous request to complete.  Default timeout value is MAX_WAIT_TIMEOUT.
    VOID SetTimeout (LONG lTimeout);

    // This method begins the processing of the next asynchronous request block on our list.
    // It gets called when a request finishes, or when a new request is inserted and no pending
    // request is being processed.  By default it calls "ProcessData" with a NULL.
    virtual VOID StartNextCommand(CTSPIConnection* pConn);

    // This method is called when a request is canceled and it has already
    // been started on the device.  It is called by the device object.
    virtual VOID OnCancelRequest (CTSPIRequest* pReq);

	// This is called by the device object each time a new request packet is
	// added to the request list.  It is called BEFORE the request has actually
	// been added, and allows the derived provider to manipulate the request list
	// or cancel this request by returning FALSE.  By default, TRUE is returned.
	virtual BOOL OnNewRequest (CTSPIConnection* pConn, CTSPIRequest* pReq, int* piPos);

    // This method is used to determine if an asynch request should be
    // generated for each available event type.  By default, the request will
    // be generated if the function is exported by the DLL.
    virtual BOOL CanHandleRequest(CTSPIConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, WORD wRequest, DWORD dwData = 0);
    
    // This method receieves control when the device receives data.  It is called
    // by the CTSPIDevice object when input becomes available.  TRUE should be returned
    // if the request was processed by this connection (i.e. expected).
    virtual BOOL ProcessData(CTSPIConnection* pConn, DWORD dwData = 0, const LPVOID lpBuff=NULL, DWORD dwSize=0);
    
    // This is called by the call object each time the call features are changed.  If
    // the H/W has restrictions as to the feature list that are not standard TAPI specifications,
    // then override this to adjust the features.
    virtual DWORD CheckCallFeatures(CTSPICallAppearance* pCall, DWORD dwCallFeatures);

//---- START OF INTERNAL METHODS - UNDOCUMENTED ----
// These should not be called by anything except the class library
public:
    // Internal interval timer routine called by SPDLL.CPP
    VOID IntervalTimer();

protected:    
    // Internal method to retrieve the default timeout used for the waiting of asynch requests.  
    LONG GetTimeout() const;

    // Internal methods used for the dynamic object creation
    // code.  This allows the base class to derive new classes for
    // each of the main objects, but the internal files may still
    // allocate the objects for the derived class through these pointers.
    CRuntimeClass* GetTSPIRequestObj() const;
    CRuntimeClass* GetTSPILineObj() const;
    CRuntimeClass* GetTSPIPhoneObj() const;
    CRuntimeClass* GetTSPIDeviceObj() const;
    CRuntimeClass* GetTSPICallObj() const;
    CRuntimeClass* GetTSPIConferenceCallObj() const;
    CRuntimeClass* GetTSPIAddressObj() const;

    // These methods retreive the line/phone creation procedures.  Simply add a new
    // line to the device to trigger these after the service provider has initialized.
    LINEEVENT GetLineCreateProc() const;
    PHONEEVENT GetPhoneCreateProc() const;
    
    // These all funnel back through the virtual function with the appropriate
    // parameters filled out when possible.
    BOOL CanHandleRequest (CTSPIConnection* pConn, WORD wRequest, DWORD dwData=0L);
    BOOL CanHandleRequest (CTSPIAddressInfo* pAddr, WORD wRequest, DWORD dwData=0L);
    BOOL CanHandleRequest (CTSPICallAppearance* pCall, WORD wRequest, DWORD dwData=0L);

    // These all funnel back through the virtual function with the appropriate
    // paramaters filled out.
    LONG ProcessCallParameters (CTSPIAddressInfo* pAddr, LPLINECALLPARAMS lpCallParams);
    LONG ProcessCallParameters (CTSPICallAppearance* pCall, LPLINECALLPARAMS lpCallParams);

private:
    // Determine the service provider capabilities and setup our flag array.
    VOID DetermineProviderCapabilities();

    // This method walks all our device/connection lists and locates
    // a connection object for a variety of identifers.
    CTSPIConnection* SearchForConnInfo(DWORD dwId, WORD wReqType);

//---- END OF UNDOCUMENTED FUNCTIONS ----
    
// TAPI callbacks
public:
    // This function is called before the TSPI_providerInit to determine
    // the number of line and phone devices supported by the service provider.
    // If the function is not available, then TAPI will read the information
    // out of the TELEPHON.INI file per TAPI 1.0.  TAPI 1.4 function
    virtual LONG providerEnumDevices(DWORD dwProviderId, LPDWORD lpNumLines,
                                 LPDWORD lpNumPhones, HPROVIDER hProvider,
                                 LINEEVENT lpfnLineCreateProc, 
                                 PHONEEVENT lpfnPhoneCreateProc);

    // This function is called by TAPI in response to the receipt of a 
    // LINE_CREATE message from the service provider which allows the dynamic
    // creation of a new line device.  The passed deviceId identifies this
    // line from TAPIs perspective.  TAPI 1.4 function
    virtual LONG providerCreateLineDevice(DWORD dwTempId, DWORD dwDeviceId);

    // This function is called by TAPI in response to the receipt of a
    // PHONE_CREATE message from the service provider which allows the dynamic
    // creation of a new phone device.  The passed deviceId identifies this
    // phone from TAPIs perspective.  TAPI 1.4 function
    virtual LONG providerCreatePhoneDevice(DWORD dwTempId, DWORD dwDeviceId);

   // This method is called when the service provider is first initialized.
   // It supplies the base line/phone ids for us and our permanent provider
   // id which has been assigned by TAPI.  It will be called right after
   // the INITIALIZE_NEGOTIATION, and should only be called once during the
   // life of the SP.
   virtual LONG providerInit(DWORD dwTSPVersion, DWORD dwProviderId, DWORD dwLineIdBase,
                                 DWORD dwPhoneIdBase, DWORD dwNumLines, 
                                 DWORD dwNumPhones, ASYNC_COMPLETION lpfnCompletionProc);

   // This method is called to shutdown our service provider.  It will
   // be called directly before the unload of our driver.
   virtual LONG providerShutdown(DWORD dwTSPVersion);

   // This method is invoked when the user selects our ServiceProvider
   // icon in the control panel.  It should invoke the configuration dialog
   // which must be provided by the derived class.
   virtual LONG providerConfig(DWORD dwPPID, CWnd* pwndOwner);

   // This method is invoked when the TSP is to be installed via the
   // TAPI install code.  It should insure that all the correct files
   // are there, and write out the initial .INI settings.
   virtual LONG providerInstall(DWORD dwPermanentProviderID, CWnd* pwndOwner);

   // This method is invoked when the TSP is being removed from the
   // system.  It should remove all its files and .INI settings.
   virtual LONG providerRemove(DWORD dwPermanentProviderID, CWnd* pwndOwner);

    // This method is called when TAPI wishes to negotiate available
    // versions with us for any line device installed.  The low and high
    // version numbers passed are ones which the installed TAPI.DLL supports,
    // and we are expected to return the highest value which we can support
    // so TAPI knows what type of calls to make to us.
    virtual LONG lineNegotiateTSPIVersion(DWORD dwDeviceId, DWORD dwLowVersion,  DWORD dwHiVersion, LPDWORD lpdwTSPVersion);

   // This method is called to display the line configuration dialog
   // when the user requests it through either the TAPI api or the control
   // panel applet.
   virtual LONG lineConfigDialog(CTSPILineConnection* pConn, CWnd* pwndOwner, CString& strDeviceClass);

   // This method is called to display the line configuration dialog with
   // a set of known parameters rather than the set currently in use.
   // Windows 95.
   virtual LONG lineConfigDialogEdit(CTSPILineConnection* pConn, CWnd* pwndOwner, CString& strDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize, LPVARSTRING lpDeviceConfigOut);

   // This method invokes the parameter configuration dialog for the
   // phone device.
   virtual LONG phoneConfigDialog(CTSPIPhoneConnection* pConn, CWnd* pwndOwner, CString& strDevClass);

   // This method retrieves an icon which represents our line device.
   virtual LONG lineGetIcon(CTSPILineConnection* pConn, CString& strDevClass, LPHICON lphIcon);

   // This method opens the specified line device based on the device
   // id passed and returns a handle for the line.  The TAPI.DLL line
   // handle must also be retained for further interaction with this
   // device.
   virtual LONG lineOpen(CTSPILineConnection* pConn, HTAPILINE htLine, LPHDRVLINE lphdLine, DWORD dwVer,  LINEEVENT lpfnEventProc);

   // This method closes the specified open line after stopping all
   // asynchronous requests on the line.
   virtual LONG lineClose(CTSPILineConnection* pConn);

   // This method deallocates a call after completing or aborting all
   // outstanding asynchronous operations on the call.
   virtual LONG lineCloseCall(CTSPICallAppearance* pCall);

   // This method drops the specified call.  This is REQUIRED to 
   // be overriden to send the correct command to drop the call.
   virtual LONG lineDrop(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId, LPCSTR lpsUserUserInfo, DWORD dwSize);

   // This method is used to drop calls which are not owned by anyone,
   // but may still have active monitors (Windows 95)
   virtual LONG lineDropOnClose(CTSPICallAppearance* pCall);

   // This method is called to drop new calls which do not have any
   // application which wishes to be the owner.  (Windows 95)
   virtual LONG lineDropNoOwner(CTSPICallAppearance* pCall);

   // This method is used to send dial digits to the device.
   virtual LONG lineDial(CTSPICallAppearance* pConn, DRV_REQUESTID dwReq, LPCSTR lpszDestAddr, DWORD dwCountryCode);

   // This method accepts the specified offering call.  It may optionally
   // send the specified User->User information to the calling party.
   virtual LONG lineAccept(CTSPICallAppearance* pCall, DRV_REQUESTID dwReq, LPCSTR lpsUserUserInfo, DWORD dwSize);

   // This method adds the specified call (hdConsultCall) to the
   // conference (hdConfCall).
   virtual LONG lineAddToConference(CTSPIConferenceCall* pConf, CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId);

   // This method allows an offering call to be answered
   virtual LONG lineAnswer(CTSPICallAppearance* pConn, DRV_REQUESTID dwReq, LPCSTR lpsUserUserInfo, DWORD dwSize);

   // This method performs a blind or single-step transfer of the
   // specified call to the specified destination address.
   virtual LONG lineBlindTransfer(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId, LPCSTR lpszDestAddr, DWORD dwCountryCode);

   // This method is used to specify how a call that could not be
   // connected normally should be completed instead.  The network or
   // switch may not be able to complete a call because the network
   // resources are busy, or the remote station is busy or doesn't answer.
   virtual LONG lineCompleteCall(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId, LPDWORD lpdwCompletionID, DWORD dwCompletionMode, DWORD dwMessageID);

   // This method completes the transfer of the specified call to the
   // party connected in the consultation call.  If 'dwTransferMode' is
   // LINETRANSFERMODE_CONFERENCE, the original call handle is changed
   // to a conference call.  Otherwise, the service provider should send
   // callstate messages change all the calls to idle.
   virtual LONG lineCompleteTransfer(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId, CTSPICallAppearance* pConsult, HTAPICALL htConfCall, LPHDRVCALL lphdConfCall, DWORD dwTransferMode);

   // This function is used as a general extension mechanims to allow
   // service providers to provide access to features not described in
   // other operations.
   virtual LONG lineDevSpecific(CTSPILineConnection* pLine, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId, LPVOID lpParams, DWORD dwSize);

   // This function is used as an extension mechanism to enable service
   // providers to provide access to features not described in other
   // operations.
   virtual LONG lineDevSpecificFeature(CTSPILineConnection* pLine, DWORD dwFeature, DRV_REQUESTID dwRequestId, LPVOID lpParams, DWORD dwSize);

   // This function parks the specified call according to the specified
   // park mode.
   virtual LONG linePark(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, DWORD dwParkMode, LPCSTR lpszDirAddr, LPVARSTRING lpNonDirAddress);

   // This function picks up a call alerting at the specified destination
   // address and returns a call handle for the picked up call.  If invoked
   // with a NULL for the 'lpszDestAddr' parameter, a group pickup is performed.
   // If required by the device capabilities, 'lpszGroupID' specifies the
   // group ID to which the alerting station belongs.
   virtual LONG linePickup(CTSPIAddressInfo* pAddr, DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, LPCSTR lpszDestAddr, LPCSTR lpszGroupID);

   // This function prepares an existing conference call for the addition of
   // another party.  It creates a new temporary consultation call.  The new
   // consultation call can subsequently be added to the conference call.
   virtual LONG linePrepareAddToConference(CTSPIConferenceCall* pCall, DRV_REQUESTID dwRequestID, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall, LPLINECALLPARAMS lpCallParams);

   // This function redirects the specified offering call to the specified
   // destination address.
   virtual LONG lineRedirect(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, LPCSTR lpszDestAddr, DWORD dwCountryCode);

   // This function forwards calls destined for the specified address
   // according to the specified forwarding instructions.
   // When an origination address is forwarded, the incoming calls for that
   // address are deflected to the other number by the switch.  This function
   // provides a combination of forward and do-not-disturb features.  This
   // function can also cancel specific forwarding currently in effect.
   virtual LONG lineForward(CTSPILineConnection* pLine, CTSPIAddressInfo* pAddr, DRV_REQUESTID dwRequestId, LPLINEFORWARDLIST const  lpForwardList, DWORD dwNumRingsAnswer, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall,
                            LPLINECALLPARAMS const lpCallParams);

   // This function initiates the buffered gathering of digits on the 
   // specified call.  TAPI.DLL specifies a buffer in which to place the digits,
   // and the maximum number of digits to be collected.
   virtual LONG lineGatherDigits(CTSPICallAppearance* pCall, DWORD dwEndToEndID, DWORD dwDigitModes, LPSTR lpszDigits, DWORD dwNumDigits, LPCSTR lpszTerminationDigits, DWORD dwFirstDigitTimeout, DWORD dwInterDigitTimeout);

   // This method initiates the generation of the specified digits
   // using the specified signal mode.
   virtual LONG lineGenerateDigits(CTSPICallAppearance* pConn, DWORD dwEndToEndID, DWORD dwDigitMode, LPCSTR lpszDigits, DWORD dwDuration);

   // This function generates the specified tone inband over the specified
   // call.  Invoking this function with a zero for 'dwToneMode' aborts any
   // tone generation currently in progress on the specified call.
   // Invoking 'lineGenerateTone' or 'lineGenerateDigit' also aborts the
   // current tone generation and initiates the generation of the newly
   // specified tone or digits.
   virtual LONG lineGenerateTone(CTSPICallAppearance* pCall, DWORD dwEndToEndID, DWORD dwToneMode, DWORD dwDuration, DWORD dwNumTones, LPLINEGENERATETONE lpTones);

   // This function enables and disables the unbuffered detection of digits
   // received on the call.  Each time a digit of the specified digit mode(s)
   // is detected, a LINE_MONITORDIGITS message is sent to the application by
   // TAPI.DLL, indicating which digit was detected.  Note that it is up
   // to the derived class to send the digit notification
   virtual LONG lineMonitorDigits(CTSPICallAppearance* pCall, DWORD dwDigitModes);

   // This function enables and disables the detection of media modes on 
   // the specified call.  When a media mode is detected, a LINE_MONITORMEDIA
   // message is sent to TAPI.DLL.
   virtual LONG lineMonitorMedia(CTSPICallAppearance* pCall, DWORD dwMediaModes);

   // This function enables and disables the detection of inband tones on
   // the call.  Each time a specified tone is detected, a message is sent
   // to the client application through TAPI.DLL
   virtual LONG lineMonitorTones(CTSPICallAppearance* pCall, DWORD dwToneListID, LPLINEMONITORTONE const lpToneList, DWORD dwNumEntries);

   // This function returns the highest extension version number the SP is
   // willing to operate under for the device given the range of possible
   // extension versions.
   virtual LONG lineNegotiateExtVersion(DWORD dwDeviceID, DWORD dwTSPIVersion, DWORD dwLowVersion, DWORD dwHiVersion, LPDWORD lpdwExtVersion);

   // This function removes the specified call from the conference call to
   // which it currently belongs.  The remaining calls in the conference call
   // are unaffected.
   virtual LONG lineRemoveFromConference(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID);

   // This function secures the call from any interruptions or interference
   // that may affect the call's media stream.
   virtual LONG lineSecureCall(CTSPICallAppearance* pCall, DRV_REQUESTID dwReqId);

   // This function selects the indicated extension version for the indicated
   // line device.  Subsequent requests operate according to that extension
   // version.
   virtual LONG lineSelectExtVersion(CTSPILineConnection* pConn, DWORD dwExtVersion);

   // This function sends user-to-user information to the remote party on the
   // specified call.
   virtual LONG lineSendUserUserInfo(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, LPCSTR lpsUserUserInfo, DWORD dwSize);

   // This function sets certain parameters for an existing call.
   virtual LONG lineSetCallParams(CTSPICallAppearance *pCall, DRV_REQUESTID dwRequestID, DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate, LPLINEDIALPARAMS const lpDialParams);

   // This function is called by TAPI whenever the address translation location
   // is changed by the user (in the Dial Helper dialog or 
   // 'lineSetCurrentLocation' function.  SPs which store parameters specific
   // to a location (e.g. touch-tone sequences specific to invoke a particular
   // PBX function) would use the location to select the set of parameters 
   // applicable to the new location.  Windows 95 only.
   virtual LONG lineSetCurrentLocation(DWORD dwLocation);

   // This method queries the specified address on the specified
   // line device to determine its telephony capabilities.
   virtual LONG lineGetAddressCaps(CTSPIAddressInfo* pAddr, DWORD dwTSPIVersion, DWORD dwExtVersion, LPLINEADDRESSCAPS lpAddressCaps);

   // This method returns the address ID associated with this line
   // in the specified format.
   virtual LONG lineGetAddressID(CTSPILineConnection* pConn, LPDWORD lpdwAddressId, DWORD dwAddressMode, LPCSTR lpszAddress, DWORD dwSize);

   // This method returns the status of the particular address on a line.
   virtual LONG lineGetAddressStatus(CTSPIAddressInfo* pAddr, LPLINEADDRESSSTATUS lpAddressStat);

   // This method returns the call address id.  
   virtual LONG lineGetCallAddressID(CTSPICallAppearance* pConn, LPDWORD lpdwAddressId);

   // This method retrieves all the information about a particular call.
   virtual LONG lineGetCallInfo(CTSPICallAppearance* pConn, LPLINECALLINFO lpCallInfo);

   // This method retrieves the status of a call on a particular line
   virtual LONG lineGetCallStatus(CTSPICallAppearance* pConn, LPLINECALLSTATUS lpCallStat);

   // This method retrieves the line capabilities for this TAPI device.
   virtual LONG lineGetDevCaps(CTSPILineConnection* pConn, DWORD dwTSPIVersion, DWORD dwExtVer, LPLINEDEVCAPS lpLineCaps);

   // This function returns a data structure object, the contents of which
   // are specific to the line (SP) and device class, giving the current
   // configuration of a device associated one-to-one with the line device.
   virtual LONG lineGetDevConfig(CTSPILineConnection* pConn, CString& strDeviceClass, LPVARSTRING lpDeviceConfig);

   // This function restores the configuration of a device associated one-to-one
   // with the line device from a data structure obtained through TSPI_lineGetDevConfig.
   // The contents of the data structure are specific to the service provider.
   virtual LONG lineSetDevConfig(CTSPILineConnection* pConn, LPVOID const lpDevConfig, DWORD dwSize, CString& strDevClass);

   // This method is used to release a block of UserUser information in a call
   // record.  It is new for TAPI 1.4
   virtual LONG lineReleaseUserUserInfo(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequest);

   // This function enables and disables control actions on the media stream
   // associated with the specified line, address, or call.  Media control actions
   // can be triggered by the detection of specified digits, media modes,
   // custom tones, and call states.  The new specified media controls replace all
   // the ones that were in effect for this line, address, or call prior to this
   // request.
   virtual LONG lineSetMediaControl(CTSPILineConnection *pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, 
                  LPLINEMEDIACONTROLDIGIT const  lpDigitList, DWORD dwNumDigitEntries, 
                  LPLINEMEDIACONTROLMEDIA const lpMediaList, DWORD dwNumMediaEntries, 
                  LPLINEMEDIACONTROLTONE const lpToneList, DWORD dwNumToneEntries, 
                  LPLINEMEDIACONTROLCALLSTATE const lpCallStateList, DWORD dwNumCallStateEntries);

   // This operation enables TAPI.DLL to specify to which terminal information
   // related to a specified line, address, or call is to be routed.  This
   // can be used while calls are in progress on the line, to allow events
   // to be routed to different devices as required.
   virtual LONG lineSetTerminal(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, DWORD dwTerminalModes, DWORD dwTerminalID, BOOL bEnable);

   // This function sets up a conference call for the addition of a third 
   // party.
   virtual LONG lineSetupConference(CTSPILineConnection* pConn, CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, HTAPICALL htConfCall,
            LPHDRVCALL lphdConfCall, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall, DWORD dwNumParties, 
            LPLINECALLPARAMS const lpLineCallParams);

   // This function is used to cancel the specified call completion request
   // on the specified line.
   virtual LONG lineUncompleteCall(CTSPILineConnection* pConn, DRV_REQUESTID dwRequestID, DWORD dwCompletionID);

   // This function retrieves the call parked at the specified
   // address and returns a call handle for it.
   virtual LONG lineUnpark(CTSPIAddressInfo* pAddr, DWORD dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, LPCSTR lpszDestAddr);

   // This function returns the extension ID that the service provider
   // supports for the indicated line device.
   virtual LONG lineGetExtensionID(CTSPILineConnection* pConn, DWORD dwTSPIVersion, LPLINEEXTENSIONID lpExtensionID);

   // This method returns specific ID information about the specified
   // line, address, or call.
   virtual LONG lineGetID(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, CString& strDevClass, LPVARSTRING lpDeviceID);

   // This method retrieves the features of the particular line.
   virtual LONG lineGetLineDevStatus(CTSPILineConnection* pConn, LPLINEDEVSTATUS lpLineDevStatus);

   // This method returns the total number of supported addresses on this
   // line.  This will be equal to the number of calls on this line.
   virtual LONG lineGetNumAddressIDs(CTSPILineConnection* pConn, LPDWORD lpNumAddr);

   // This method places the specified call appearance on hold
   virtual LONG lineHold(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID);

   // This method places a call on the specified line to the specified
   // destination address.  Optionally, the call parameters can be
   // specified if anything but a default call setup is required.
   virtual LONG lineMakeCall(CTSPILineConnection* pConn, DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, LPCSTR lpszDestAddr, DWORD dwCountryCode, LPLINECALLPARAMS const lpCallParams);

   // This method sets the application-specific data for the calls
   // LINEINFO structure when passed back to the user.
   virtual LONG lineSetAppSpecific(CTSPICallAppearance* pConn, DWORD dwAppSpecific);

   // This method is invoked by TAPI.DLL when the application requests a
   // line open using the LINEMAPPER.  This method will check the 
   // requested media modes and return an acknowledgement based on whether 
   // we can monitor all the requested modes.
   virtual LONG lineConditionalMediaDetection(CTSPILineConnection* pConn, DWORD dwMediaModes, LPLINECALLPARAMS const lpCallParams);

   // This method tells us the new set of media modes to watch for on 
   // this line (inbound or outbound).
   virtual LONG lineSetDefaultMediaDetection(CTSPILineConnection* pConn, DWORD dwMediaModes);

   // This method sets the current media mode for the device.
   virtual LONG lineSetMediaMode(CTSPICallAppearance* pConn, DWORD dwMediaMode);

   // This method tells us which events to notify TAPI about when
   // address or status changes about the specified line.
   virtual LONG lineSetStatusMessages(CTSPILineConnection* pConn, DWORD dwLineStates, DWORD dwAddressStates);

   // This function sets up a call for transfer to a destination address.
   // A new call handle is created which represents the destination
   // address.
   virtual LONG lineSetupTransfer(CTSPICallAppearance *pCall, DRV_REQUESTID dwRequestID, 
                     HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall, 
                     LPLINECALLPARAMS const lpCallParams);

   // This function swaps the specified active call with the specified
   // call on hold.
   virtual LONG lineSwapHold(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, 
                             CTSPICallAppearance* pHeldCall);

   // This method retrieves the specified call appearance off hold status.
   virtual LONG lineUnhold(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID);

    // This method is called when TAPI wishes to negotiate the phone 
    // version supported by this service prover.  The low and high
    // version numbers passed are ones which the installed TAPI.DLL supports,
    // and we are expected to return the highest value which we can support
    // so TAPI knows what type of calls to make to us.
    virtual LONG phoneNegotiateTSPIVersion(DWORD dwDeviceID, DWORD dwLowVersion,
                              DWORD dwHighVersion, LPDWORD lpdwVersion);

    // This method is called to negotiate the extensions version supported
    // by the phone side of this service provider.
    virtual LONG phoneNegotiateExtVersion(DWORD dwDeviceID, DWORD dwTSPIVersion,
                        DWORD dwLowVersion, DWORD dwHighVersion, LPDWORD lpdwExtVersion);

   // This method opens the phone device whose device ID is given,
   // returning the service provider's opaque handle for the device and
   // retaining the TAPI opaque handle.
   virtual LONG phoneOpen(CTSPIPhoneConnection * pConn,
                     HTAPIPHONE htPhone, LPHDRVPHONE lphdPhone,
                     DWORD dwTSPIVersion, PHONEEVENT lpfnEventProc);
   
   // This method closes the specified open phone device after completing
   // or aborting all outstanding asynchronous requests on the device.
   virtual LONG phoneClose(CTSPIPhoneConnection* pConn);

   // This method is used as a general extension mechanism to enable
   // a TAPI implementation to provide features not generally available
   // to the specification.
   virtual LONG phoneDevSpecific(CTSPIPhoneConnection* pConn, 
                     DRV_REQUESTID dwRequestID, LPVOID lpParams, DWORD dwSize);

   // This method returns information about the specified phone 
   // button.
   virtual LONG phoneGetButtonInfo(CTSPIPhoneConnection* pConn, DWORD dwButtonId, LPPHONEBUTTONINFO lpButtonInfo);

   // This method uploads the information from the specified location
   // in the open phone device to the specified buffer.
   virtual LONG phoneGetData(CTSPIPhoneConnection* pConn, DWORD dwDataId, 
                             LPVOID lpData, DWORD dwSize);

   // This method queries a specified phone device to determine its
   // telephony capabilities
   virtual LONG phoneGetDevCaps(CTSPIPhoneConnection* pConn, DWORD dwTSPIVersion, 
                                DWORD dwExtVersion, LPPHONECAPS lpPhoneCaps);
   
   // This method returns the current contents of the specified phone
   // display.
   virtual LONG phoneGetDisplay(CTSPIPhoneConnection* pConn, LPVARSTRING lpString);

   // This method retrieves the extension ID that the service provider
   // supports for the indicated device.
   virtual LONG phoneGetExtensionID(CTSPIPhoneConnection* pConn, DWORD dwTSPIVersion,
                               LPPHONEEXTENSIONID lpExtensionId);

   // This method returns the gain setting of the microphone of the
   // specified phone's hookswitch device.
   virtual LONG phoneGetGain(CTSPIPhoneConnection* pPhone, DWORD dwHookSwitchDev, LPDWORD lpdwGain);

   // This function retrieves the current hook switch setting of the
   // specified open phone device
   virtual LONG phoneGetHookSwitch(CTSPIPhoneConnection* pConn, LPDWORD lpdwHookSwitchDevs);

   // This function retrieves a specific icon for display from an
   // application.  This icon will represent the phone device.
   virtual LONG phoneGetIcon(CTSPIPhoneConnection* pConn, CString& strDevClass, LPHICON lphIcon);

   // This function retrieves the device id of the specified open phone
   // handle (or some other media handle if available).
   virtual LONG phoneGetID(CTSPIPhoneConnection* pConn, CString& strDevClass, LPVARSTRING lpDeviceId);

   // This function returns the current lamp mode of the specified
   // lamp.
   virtual LONG phoneGetLamp(CTSPIPhoneConnection* pConn, DWORD dwButtonLampId, LPDWORD lpdwLampMode);

   // This function enables an application to query the specified open
   // phone device as to its current ring mode.
   virtual LONG phoneGetRing(CTSPIPhoneConnection* pConn, LPDWORD lpdwRingMode, LPDWORD lpdwVolume);

   // This function queries the specified open phone device for its
   // overall status.
   virtual LONG phoneGetStatus(CTSPIPhoneConnection* pConn, LPPHONESTATUS lpPhoneStatus);

   // This function returns the volume setting of the phone device.
   virtual LONG phoneGetVolume(CTSPIPhoneConnection* pConn, DWORD dwHookSwitchDev, LPDWORD lpdwVolume);

   // This function selects the indicated extension version for the
   // indicated phone device.  Subsequent requests operate according to
   // that extension version.
   virtual LONG phoneSelectExtVersion(CTSPIPhoneConnection* pConn, DWORD dwExtVersion);

   // This function sets information about the specified button on the
   // phone device.
   virtual LONG phoneSetButtonInfo(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestID, DWORD dwButtonId, LPPHONEBUTTONINFO const lpPhoneInfo);

   // This function downloads the information in the specified buffer
   // to the opened phone device at the selected data id.
   virtual LONG phoneSetData(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestId, DWORD dwDataId, LPCVOID lpData, DWORD dwSize);

   // This function causes the specified string to be displayed on the
   // phone device.
   virtual LONG phoneSetDisplay(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestID, DWORD dwRow, DWORD dwCol, LPCSTR lpszDisplay, DWORD dwSize);

   // This function sets the gain of the microphone of the specified hook
   // switch device.
   virtual LONG phoneSetGain(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestId, DWORD dwHookSwitchDev, DWORD dwGain);

   // This function sets the hook state of the specified open phone's
   // hookswitch device to the specified mode.  Only the hookswitch
   // state of the hookswitch devices listed is affected.
   virtual LONG phoneSetHookSwitch(CTSPIPhoneConnection*, DRV_REQUESTID dwRequestId, DWORD dwHookSwitchDevs, DWORD dwHookSwitchMode);

   // This function causes the specified lamp to be set on the phone
   // device to the specified mode.
   virtual LONG phoneSetLamp(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestId, DWORD dwButtonLampId, DWORD dwLampMode);

   // This function rings the specified open phone device using the
   // specified ring mode and volume.
   virtual LONG phoneSetRing(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestId, DWORD dwRingMode, DWORD dwVolume);

   // This function causes the service provider to filter status messages
   // which are not currently of interest to any application.
   virtual LONG phoneSetStatusMessages(CTSPIPhoneConnection* pConn, DWORD dwPhoneStates, DWORD dwButtonModes, DWORD dwButtonStates);

   // This function either sets the volume of the speaker or the 
   // specified hookswitch device on the phone
   virtual LONG phoneSetVolume(CTSPIPhoneConnection*, DRV_REQUESTID dwRequestId, DWORD dwHookSwitchDev, DWORD dwVolume);

// Define the friends of this class.
public:
    friend class CTSPIDevice;
    friend class CTSPIConnection;
    friend class CTSPILineConnection;
    friend class CTSPIPhoneConnection;
    friend class CTSPIAddressInfo;
    friend class CTSPICallAppearance;
    friend class CTSPIConferenceCall;
};

/******************************************************************************/
// GetSP
//
// Public method to retrieve a pointer to the main service provider
// application object.
//
/******************************************************************************/
inline CServiceProvider* GetSP() { return (CServiceProvider*) AfxGetApp(); }

/******************************************************************************/
// AllocMem
//
// Global function to allocate memory for non-Object allocations
//
/******************************************************************************/
LPVOID AllocMem (DWORD dwSize);

/******************************************************************************/
// FreeMem
//
// Global function to deallocate non-Object memory
//
/******************************************************************************/
VOID FreeMem (LPVOID lpBuff);

/******************************************************************************/
//
// CopyVarString
//
// Copy the buffer (either LPCSTR or LPVOID) into the VARSTRING pointer.
//
/******************************************************************************/
void CopyVarString (LPVARSTRING lpVarString, LPCSTR lpszBuff);
void CopyVarString (LPVARSTRING lpVarString, LPVOID lpBuff, DWORD dwSize);

/******************************************************************************/
// CopyBuffer
//
// Global function to copy one buffer to another.
//
/******************************************************************************/
VOID CopyBuffer (LPVOID lpDest, LPCVOID lpSource, DWORD dwSize);

/******************************************************************************/
//
// FillBuffer
//
// Initialize a buffer with a known value.
//
/******************************************************************************/
VOID FillBuffer (LPVOID lpDest, BYTE bValue, DWORD dwSize);

/******************************************************************************/
//
// ReportError
//
// Return TRUE/FALSE as to whether a TAPI result is an error
// condition.  Also outputs to debug monitor if debug.
//
/******************************************************************************/
BOOL ReportError (LONG lResult);

/******************************************************************************/
//
// DumpMem
//        
// Global DEBUG function to dump out a block of data in binary/ascii to
// the debug terminal.
//
/******************************************************************************/
#ifdef _DEBUG
void DumpMem (LPCSTR lpszBuff, LPVOID lpBuff, DWORD dwSize);

/******************************************************************************/
// 
// DumpVarString
// 
// Global DEBUG function to dump out a VARSTRING data block to the
// debug terminal
//
/******************************************************************************/
void DumpVarString (LPVARSTRING lpVarString);

/******************************************************************************/
// 
// g_iShowAPITraceLevel
//
// This controls the output of the API entrypoints.
//
// = 0 -> No output from API level trace
// = 1 -> Basic In/Out log
// = 2 -> Full structure dumps
//
//
// Defaults to '2'.
//
/******************************************************************************/
extern int g_iShowAPITraceLevel;
#endif // _DEBUG

#ifndef RC_INVOKED
#pragma pack()      // Revert to original packing
#endif 

#endif // _SP_INC_
