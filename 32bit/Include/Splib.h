/******************************************************************************/
//                                                                        
// SPLIB.H - TAPI Service Provider C++ Library header                     
//
// VERSION 2.24 06/15/2000
//                                             
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// The SPLIB classes provide a basis for developing MS-TAPI complient     
// Service Providers.  They provide basic handling for all of the TSPI    
// APIs and a C-based handler which routes all requests through a set of C++     
// classes.                                                                 
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

#ifndef _SPLIB_INC_
#define _SPLIB_INC_

#ifndef TAPI_CURRENT_VERSION
	#error "TAPI.H and TSPI.H must be included before SPLIB.H"
#endif

#if TAPI_CURRENT_VERSION > 0x00020001
	#error "TSP++ 2.2 doesn't support anything beyong TAPI 2.1, add a TAPI_CURRENT_VERSION tag in your source code before including TSPI.H""
#endif

#ifndef STRICT
	#error "You must use the STRICT definition for projects"
#endif

#ifndef RC_INVOKED
	#if (TAPI_CURRENT_VERSION < 0x00020000)
		#error "This product is only for 32-bit TSP development"
	#endif
	#pragma pack(1)
#endif 

// New const void* ptr.
typedef const void FAR* LPCVOID;

// Total # of standard buttons for TAPI (0-9, *, #).
#define TOTAL_STD_BUTTONS  12

// Define supported versions of TAPI
#define TAPIVER_13 (0x00010003)		// Add-on to Windows 3.1
#define TAPIVER_14 (0x00010004)		// Shipped with Windows 95
#define TAPIVER_20 (0x00020000)		// Shipped with Windows NT 4.0
#define TAPIVER_21 (0x00020001)		// Add-on to Windows 95, NT
#define TAPIVER_22 (0x00020002)		// Windows 98, Windows 2000
 
// Include the _DEBUG macro if it isn't defined.
#ifdef DEBUG
#ifndef _DEBUG
#define _DEBUG
#endif
#endif

// Include the proper library
#ifdef _UNICODE
#ifdef _DEBUG
#pragma comment(lib, "splib32ud.lib")
#else
#pragma comment(lib, "splib32u.lib")
#endif
#else
#ifdef _DEBUG
#pragma comment(lib, "splib32d.lib")
#else
#pragma comment(lib, "splib32.lib")
#endif
#endif

// Replace the ASSERT macro for TAPI 2.0 - The TAPI system
// runs as a subsystem (TAPISRV) and therefore has no UI access.
// Any attempt to pop up a UI element (such as a messagebox) will
// result in a GP-fault or deadlock in NT.
#define TRC_MIN		1
#define TRC_API		2
#define TRC_DUMP	3
#define TRC_STRUCT	4

#undef ASSERT
#undef VERIFY
#undef ASSERT_VALID
#undef DTRACE
#undef TRACE

#ifdef _DEBUG
	#define _NOINLINES_
	int DbgTraceLevel();
	void __cdecl _TspTrace(int iTraceLevel, LPCTSTR pszTrace, ...);
	void __cdecl _TspTrace2(LPCTSTR pszTrace, ...);
	void __stdcall _AssertFailedLine(LPCSTR lpszFileName, int nLine, LPCSTR pszExpr);
	#define ASSERT_VALID(pOb)
	#define ASSERT(f) \
	        do { if (!(f)) \
			_AssertFailedLine(__FILE__, __LINE__, #f); \
		} while (0)


	#define VERIFY(f) ASSERT(f)
	#define DTRACE ::_TspTrace
	#define TRACE ::_TspTrace2
	#define DUMPMEM ::DumpMem
#else
#define VERIFY(f) ((void)(f))
#if _MSC_VER >= 1210
	#define ASSERT(f) (__noop)
	#define ASSERT_VALID(f) (__noop)
	#define DTRACE (__noop)
	#define TRACE (__noop)
	#define DUMPMEM (__noop)
#else
	#define ASSERT(f) ((void)0)
	#define ASSERT_VALID(f) ((void)0)
	#define DTRACE (1 ? (void)0 : (void)0)
	#define TRACE (1 ? (void)0 : (void)0)
	#define DUMPMEM (1 ? (void)0 : (void)0)
#endif
#endif // _DEBUG

// Wait timeout used for various things in the library.  We assume that
// things that we are forced to wait for (synchronous) will complete within
// this timeframe.  This can be adjusted through the CServiceProvider::SetTimeout
// method.
#define MAX_WAIT_TIMEOUT (10000L)

// These keys are used during TSPI_providerGenericDialogData
// to convert line/phone identifiers into the associated provider
// identifier.
#define GDD_LINEPHONETOPROVIDER		(0xab110301)
#define GDD_LINEPHONETOPROVIDEROK	(0xab110302)
#define GDD_LINEPHONETOPERMANENT    (0xab110303)
#define GDD_LINEPHONETOPERMANENTOK  (0xab110304)

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
// CMapDWordToString
//
// New map supporting DWORD indexing with string descriptions.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class CMapDWordToString : public CObject
{
	DECLARE_SERIAL (CMapDWordToString)
// Class data
protected:
	// Association structure which is stored in our list.
	struct CAssoc
	{
		CAssoc* pNext;
		UINT nHashValue;
		DWORD key;
		CString value;
	};

	CAssoc** m_pHashTable;						// List of associations
	UINT m_nHashTableSize;						// Current table size
	int m_nCount;								// Count of entries within table
	CAssoc* m_pFreeList;						// Current association free list
	struct CPlex* m_pBlocks;					// Blocks of data
	int m_nBlockSize;							// Current block size

// Constructor
public:
	CMapDWordToString(int nBlockSize = 10);
	virtual ~CMapDWordToString();

// Operators
public:
	CString& operator[](DWORD key);

// Attributes
public:
	int GetCount() const;
	BOOL IsEmpty() const;
	BOOL Lookup(DWORD key, CString& rValue) const;
	void SetAt(DWORD key, LPCTSTR newValue);
	BOOL RemoveKey(DWORD key);
	void RemoveAll();
	POSITION GetStartPosition() const;
	void GetNextAssoc(POSITION& rNextPosition, DWORD& rKey, CString& rValue) const;

// Internal members
protected:
	CAssoc* NewAssoc();
	void FreeAssoc(CAssoc*);
	CAssoc* GetAssocAt(DWORD, UINT&) const;
	UINT GetHashTableSize() const;
	void InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);
	UINT HashKey(DWORD key) const;

	// local typedefs for CTypedPtrMap class template
	typedef DWORD BASE_KEY;
	typedef DWORD BASE_ARG_KEY;
	typedef CString BASE_VALUE;
	typedef LPCTSTR BASE_ARG_VALUE;
};
  
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
// CIntCriticalSection
//
// Our own private critical section which supports time-outs under Win95.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
class CIntCriticalSection : public CSyncObject
{
// Class data
protected:
	LONG	m_lLockCount;
	LONG	m_lInThreadCount;
	DWORD	m_dwThreadId;
	CEvent  m_evtLock;

// Constructor
public:
	CIntCriticalSection();

// Methods
public:
	virtual BOOL Lock(DWORD dwTimeout = INFINITE);
	virtual BOOL Unlock();
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
class CFlagArray : public CObject
{ 
// Class data
protected:
    CByteArray  m_arrValues;
    
// Constructor
public:
    CFlagArray();
	virtual ~CFlagArray();
    
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
    DWORD dwRefCount;               // Reference Count
    DWORD dwForwardMode;            // Forwarding mode (LINEFORWARDMODE_xxx)
    DWORD dwDestCountry;            // Destination country for addresses
	DWORD dwTotalSize;				// Total size of all the forwarding information	
    CADObArray arrCallerAddress;    // Input caller addresses (DIALINFO)
    CADObArray arrDestAddress;      // Destination addresses (DIALINFO)
    void IncUsage();
    void DecUsage();
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
    TSPICALLPARAMS();			// Constructor
    DWORD dwBearerMode;			// New bearer mode for call
    DWORD dwMinRate;			// Low bound for call data rate
    DWORD dwMaxRate;			// Hi bound for call data rate
    LINEDIALPARAMS DialParams;	// New dial parameters
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
    // WARNING: The derived class must verify this parameter BEFORE 
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
    LPWSTR lpBuffer;                    // Buffer for the call to place collected digits
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
public:            
    TSPIMEDIACONTROL();             // Constructor
    virtual ~TSPIMEDIACONTROL();    // Destructor for the media control
    DWORD dwRefCount;				// Reference count for shared structures
    CPtrArray arrDigits;            // Array of digit monitoring media structures
    CPtrArray arrMedia;             // Array of media monitoring media structures
    CPtrArray arrTones;             // Array of tone monitoring media structures
    CPtrArray arrCallStates;        // Array of callstate monitoring media structures
    void DecUsage();                // Auto-delete mechanism after no more usage
    void IncUsage();                // Incremenent usage count
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
    TSPISETBUTTONINFO();		// Constructor
    DWORD dwButtonLampId;       // Button id to change
    DWORD dwFunction;           // Function to set button to
    DWORD dwMode;               // Mode for function
    CString strText;            // Text for button.
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
    TSPIPHONESETDISPLAY();      // Constructor
    virtual ~TSPIPHONESETDISPLAY(); // Destructor
    DWORD  dwRow;               // Row in display to modify
    DWORD  dwColumn;            // Column in display to modify
    LPVOID lpvDisplay;          // Display changes
    DWORD  dwSize;              // Size of above.
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
    TSPIPHONEDATA();            // Constructor
    virtual ~TSPIPHONEDATA();   // Destructor to delete additional memory
    DWORD dwDataID;             // Data buffer to set
    LPVOID lpBuffer;            // Buffer to set (Allocated with AllocMem)
    DWORD dwSize;               // Size of the buffer.
};    

/******************************************************************************/
// 
// TSPICALLDATA
//
// Sets the call data information for a call.
//
// This is passed in response to a REQUEST_SETCALLDATA command.
//
/******************************************************************************/
class TSPICALLDATA : public CObject
{
public:
	virtual ~TSPICALLDATA();	// Destructor to delete buffer
	LPVOID lpvCallData;			// Pointer to call data
	DWORD dwSize;				// Size of call data
};

/******************************************************************************/
// 
// TSPIQOS
//
// Quality of Service command structure
//
// This is passed in response to a REQUEST_SETQOS command.
//
/******************************************************************************/
class TSPIQOS : public CObject
{
public:
	virtual ~TSPIQOS();				// Destructor to delete data
	LPVOID lpvSendingFlowSpec;		// Sending Win32 FLOWSPEC
	DWORD dwSendingSize;			// Size of above
	LPVOID lpvReceivingFlowSpec;	// Receiving Win32 FLOWSPEC
	DWORD dwReceivingSize;			// Size of above
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
		MediaControlMedia = 1,		// Timer started by Media duration
		MediaControlTone,			// Timer started by Tone duration 
		ToneDetect					// Timer stated by tone detection.
	};

