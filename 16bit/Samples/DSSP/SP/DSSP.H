/*****************************************************************************/
//
// DSSP.H - Digital Switch Service Provider Sample
//                                                                        
// This file contains the main service provider override class.
// All driver related calls are run through this class.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
// is designed as a sample of how to implement a service provider using
// the TAPI C++ SP class library.  
//
// This source code is only intended as a supplement to the
// documentation provided with the TAPI C++ SP class library.
//
/*****************************************************************************/

#ifndef _DSSP_INC_
#define _DSSP_INC_

#include "emintf.h"

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

// Device specific functions
public:
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

// Initialization method
public:
	void InitializeLineDevice (const char * pszSwitchInfo);

// Overrides from CTSPILineConnection
public:
	virtual LONG SetDefaultMediaDetection (DWORD dwMediaModes);
	CDSDevice* GetDeviceInfo() { return (CDSDevice*) CTSPIConnection::GetDeviceInfo(); }

protected:
    virtual LONG GetDevConfig(CString& strDeviceClass, LPVARSTRING lpDeviceConfig);
    virtual LONG SetDevConfig(CString& strDeviceClass, LPVOID const lpDevConfig, DWORD dwSize);
    virtual BOOL ReceiveData (DWORD dwData=0, const LPVOID lpBuff=NULL, DWORD dwSize=0);
	virtual DWORD OnCallFeaturesChanged(CTSPICallAppearance* pCall, DWORD dwCallFeatures);

// Internal functions
private:
	void ProcessAsynchDeviceResponse(WORD wResult, const LPVOID lpData);
	void UpdateCallState (DWORD dwAddressID, int iNewState, int iStateInfo, DWORD dwMediaModes);
	void HandleNewCall (DWORD dwAddressID, DWORD dwMediaModes, DWORD dwCompletionID);
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

// Initialization method
public:
	void InitializePhoneDevice (const char * pszSwitchInfo);

// CTSPIPhoneConnection Overrides
protected:
    virtual BOOL ReceiveData (DWORD dwData, const LPVOID lpBuff, DWORD dwSize);
	CDSDevice* GetDeviceInfo() { return (CDSDevice*) CTSPIConnection::GetDeviceInfo(); }

// Internal methods
private:
	void ProcessAsynchDeviceResponse(WORD wResult, const LPVOID lpData);
};

/**************************************************************************
** CDSProvider
**
** Main service provider object
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
    virtual LONG providerInit(DWORD dwTSPVersion, DWORD dwProviderId, DWORD dwLineBase, DWORD dwPhoneBase, DWORD dwNumLines, DWORD dwNumPhones, ASYNC_COMPLETION lpfnCallback);
    virtual LONG lineGetID(CTSPILineConnection* pConn, CTSPIAddressInfo* pAddr, CTSPICallAppearance* pCall, CString& strDevClass, LPVARSTRING lpDeviceID);

    // UI methods
    virtual LONG lineConfigDialog(CTSPILineConnection* pLine, CWnd* pwndOwner, CString& strDeviceClass);
    virtual LONG phoneConfigDialog(CTSPIPhoneConnection* pPhone, CWnd* pwndOwner, CString& strDevClass);
    virtual LONG providerConfig(DWORD dwPPID, CWnd* pwndOwner);
    virtual LONG providerInstall(DWORD dwPermanentProviderID, CWnd* pwndOwner);

// Other overrides
public:
    virtual VOID UnknownDeviceNotify (WORD wCommand, DWORD dwConnId, DWORD dwResult, LPVOID lpvData, DWORD dwSize);

// Private methods
protected:
    void Perform16bitInit(DWORD dwProviderId);
};

#endif // _DSSP_INC_
