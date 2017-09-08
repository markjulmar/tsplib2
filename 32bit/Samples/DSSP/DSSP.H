/*****************************************************************************/
//
// DSSP.H - Digital Switch Service Provider Sample
//                                                                        
// This file contains the main service provider override class.
// All driver related calls are run through this class.
// 
// Copyright (C) 1997-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
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

#ifndef _DSSP_INC_
#define _DSSP_INC_

#include "emintf.h"

// New request types
#define REQUEST_WAITFORREQ	   (REQUEST_END + 1)

// Define the states supported by all the state machines
#define STATE_CHECKFORDIALTONE (STATE_INITIAL+1)
#define STATE_DIALING          (STATE_INITIAL+2)
#define STATE_WAITFORONLINE    (STATE_INITIAL+3)
#define STATE_WAITFORCONNECT   (STATE_INITIAL+4)
#define STATE_WAITFOROFFLINE   (STATE_INITIAL+5)
#define STATE_CHECKFORHOLD     (STATE_INITIAL+6)
#define STATE_CHECKLAMP        (STATE_INITIAL+7)
#define STATE_STARTDROP        (STATE_INITIAL+8)
#define STATE_WAITFORCONF      (STATE_INITIAL+9)
#define STATE_WAITFORCOMPLETE  (STATE_INITIAL+10)

/**************************************************************************
** CDSDevice
**
** Device object which manages a connection to the emulator.
**
***************************************************************************/
class CDSDevice : public CTSPIDevice
{
	DECLARE_DYNCREATE (CDSDevice)

// Class data
protected:
	CWinThread* volatile m_pThreadInput;	// Thread which processes asynch input from the emulator.
	BOOL volatile m_fConnected;				// TRUE if we are connected to the emulator

// Constructor/Destructor
public:
	CDSDevice();
	~CDSDevice();

// Callable functions
public:
	BOOL SendThreadRequest (UINT uCommand, LPVOID pData, DWORD dwSize);
	BOOL SendThreadRequest (UINT uCommand, DWORD dwData);
	BOOL WaitForResponse(DWORD dwRequest, LPVOID lpSend, DWORD dwSendSize, LPVOID lpBuff, DWORD dwSize);

// Device specific functions
public:
	BOOL ConnectServer();
	BOOL DRV_Connect();
    BOOL DRV_GetSwitchSettings (LPEMSETTINGS lpSettings);
    BOOL DRV_GetAddressInfo (LPEMADDRESSINFO lpAddressInfo);
    BOOL DRV_PrepareCall (DWORD dwAddressID);
    BOOL DRV_DropCall (DWORD dwAddressID);
    BOOL DRV_Dial (DWORD dwAddressID, DIALINFO* pDialInfo, DWORD dwCountryCode);
    BOOL DRV_AnswerCall(DWORD dwAddressID);
    BOOL DRV_GetVersionInfo (LPEMVERSIONINFO lpVerInfo);
    BOOL DRV_HoldCall (DWORD dwAddressID);
    BOOL DRV_Flash (DWORD dwAddressID);
    BOOL DRV_UnholdCall (DWORD dwAddressID);
    BOOL DRV_Transfer (DWORD dwAddressID, LPCSTR lpszAddr, DWORD dwAddressToID=-1L, BOOL fConf=FALSE);
    BOOL DRV_Forward (DWORD dwAddressID, LPCSTR lpszAddr);
    BOOL DRV_SetGain (DWORD dwHSDevice, DWORD dwGain);
    BOOL DRV_SetVolume (DWORD dwHSDevice, DWORD dwGain);
    BOOL DRV_SetHookswitch (DWORD dwHSDevice, DWORD dwGain);
    BOOL DRV_Conference(DWORD dwAddressID, DWORD dwAddressID2, WORD wCommand);
    BOOL DRV_Redirect (DWORD dwAddressID, LPCSTR lpszAddr, DWORD dwCountryCode);
    BOOL DRV_SetRing (DWORD dwRingMode);
    BOOL DRV_CompleteCall (DWORD dwAddressID, DWORD dwComplType, LPCSTR pszMsg);
    BOOL DRV_Park (DWORD dwAddressID, LPCSTR pszAddr);
    BOOL DRV_Unpark (DWORD dwAddressID, LPCSTR pszAddr);

// Overriden functions from CTSPIDevice
public:
	virtual BOOL OpenDevice (CTSPIConnection* pConn);
protected:
	friend class CSocketMgr;
    virtual VOID Init(DWORD dwProviderId, DWORD dwBaseLine, DWORD dwBasePhone, DWORD dwLines, DWORD dwPhones, HPROVIDER hProvider, ASYNC_COMPLETION lpfnCompletion);
};

