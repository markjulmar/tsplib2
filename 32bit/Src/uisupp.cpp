/******************************************************************************/
//                                                                        
// UISUPP.CPP - User-interface support functions
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the methods which can be called fromt eh
// user-interface functions (UIDLL) of the service provider.
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
#include <ctype.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::IsProviderInstalled
//
// This queries the TAPI subsystem to see if a provider is installed.
// It returns LINEERR_NOMULTIPLEINSTANCE if it is.
//
LONG CServiceProvider::IsProviderInstalled(LPCTSTR pszProviderName, LPDWORD lpdwPPid) const
{
	HINSTANCE hTapiDLL = NULL;
	LPLINEPROVIDERLIST pProviderList = NULL;
	LONG lResult = 0;
	CString strThisProvider (pszProviderName);

	// See if this provider is already installed.  We do this by querying
	// TAPI for the list of installed providers.
	hTapiDLL = LoadLibrary(_T("TAPI32.DLL"));
	if (hTapiDLL == NULL)
		return LINEERR_OPERATIONFAILED;

	// Locate our entrypoint for "lineGetProviderList".  This will return
	// the list of providers installed in this system.
	LONG (WINAPI *pfnGetProviderList)(DWORD, LPLINEPROVIDERLIST);
	pfnGetProviderList = (LONG (WINAPI*) (DWORD, LPLINEPROVIDERLIST)) GetProcAddress(hTapiDLL, "lineGetProviderList");
	if (pfnGetProviderList == NULL)
	{
		FreeLibrary (hTapiDLL);
		return LINEERR_OPERATIONFAILED;
	}

	// Retrieve all the providers.
	DWORD dwReqSize = sizeof(LINEPROVIDERLIST)*10;
	while (TRUE)
	{
		pProviderList = (LPLINEPROVIDERLIST) AllocMem(dwReqSize);
		if (pProviderList == NULL)
		{
			lResult = LINEERR_NOMEM;
			break;
		}

		pProviderList->dwTotalSize = dwReqSize;
		if (pfnGetProviderList (TAPIVER_20, pProviderList) != 0)
		{
			lResult = LINEERR_OPERATIONFAILED;
			break;
		}

		// Go through the list of retrieved providers and see if we are included
		// in this list - TAPI will not add us to the registry until we return 
		// success to this function, so we should not currently be here.
		if (pProviderList->dwNeededSize <= pProviderList->dwTotalSize)
		{
			LPLINEPROVIDERENTRY pProviderEntry = (LPLINEPROVIDERENTRY) (((LPBYTE) pProviderList) +
					pProviderList->dwProviderListOffset);
			for (DWORD i = 0; i < pProviderList->dwNumProviders; i++)
			{
				// Get the name of this provider.
				LPCSTR pszProvider = (LPCSTR) pProviderList+pProviderEntry->dwProviderFilenameOffset;

				// Make sure we are pointing at the TSP module, and not any path
				if (strrchr(pszProvider, '\\') != NULL)
					pszProvider = strrchr(pszProvider, '\\') + sizeof(char);

				// If this is OUR provider, then error out.
				if (strThisProvider.CompareNoCase((LPTSTR)pszProvider) == 0)
				{
					*lpdwPPid = pProviderEntry->dwPermanentProviderID;
					lResult = LINEERR_NOMULTIPLEINSTANCE;
					break;
				}
				pProviderEntry++;
			}
			break;
		}
		else
		{
			dwReqSize = pProviderList->dwNeededSize;
			FreeMem ((LPSTR)pProviderList);
			pProviderList = NULL;
		}
	}

	// Cleanup and call the providerConfig function if we can install.
	if (pProviderList != NULL)
		FreeMem ((LPSTR)pProviderList);
	if (hTapiDLL)
		FreeLibrary(hTapiDLL);

	return lResult;

}// CServiceProvider::IsProviderInstalled

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetProviderIDFromDeviceID
//
// This converts a line or phone device ID into a provider id.
//
LONG CServiceProvider::GetProviderIDFromDeviceID(TUISPIDLLCALLBACK lpfnDLLCallback, 
							DWORD dwDeviceID, BOOL fIsLine, LPDWORD lpdwPPid)
{
	DWORD arrInfo[2] = { GDD_LINEPHONETOPROVIDER, dwDeviceID };

	ASSERT (lpfnDLLCallback != NULL);
	ASSERT (lpdwPPid != NULL);

	*lpdwPPid = 0;

	LONG lResult = (*lpfnDLLCallback)(dwDeviceID, (fIsLine) ? TUISPIDLL_OBJECT_LINEID : TUISPIDLL_OBJECT_PHONEID,
			&arrInfo, sizeof(DWORD)*2);
	
	if (lResult == 0 && arrInfo[0] == GDD_LINEPHONETOPROVIDEROK)
		*lpdwPPid = arrInfo[1];
	return lResult;

}// CServiceProvider::GetProviderIDFromDeviceID

/////////////////////////////////////////////////////////////////////////////
// CServiceProvider::GetPermanentIDFromDeviceID
//
// This converts a line or phone device ID into a permanent line/phone id.
//
LONG CServiceProvider::GetPermanentIDFromDeviceID(TUISPIDLLCALLBACK lpfnDLLCallback, 
							DWORD dwDeviceID, BOOL fIsLine, LPDWORD lpdwPPid)
{
	DWORD arrInfo[2] = { GDD_LINEPHONETOPERMANENT, dwDeviceID };

	ASSERT (lpfnDLLCallback != NULL);
	ASSERT (lpdwPPid != NULL);

	*lpdwPPid = 0;

	LONG lResult = (*lpfnDLLCallback)(dwDeviceID, (fIsLine) ? TUISPIDLL_OBJECT_LINEID : TUISPIDLL_OBJECT_PHONEID,
			&arrInfo, sizeof(DWORD)*2);
	
	if (lResult == 0 && arrInfo[0] == GDD_LINEPHONETOPERMANENTOK)
		*lpdwPPid = arrInfo[1];
	return lResult;

}// CServiceProvider::GetProviderIDFromDeviceID
