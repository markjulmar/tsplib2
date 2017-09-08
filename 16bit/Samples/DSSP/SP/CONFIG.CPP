/*****************************************************************************/
//
// CONFIG.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the configuration support code.
// 
// Copyright (C) 1994-1997 JulMar Entertainment Technology, Inc.// All rights reserved                                                    
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

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CDSProvider::lineConfigDialog
//
// Invoked when the user uses TAPI to invoke our configuration dialog
// vs. the control panel (providerConfig).
//
LONG CDSProvider::lineConfigDialog(CTSPILineConnection* /*pLine*/, CWnd* pwndOwner, 
                                   CString& /*strDeviceClass*/)
{
    return providerConfig (0, pwndOwner);

}// CDSProvider::lineConfigDialog

/////////////////////////////////////////////////////////////////////////////
// CDSProvider::phoneConfigDialog
//
// This invokes the phone configuration portion of the setup process.
//
LONG CDSProvider::phoneConfigDialog(CTSPIPhoneConnection* /*pPhone*/, CWnd* pwndOwner, 
                              CString& /*strDevClass*/)
{
    return providerConfig (0, pwndOwner);
    
}// CDSProvider::phoneConfigDialog

///////////////////////////////////////////////////////////////////////////
// CDSProvider::providerConfig
//
// Invoke the configuration dialog (General panel)
//
LONG CDSProvider::providerConfig(DWORD /*dwPPID*/, CWnd* pwndOwner)
{   
    MessageBox (pwndOwner->GetSafeHwnd(), "You must configure the DSSP sample through the emulator.",
                "DSSP Sample", MB_OK);
    return FALSE;

}// CDSProvider::providerConfig

///////////////////////////////////////////////////////////////////////////
// CDSProvider::providerInstall
//
// Install the service provider onto the system - do not allow multiple
// instances of the provider to exist.
//
LONG CDSProvider::providerInstall(DWORD dwPermanentProviderID, CWnd* /*pwndOwner*/)
{                               
    // Grab the section from the telephon.ini which details provider information.
    int iProviderCount = GetProfileInt ("Providers", "NumProviders", 0);
    for (int iCount = 0; iCount < iProviderCount; iCount++)
    {
        char szBuff[50];
        wsprintf (szBuff, "ProviderFilename%d", iCount);
        CString strBuff = CWinApp::GetProfileString ("Providers", szBuff);
        if (!strBuff.IsEmpty())
        {
            if (strBuff.CompareNoCase ("DSSP.TSP") == 0)
                return LINEERR_NOMULTIPLEINSTANCE;
        }
    }                                             
    
    // Force our line/phone count out to the .INI file.  This is not required anymore
    // since we have a providerEnumDevices entry.
	char szBuff[30];
	wsprintf (szBuff, "Provider%ld", dwPermanentProviderID);
    CWinApp::WriteProfileString (szBuff, "NumLines", "1");
    CWinApp::WriteProfileString (szBuff, "NumPhones", "1");
    
    return FALSE;

}// CDSProvider::providerInstall
