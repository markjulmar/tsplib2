/*****************************************************************************/
//
// DRV.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the connection methods for driving the emulator.
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

#include "stdafx.h"
#include "dssp.h"
#include "dsspint.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_GetVersionInfo
//
// Return the version information from the emulator
//
BOOL CDSDevice::DRV_GetVersionInfo (LPEMVERSIONINFO lpVerInfo)
{
    // WARNING: This is really a bad usage of SendThreadRequest - we are passing a pointer
    // to the companion application which will pass through to the emulator.  This means
    // that all three need to be 16-bit (which is fine for this sample).  The thunking
    // mechanism implemented by SendThreadMessage for pointers (i.e. 16:16 ptr in lParam)
    // is not designed for R/W pointers - it will COPY the data into the address space of
    // the receiver, but not copy changes made to the data back.
    return GetSP()->SendThreadRequest (COMMAND_GETVERSIONINFO, (LPARAM) lpVerInfo);

}// CDSDevice::DRV_GetVersionInfo

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_GetSwitchSettings
//
// Ask the emulator for our switch settings (configuration)
//
BOOL CDSDevice::DRV_GetSwitchSettings (LPEMSETTINGS lpSettings)
{
    // WARNING: This is really a bad usage of SendThreadRequest - we are passing a pointer
    // to the companion application which will pass through to the emulator.  This means
    // that all three need to be 16-bit (which is fine for this sample).  The thunking
    // mechanism implemented by SendThreadMessage for pointers (i.e. 16:16 ptr in lParam)
    // is not designed for R/W pointers - it will COPY the data into the address space of
    // the receiver, but not copy changes made to the data back.
    //
    // If the companion is 32-bit, this technique will NOT work, instead, the companion
    // must send the data back through a user-defined response which will thunk pointers
    // properly.
    return GetSP()->SendThreadRequest (COMMAND_GETEMULATORSETTINGS, (LPARAM) lpSettings);

}// CDSDevice::DRV_GetSwitchSettings

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_GetAddressInfo
//
// Ask the emulator for the address information
//
BOOL CDSDevice::DRV_GetAddressInfo (LPEMADDRESSINFO lpAddressInfo)
{
    // WARNING: This is really a bad usage of SendThreadRequest - we are passing a pointer
    // to the companion application which will pass through to the emulator.  This means
    // that all three need to be 16-bit (which is fine for this sample).  The thunking
    // mechanism implemented by SendThreadMessage for pointers (i.e. 16:16 ptr in lParam)
    // is not designed for R/W pointers - it will COPY the data into the address space of
    // the receiver, but not copy changes made to the data back.
    //
    // If the companion is 32-bit, this technique will NOT work, instead, the companion
    // must send the data back through a user-defined response which will thunk pointers
    // properly.
    return GetSP()->SendThreadRequest (COMMAND_GETEMULATORADDRESSINFO, (LPARAM) lpAddressInfo);
    
}// CDSDevice::DRV_GetAddressInfo

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_PrepareCall
//
// Prepare an address for an outgoing call.
//
BOOL CDSDevice::DRV_PrepareCall (DWORD dwAddressID)
{
    return GetSP()->SendThreadRequest (COMMAND_EMULATORPREPARECALL, (LPARAM) dwAddressID);

}// CDSDevice::DRV_PrepareCall

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_DropCall
//
// Drop an existing connected/busy call
//
BOOL CDSDevice::DRV_DropCall (DWORD dwAddressID)
{
    return GetSP()->SendThreadRequest (COMMAND_EMULATORDROPCALL, (LPARAM) dwAddressID);

}// CDSDevice::DRV_DropCall

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Dial
//
// Dial on a "prepared" or connected call.
//
BOOL CDSDevice::DRV_Dial (DWORD dwAddressID, DIALINFO* pDialInfo, DWORD /*dwCountryCode*/)
{                          
    EMADDRESSINFO AddressInfo;
    AddressInfo.wAddressID = (WORD) dwAddressID;
    strncpy ((LPSTR)&AddressInfo.szAddress, (LPCSTR) pDialInfo->strNumber, ADDRESS_SIZE);
    AddressInfo.wAddressState = 0;
    AddressInfo.dwMediaMode = 0;
    return GetSP()->SendThreadRequest (COMMAND_EMULATORDIAL, (LPARAM)&AddressInfo, TRUE, sizeof(EMADDRESSINFO));

}// CDSDevice::DRV_Dial

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_AnswerCall
//
// This asks the emulator to answer a specific address id
//
BOOL CDSDevice::DRV_AnswerCall(DWORD dwAddressID)
{                              
    return GetSP()->SendThreadRequest (COMMAND_EMULATORANSWERCALL, (LPARAM) dwAddressID);

}// CDSDevice::DRV_AnswerCall

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_HoldCall
//
// Ask the emulator to place a call on Hold.
//
BOOL CDSDevice::DRV_HoldCall (DWORD dwAddressID)
{                            
    return GetSP()->SendThreadRequest (COMMAND_EMULATORHOLDCALL, (LPARAM) dwAddressID);
    
}// CDSDevice::DRV_HoldCall 

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_UnholdCall
//
// Ask the emulator to take a call off Hold.
//
BOOL CDSDevice::DRV_UnholdCall (DWORD dwAddressID)
{                            
    return GetSP()->SendThreadRequest (COMMAND_EMULATORUNHOLDCALL, (LPARAM) dwAddressID);
    
}// CDSDevice::DRV_HoldCall

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Transfer
//
// Ask the emulator to perform a portion of a transfer request
//
BOOL CDSDevice::DRV_Transfer (DWORD dwAddressID, LPCSTR lpszAddr, DWORD dwAddressToID,
                                BOOL fMoveToConf)
{                            
    EMTRANSFERINFO TranInfo;
    TranInfo.wAddressID = (WORD) dwAddressID;
    TranInfo.wTransferAddress = (WORD) dwAddressToID;
    TranInfo.fMoveToConference = (WORD)fMoveToConf;
    
    if (lpszAddr != NULL)
        strncpy (TranInfo.szAddress, lpszAddr, ADDRESS_SIZE);
    else
        TranInfo.szAddress[0] = '\0';
    return GetSP()->SendThreadRequest (COMMAND_EMULATORTRANSFER, (LPARAM)&TranInfo, TRUE, sizeof(EMTRANSFERINFO));

}// CDSDevice::DRV_Transfer

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Forward
//
// Ask the emulator to perform a portion of a forward request
//
BOOL CDSDevice::DRV_Forward (DWORD dwAddressID, LPCSTR lpszAddr)
{                            
    EMFORWARDINFO ForwInfo;
    ForwInfo.wAddressID = (WORD) dwAddressID;
    if (lpszAddr != NULL) // Unforward request?
        strncpy (ForwInfo.szAddress, lpszAddr, ADDRESS_SIZE);
    else
        ForwInfo.szAddress[0] = '\0';
    return GetSP()->SendThreadRequest (COMMAND_EMULATORFORWARD, (LPARAM)&ForwInfo, TRUE, sizeof(EMFORWARDINFO));

}// CDSDevice::DRV_Transfer

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Flash
//
// Signal an address with a hookflash event
//
BOOL CDSDevice::DRV_Flash (DWORD dwAddressID)
{                         
    return GetSP()->SendThreadRequest (COMMAND_EMULATORFLASH, (LPARAM) dwAddressID);

}// CDSDevice::DRV_Flash

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_SetGain
//
// Set the gain of the microphone
//
BOOL CDSDevice::DRV_SetGain (DWORD /*dwHookSwitch*/, DWORD dwLevel)
{                           
    EMLEVELCHANGE emChange;
    emChange.wLevelType = LEVELTYPE_MIC;
    emChange.wLevel = (WORD) dwLevel;
    return GetSP()->SendThreadRequest (COMMAND_SETGAIN, (LPARAM)&emChange, TRUE, sizeof(EMLEVELCHANGE));
    
}// CDSDevice::DRV_SetGain

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_SetVolume
//
// Set the volume of the speaker
//
BOOL CDSDevice::DRV_SetVolume (DWORD /*dwHookSwitch*/, DWORD dwLevel)
{                           
    EMLEVELCHANGE emChange;
    emChange.wLevelType = LEVELTYPE_SPEAKER;
    emChange.wLevel = (WORD) dwLevel;
    return GetSP()->SendThreadRequest (COMMAND_SETVOL, (LPARAM)&emChange, TRUE, sizeof(EMLEVELCHANGE));
    
}// CDSDevice::DRV_SetVolume

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_SetHookswitch
//
// Set the state of the hookswitch
//
BOOL CDSDevice::DRV_SetHookswitch (DWORD /*dwHookSwitch*/, DWORD dwType)
{   
    EMHOOKSWITCHCHANGE emChange;
    emChange.wHookswitchID = HSDEVICE_HANDSET;
    switch (dwType)
    {
        case PHONEHOOKSWITCHMODE_ONHOOK: 
            emChange.wHookswitchState = HSSTATE_ONHOOK; 
            break;
        case PHONEHOOKSWITCHMODE_MICSPEAKER:
            emChange.wHookswitchState = HSSTATE_OFFHOOKMICSPEAKER;
            break;
        case PHONEHOOKSWITCHMODE_MIC:
            emChange.wHookswitchState = HSSTATE_OFFHOOKMIC;
            break;
        case PHONEHOOKSWITCHMODE_SPEAKER:
            emChange.wHookswitchState = HSSTATE_OFFHOOKSPEAKER;
            break;
        default:
            ASSERT (FALSE);
            emChange.wHookswitchState = HSSTATE_OFFHOOKMICSPEAKER;
            break;
    }            
                            
    return GetSP()->SendThreadRequest (COMMAND_SETHOOKSWITCH, (LPARAM)&emChange, TRUE, sizeof(EMHOOKSWITCHCHANGE));
    
}// CDSDevice::DRV_SetGain

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Conference
//
// Send a conference command
//
BOOL CDSDevice::DRV_Conference(DWORD dwAddressID, DWORD dwAddressID2, WORD wCommand)
{
    EMCONFERENCEINFO ConfInfo;
    ConfInfo.wAddressID = (WORD) dwAddressID;
    ConfInfo.wConfAddress = (WORD) dwAddressID2;
    ConfInfo.wCommand = wCommand;
    return GetSP()->SendThreadRequest (COMMAND_EMULATORCONFERENCE, (LPARAM)&ConfInfo, TRUE, sizeof(EMCONFERENCEINFO));

}// CDSDevice::DRV_Conference

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Redirect
//
// Redirect the call to another address
//
BOOL CDSDevice::DRV_Redirect (DWORD dwAddressID, LPCSTR lpszAddress, DWORD /*dwCountryCode*/)
{
    EMFORWARDINFO RedirectInfo;
    RedirectInfo.wAddressID = (WORD) dwAddressID;
    ASSERT (lpszAddress != NULL);
    strncpy (RedirectInfo.szAddress, lpszAddress, ADDRESS_SIZE);
    return GetSP()->SendThreadRequest (COMMAND_EMULATORREDIRECT, (LPARAM)&RedirectInfo, TRUE, sizeof(EMFORWARDINFO));

}// CDSDevice::DRV_Redirect

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_SetRing
//
// Set the ringmode of the phone device
//
BOOL CDSDevice::DRV_SetRing (DWORD dwRingMode)
{                           
    return GetSP()->SendThreadRequest (COMMAND_SETRINGMODE, (LPARAM)dwRingMode);
    
}// CDSDevice::DRV_SetRing

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_CompleteCall
//
// Send a call completion request to the switch emulator.
//
BOOL CDSDevice::DRV_CompleteCall (DWORD dwAddressID, DWORD dwComplType, const char * pszMsg)
{                           
    EMCOMPLETECALL CCall;
    CCall.wAddressID = (WORD) dwAddressID;
    switch (dwComplType)
    {
        case LINECALLCOMPLMODE_CAMPON:     
            CCall.wCompletionType = CALLCOMPLTYPE_CAMP; 
            break;
        case LINECALLCOMPLMODE_CALLBACK:   
            CCall.wCompletionType = CALLCOMPLTYPE_CALLBACK; 
            break;
        case LINECALLCOMPLMODE_INTRUDE:    
            CCall.wCompletionType = CALLCOMPLTYPE_INTRUDE; 
            break;
        case LINECALLCOMPLMODE_MESSAGE:    
            CCall.wCompletionType = CALLCOMPLTYPE_MESSAGE; 
            break;
        default: 
            ASSERT (FALSE);
            break;          
    }
    
    // Copy the message into the structure.
    if (pszMsg != NULL)
        strncpy (CCall.szMessage, pszMsg, COMPLMESSAGE_SIZE);
    else
        CCall.szMessage[0] = '\0';
    return GetSP()->SendThreadRequest (COMMAND_COMPLETECALL, (LPARAM)&CCall, TRUE, sizeof(EMCOMPLETECALL));
    
}// CDSDevice::DRV_CompleteCall

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Park
//
// Park the specified address into a specified address
//
BOOL CDSDevice::DRV_Park (DWORD dwAddressID, const char * pszAddr)
{                                                  
    EMPARKINFO ParkInfo;
    ParkInfo.wAddressID = (WORD) dwAddressID;
    ASSERT (pszAddr != NULL);
    strncpy (ParkInfo.szAddress, pszAddr, ADDRESS_SIZE);
    return GetSP()->SendThreadRequest (COMMAND_PARKCALL, (LPARAM)&ParkInfo, TRUE, sizeof(EMPARKINFO));
    
}// CDSDevice::DRV_Park

////////////////////////////////////////////////////////////////////////////
// CDSDevice::DRV_Unpark
//
// Unpark the specified address into a specified address
//
BOOL CDSDevice::DRV_Unpark (DWORD dwAddressID, const char * pszAddr)
{                                                  
    EMPARKINFO ParkInfo;
    ParkInfo.wAddressID = (WORD) dwAddressID;
    ASSERT (pszAddr != NULL);
    strncpy (ParkInfo.szAddress, pszAddr, ADDRESS_SIZE);
    return GetSP()->SendThreadRequest (COMMAND_UNPARKCALL, (LPARAM)&ParkInfo, TRUE, sizeof(EMPARKINFO));
    
}// CDSDevice::DRV_Unpark
