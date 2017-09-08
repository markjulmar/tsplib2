/*****************************************************************************/
//                                                                        
// ATSP.CPP - TAPI Service Provider for AT style modems
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

#include "stdafx.h"
#include "resource.h"
#include "atsp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////
// Globals

CATSPProvider theSP;

////////////////////////////////////////////////////////////////////////////
// CATSPProvider::CATSPProvider
//
// Constructor for the service provider
//
CATSPProvider::CATSPProvider() : 
    CServiceProvider (_T("ATSP32.TSP"),							// Name of UI dll
					  _T("ATSP32 C++ Service Provider Sample"),	// Provider Info
					  TAPI_CURRENT_VERSION)						// Supported TAPI version
{
#ifdef _DEBUG
	// Turn debug output to minimum level.
	g_iShowAPITraceLevel = 1;
#endif

	// Setup our line device object override.
	SetRuntimeObjects (NULL, NULL, 
					   RUNTIME_CLASS(CATSPLine));

}// CATSPProvider::CATSPProvider

/////////////////////////////////////////////////////////////////////////////
// CATSPProvider::providerEnumDevices
//
// Force the line/phone count to be specified by the provider
// rather than TAPI.
//
LONG CATSPProvider::providerEnumDevices(DWORD dwProviderId, 
					LPDWORD lpNumLines,
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

	*lpNumLines = ReadProfileDWord(0, _T("Count"), 1);
	*lpNumPhones = 0;
	return lResult;

}// CATSPProvider::providerEnumDevices