    DWORD dwEndTime;                // Time this event expires (TickCount)
    int iEventType;                 // Event type (EventType or user-defined)
    DWORD dwData1;                  // Data dependant upon event type.
    DWORD dwData2;                  // Data dependant upon event type.
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
    CString strPartyId;            // Party id number information
    CString strPartyName;          // Name of party
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
    CString strName;			// Name of the terminal
    LINETERMCAPS Capabilities;  // Capabilities of the terminal
    DWORD dwMode;				// Base line terminal mode
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

/******************************************************************************/
//
// DEVICECLASSINFO
//
// This structure describes a device-class information record which can
// be retrieved through the lineGetID and phoneGetID apis.  Each line, address,
// call, and phone, have an array which holds these structures.
//
/******************************************************************************/
class DEVICECLASSINFO : public CObject
{
public:
	CString	strName;		// Name of the device class ("tapi/line")
	DWORD dwStringFormat;	// String format (STRINGFORMAT_xxx)
	LPVOID lpvData;			// Data (may be NULL)
	DWORD dwSize;			// Size of data
	HANDLE hHandle;			// Handle which is COPIED

// Constructor
public:
	DEVICECLASSINFO(LPCTSTR pszName, DWORD dwStringFormat,
					LPVOID lpData=NULL, DWORD dwSize=0, 
					HANDLE hHandle=INVALID_HANDLE_VALUE);
	virtual ~DEVICECLASSINFO();
};

/******************************************************************************/
// 
// LINEUIDIALOG
//
// This structure is used to manage dynamically created dialogs
// so that we can track them.  It is stored in an array maintained
// by the line connection object.
//
// INTERNAL DATA STRUCTURE
//
/******************************************************************************/
typedef struct _tagLINEUIDIALOG
{
	DRV_REQUESTID dwRequestID;			// Request ID which generated this dialog.
	HTAPIDIALOGINSTANCE htDlgInstance;	// TAPI's handle to dialog	
	CTSPILineConnection* pLineOwner;	// Owner of UI dialog

} LINEUIDIALOG;

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
	CEvent m_evtComplete;				// Event for completion of this request.
    DRV_REQUESTID m_dwRequestId;        // Asynch request id for this request
    LPVOID m_lpData;                    // Ptr to data for request (may be object)
    DWORD m_dwSize;                     // Size of above data block if not str.
    DWORD m_dwAppData;                  // State-specific data
    LONG m_lResult;                     // This is the final result for this request.
    int m_iReqState;                    // Current state (set by processor)
    BOOL m_fResponseSent;				// Flag indicating we have already responded to TAPI.
    WORD m_wReqType;                    // Type of request (COMMAND)

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
    void SetCommand(WORD wCommand);
    void SetState(int iState);
    void SetStateData(DWORD dwStateData);
	void SetCallInfo(CTSPICallAppearance* pCall);
	void SetDataPtr(LPVOID lpBuff);
	void SetDataSize(DWORD dwSize);

    static BOOL IsAddressOk(LPVOID lpBuff, DWORD dwSize);

// Overridable methods
public:
    // This function pauses a thread when it wants to wait for the request to complete.
    virtual LONG WaitForCompletion (DWORD dwMSecs=INFINITE);

#ifdef _DEBUG
    virtual void Dump(CDumpContext& de) const;
	LPCTSTR GetRequestTypeName() const;
#endif

// Internal functions
protected:
    friend class CTSPIConnection;
    friend class CTSPILineConnection;
	friend class CTSPIAddressInfo;

    virtual void Init(CTSPIConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, WORD wRequest, DRV_REQUESTID dwRequestId, LPVOID lpBuff, DWORD dwSize);
    virtual void Complete (LONG lResult=0L, BOOL fSentTapiNotification=TRUE);
};

/*************************************************************************/
//
// Constants for each of the TAPI functions which we may or may not
// support - each of these functions is queried for at init-time and
// may later be tested for using GetSP()->CanHandleRequest()
//
// If the function isn't listed here, it is either a required BASIC
// exported function, or is completely implemented within the library
// and requires no user-interaction.
//
/*************************************************************************/

#define TSPI_LINEACCEPT                     0
#define TSPI_LINEADDTOCONFERENCE            1
#define TSPI_LINEANSWER                     2
#define TSPI_LINEBLINDTRANSFER              3
#define TSPI_LINECOMPLETECALL               4
#define TSPI_LINECOMPLETETRANSFER           5
#define TSPI_LINECONDITIONALMEDIADETECTION  6
#define TSPI_LINEDEVSPECIFIC                7
#define TSPI_LINEDEVSPECIFICFEATURE         8
#define TSPI_LINEDIAL                       9
#define TSPI_LINEFORWARD                    10
#define TSPI_LINEGATHERDIGITS               11
#define TSPI_LINEGENERATEDIGITS             12
#define TSPI_LINEGENERATETONE               13
#define TSPI_LINEGETDEVCONFIG               14
#define TSPI_LINEGETEXTENSIONID             15
#define TSPI_LINEGETICON                    16
#define TSPI_LINEGETID                      17
#define TSPI_LINEGETLINEDEVSTATUS           18
#define TSPI_LINEHOLD                       19
#define TSPI_LINEMAKECALL                   20
#define TSPI_LINEMONITORDIGITS              21
#define TSPI_LINEMONITORMEDIA               22
#define TSPI_LINEMONITORTONES               23
#define TSPI_LINENEGOTIATEEXTVERSION        24
#define TSPI_LINEPARK                       25
#define TSPI_LINEPICKUP                     26
#define TSPI_LINEPREPAREADDTOCONFERENCE     27
#define TSPI_LINEREDIRECT                   28
#define TSPI_LINERELEASEUSERUSERINFO        29
#define TSPI_LINEREMOVEFROMCONFERENCE       30
#define TSPI_LINESECURECALL                 31
#define TSPI_LINESELECTEXTVERSION           32
#define TSPI_LINESENDUSERUSERINFO           33
#define TSPI_LINESETCALLDATA                34
#define TSPI_LINESETCALLPARAMS              35
#define TSPI_LINESETCALLQUALITYOFSERVICE    36
#define TSPI_LINESETCALLTREATMENT			37
#define TSPI_LINESETDEVCONFIG               38
#define TSPI_LINESETLINEDEVSTATUS           39
#define TSPI_LINESETMEDIACONTROL            40
#define TSPI_LINESETTERMINAL                41
#define TSPI_LINESETUPCONFERENCE            42
#define TSPI_LINESETUPTRANSFER              43
#define TSPI_LINESWAPHOLD                   44
#define TSPI_LINEUNCOMPLETECALL             45
#define TSPI_LINEUNHOLD                     46
#define TSPI_LINEUNPARK                     47
#define TSPI_PHONEDEVSPECIFIC               48
#define TSPI_PHONEGETBUTTONINFO             49
#define TSPI_PHONEGETDATA                   50
#define TSPI_PHONEGETDISPLAY                51
#define TSPI_PHONEGETEXTENSIONID            52
#define TSPI_PHONEGETGAIN                   53
#define TSPI_PHONEGETHOOKSWITCH             54
#define TSPI_PHONEGETICON                   55
#define TSPI_PHONEGETID                     56
#define TSPI_PHONEGETLAMP                   57
#define TSPI_PHONEGETRING                   58
#define TSPI_PHONEGETVOLUME                 59
#define TSPI_PHONENEGOTIATEEXTVERSION       60
#define TSPI_PHONESELECTEXTVERSION          61
#define TSPI_PHONESETBUTTONINFO             62
#define TSPI_PHONESETDATA                   63
#define TSPI_PHONESETDISPLAY                64
#define TSPI_PHONESETGAIN                   65
#define TSPI_PHONESETHOOKSWITCH             66
#define TSPI_PHONESETLAMP                   67
#define TSPI_PHONESETRING                   68
#define TSPI_PHONESETVOLUME                 69
#define TSPI_PROVIDERCREATELINEDEVICE       70
#define TSPI_PROVIDERCREATEPHONEDEVICE      71
#define TSPI_ENDOFLIST                      71

// This is the token sent to ReceiveData() when the library is initiating
// a new request due to it being added to the asynch request list.
#define STARTING_COMMAND        0x0000

// Define common states used in most state machines
#define STATE_INITIAL        0
#define STATE_UNINITIALIZED  -1
#define STATE_IGNORE         0xff

// Define all the asynchronous request types supported directly
// by the base class.  For each TAPI function which passes a 
// DRV_REQUESTID, there should be a request.

// TSPI_line requests
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
#define REQUEST_PREPAREADDCONF   0x001B      // TSPI_linePrepareAddToConference
#define REQUEST_GENERATEDIGITS   0x001C      // TSPI_lineGenerateDigits
#define REQUEST_GENERATETONE     0x001D      // TSPI_lineGenerateTones
#define REQUEST_RELEASEUSERINFO  0x001E      // TSPI_lineReleaseUserUserInfo
#define REQUEST_SETCALLDATA      0x001F      // TSPI_lineSetCallData
#define REQUEST_SETQOS           0x0020      // TSPI_lineSetQualityOfService
#define REQUEST_SETCALLTREATMENT 0x0021		 // TSPI_lineSetCallTreatment
#define REQUEST_SETDEVSTATUS     0x0022      // TSPI_lineSetLineDevStatus

// TSPI_phone requests
#define REQUEST_SETBUTTONINFO     0x0023     // TSPI_phoneSetButtonInfo
#define REQUEST_SETDISPLAY        0x0024     // TSPI_phoneSetDisplay
#define REQUEST_SETHOOKSWITCHGAIN 0x0025	 // TSPI_phoneSetGain
#define REQUEST_SETHOOKSWITCH     0x0026     // TSPI_phoneSetHookswitch
#define REQUEST_SETLAMP           0x0027     // TSPI_phoneSetLamp
#define REQUEST_SETRING           0x0028     // TSPI_phoneSetRing
#define REQUEST_SETHOOKSWITCHVOL  0x0029     // TSPI_phoneSetVolume
#define REQUEST_SETPHONEDATA      0x002A     // TSPI_phoneSetData
#define REQUEST_GETPHONEDATA      0x002B     // TSPI_phoneGetData

// All derived request types should follow this entry.  These would
// include specialized request commands for TSPI_lineDevSpecific
// and TSPI_lineDevSpecificFeature processing.  
//
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
	CIntCriticalSection m_csSection;	// Critical Section for modifying object
	DWORD m_dwItemData;					// Item data for developer use.
// Members
public:
	CTSPIBaseObject();
	CIntCriticalSection* GetSyncObject() const;
	DWORD GetItemData() const;
	void* GetItemDataPtr() const;
	void SetItemData(DWORD dwItem);
	void SetItemDataPtr(void* pItem);
};

/******************************************************************************/
//
// Function to Lock/Unlock objects
//
/******************************************************************************/
class CEnterCode : public CSingleLock
{
// Constructor
public:
	CEnterCode(const CTSPIBaseObject* pObj, BOOL fAutoLock=TRUE) :
	  CSingleLock(pObj->GetSyncObject(), fAutoLock) {/* */}
};

// This will cause the usage "CEnterCode(object)" to be invalid as
// that goes out of scope immediately and causes an unlock when one
// is not intended. This will cause an error which alerts the user
// to the problem.  The proper syntax is: "CEnterCode var(object)".
#define CEnterCode(x) __unnamed_LockErr

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
    DECLARE_DYNCREATE (CTSPIConnection)

