/*****************************************************************************/
//
// INIT.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the initialization code for the service provider.
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

#include "stdafx.h"
#include "dssp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/*****************************************************************************
** Procedure:  CDSProvider::providerEnumDevices
**
** Arguments:  'dwProviderId'	-	Passed provider ID from TAPI
**             'lpNumLines'		-	Returning count of lines
**             'lpNumPhones'	-	Returning count of phones
**			   'hProvider'		-	Handle to the provider from TAPI
**			   'lpfnLineC..'	-	Line creation PROC for LINE_CREATE
**			   'lpfnPhoneC..'	-	Phone creation PROC for PHONE_CREATE	
**
** Returns:    TAPI result code
**
** Description:  This is called after the constructor to determine how many
**				 lines and phones the provider is managing.  This is called
**				 BEFORE providerInit.
**
**				 It is a required function under TAPI 2.x
**
*****************************************************************************/
LONG CDSProvider::providerEnumDevices(DWORD dwProviderId, LPDWORD lpNumLines,
                                 LPDWORD lpNumPhones, HPROVIDER hProvider,
                                 LINEEVENT lpfnLineCreateProc, 
                                 PHONEEVENT lpfnPhoneCreateProc)
{   
	// Pass it onto the base class and let the library know
	// about the provider handle.  This is required for 
	// dynamic UI dialog support.
	LONG lResult = CServiceProvider::providerEnumDevices(dwProviderId,
								lpNumLines, lpNumPhones, hProvider,
								lpfnLineCreateProc, lpfnPhoneCreateProc);

	// Enforce the line/phone device counts.
    *lpNumLines	 = 1;
    *lpNumPhones = 1;

    return lResult;

}// CDSProvider::providerEnumDevices

/*****************************************************************************
** Procedure:  CDSProvider::providerInit
**
** Arguments:  'dwTSPVersion'	-	Version to initialize at
**             'dwProviderId'	-	Provider ID
**             'dwLineIdBase'	-   Line ID base
**             'dwPhoneIdBase'	-	Phone ID base
**			   'dwNumLines'		-   Number of lines
**             'dwNumPhones'	-	Number of phones
**             'lpfnCompletion' -   Completion proc callback
**             'lpdwTSPOption'  -   TSP options
**
** Returns:    TAPI result code
**
** Description:  This is called after the constructor to initialize the provider.
**
*****************************************************************************/
LONG CDSProvider::providerInit(DWORD dwTSPVersion, DWORD dwProviderId, DWORD dwLineIdBase,
                                 DWORD dwPhoneIdBase, DWORD dwNumLines, 
                                 DWORD dwNumPhones, ASYNC_COMPLETION lpfnCompletionProc,
								 LPDWORD lpdwTSPIOptions)
{
	// Pass it onto the base clase
	LONG lResult = CServiceProvider::providerInit(dwTSPVersion, dwProviderId, dwLineIdBase,
								dwPhoneIdBase, dwNumLines, dwNumPhones, lpfnCompletionProc,
								lpdwTSPIOptions);
	// If the device failed to connect to the provider, fail the init.
	CTSPIDevice* pDevice = GetDevice(dwProviderId);
	if (pDevice->GetLineCount() != (int) dwNumLines || pDevice->GetPhoneCount() != (int) dwNumPhones)
		return LINEERR_NODEVICE;
	return lResult;

}// CDSProvider::providerInit
