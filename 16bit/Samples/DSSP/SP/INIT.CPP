/*****************************************************************************/
//
// INIT.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the initialization code for the service provider.
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
// CDSProvider::providerInit
//
// This method is called when the service provider is being initialized.
// It loads the line variables into our class specific data.
//
LONG CDSProvider::providerInit(DWORD dwTSPVersion, DWORD dwProviderId, DWORD dwLineBase, 
                         DWORD dwPhoneBase, DWORD dwNumLines, DWORD dwNumPhones, 
                         ASYNC_COMPLETION lpfnCallback)
{
    // Call the base class for initialization
    LONG lResult = CServiceProvider::providerInit(dwTSPVersion, dwProviderId, dwLineBase, dwPhoneBase, dwNumLines, dwNumPhones, lpfnCallback);
    if (lResult < 0)
        return lResult;

    // Wait for our companion application to start.
    DWORD dwCurrTime = GetTickCount();
    while (m_pMainWnd == NULL && dwCurrTime+5000L > GetTickCount())
        Yield();
    
    // If we failed to connect to the emulator, then exit.
    if (m_pMainWnd == NULL)
        return LINEERR_NODRIVER;

    // Make sure our .INI file is correct.
    ASSERT(dwNumLines == 1);
    ASSERT(dwNumPhones == 1);

    // Do the rest of the initialization on the companion application thread - 
    // if this was called as a result of the device being added WHILE TAPI WAS RUNNING,
    // then there is a chance that we are on a 32-bit context right now.
    //
    // Since our communication method is to send messages back and forth, and we
    // pass structures which are on the stack of the calling application thread,
    // this can cause a crash if we don't make sure to be on a 16-bit context.
    //
    // This will call into the companion application and then call right back to us
    // on the "UnknownDeviceNotify" function.
    //
    SendThreadRequest (COMMAND_PERFORMINIT, dwProviderId);
    return FALSE;

}// CDSProvider::providerInit

////////////////////////////////////////////////////////////////////////////
// CDSProvider::UnknownDeviceNotify
//
// This is overriden to manage the new INIT command which is
// called in the context of our 16-bit companion application.
//
VOID CDSProvider::UnknownDeviceNotify (WORD wCommand, DWORD dwConnId, 
                    DWORD /*dwResult*/, LPVOID /*lpvData*/, DWORD /*dwSize*/)
{                         
    if (wCommand == RESULT_PERFORMINIT)
        Perform16bitInit(dwConnId);

}// CDSProvider::UnknownDeviceNotify
   
////////////////////////////////////////////////////////////////////////////
// CDSProvider::Perform16bitInit
//
// This gets called in response to the companion application calling us
// back for initialization (COMMAND_PERFORMINIT).  It is setup this way 
// because the lineAddprovider32 causes us to get called in the context 
// of the 32-bit application which invoked the API.
//
void CDSProvider::Perform16bitInit(DWORD dwProviderId)
{                                
    // This is our device object representing this particular provider id.
    CDSDevice* pDevice = (CDSDevice*) GetDevice(dwProviderId);

    // Grab the version information from the emulator.
    EMVERSIONINFO VerInfo;
    pDevice->DRV_GetVersionInfo(&VerInfo);

    // Initialize our phone device first so the line will see it.
    CDSPhone* pPhone = (CDSPhone*) pDevice->GetPhoneConnectionInfo(0);
    ASSERT (pPhone != NULL);      
    pPhone->InitializePhoneDevice(VerInfo.szSwitchInfo);
    
    // Initialize our line device
    CDSLine* pLine = (CDSLine*) pDevice->GetLineConnectionInfo(0);
    ASSERT (pLine != NULL);      
    pLine->InitializeLineDevice(VerInfo.szSwitchInfo);
    
    // Cancel any forwarding in effect - we cannot DETECT where it is
    // forwarded to, so we will initialize it to something we know and therefore
    // be able to correctly report forwarding status information
    pDevice->DRV_Forward(-1, NULL);

}// CDSProvider::Perform16bitInit