// Class data.
protected:
	enum {
		_IsDeleted = 0x00000001,		// Line/Phone has been removed.
		_IsRemoved = 0x00000002			// Line/Phone has been removed
			// Additional may be added in the future.
	};
	DWORD m_dwFlags;				// Flags for this connection
    CTSPIDevice* m_pDevice;			// Pointer to our device association
    CString m_strName;				// Line/Phone name "MyPhone"
    CString m_strDevInfo;			// Device information for this line or phone.
    DWORD m_dwDeviceID;				// TAPI line/phone device identifier.
    DWORD m_dwNegotiatedVersion;	// Negotiated TAPI version for this connection.
    CObList m_oblAsynchRequests;	// Asynchronous requests list for this connection.
	CADObArray m_arrDeviceClass;	// Device class array for line/phoneGetID

// Constructors
protected:
    CTSPIConnection();				// Should only be created by the CreateObject() method.
public:
    virtual ~CTSPIConnection();

// Methods
public:
    // These are the QUERYxxx functions
    CTSPIDevice* GetDeviceInfo() const;
    LPCTSTR GetName() const;
    LPCTSTR GetConnInfo() const;
    DWORD GetDeviceID() const;
    DWORD GetNegotiatedVersion() const;
    BOOL IsLineDevice() const;
    BOOL IsPhoneDevice() const;
	DWORD GetFlags() const;
	BOOL HasBeenDeleted() const;

    // These are the SETxxx functions
    void SetName(LPCTSTR lpszName);   
    void SetConnInfo(LPCTSTR lpszInfo);

	// This function calls SendData()
    BOOL SendString (LPCTSTR lpszBuff);

   	// Functions to manipulate the device class array
	int AddDeviceClass (LPCTSTR pszClass, DWORD dwData);
	int AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPCTSTR lpszBuff);
	int AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPVOID lpBuff, DWORD dwSize);
	int AddDeviceClass (LPCTSTR pszClass, DWORD dwFormat, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle=INVALID_HANDLE_VALUE);
	int AddDeviceClass (LPCTSTR pszClass, LPCTSTR pszBuff, DWORD dwType=-1L);
	BOOL RemoveDeviceClass (LPCTSTR pszClass);
	DEVICECLASSINFO* GetDeviceClass(LPCTSTR pszClass);

    // These functions manage our asynchronous request list.
    CTSPIRequest* AddAsynchRequest(CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
    CTSPIRequest* AddAsynchRequest(CTSPICallAppearance* pCall, WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
    BOOL RemoveRequest(CTSPIRequest* pReq, BOOL fDelete=FALSE);
    int GetRequestCount() const;
    CTSPIRequest* GetRequest(int iPos) const;
	void RemovePendingRequests(CTSPICallAppearance* pCall=NULL, WORD wReqType=REQUEST_ALL, LONG lErrorCode=LINEERR_OPERATIONFAILED, BOOL fOnlyUnstarted=FALSE, CTSPIRequest* preqIgnore=NULL);
    void CompleteRequest (CTSPIRequest* pReq, LONG lResult = 0, BOOL fTellTAPI = TRUE, BOOL fRemove = TRUE);
    BOOL CompleteCurrentRequest(LONG lResult = 0, BOOL fTellTAPI = TRUE, BOOL fRemove = TRUE);
    CTSPIRequest* FindRequest(CTSPICallAppearance* pCall, WORD wReqType);
    void WaitForAllRequests(CTSPICallAppearance* pCall=NULL, WORD wRequest=REQUEST_ALL);

// Overridable functions
public:
    // This is the init method required to initialize our connection object.
    virtual void Init(CTSPIDevice* pDeviceOwner, DWORD dwDeviceID);

	// Required overrides by derived classes
    virtual DWORD GetPermanentDeviceID() const = 0;

	// Request list management.
	virtual LONG WaitForRequest(DWORD dwTimeout = 0L, CTSPIRequest* pReq = NULL);
    virtual CTSPIRequest* GetCurrentRequest() const;

    // These functions will call back to the service provider if not supplied.
    virtual BOOL OpenDevice();
    virtual BOOL CloseDevice();
    virtual BOOL SendData (LPCVOID lpBuff, DWORD dwSize);
    virtual BOOL ReceiveData (DWORD dwData=0, const LPVOID lpBuff=NULL, DWORD dwSize=0);

#ifdef _DEBUG
    virtual void Dump(CDumpContext& de) const;
#endif

// Internal methods of the object.
protected:
    friend class CTSPIRequest;
    friend class CServiceProvider;
    friend class CTSPIDevice;
	virtual BOOL AddAsynchRequest (CTSPIRequest* pReq, int iPos);
	virtual BOOL OnNewRequest (CTSPIRequest* pReq, int* piPos);
	virtual void OnCancelRequest (CTSPIRequest* pReq);
    virtual void OnRequestComplete (CTSPIRequest* pReq, LONG lResult) = 0;
    virtual void OnTimer();
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
	HPROVIDER   m_hProvider;		// Our provider handle assigned by TAPI.	
    CObArray    m_arrayLines;       // List of CTSPIConnection line structures
    CObArray    m_arrayPhones;      // List of CTSPIConnection phone structures
    ASYNC_COMPLETION m_lpfnCompletionProc;  // Our asynchronous completion callback
	CEvent	    m_evtDeviceShutdown;		// Set when the device is shutdown
	CWinThread* m_pIntervalTimer;	// Interval timer for this device
	DWORD		m_dwIntervalTimeout;		// Timeout for interval timer

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
	HPROVIDER GetProviderHandle() const;
	void SetIntervalTimer (DWORD dwTimeout);

    // These support Plug&Play for the devices
    int CreateLine(DWORD dwItemData=0);
    int CreatePhone(DWORD dwItemData=0);
	void RemoveLine(CTSPILineConnection* pLine);
	void RemovePhone(CTSPIPhoneConnection* pPhone);

	// Function to associate a line/phone together
	void AssociateLineWithPhone(int iLine, int iPhone);

    // This set of methods accesses the connection arrays to retrieve connection info structures.
    CTSPILineConnection* GetLineConnectionInfo(int nIndex) const;
    CTSPIPhoneConnection* GetPhoneConnectionInfo(int nIndex) const;
    CTSPILineConnection* FindLineConnectionByDeviceID(DWORD dwDeviceId) const;
    CTSPIPhoneConnection* FindPhoneConnectionByDeviceID(DWORD dwDeviceId) const;

// Overridable methods
public:
    // This distributes a received data buffer to all or a particular connection.
    virtual void ReceiveData (DWORD dwConnID, DWORD dwData, const LPVOID lpBuff, DWORD dwSize);

	// This is called by the connection object to open the device.  Default behavior is
	// to pass it through to the service provider object.
	virtual BOOL OpenDevice (CTSPIConnection* pConn);

	// This is called by the connection object to close the device.  Default behavior is
	// to pass it through to the service provider object.
	virtual BOOL CloseDevice (CTSPIConnection* pConn);

	// This is called by the connections to send data - default behavior is to pass through
	// to the CServiceProvider class
	virtual BOOL SendData (CTSPIConnection* pConn, LPCVOID lpBuff, DWORD dwSize);

	// This is called to process UI dialog data
	virtual LONG GenericDialogData (LPVOID lpParam, DWORD dwSize);

// Internal methods
protected:
    friend class CTSPILineConnection;
    friend class CTSPIPhoneConnection;
    friend class CTSPICallAppearance;
    friend class CServiceProvider;
    friend class CTSPIConnection;
	friend UINT _IntervalTimerThread(LPVOID pParam);

    // This function is called directly after the constructor to actually initialize the
    // device object.  Once this completes, the device should be ready to be queried by TAPI.
    virtual void Init(DWORD dwProviderId, DWORD dwBaseLine, DWORD dwBasePhone, DWORD dwLines, DWORD dwPhones, HPROVIDER hProvider, ASYNC_COMPLETION lpfnCompletion);
    
    // The asynchronous callback is used when a TAPI request has finished
    // and we used the asynchronous handle given to us by TAPI.
    virtual void OnAsynchRequestComplete(LONG lResult = 0L, CTSPIRequest* pReq = NULL);

	// This function is called peridically to process the interval timer.
    virtual void OnTimer();

	// This is called when a new request is inserted by the line or phone connection.
	// It passes the notification onto the CServiceProvider object by default.
	virtual BOOL OnNewRequest (CTSPIConnection* pConn, CTSPIRequest* pReq, int* piPos);

    // This method is called when a request is canceled and it has already
    // been started on the device.  It is called by the connection object when
	// the request is being deleted.  Default behavior is to pass through to the
	// CServiceProvider class.
    virtual void OnCancelRequest (CTSPIRequest* pReq);

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
	enum {
		NotifyNumCalls = 0x00000004		// Transient flag -- set on PreCallStateChange
			// Additional may be added in the future.
	};

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
	CPtrArray m_arrUIDialogs;       // Array of active UI dialogs

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
    DWORD CreateAddress (LPCTSTR lpszDialableAddr=NULL, LPCTSTR lpszAddrName=NULL, 
                         BOOL fAllowIncoming=TRUE, BOOL fAllowOutgoing=NULL,
                         DWORD dwAvailMediaModes=LINEMEDIAMODE_UNKNOWN,
                         DWORD dwBearerMode=LINEBEARERMODE_VOICE,
                         DWORD dwMinRate=0L, DWORD dwMaxRate=0L, LPLINEDIALPARAMS lpDialParams=NULL,
                         DWORD dwMaxNumActiveCalls=1, DWORD dwMaxNumOnHoldCalls=0, 
                         DWORD dwMaxNumOnHoldPendCalls=0, DWORD dwMaxNumConference=0, 
                         DWORD dwMaxNumTransConf=0);

    int GetAddressCount() const;
    CTSPIAddressInfo* GetAddress (int iAddressID) const;
    CTSPIAddressInfo* GetAddress (DWORD dwAddressID) const;
    CTSPIAddressInfo* GetAddress (LPCTSTR lpszDialableAddr) const;

	// Create a new UI instance dialog
	HTAPIDIALOGINSTANCE CreateUIDialog (DRV_REQUESTID dwRequestID, LPVOID lpParams=NULL, DWORD dwSize=0L, LPCTSTR lpszUIDLLName=NULL);
	void SendDialogInstanceData (HTAPIDIALOGINSTANCE htDlgInstance, LPVOID lpParams=NULL, DWORD dwSize=0L);
	LINEUIDIALOG* GetUIDialog (HTAPIDIALOGINSTANCE htDlgInstance);

	// Returns the associated phone (if any)
	CTSPIPhoneConnection* GetAssociatedPhone() const;
    
	// This function will run through all our addresses and see if any support
    // the specified media modes.  It will return success if any do.    
    DWORD GetDefaultMediaDetection() const;

    // Get a pointer to the line device capabilities
    LPLINEDEVCAPS GetLineDevCaps();
    LPLINEDEVSTATUS GetLineDevStatus();

	// Call location methods
    CTSPICallAppearance* FindCallByState(DWORD dwCallState) const;

    // Call completion support
    TSPICOMPLETECALL* FindCallCompletionRequest (DWORD dwSwitchInfo, LPCTSTR pszSwitchInfo);
	TSPICOMPLETECALL* FindCallCompletionRequest(CTSPICallAppearance* pCall);
	TSPICOMPLETECALL* FindCallCompletionRequest(DWORD dwCompletionID);
    void RemoveCallCompletionRequest(DWORD dwCompletionID, BOOL fNotifyTAPI=FALSE);

    // Terminal support functions.  A terminal is a notification device of a line.  The various
    // types are defined by the LINETERMMODE_xxx bits.  To support terminals, simply add each terminal
    // at any time (TAPI is notified).
    int AddTerminal (LPCTSTR lpszName, LINETERMCAPS& Caps, DWORD dwModes=0L);
    void RemoveTerminal (int iTerminalId);
    int GetTerminalCount() const;
    DWORD GetTerminalInformation (int iTerminalID) const;

	// This is used to send a notification to TAPI for this line device.
    void Send_TAPI_Event(CTSPICallAppearance* pCall, DWORD dwMsg, DWORD dwP1 = 0L, DWORD dwP2 = 0L, DWORD dwP3 = 0L);

    // Members which set status information for this line.
    void SetBatteryLevel (DWORD dwBattery);
    void SetSignalLevel (DWORD dwSignal);
    void SetRoamMode (DWORD dwRoamMode);
    void SetDeviceStatusFlags (DWORD dwStatus);
    void SetRingMode (DWORD dwRingMode);
    void SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal);
	void SetLineFeatures (DWORD dwFeatures);
    void SetMediaControl (TSPIMEDIACONTROL* lpMediaControl);

	// Force the line to close
	void ForceClose();

    // Method which should be called when a ring is detected on this line.
    void OnRingDetected (DWORD dwRingMode, BOOL fFirstRing = FALSE);

// Overridable functions
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
    virtual LONG GetAddressID(LPDWORD lpdwAddressId, DWORD dwAddressMode, LPCTSTR lpszAddress, DWORD dwSize);
    virtual LONG SetTerminal (DRV_REQUESTID dwReqID, TSPILINESETTERMINAL* lpLine);
    virtual LONG Forward (DRV_REQUESTID dwRequestId, CTSPIAddressInfo* pAddr, TSPILINEFORWARD* lpForwardInfo, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall);
    virtual LONG MakeCall (DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, TSPIMAKECALL* lpMakeCall);
    virtual LONG SetDefaultMediaDetection (DWORD dwMediaModes);
    virtual LONG SetStatusMessages(DWORD dwLineStates, DWORD dwAddressStates);
    virtual LONG ConditionalMediaDetection(DWORD dwMediaModes, const LPLINECALLPARAMS lpCallParams);
    virtual LONG GatherCapabilities (DWORD dwTSPIVersion, DWORD dwExtVer, LPLINEDEVCAPS lpLineCaps);
    virtual LONG GatherStatus (LPLINEDEVSTATUS lpStatus);
    virtual LONG UncompleteCall (DRV_REQUESTID dwRequestID, DWORD dwCompletionID);
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID, HANDLE hTargetProcess);
    virtual LONG GetDevConfig(CString& strDeviceClass, LPVARSTRING lpDeviceConfig);
    virtual LONG SetDevConfig(CString& strDeviceClass, const LPVOID lpDevConfig, DWORD dwSize);
    virtual LONG DevSpecificFeature(DWORD dwFeature, DRV_REQUESTID dwRequestId, LPVOID lpParams, DWORD dwSize);
    virtual LONG GetIcon (CString& strDevClass, LPHICON lphIcon);
	virtual LONG SetLineDevStatus (DRV_REQUESTID dwRequestID, DWORD dwStatusToChange, BOOL fStatus);
	virtual LONG GenericDialogData (LINEUIDIALOG* pLineDlg, LPVOID lpParam, DWORD dwSize);

	// These can be used to notify TAPI about changes to LINEDEVCAPS or LINEDEVSTATUS.
    virtual void OnLineCapabiltiesChanged();
    virtual void OnLineStatusChange (DWORD dwState, DWORD dwP2=0L, DWORD dwP3=0L);
	virtual void OnMediaConfigChanged();
    virtual void OnMediaControl (CTSPICallAppearance* pCall, DWORD dwMediaControl);
	virtual void RecalcLineFeatures();