/**************************************************************************
** CDSLine
**
** Line object which manages a single line on the switch.
**
***************************************************************************/
class CDSLine : public CTSPILineConnection
{
	DECLARE_DYNCREATE (CDSLine)

// Constructor/Destructor
public:
    CDSLine();
	virtual ~CDSLine();

// Overrides from CTSPILineConnection
public:
	CDSDevice* GetDeviceInfo() { return (CDSDevice*) CTSPIConnection::GetDeviceInfo(); }

protected:
	virtual VOID Init(CTSPIDevice* pDev, DWORD dwLineDeviceID, DWORD dwPos, DWORD dwItemData);
    virtual LONG GetDevConfig(CString& strDeviceClass, LPVARSTRING lpDeviceConfig);
    virtual LONG SetDevConfig(CString& strDeviceClass, LPVOID const lpDevConfig, DWORD dwSize);
    virtual BOOL ReceiveData (DWORD dwData=0, const LPVOID lpBuff=NULL, DWORD dwSize=0);
	virtual LONG SetDefaultMediaDetection (DWORD dwMediaModes);
	virtual DWORD OnCallFeaturesChanged (CTSPICallAppearance* pCall, DWORD dwFeatures);
	virtual DWORD OnAddressFeaturesChanged (CTSPIAddressInfo* pAddr, DWORD dwFeatures);
	virtual DWORD OnLineFeaturesChanged(DWORD dwFeatures);

// Internal functions
private:
	void ProcessAsynchDeviceResponse(WORD wResult, const LPVOID lpData);
	void UpdateCallState (DWORD dwAddressID, int iNewState, int iStateInfo, DWORD dwMediaModes);
	void HandleNewCall (DWORD dwAddressID, DWORD dwMediaModes, DWORD dwCompletionID, BOOL fExternal);
	BOOL HandleDialEvent (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff, CADObArray* parrAddress, DWORD dwCountryCode);

// TAPI function handlers
protected:
    BOOL processAnswer(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processBlindXfer(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processCompleteCall(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processMakeCall(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processDial(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processDropCall(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processGenDigits (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processForward (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processHold (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processUnhold (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processSwapHold (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processSetupXfer (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processCompleteXfer (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processSetupConf(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processAddConf(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processRemoveConf(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processDropConference(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processRedirect(CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processPark (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processUnpark (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
    BOOL processPickup (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff);
	BOOL processWaitReq (CTSPIRequest* pReq, WORD wResponse, const LPVOID lpBuff, DWORD dwSize);
};

/**************************************************************************
** CDSPhone
**
** Phone object which manages a single phone on the switch.
**
***************************************************************************/
class CDSPhone : public CTSPIPhoneConnection
{
	DECLARE_DYNCREATE (CDSPhone)

// Constructor/Destructor
public:
    CDSPhone();
	virtual ~CDSPhone();

// CTSPIPhoneConnection Overrides
protected:
	virtual VOID Init(CTSPIDevice* pDev, DWORD dwPhoneDeviceID, DWORD dwPos, DWORD dwItemData);
    virtual BOOL ReceiveData (DWORD dwData=0, const LPVOID lpBuff=NULL, DWORD dwSize=0);
	CDSDevice* GetDeviceInfo() { return (CDSDevice*) CTSPIConnection::GetDeviceInfo(); }

// Internal methods
private:
	void ProcessAsynchDeviceResponse(WORD wResult, const LPVOID lpData);
};

/**************************************************************************
** CDSProvider
**
** Main provider object which manages the lifetime of the TSP
**
***************************************************************************/
class CDSProvider : public CServiceProvider
{
// Constructor
public:
    CDSProvider();

// TSPI Methods
public:
    virtual LONG providerEnumDevices(DWORD dwProviderId, LPDWORD lpNumLines, LPDWORD lpNumPhones, HPROVIDER hProvider, LINEEVENT lpfnLineCreateProc, PHONEEVENT lpfnPhoneCreateProc);
	virtual LONG providerInit(DWORD dwTSPVersion, DWORD dwProviderId, DWORD dwLineIdBase, DWORD dwPhoneIdBase, DWORD dwNumLines, DWORD dwNumPhones, ASYNC_COMPLETION lpfnCompletionProc, LPDWORD lpdwTSPIOptions);

	// Configuration overrides
    virtual LONG providerInstall(DWORD dwPermanentProviderID, CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback);
	virtual LONG providerConfig(DWORD dwPPID, CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback);
	virtual LONG lineConfigDialog(DWORD dwDeviceID, CWnd* pwndOwner, CString& strDeviceClass, TUISPIDLLCALLBACK lpfnDLLCallback);
	virtual LONG lineConfigDialogEdit(DWORD dwDeviceID, CWnd* pwndOwner, CString& strDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize, LPVARSTRING lpDeviceConfig, TUISPIDLLCALLBACK lpfnDLLCallback);
};

////////////////////////////////////////////////////////////////////////////
// Inline functions for CDSDevice

inline BOOL CDSDevice::DRV_GetVersionInfo (LPEMVERSIONINFO lpVerInfo)
{
	return WaitForResponse(EMCOMMAND_GETVERSION, NULL, 0, lpVerInfo, sizeof(EMVERSIONINFO));

}// CDSDevice::DRV_GetVersionInfo

inline BOOL CDSDevice::DRV_GetSwitchSettings (LPEMSETTINGS lpSettings)
{
	return WaitForResponse(EMCOMMAND_QUERYCAPS, NULL, 0, lpSettings, sizeof(EMSETTINGS));

}// CDSDevice::DRV_GetSwitchSettings

inline BOOL CDSDevice::DRV_GetAddressInfo (LPEMADDRESSINFO lpAddressInfo)
{
	WORD wAddr = lpAddressInfo->wAddressID;
	return WaitForResponse(EMCOMMAND_GETADDRESSINFO, &wAddr, sizeof(WORD), lpAddressInfo, sizeof(EMADDRESSINFO));
    
}// CDSDevice::DRV_GetAddressInfo

inline BOOL CDSDevice::DRV_PrepareCall (DWORD dwAddressID)
{
    return SendThreadRequest (EMCOMMAND_PREPAREADDR, dwAddressID);

}// CDSDevice::DRV_PrepareCall

inline BOOL CDSDevice::DRV_DropCall (DWORD dwAddressID)
{
    return SendThreadRequest (EMCOMMAND_DROPCALL, dwAddressID);

}// CDSDevice::DRV_DropCall

inline BOOL CDSDevice::DRV_AnswerCall(DWORD dwAddressID)
{                              
    return SendThreadRequest (EMCOMMAND_ANSWER, dwAddressID);

}// CDSDevice::DRV_AnswerCall

inline BOOL CDSDevice::DRV_HoldCall (DWORD dwAddressID)
{                            
    return SendThreadRequest (EMCOMMAND_HOLDCALL, dwAddressID);
    
}// CDSDevice::DRV_HoldCall 

inline BOOL CDSDevice::DRV_UnholdCall (DWORD dwAddressID)
{                            
    return SendThreadRequest (EMCOMMAND_UNHOLDCALL, dwAddressID);
    
}// CDSDevice::DRV_HoldCall

inline BOOL CDSDevice::DRV_Flash (DWORD dwAddressID)
{                         
    return SendThreadRequest (EMCOMMAND_FLASH, dwAddressID);

}// CDSDevice::DRV_Flash

inline BOOL CDSDevice::DRV_SetRing (DWORD dwRingMode)
{                           
    return SendThreadRequest (EMCOMMAND_SETRINGMODE, dwRingMode);
    
}// CDSDevice::DRV_SetRing


#endif // _DSSP_INC_