// Internal methods
protected:
    friend class CTSPIAddressInfo;
    friend class CTSPICallAppearance;
    friend class CServiceProvider;
	friend class CTSPIConnection;
    friend class CTSPIDevice;

	// Overridable functions and notifications
    virtual void Init(CTSPIDevice* pDev, DWORD dwLineDeviceID, DWORD dwPos, DWORD dwItemData=0);
	virtual DWORD OnAddressFeaturesChanged (CTSPIAddressInfo* pAddr, DWORD dwFeatures);
    virtual void OnCallDeleted(CTSPICallAppearance* pCall);
	virtual DWORD OnCallFeaturesChanged(CTSPICallAppearance* pCall, DWORD dwCallFeatures);
	virtual void OnPreCallStateChange (CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, DWORD dwNewState, DWORD dwOldState);
    virtual void OnCallStateChange (CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, DWORD dwNewState, DWORD dwOldState);
	virtual DWORD OnLineFeaturesChanged(DWORD dwLineFeatures);
    virtual void OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
    virtual void OnTimer();
	virtual void OnConnectedCallCountChange(CTSPIAddressInfo* pInfo, int iDelta);

	// Callable functions from within the line object
    BOOL CanHandleRequest(WORD wRequest, DWORD dwData=0);
    BOOL IsConferenceAvailable(CTSPICallAppearance* pCall);
    BOOL IsTransferConsultAvailable(CTSPICallAppearance* pCall);
	LONG FreeDialogInstance(HTAPIDIALOGINSTANCE htDlgInst);
};

/******************************************************************************/
// 
// CTSPIAddressInfo
//
// This class defines the address information for a single address on 
// a line.  The address may contain one or more call appearances (although 
// generally only one is active at a time).
//
/******************************************************************************/
class CTSPIAddressInfo : public CTSPIBaseObject
{
    DECLARE_DYNCREATE (CTSPIAddressInfo) // Allow dynamic creation

// Class data
protected:
	enum {
		InputAvail		= 0x1,			// Address may answer calls
		OutputAvail		= 0x2,			// Address may place calls
		NotifyNumCalls	= 0x4			// Transient flag -- set on PreCallStateChange
	};
	
	DWORD m_dwFlags;					// Flags for this address
    CTSPILineConnection* m_pLine;		// Line owner
    LINEADDRESSCAPS m_AddressCaps;		// Basic address capabilities
    LINEADDRESSSTATUS m_AddressStatus;	// Current available status on this address
    DWORD m_dwAddressID;				// Address identifier (0-numAddr).
    DWORD m_dwAddressStates;			// Which address state change messages need to be sent to TAPI.DLL?
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
    CStringArray m_arrCompletionMsgs;	// Completion message information
    LINEDIALPARAMS m_DialParams;		// Dialing parameters supported on address
    TSPIMEDIACONTROL* m_lpMediaControl; // Current MEDIACONTROL in effect (NULL if none).
	CADObArray m_arrDeviceClass;		// Device class array for lineGetID
	CMapDWordToString m_mapCallTreatment; // Call treatments available on this address.
        
// Constructor
protected:
    CTSPIAddressInfo();					// Should only be created by the CreateObject method.
public:
    virtual ~CTSPIAddressInfo();

// Access methods
public:
    // The following are the QUERYxxx functions for the static non-changing data
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
    LPCTSTR GetDialableAddress() const;
	void SetDialableAddress(LPCTSTR pwszAddress);
    LPCTSTR GetName() const;
	void SetName (LPCTSTR pwszName);

    // Media mode support
    DWORD GetAvailableMediaModes () const;

	// This function creates a new call on the address.  This would be used for incoming/outgoing calls.
    CTSPICallAppearance* CreateCallAppearance(HTAPICALL hCall=NULL, DWORD dwCallParamFlags=0, DWORD dwOrigin=LINECALLORIGIN_UNKNOWN,
                                    DWORD dwReason=LINECALLREASON_UNKNOWN, DWORD dwTrunk=0xffffffff, DWORD dwCompletionID=0);

	// This function creates a new CONFERENCE object to manage a conference call on the address.
    CTSPIConferenceCall* CreateConferenceCall(HTAPICALL hCall);

	// This function removes a call object (either normal or conference).  The call is deleted.
    void RemoveCallAppearance(CTSPICallAppearance* pCall);

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
    int AddForwardEntry (DWORD dwForwardMode, LPCTSTR pszCaller, LPCTSTR pszDestination, DWORD dwDestCountry);

	// Functions to manipulate the device class array
	int AddDeviceClass (LPCTSTR pszClass, DWORD dwData);
	int AddDeviceClass (LPCTSTR pszClass, LPCTSTR lpszBuff, DWORD dwType = -1L);
	int AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPCTSTR lpszBuff);
	int AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPVOID lpBuff, DWORD dwSize);
	int AddDeviceClass (LPCTSTR pszClass, DWORD dwFormat, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle=INVALID_HANDLE_VALUE);
	BOOL RemoveDeviceClass (LPCTSTR pszClass);
	DEVICECLASSINFO* GetDeviceClass(LPCTSTR pszClass);

	// This function adds a new call treatment entry to our array.
	void AddCallTreatment (DWORD dwCallTreatment, LPCTSTR pszName);
	void RemoveCallTreatment (DWORD dwCallTreatment);
	CString GetCallTreatmentName (DWORD dwCallTreatment) const;

    // Completion message support
    int AddCompletionMessage (LPCTSTR pszBuff);
    int GetCompletionMessageCount() const;
    LPCTSTR GetCompletionMessage (int iPos) const;

    // The following are the SETxxx functions
    void SetNumRingsNoAnswer (DWORD dwNumRings);
    void SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal);
	void SetCurrentRate (DWORD dwRate);
	void SetAddressFeatures(DWORD dwFeatures);
    void SetMediaControl (TSPIMEDIACONTROL* lpMediaControl);

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
    virtual LONG GatherCapabilities (DWORD dwTSPIVersion,DWORD dwExtVersion,LPLINEADDRESSCAPS lpAddressCaps);
    virtual LONG GatherStatusInformation (LPLINEADDRESSSTATUS lpAddressStatus);
    virtual void SetStatusMessages(DWORD dwStates);
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID, HANDLE hTargetProcess);

#ifdef _DEBUG
    virtual void Dump(CDumpContext& dc) const;
#endif

    // The following is called when any of the ADDRESSSTATE entries have changed.  It
    // is used internally, but should also be called if the derived service provider
    // changes address state information itself.
    virtual void OnAddressStateChange (DWORD dwAddressState);

    // The following should be called if any of the address capabilities change
    // during the life of the service provider.
    virtual void OnAddressCapabiltiesChanged();

	// Recalc the address features using the line
	virtual void RecalcAddrFeatures();

// Internal methods
protected:
	friend class CServiceProvider;
    friend class CTSPILineConnection;
    friend class CTSPIRequest;
    friend class CTSPICallAppearance;

    // This method is called when the address connection is created directly after
    // the constructor, it is called by the CTSPILineConnection object
    virtual void Init (CTSPILineConnection* pLine, DWORD dwAddressID, LPCTSTR lpszAddress, 
					   LPCTSTR lpszName, BOOL fIncoming, BOOL fOutgoing, DWORD dwAvailMediaModes, 
                       DWORD dwAvailBearerModes, DWORD dwMinRate, DWORD dwMaxRate,
                       DWORD dwMaxNumActiveCalls, DWORD dwMaxNumOnHoldCalls, 
                       DWORD dwMaxNumOnHoldPendCalls, DWORD dwMaxNumConference, 
                       DWORD dwMaxNumTransConf);

    // Notification for a new call appearance created
    virtual void OnCreateCall (CTSPICallAppearance* pCall);

    // This method is called by the call appearances when the call state changes.
	virtual void OnPreCallStateChange (CTSPICallAppearance* pCall, DWORD dwNewState, DWORD dwOldState);
    virtual void OnCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD dwOldState);

    // This method is called whenever the terminal line count changes.              
    virtual void OnTerminalCountChanged (BOOL fAdded, int iPos, DWORD dwMode=0L);

	// Called when a request completes on this address
    virtual void OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
    
	// The following is called by the call appearance when the call features for the
	// call have changed.  The return value is the adjusted call features.
	virtual DWORD OnCallFeaturesChanged(CTSPICallAppearance* pCall, DWORD dwCallFeatures);
	virtual DWORD OnAddressFeaturesChanged (DWORD dwFeatures);

    // Various support methods
    CTSPIRequest* AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
    BOOL CanHandleRequest(WORD wRequest, DWORD dwData=0);
    void DeleteForwardingInfo();
    DWORD GetTerminalInformation (int iTerminalID) const;
	BOOL NotifyInUseZero();
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
		InitNotify  = 0x0008,			// Bit is set when TAPI is told about call 1st time.
		IsChgState  = 0x0010			// Bit set when in SetCallState
	};

    CTSPIAddressInfo* m_pAddr;			// Address identifier for this call.
	DWORD m_dwFlags;					// Call flags
	LONG m_lRefCount;					// Reference count for call
    LINECALLINFO m_CallInfo;			// Current call information for this object
    LINECALLSTATUS m_CallStatus;		// Current call status
    HTAPICALL m_htCall;					// TAPI opaque call handle
    CALLIDENTIFIER m_CallerID;			// Caller ID information
    CALLIDENTIFIER m_CalledID;			// Called ID information
    CALLIDENTIFIER m_ConnectedID;		// Connected ID information
    CALLIDENTIFIER m_RedirectionID;		// Redirection ID information
    CALLIDENTIFIER m_RedirectingID;		// Redirecting ID information
    CDWordArray m_arrTerminals;			// Terminal array
    TSPIDIGITGATHER* m_lpGather;		// Current DIGITGATHER in effect (NULL if none).
    TSPIMEDIACONTROL* m_lpMediaControl; // Current MEDIACONTROL in effect (NULL if none).
    CObArray m_arrMonitorTones;			// Current tones being monitored for.
    CObArray m_arrEvents;				// Pending timer events for MEDIACONTROL and TONE DETECT.
    CTSPICallAppearance* m_pConsult;	// Attached consultant call (NULL if none)
	CTSPIConferenceCall* m_pConf;		// Attached conference call (NULL if none)
    int m_iCallType;					// Call type (CALLTYPE_xxxx)
	LPVOID m_lpvCallData;				// Call Data (2.0)
	DWORD m_dwCallDataSize;				// Data size (2.0)
	LPVOID m_lpvSendingFlowSpec;		// FLOWSPEC for QOS (2.0)
	DWORD m_dwSendingFlowSpecSize;		// Size of above (2.0)
	LPVOID m_lpvReceivingFlowSpec;		// FLOWSPEC for QOS (2.0)
	DWORD m_dwReceivingFlowSpecSize;	// Size of above (2.0)
	CADObArray m_arrDeviceClass;		// Device class array for lineGetID (2.0)
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
    CTSPILineConnection* GetLineOwner() const;
    CTSPIAddressInfo* GetAddressOwner() const;
    DWORD GetCallState() const;
    LPLINECALLINFO GetCallInfo();
    LPLINECALLSTATUS GetCallStatus();
    CTSPICallAppearance* GetAttachedCall() const;
	CTSPIConferenceCall* GetConferenceOwner() const;

	// Backward compatible stubs
	CTSPILineConnection* GetLineConnectionInfo() const { return GetLineOwner(); }
    CTSPIAddressInfo* GetAddressInfo() const { return GetAddressOwner(); }

	// Consultation call attachment
	CTSPICallAppearance* CreateConsultationCall(HTAPICALL hCall=NULL, DWORD dwCallParamFlags=0);
	void SetConsultationCall(CTSPICallAppearance* pCall);
	CTSPICallAppearance* GetConsultationCall() const;

	// This allows query/set of the call type (consultant/conference/normal)
    int GetCallType() const;
    void SetCallType (int iCallType);

	// Reference count functions for request management
	// Call will NOT be deleted until final reference count removal.
	void IncRefCount();
	void DecRefCount();

	// Functions to manipulate the device class array
	int AddDeviceClass (LPCTSTR pszClass, DWORD dwData);
	int AddDeviceClass (LPCTSTR pszClass, LPCTSTR lpszBuff, DWORD dwType = -1L);
	int AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPCTSTR lpszBuff);
	int AddDeviceClass (LPCTSTR pszClass, HANDLE hHandle, LPVOID lpBuff, DWORD dwSize);
	int AddDeviceClass (LPCTSTR pszClass, DWORD dwFormat, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle=INVALID_HANDLE_VALUE);
	BOOL RemoveDeviceClass (LPCTSTR pszClass);
	DEVICECLASSINFO* GetDeviceClass(LPCTSTR pszClass);

    // Internal methods for consultant call management.
    void AttachCall(CTSPICallAppearance* pCall);
	void SetConferenceOwner(CTSPIConferenceCall* pCall);
    void DetachCall();

#ifdef _DEBUG
    LPCTSTR GetCallStateName (DWORD dwState=0L) const;
#endif
    // Return whether the supplied callstate is ACTIVE according to TAPI rules.
    static BOOL IsActiveCallState(DWORD dwState);
	static BOOL IsConnectedCallState(DWORD dwState);

    // This method should be called whenever a digit is detected.  It manages any monitor
    // or gathering being performed on the call.  Two versions are supplied so that
	// the passed digit may or may not be double-byte.
#ifdef _UNICODE
    void OnDigit (DWORD dwType, char cDigit);
#endif
	void OnDigit (DWORD dwType, TCHAR cDigit);
            
    // This method should be called whenever a tone generation is detected.  It manages
    // any monitor or gathering being performed on the call.                
    void OnTone (DWORD dwFreq1, DWORD dwFreq2=0, DWORD dwFreq3=0);

    // This method is called during the initial call setup, or when a new media type
    // begins playing over the media stream.  It is automatically called during call state
    // changes if the media mode is adjusted by the SetCallState method.
    void OnDetectedNewMediaModes (DWORD dwMediaModes);
    
    // This method should be called if user-user information is received from
    // the underlying network.  The data is COPIED into an internal buffer and
    // may be deleted after the call.
    void OnReceivedUserUserInfo (LPVOID lpBuff, DWORD dwSize);
    
    // TAPI methods called by the CServiceProvider class.
    virtual LONG Close();
    virtual LONG Drop(DRV_REQUESTID dwRequestId=0, LPCSTR lpszUserUserInfo=NULL, DWORD dwSize=0);
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
    virtual LONG CompleteCall (DRV_REQUESTID dwRequestId, LPDWORD lpdwCompletionID, TSPICOMPLETECALL* lpCompCall);
    virtual LONG GatherStatusInformation(LPLINECALLSTATUS lpCallStatus);
    virtual LONG GatherCallInformation (LPLINECALLINFO lpCallInfo);
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID, HANDLE hTargetProcess);
    virtual LONG ReleaseUserUserInfo(DRV_REQUESTID dwRequest);
	virtual LONG SetQualityOfService (DRV_REQUESTID dwRequestID, TSPIQOS* pQOS);
	virtual LONG SetCallTreatment(DRV_REQUESTID dwRequestID, DWORD dwCallTreatment);
	virtual LONG SetCallData (DRV_REQUESTID dwRequestID, TSPICALLDATA* pCallData);

    // The following are the SETxxx functions for the CALLINFO of the call appearance.
    // They will cause the appropriate LINECALLSTATE message to be generated.
    void SetBearerMode(DWORD dwBearerMode);
    void SetDataRate(DWORD dwDataRate);
    void SetAppSpecificData(DWORD dwAppSpecific);
    void SetCallID (DWORD dwCallID);
    void SetRelatedCallID (DWORD dwCallID);
    void SetCallParameterFlags (DWORD dwFlags);
    void SetDialParameters (LINEDIALPARAMS& dp);
    void SetCallOrigin(DWORD dwOrigin);
    void SetCallReason(DWORD dwReason);
    void SetDestinationCountry (DWORD dwCountryCode);
    void SetTrunkID (DWORD dwTrunkID);
    void SetCallerIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID=NULL, LPCTSTR lpszName=NULL, DWORD dwCountryCode=0);
    void SetCalledIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID=NULL, LPCTSTR lpszName=NULL, DWORD dwCountryCode=0);
    void SetConnectedIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID=NULL, LPCTSTR lpszName=NULL, DWORD dwCountryCode=0);
    void SetRedirectionIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID=NULL, LPCTSTR lpszName=NULL, DWORD dwCountryCode=0);
    void SetRedirectingIDInformation (DWORD dwFlags, LPCTSTR lpszPartyID=NULL, LPCTSTR lpszName=NULL, DWORD dwCountryCode=0);
    void SetCallState(DWORD dwState, DWORD dwMode=0L, DWORD dwMediaMode=0L, BOOL fTellTapi=TRUE);
    void SetTerminalModes (int iTerminalID, DWORD dwTerminalModes, BOOL fRouteToTerminal);
	void SetCallData (LPVOID lpvCallData, DWORD dwSize);
	void SetQualityOfService (LPVOID lpSendingFlowSpec, DWORD dwSendSize, LPVOID lpReceivingFlowSpec, DWORD dwReceiveSize);
	void SetCallTreatment(DWORD dwCallTreatment);
    void SetDigitMonitor(DWORD dwDigitModes);
    void SetMediaMonitor(DWORD dwModes);
	void SetCallFeatures(DWORD dwFeatures, BOOL fNotifyTAPI=TRUE);
	void SetCallFeatures2(DWORD dwCallFeatures2, BOOL fNotifyTAPI=TRUE);
    void SetMediaControl (TSPIMEDIACONTROL* lpMediaControl);

// Internal methods
protected:
	friend class CServiceProvider;
    friend class CTSPIAddressInfo;
    friend class CTSPIConferenceCall;
    friend class CTSPIRequest;

    void CompleteDigitGather (DWORD dwReason);
    void DeleteToneMonitorList();
    BOOL CanHandleRequest (WORD wRequest, DWORD dwData=0);
    CTSPIRequest* AddAsynchRequest(WORD wReqId, DRV_REQUESTID dwReqId=0, LPCVOID lpBuff=NULL, DWORD dwSize=0);
	void NotifyCallStatusChanged();

    // This is the INIT function which is called directly after the constructor
    virtual void Init (CTSPIAddressInfo* pAddr, HTAPICALL hCall, DWORD dwBearerMode = LINEBEARERMODE_VOICE,
                       DWORD dwRate=0, DWORD dwCallParamFlags=0, DWORD dwOrigin=LINECALLORIGIN_UNKNOWN,
                       DWORD dwReason=LINECALLREASON_UNKNOWN, DWORD dwTrunk=0xffffffff, 
                       DWORD dwCompletionID=0, BOOL fNewCall=FALSE);

    // This method is called when information in our CALL status record has changed.
    // If the derived class changes data directly in the LINECALLSTATUS record, it
    // should invoke this method to tell TAPI. (Generally this won't happen).
    virtual void OnCallStatusChange (DWORD dwCallState, DWORD dwCallInfo, DWORD dwMediaModes);
    
    // This method is called when a Media Control event is detected.
    virtual void OnMediaControl (DWORD dwMediaControl);
    
    // This method is called when a TONE being monitored for is detected.
    virtual void OnToneMonitorDetect (DWORD dwToneListID, DWORD dwAppSpecific);

    // This method is called whenever the line terminal count is changed.
    virtual void OnTerminalCountChanged (BOOL fAdded, int iPos, DWORD dwMode=0L);

	// This method is called when an attached consultation call goes IDLE.
	virtual void OnConsultantCallIdle(CTSPICallAppearance* pConsultCall);

    // This method is called whenever information in our CALL information record
    // has changed.  If the derived class changes data directly in the LINECALLINFO
    // record, it should invoke this method to tell TAPI.
    virtual void OnCallInfoChange (DWORD dwCallInfo);

    // This method is called whenever a call which is related to this call changes
    // state.  The call relationship is made through the CALLINFO dwRelatedCallID field
    // and is used by conference and consultation calls to relate them to a call appearance.
    virtual void OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD dwOldState);

    // This method is called internally during the periodic interval timer.  It is used
    // to cancel digit gathering on timeouts.
	virtual BOOL OnInternalTimer();

	// This method is invoked when any request associated with this call has completed.
    virtual void OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
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
    CObArray m_arrConference;	// Array of CTSPICallAppearance ptrs

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
    void RemoveConferenceCall(CTSPICallAppearance* pCall, BOOL fForceBreakdown=TRUE);

// Internal methods
protected:
    friend CTSPICallAppearance;
    BOOL IsCallInConference(CTSPICallAppearance* pCall) const;
    BOOL CanRemoveFromConference(CTSPICallAppearance* pCall) const;
    virtual void OnRelatedCallStateChange (CTSPICallAppearance* pCall, DWORD dwState, DWORD dwOldState);
    virtual void OnCallStatusChange (DWORD dwState, DWORD dwMode, DWORD dwMediaMode);
    virtual void OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
};

/******************************************************************************/
//
// CPhoneButtonInfo
//
// This class contains all the elements from the PHONEBUTTONINFO
// structure, but allows the object to be stored in an object list
// and serialized.
//
// INTERNAL OBJECT
//
/******************************************************************************/
class CPhoneButtonInfo : public CObject
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
    CPhoneButtonInfo(DWORD dwButtonFunction, DWORD dwButtonMode, DWORD dwAvailLamp, DWORD dwLampMode, LPCTSTR lpszDesc);

// Access Methods
public:
    LPCTSTR GetDescription() const;
    DWORD GetFunction() const;
    DWORD GetButtonMode() const;
    DWORD GetButtonState() const;
    DWORD GetLampMode() const;
    DWORD GetAvailLampModes() const;
    void SetLampMode(DWORD dwLampMode);
    const CPhoneButtonInfo& operator=(LPPHONEBUTTONINFO lpPhoneInfo);
    void SetButtonInfo (DWORD dwFunction, DWORD dwMode, LPCTSTR lpszDesc);
    void SetButtonState (DWORD dwState);
    void  Serialize(CArchive& ar);
};

/******************************************************************************/
//
// CPhoneButtonArray
//
// This superclass of the CObArray provides support directly for
// CPhoneButtonInfo objects.
//
// INTERNAL OBJECT
//
/******************************************************************************/
class CPhoneButtonArray : public CObArray
{
    DECLARE_SERIAL( CPhoneButtonArray )
// Class data
protected:
    BOOL m_fDirty;		// Dirty flag (needs to be written).

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
    void SetDirtyFlag(BOOL fDirty=TRUE);
    int  FindButton(DWORD dwFunction, DWORD dwMode=PHONEBUTTONMODE_DUMMY, int iCount=1);
    const CPhoneButtonArray& operator=(CPhoneButtonArray& a);
    int Add(CPhoneButtonInfo* pObj);
    int Add(DWORD dwMode, DWORD dwFunc, DWORD dwAvailLampModes, DWORD dwLamp, LPCTSTR pszDesc);
    CPhoneButtonInfo * & ElementAt (int nIndex);
    CPhoneButtonInfo * GetAt(int nIndex) const;
    BOOL IsEmpty () const;
    BOOL IsValidIndex (int nIndex) const;
    void RemoveAll();
    void RemoveAt (int nIndex, int nCount = 1);
    void SetAt(int nIndex, CPhoneButtonInfo* pObj);
    void Serialize(CArchive& ar);
};

/******************************************************************************/
//
// CPhoneDisplay class
//
// This class manages a virtual "display" with cursor positioning, and
// linefeed interpretation.
//
// INTERNAL OBJECT
//
/******************************************************************************/
class CPhoneDisplay
{
// Class data
protected:
    LPTSTR m_lpsDisplay;	// Buffer for our display
    CSize m_sizDisplay;     // Buffer size
    CPoint m_ptCursor;      // Cursor position
    TCHAR m_cLF;            // Line feed character
    
// Methods
public:
    CPhoneDisplay();
    ~CPhoneDisplay();

// Access methods
public:
    void Init(int iCols, int iRows, TCHAR cLF=_T('\n'));
    
    // These query different capabilities of the device.
    LPCTSTR GetTextBuffer() const;
    CPoint GetCursorPosition() const;
    CSize GetDisplaySize() const;
    
    // These modify the display buffer
    void AddCharacter(TCHAR c);
    void AddString(LPCTSTR lpszText);
    void SetCharacterAtPosition(int iCol=-1, int iRow=-1, TCHAR c = 0);
    void SetCursorPosition(int iCol=-1, int iRow=-1);                  
    void Reset();
    void ClearRow(int iRow);
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
    void SetupDisplay (int iColumns, int iRows, char cLineFeed=_T('\n'));
    int AddButton (DWORD dwFunction, DWORD dwMode, DWORD dwAvailLampStates, DWORD dwLampState, LPCTSTR lpszText);
	void AddHookSwitchDevice (DWORD dwHookSwitchDev, DWORD dwAvailModes, DWORD dwCurrMode, 
							  DWORD dwVolume=-1L, DWORD dwGain=-1L, 
							  DWORD dwSettableModes=-1L, DWORD dwMonitoredModes=-1L);

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
    void AddDisplayChar (TCHAR cChar);
    void SetDisplayChar (int iColumn, int iRow, TCHAR cChar);
    void ResetDisplay();        
    void SetDisplay(LPCTSTR pszDisplayBuff);
    void SetDisplayCursorPos (int iColumn, int iRow);
    void ClearDisplayLine (int iRow);
    void AddDisplayString (LPCTSTR lpszText);    

	// Returns the associated line (if any)
	CTSPILineConnection* GetAssociatedLine() const;
    
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
    virtual LONG GetID (CString& strDevClass, LPVARSTRING lpDeviceID, HANDLE hTargetProcess);
	virtual LONG GenericDialogData (LPVOID lpParam, DWORD dwSize);
    
    // These are the SETxxx functions which notify TAPI.  They should only
    // be called by the worker code (not by TAPI).
    void SetButtonInfo (int iButtonID, DWORD dwFunction, DWORD dwMode, LPCTSTR pszName);
    DWORD SetLampState (int iButtonID, DWORD dwLampState);
    DWORD SetButtonState (int iButtonId, DWORD dwButtonState);
    DWORD SetStatusFlags (DWORD dwStatus);
    void SetRingMode (DWORD dwRingMode);
    void SetRingVolume (DWORD dwRingVolume);
    void SetHookSwitch (DWORD dwHookSwitchDev, DWORD dwMode);
    void SetVolume (DWORD dwHookSwitchDev, DWORD dwVolume);
    void SetGain (DWORD dwHookSwitchDev, DWORD dwGain);
	void SetPhoneFeatures (DWORD dwFeatures);

	// Force the phone to close
	void ForceClose();

	// Functions which notify TAPI about changes within data structures
    virtual void OnPhoneCapabiltiesChanged();
    virtual void OnPhoneStatusChange(DWORD dwState, DWORD dwParam = 0);

    // The event procedure is used as a callback into TAPISRV.EXE when
    // some event (phone state change, etc.) happens.  It will
    // be initialized when the phone is opened.
    void Send_TAPI_Event(DWORD dwMsg, DWORD dwP1 = 0L, DWORD dwP2 = 0L, DWORD dwP3 = 0L);

// Internal methods
protected:
    friend class CTSPIDevice;
	friend class CTSPIConnection;

    // This is called directly after the constructor to INIT the phone device.
    virtual void Init(CTSPIDevice* pDevice, DWORD dwPhoneId, DWORD dwPos, DWORD dwItemData=0);

    // Method to send phonestate notifications
    virtual void OnRequestComplete (CTSPIRequest* pReq, LONG lResult);
    virtual void OnButtonStateChange (DWORD dwButtonID, DWORD dwMode, DWORD dwState);
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
//
/******************************************************************************/
class CServiceProvider : public CWinApp
{
// Class data
protected:
    CObArray m_arrDevices;				// List of CTSPIDevice structures
	HPROVIDER m_hProvider;				// Handle to device for this provider
    CRuntimeClass* m_pRequestObj;		// dynamic creation of CTSPIRequest
    CRuntimeClass* m_pLineObj;			// dynamic creation of CTSPILineConnection   
    CRuntimeClass* m_pPhoneObj;			// dynamic creation of CTSPIPhoneConnection
    CRuntimeClass* m_pDeviceObj;		// dynamic creation of CTSPIDevice
    CRuntimeClass* m_pCallObj;			// dynamic creation of CTSPICallAppearance
    CRuntimeClass* m_pConfCallObj;		// dynamic creation of CTSPIConferenceCall
    CRuntimeClass* m_pAddrObj;			// dynamic creation of CTSPIAddressInfo
    CFlagArray m_arrProviderCaps;		// Provider capabilities
	CEvent m_evtShutdown;				// Shutdown event.
	CWinThread* m_pThreadI;				// Interval timer thread pointer
	LONG m_lTimeout;					// Timeout for the requests
    LINEEVENT m_lpfnLineCreateProc;		// Support Plug&Play dynamic line creation
    PHONEEVENT m_lpfnPhoneCreateProc;	// Support Plug&Play dynamic phone creation
    LPCTSTR m_pszProviderInfo;			// Provider information
    DWORD m_dwCurrentLocation;			// Current location according to TAPI (v1.4)
    DWORD m_dwTapiVerSupported;			// TAPI versions supported
    DWORD m_dwTAPIVersionFound;			// TAPI version on this system.
	CCriticalSection m_csProvider;		// Critical section for list
	CObList m_lstTimedCalls;			// Calls being serviced with timers.

// CWinApp specific overrides
public:
    CServiceProvider(LPCTSTR pszAppName, LPCTSTR pszProviderInfo=NULL, DWORD dwTapiVer = TAPIVER_20); 
    virtual BOOL InitInstance(); // Initialization
    virtual int ExitInstance();  // Termination 
       
// Access methods
public:
    // This method retreives specific device connection objects
    CTSPIDevice* GetDevice(DWORD dwProviderID) const;
	DWORD GetDeviceCount() const;
	CTSPIDevice* GetDeviceByIndex(int iIndex) const;

    // Return what TAPI says the current location of this computer is.
    DWORD GetCurrentLocation() const;

    // Return the name of the executable module (passed to CServiceProvider constructor).
    LPCTSTR GetProviderName() const;

    // Return provider information string (passed to CServiceProvider constructor).
    LPCTSTR GetProviderInfo() const;

    // Returns the support TAPI version (passed to CServiceProvider constructor).
    DWORD GetSupportedVersion() const;

	// Returns the version of TAPI installed in the system.
    DWORD GetSystemVersion() const;

	// Registry storage manipulation methods which can be used to store information
	// about devices in the provider.
	CString ReadProfileString (DWORD dwDeviceID, LPCTSTR pszEntry, LPCTSTR pszDefault = _T(""));
	DWORD ReadProfileDWord (DWORD dwDeviceID, LPCTSTR pszEntry, DWORD dwDefault = 0L);
	BOOL WriteProfileString (DWORD dwDeviceID, LPCTSTR pszEntry, LPCTSTR pszValue);
	BOOL WriteProfileDWord (DWORD dwDeviceID, LPCTSTR pszEntry, DWORD dwValue);
	BOOL DeleteProfile (DWORD dwDeviceID);
	BOOL RenameProfile (DWORD dwOldDevice, DWORD dwNewDevice);

    // These methods search all our connections for a specific phone or line connection entry.
    CTSPILineConnection* GetConnInfoFromLineDeviceID(DWORD dwDevId);
    CTSPIPhoneConnection* GetConnInfoFromPhoneDeviceID(DWORD dwDevId);

	// Helper functions which are "stand-alone" and can be called from UI dlls.
	LONG IsProviderInstalled(LPCTSTR pszProviderName, LPDWORD lpdwPPid) const;
	LONG GetProviderIDFromDeviceID(TUISPIDLLCALLBACK lpfnDLLCallback, DWORD dwDeviceID, BOOL fIsLine, LPDWORD lpdwPPid);
	LONG GetPermanentIDFromDeviceID(TUISPIDLLCALLBACK lpfnDLLCallback, DWORD dwDeviceID, BOOL fIsLine, LPDWORD lpdwPPid);

	void AddTimedCall(CTSPICallAppearance* pCall);
	void RemoveTimedCall(CTSPICallAppearance* pCall);

// Overridable methods
public:
    // This method checks the call parameters based on the line and specific address.  
    // It runs through all the addresses depending on the parameters passed.
    virtual LONG ProcessCallParameters(CTSPILineConnection* pLine, LPLINECALLPARAMS lpCallParams);

    // This method checks the number to determine if we support the
    // dialable address.  The final form address is returned in the buffer specified.
    virtual LONG CheckDialableNumber(CTSPILineConnection* pLine, CTSPIAddressInfo* pAddr, LPCTSTR lpszDigits, CObArray* parrEntries, DWORD dwCountry, LPCTSTR pszValidChars=NULL);
    virtual CString GetDialableNumber (LPCTSTR pszNumber, LPCTSTR pszAllowChar=NULL) const;
    virtual CString ConvertDialableToCanonical (LPCTSTR pszNumber, DWORD dwCountry=0);

    // These functions are routed back to the SP class by the device class.  The device
    // may be overriden in order to handle them there, but we maintain compatibility with
    // previous versions of the library through the reroute here.
    virtual BOOL OpenDevice (CTSPIConnection* pConn);
    virtual BOOL CloseDevice (CTSPIConnection* pConn);
    virtual BOOL SendData(CTSPIConnection* pConn, LPCVOID lpBuff, DWORD dwSize);
    virtual void OnTimer(CTSPIConnection* pConn);

    // This function is called when matching a tone against one seen on the media stream.
    virtual BOOL MatchTones (DWORD dwSFreq1, DWORD dwSFreq2, DWORD dwSFreq3, DWORD dwTFreq1, DWORD dwTFreq2, DWORD dwTFreq3);

	// Used in DEBUG builds to output trace responses
	virtual void TraceOut(LPCTSTR pszBuff);

// Set methods for the derived class to use
protected:
    // Set the C++ objects to use for each basic telephony object in the system.  This must be
	// done during the constructor of the service provider.
    void SetRuntimeObjects(CRuntimeClass* pDevObj, CRuntimeClass* pReqObj = NULL, CRuntimeClass* pLineObj = NULL, 
						   CRuntimeClass* pAddrObj = NULL, CRuntimeClass* pCallObj = NULL, 
						   CRuntimeClass* pConfCall = NULL, CRuntimeClass* pPhoneObj = NULL);

    // This changes the timeout value used during WaitForRequest() when waiting for an
	// asynchronous request to complete.  Default timeout value is MAX_WAIT_TIMEOUT.
    void SetTimeout (LONG lTimeout);

    // This method begins the processing of the next asynchronous request block on our list.
    // It gets called when a request finishes, or when a new request is inserted and no pending
    // request is being processed.
    virtual void StartNextCommand(CTSPIConnection* pConn);

    // This method is called when a request is canceled and it has already
    // been started on the device.  It is called by the device object.
    virtual void OnCancelRequest (CTSPIRequest* pReq);

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
protected:    
	friend UINT IntervalTimerThread(LPVOID pParam);

    // Internal method to retrieve the default timeout used for the waiting of asynch requests.  
    LONG GetTimeout() const;

    // Internal timer function used by timer thread.
	BOOL IntervalTimer();

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

	// These are called to perform actions on the device class array by all
	// the difference objects (line,address,call,phone).
	int AddDeviceClassInfo (CObArray& arrElem, LPCTSTR pszName, DWORD dwType, LPVOID lpBuff, DWORD dwSize, HANDLE hHandle=INVALID_HANDLE_VALUE);
	BOOL RemoveDeviceClassInfo (CObArray& arrElem, LPCTSTR pszName);
	DEVICECLASSINFO* FindDeviceClassInfo (CObArray& arrElem, LPCTSTR pszName);
	LONG CopyDeviceClass (DEVICECLASSINFO* pDeviceClass, LPVARSTRING lpDeviceID, HANDLE hTargetProcess);

private:
    // Determine the service provider capabilities and setup our flag array.
    void DetermineProviderCapabilities();

    // This method walks all our device/connection lists and locates
    // a connection object for a variety of identifers.
    CTSPIConnection* SearchForConnInfo(DWORD dwId, WORD wReqType);

	// Used to delete registry tree during providerRemove when running under Windows NT.
	BOOL IntRegDeleteKey (HKEY hKeyTelephony, LPCTSTR pszMainDir);

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
	// id which has been assigned by TAPI.
	virtual LONG providerInit(DWORD dwTSPVersion, DWORD dwProviderId, DWORD dwLineIdBase,
                                 DWORD dwPhoneIdBase, DWORD dwNumLines, 
                                 DWORD dwNumPhones, ASYNC_COMPLETION lpfnCompletionProc,
								 LPDWORD lpdwTSPIOptions);

	// This method is called to shutdown our service provider.  It will
	// be called to shutdown a particular instance of our TSP.
	virtual LONG providerShutdown(DWORD dwTSPVersion, CTSPIDevice* pDevice);

	// This method is invoked when the user selects our ServiceProvider
	// icon in the control panel.  It should invoke the configuration dialog
	// which must be provided by the derived class.
	virtual LONG providerConfig(DWORD dwPPID, CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback);

	// This method is invoked when the TSP is to be installed via the
	// TAPI install code.  It should insure that all the correct files
	// are there, and write out the initial .INI settings.
	virtual LONG providerInstall(DWORD dwPermanentProviderID, CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback);

	// This method is invoked when the TSP is being removed from the
	// system.  It should remove all its files and .INI settings.
	virtual LONG providerRemove(DWORD dwPermanentProviderID, CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback);

	// This method is called when a UI dialog is terminating.
	virtual LONG providerFreeDialogInstance (HDRVDIALOGINSTANCE hdDlgInstance);

	// This method is called when the UI DLL sends information to the provider.
	virtual LONG providerGenericDialogData (CTSPIDevice* pDev, CTSPILineConnection* pLine, CTSPIPhoneConnection* pPhone, 
											HDRVDIALOGINSTANCE hdDlgInstance, LPVOID lpBuff, DWORD dwSize);

	// This method is called to identify the provider UI DLL
	virtual LONG providerUIIdentify (LPWSTR lpszUIDLLName);

	// This method is called for the UI DLL when the provider needs a dialog displayed.
	virtual LONG providerGenericDialog (HTAPIDIALOGINSTANCE htDlgInst, LPVOID lpParams, DWORD dwSize, HANDLE hEvent, TUISPIDLLCALLBACK lpfnDLLCallback);

	// This method is called for the UI DLL when the provider sends information.
	virtual LONG providerGenericDialogData (HTAPIDIALOGINSTANCE htDlgInst, LPVOID lpParams, DWORD dwSize);

    // This method is called when TAPI wishes to negotiate available
    // versions with us for any line device installed.  The low and high
    // version numbers passed are ones which the installed TAPI.DLL supports,
    // and we are expected to return the highest value which we can support
    // so TAPI knows what type of calls to make to us.
    virtual LONG lineNegotiateTSPIVersion(DWORD dwDeviceId, DWORD dwLowVersion,  DWORD dwHiVersion, LPDWORD lpdwTSPVersion);

   // This method is called to display the line configuration dialog
   // when the user requests it through either the TAPI api or the control
   // panel applet.
   virtual LONG lineConfigDialog(DWORD dwDeviceID, CWnd* pwndOwner, CString& strDeviceClass, TUISPIDLLCALLBACK lpfnDLLCallback);

   // This method is called to display the line configuration dialog with
   // a set of known parameters rather than the set currently in use.
   virtual LONG lineConfigDialogEdit(DWORD dwDeviceID, CWnd* pwndOwner, CString& strDeviceClass, const LPVOID lpDeviceConfigIn, DWORD dwSize, LPVARSTRING lpDeviceConfigOut, TUISPIDLLCALLBACK lpfnDLLCallback);

   // This method invokes the parameter configuration dialog for the
   // phone device.
   virtual LONG phoneConfigDialog(DWORD dwDeviceID, CWnd* pwndOwner, CString& strDevClass, TUISPIDLLCALLBACK lpfnDLLCallback);

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

   // This method is used to send dial digits to the device.
   virtual LONG lineDial(CTSPICallAppearance* pConn, DRV_REQUESTID dwReq, LPCTSTR lpszDestAddr, DWORD dwCountryCode);

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
   virtual LONG lineBlindTransfer(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId, LPCTSTR lpszDestAddr, DWORD dwCountryCode);

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
   virtual LONG linePark(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, DWORD dwParkMode, LPCTSTR lpszDirAddr, LPVARSTRING lpNonDirAddress);

   // This function picks up a call alerting at the specified destination
   // address and returns a call handle for the picked up call.  If invoked
   // with a NULL for the 'lpszDestAddr' parameter, a group pickup is performed.
   // If required by the device capabilities, 'lpszGroupID' specifies the
   // group ID to which the alerting station belongs.
   virtual LONG linePickup(CTSPIAddressInfo* pAddr, DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, LPCTSTR lpszDestAddr, LPCTSTR lpszGroupID);

   // This function prepares an existing conference call for the addition of
   // another party.  It creates a new temporary consultation call.  The new
   // consultation call can subsequently be added to the conference call.
   virtual LONG linePrepareAddToConference(CTSPIConferenceCall* pCall, DRV_REQUESTID dwRequestID, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall, LPLINECALLPARAMS lpCallParams);

   // This function redirects the specified offering call to the specified
   // destination address.
   virtual LONG lineRedirect(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, LPCTSTR lpszDestAddr, DWORD dwCountryCode);

   // This function forwards calls destined for the specified address
   // according to the specified forwarding instructions.
   // When an origination address is forwarded, the incoming calls for that
   // address are deflected to the other number by the switch.  This function
   // provides a combination of forward and do-not-disturb features.  This
   // function can also cancel specific forwarding currently in effect.
   virtual LONG lineForward(CTSPILineConnection* pLine, CTSPIAddressInfo* pAddr, DRV_REQUESTID dwRequestId, const LPLINEFORWARDLIST lpForwardList, DWORD dwNumRingsAnswer, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall,
                            const LPLINECALLPARAMS lpCallParams);

   // This function initiates the buffered gathering of digits on the 
   // specified call.  TAPI.DLL specifies a buffer in which to place the digits,
   // and the maximum number of digits to be collected.
   virtual LONG lineGatherDigits(CTSPICallAppearance* pCall, DWORD dwEndToEndID, DWORD dwDigitModes, LPWSTR lpszDigits, DWORD dwNumDigits, LPCTSTR lpszTerminationDigits, DWORD dwFirstDigitTimeout, DWORD dwInterDigitTimeout);

   // This method initiates the generation of the specified digits
   // using the specified signal mode.
   virtual LONG lineGenerateDigits(CTSPICallAppearance* pConn, DWORD dwEndToEndID, DWORD dwDigitMode, LPCTSTR lpszDigits, DWORD dwDuration);

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
   virtual LONG lineMonitorTones(CTSPICallAppearance* pCall, DWORD dwToneListID, const LPLINEMONITORTONE lpToneList, DWORD dwNumEntries);

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
   virtual LONG lineSetCallParams(CTSPICallAppearance *pCall, DRV_REQUESTID dwRequestID, DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate, const LPLINEDIALPARAMS lpDialParams);

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
   virtual LONG lineGetAddressID(CTSPILineConnection* pConn, LPDWORD lpdwAddressId, DWORD dwAddressMode, LPCTSTR lpszAddress, DWORD dwSize);

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
   virtual LONG lineSetDevConfig(CTSPILineConnection* pConn, const LPVOID lpDevConfig, DWORD dwSize, CString& strDevClass);

   // This method is used to release a block of UserUser information in a call
   // record.  It is new for TAPI 1.4
   virtual LONG lineReleaseUserUserInfo(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequest);

   // Set calldata into the calls CALLINFO record.
   virtual LONG lineSetCallData(CTSPICallAppearance* pCall,DRV_REQUESTID dwRequestId,LPVOID lpCallData,DWORD dwSize);

   // Negotiates a new QOS for the call.
   virtual LONG lineSetQualityOfService(CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestId, LPVOID lpSendingFlowSpec, DWORD dwSendingFlowSpecSize, LPVOID lpReceivingFlowSpec, DWORD dwReceivingFlowSpecSize);

   // The specified call treatment value is stored off in the
   // LINECALLINFO record and TAPI is notified of the change.  If
   // the call is currently in a state where the call treatment is
   // relevent, then it goes into effect immediately.  Otherwise,
   // the treatment will take effect the next time the call enters a
   // relevent state.
   virtual LONG lineSetCallTreatment (CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, DWORD dwCallTreatment);

   // The service provider will set the device status as indicated,
   // sending the appropriate LINE_LINEDEVSTATE messages to indicate
   // the new status.
   virtual LONG lineSetLineDevStatus (CTSPILineConnection* pLine, DRV_REQUESTID dwRequestID, DWORD dwStatusToChange, DWORD fStatus);

   // This function enables and disables control actions on the media stream
   // associated with the specified line, address, or call.  Media control actions
   // can be triggered by the detection of specified digits, media modes,
   // custom tones, and call states.  The new specified media controls replace all
   // the ones that were in effect for this line, address, or call prior to this
   // request.
   virtual LONG lineSetMediaControl(CTSPILineConnection *pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, 
                  const LPLINEMEDIACONTROLDIGIT lpDigitList, DWORD dwNumDigitEntries, 
                  const LPLINEMEDIACONTROLMEDIA lpMediaList, DWORD dwNumMediaEntries, 
                  const LPLINEMEDIACONTROLTONE lpToneList, DWORD dwNumToneEntries, 
                  const LPLINEMEDIACONTROLCALLSTATE lpCallStateList, DWORD dwNumCallStateEntries);

   // This operation enables TAPI.DLL to specify to which terminal information
   // related to a specified line, address, or call is to be routed.  This
   // can be used while calls are in progress on the line, to allow events
   // to be routed to different devices as required.
   virtual LONG lineSetTerminal(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, DWORD dwTerminalModes, DWORD dwTerminalID, BOOL bEnable);

   // This function sets up a conference call for the addition of a third 
   // party.
   virtual LONG lineSetupConference(CTSPILineConnection* pConn, CTSPICallAppearance* pCall, DRV_REQUESTID dwRequestID, HTAPICALL htConfCall,
            LPHDRVCALL lphdConfCall, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall, DWORD dwNumParties, 
            const LPLINECALLPARAMS lpLineCallParams);

   // This function is used to cancel the specified call completion request
   // on the specified line.
   virtual LONG lineUncompleteCall(CTSPILineConnection* pConn, DRV_REQUESTID dwRequestID, DWORD dwCompletionID);

   // This function retrieves the call parked at the specified
   // address and returns a call handle for it.
   virtual LONG lineUnpark(CTSPIAddressInfo* pAddr, DWORD dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, LPCTSTR lpszDestAddr);

   // This function returns the extension ID that the service provider
   // supports for the indicated line device.
   virtual LONG lineGetExtensionID(CTSPILineConnection* pConn, DWORD dwTSPIVersion, LPLINEEXTENSIONID lpExtensionID);

   // This method returns specific ID information about the specified
   // line, address, or call.
   virtual LONG lineGetID(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, CString& strDevClass, LPVARSTRING lpDeviceID, HANDLE hTargetProcess);

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
   virtual LONG lineMakeCall(CTSPILineConnection* pConn, DRV_REQUESTID dwRequestID, HTAPICALL htCall, LPHDRVCALL lphdCall, 
				LPCTSTR lpszDestAddr, DWORD dwCountryCode, const LPLINECALLPARAMS lpCallParams);

   // This method sets the application-specific data for the calls
   // LINEINFO structure when passed back to the user.
   virtual LONG lineSetAppSpecific(CTSPICallAppearance* pConn, DWORD dwAppSpecific);

   // This method is invoked by TAPI.DLL when the application requests a
   // line open using the LINEMAPPER.  This method will check the 
   // requested media modes and return an acknowledgement based on whether 
   // we can monitor all the requested modes.
   virtual LONG lineConditionalMediaDetection(CTSPILineConnection* pConn, DWORD dwMediaModes, const LPLINECALLPARAMS lpCallParams);

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
                     const LPLINECALLPARAMS lpCallParams);

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
   virtual LONG phoneGetID(CTSPIPhoneConnection* pConn, CString& strDevClass, LPVARSTRING lpDeviceId, HANDLE hTargetProcess);

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
   virtual LONG phoneSetButtonInfo(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestID, DWORD dwButtonId, const LPPHONEBUTTONINFO lpPhoneInfo);

   // This function downloads the information in the specified buffer
   // to the opened phone device at the selected data id.
   virtual LONG phoneSetData(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestId, DWORD dwDataId, LPCVOID lpData, DWORD dwSize);

   // This function causes the specified string to be displayed on the
   // phone device.
   virtual LONG phoneSetDisplay(CTSPIPhoneConnection* pConn, DRV_REQUESTID dwRequestID, DWORD dwRow, DWORD dwCol, LPCTSTR lpszDisplay, DWORD dwSize);

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
//
// CopyVarString
//
// Copy the buffer (either LPCTSTR or LPVOID) into the VARSTRING pointer.
//
/******************************************************************************/
void CopyVarString (LPVARSTRING lpVarString, LPCTSTR lpszBuff);
void CopyVarString (LPVARSTRING lpVarString, LPVOID lpBuff, DWORD dwSize);

/******************************************************************************/
// 
// ConvertWideToAnsi
//
// Convert a WIDE (unicode) string into a single-byte ANSI string.
//
/******************************************************************************/
#ifndef _UNICODE
CString ConvertWideToAnsi (LPCWSTR lpszInput);
#endif

/******************************************************************************/
// 
// AddDataBlock
// 
// Fill in a VARLEN buffer with data.
//
/******************************************************************************/
BOOL AddDataBlock (LPVOID lpVB, DWORD& dwOffset, DWORD& dwSize, 
				   LPCVOID lpBuffer, DWORD dwBufferSize);
BOOL AddDataBlock (LPVOID lpVB, DWORD& dwOffset, DWORD& dwSize, LPCSTR lpszBuff);
BOOL AddDataBlock (LPVOID lpVB, DWORD& dwOffset, DWORD& dwSize, LPCWSTR lpszBuff);

/******************************************************************************/
//
// DumpMem
//        
// Global DEBUG function to dump out a block of data in binary/ascii to
// the debug terminal.
//
/******************************************************************************/
#ifdef _DEBUG
void DumpMem (LPCTSTR lpszBuff, LPVOID lpBuff, DWORD dwSize);

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

#ifdef _NOINLINES_
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
void FreeMem (LPVOID lpBuff);

/******************************************************************************/
// CopyBuffer
//
// Global function to copy one buffer to another.
//
/******************************************************************************/
void CopyBuffer (LPVOID lpDest, LPCVOID lpSource, DWORD dwSize);

/******************************************************************************/
//
// FillBuffer
//
// Initialize a buffer with a known value.
//
/******************************************************************************/
void FillBuffer (LPVOID lpDest, BYTE bValue, DWORD dwSize);

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
// DwordAlignPtr
//
// Used for non-Intel machines to DWORD align addresses.
//
/******************************************************************************/
#ifndef _X86_
LPVOID DwordAlignPtr (LPVOID lpBuff)
#else
#define DwordAlignPtr(p) p
#endif

#else  // Using INLINE functions
#include "splib.inl"
#endif

#ifndef RC_INVOKED
#pragma pack()		// Revert to original packing
#endif 

#endif // _SPLIB_INC_
