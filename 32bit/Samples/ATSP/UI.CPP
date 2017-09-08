/******************************************************************************/
//                                                                        
// UI.CPP - TAPI Service Provider for AT style modems
//                                                                        
// This file contains all the user-interface code which drives
// the TSPUI portion of our TSP.
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
#include "config.h"
#include "pconfig.h"
#include "talkdrop.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////////
// Globals from other files

extern BOOL CALLBACK TalkDropDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

////////////////////////////////////////////////////////////////////////////
// CATSPProvider::providerInstall
//
// Set up the initial .INI file entries and invoke the configuration
// dialog.
//
LONG CATSPProvider::providerInstall(DWORD dwPermanentProviderID, 
					CWnd* pwndOwner, TUISPIDLLCALLBACK lpfnDLLCallback)
{
	LONG lResult = CServiceProvider::providerInstall(dwPermanentProviderID, pwndOwner, lpfnDLLCallback);
	if (lResult == 0)
	{
		DWORD dwMyPPid;
		lResult = IsProviderInstalled(AfxGetAppName(), &dwMyPPid);
		if (lResult == 0)
			lResult = providerConfig(dwPermanentProviderID, pwndOwner, lpfnDLLCallback);
	}
	return lResult;

}// CATSPProvider::providerInstall

/////////////////////////////////////////////////////////////////////////////
// CATSPProvider::lineConfigDialogEdit
//
// Invoked when an application invokes our configuration dialog for a 
// single session.  We simply return OK since we have no configurable
// options, but still want to work with TAPICOMM.EXE
//
LONG CATSPProvider::lineConfigDialogEdit(DWORD dwDeviceID, CWnd* pwndOwner, 
						CString& strDeviceClass, LPVOID const /*lpDeviceConfigIn*/, 
						DWORD /*dwSize*/, LPVARSTRING lpDeviceConfig, TUISPIDLLCALLBACK lpfnDLLCallback)
{
	// Set it to junk which we ignore.
	lpDeviceConfig->dwUsedSize = lpDeviceConfig->dwNeededSize = sizeof(VARSTRING)+1;
	lpDeviceConfig->dwStringFormat = STRINGFORMAT_BINARY;
	lpDeviceConfig->dwStringOffset = sizeof(VARSTRING);
	lpDeviceConfig->dwStringSize = 1;

	// Configure the line device.
	return lineConfigDialog (dwDeviceID, pwndOwner, strDeviceClass, lpfnDLLCallback);

}// CATSPProvider::lineConfigDialogEdit

/////////////////////////////////////////////////////////////////////////////
// CATSPProvider::lineConfigDialog
//
// Invoked when the user uses TAPI to invoke our configuration dialog
// vs. the control panel (providerConfig).
//
LONG CATSPProvider::lineConfigDialog(DWORD dwDeviceID, CWnd* pwndOwner, 
		     CString& /*strDeviceClass*/, TUISPIDLLCALLBACK lpfnDLLCallback)
{
	// Ask the provider which provider id this line relates to.
	DWORD dwPPid = 0;
	if (CServiceProvider::GetPermanentIDFromDeviceID(lpfnDLLCallback, dwDeviceID, TRUE, &dwPPid) == 0)
	{
		// Invoke the dialog.
		CSpConfigDlg Config(pwndOwner, dwPPid, lpfnDLLCallback);
		Config.DoModal();
	}
	return FALSE;

}// CATSPProvider::lineConfigDialog

///////////////////////////////////////////////////////////////////////////
// CATSPProvider::providerConfig
//
// Invoke the main provider configuration.
//
LONG CATSPProvider::providerConfig(DWORD dwPPID, CWnd* pwndOwner,
						   TUISPIDLLCALLBACK lpfnDLLCallback)
{
	CProvConfig Config (pwndOwner, dwPPID, lpfnDLLCallback);
	Config.DoModal();
	return FALSE;

}// CATSPProvider::providerConfig

////////////////////////////////////////////////////////////////////////////
// CATSPProvider::providerGenericDialogData
//
// One of the UI DLLs is sending us a response/command.  In all cases, we 
// sent/receive a DWORD of information.  This could just as easily have been
// a structure.
//
LONG CATSPProvider::providerGenericDialogData (CTSPIDevice* /*pDevice*/, 
				CTSPILineConnection* pLine, 
				CTSPIPhoneConnection* /*pPhone*/, 
				HDRVDIALOGINSTANCE hdDlgInstance, 
				LPVOID lpBuff, DWORD dwSize)
{
	if (hdDlgInstance != NULL)
	{
		if (dwSize == sizeof(DWORD))
		{
			// Get the LINEUIDIALOG structure - it is simply the htDlgInstance passed
			// to TAPI when we instantiated the UI dialog.
			LINEUIDIALOG* pLineDlg = (LINEUIDIALOG*) hdDlgInstance;
			pLine = pLineDlg->pLineOwner;
			if (pLine != NULL && pLineDlg != NULL)
			{
				ASSERT (pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
				if (*((LPDWORD)lpBuff) == 1L)  // Talk pushed
					pLine->ReceiveData (MODEM_TALKDROP_TALK);
				else // Drop pushed
					pLine->ReceiveData (MODEM_TALKDROP_DROP);
			}
		}
	}

	// If the line object is valid, then this is a request from the LINE configuration
	// dialog to return the permanent line identifier.
	else if (pLine != NULL)
	{
		if (dwSize >= sizeof(DWORD))
			*((LPDWORD)lpBuff) = pLine->GetPermanentDeviceID();
	}

	return FALSE;

}// CATSPProvider::providerGenericDialogData

///////////////////////////////////////////////////////////////////////////
// CATSPProvider::providerGenericDialogData
//
// The SPDLL is sending our UIDLL a command.
//
LONG CATSPProvider::providerGenericDialogData(HTAPIDIALOGINSTANCE htDlgInst,
					LPVOID lpParams, DWORD dwSize)
{
	// Locate the window object which matches the dialog instance.
	CWnd* pMainWnd = NULL;
	if (m_UIMap.Lookup((void*)htDlgInst, (void*&)pMainWnd))
	{
		// If this is a valid command, then perform the action requested.
		if (dwSize == sizeof(DWORD) && pMainWnd != NULL)
		{
			HWND hwnd = pMainWnd->GetSafeHwnd();
			if (hwnd != NULL)
			{
				switch (*((LPDWORD)lpParams))
				{
					// Destroy the dialog due to call deactivation.
					case TALKDROP_DESTROY:
						pMainWnd->Detach();
						delete pMainWnd;
						m_UIMap.SetAt((void*)htDlgInst, NULL);
						PostMessage (hwnd, WM_COMMAND, (WPARAM)MAKELPARAM(IDCANCEL,BN_CLICKED), (LPARAM)GetDlgItem(hwnd,IDCANCEL));
						break;

					// Show the dialog.
					case TALKDROP_SHOW:
						ShowWindow (hwnd, SW_SHOW);
						break;
				}
				return FALSE;
			}
		}
	}
	return LINEERR_OPERATIONFAILED;

}// CATSPProvider::providerGenericDialogData

///////////////////////////////////////////////////////////////////////////
// CATSPProvider::providerGenericDialog
//
// This method is called for the UI DLL when the provider 
// needs a dialog displayed.  This will only be called when
// we display the Talk/Drop dialog.
//
LONG CATSPProvider::providerGenericDialog (HTAPIDIALOGINSTANCE htDlgInst, 
					LPVOID lpParams, DWORD dwSize, HANDLE hEvent, 
					TUISPIDLLCALLBACK lpfnDLLCallback)
{
	// Verify that this is for the talk drop dialog.
	if (lpParams != NULL || dwSize != 0L)
		return LINEERR_OPERATIONFAILED;
	if (m_pMainWnd != NULL)
		return LINEERR_OPERATIONUNAVAIL;

	// Create our TalkDrop dialog.  The thread state hasn't been
	// setup here so we didn't use MFC here, although if an
	// _AFX_THREAD_STATE and AFX_MODULE_THREAD_STATE were setup,
	// we probably could.
	DWORD dwResult = 0L;
	HWND hwnd = CreateDialogParam (AfxGetInstanceHandle(), 
				MAKEINTRESOURCE(IDD_TALKDROP),
				NULL, TalkDropDlgProc, (LPARAM)&dwResult);
	if (hwnd == NULL)
		return LINEERR_OPERATIONFAILED;

	CWnd* pMainWnd = new CWnd;
	pMainWnd->Attach(hwnd);
	m_UIMap.SetAt((void*)htDlgInst, pMainWnd);

	// Set the INIT event.
	SetEvent (hEvent);

	// Run a message loop waiting for an END result.
	MSG msg;
	while (GetMessage (&msg, hwnd, NULL, NULL) == TRUE)
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	// Delete the handle created by the dialog.  If it is already
	// NULL, then the service provider killed our dialog through 
	// the above providerGenericDialogData.
	if (m_UIMap.Lookup((void*)htDlgInst, (void*&)pMainWnd) && pMainWnd != NULL)
	{
		pMainWnd->Detach();
		delete pMainWnd;
		m_UIMap.SetAt((void*)htDlgInst, NULL);

		// Send our result to TAPI.
		lpfnDLLCallback ((DWORD)htDlgInst, TUISPIDLL_OBJECT_DIALOGINSTANCE, &dwResult, sizeof(DWORD));  
	}

	return FALSE;

}// CATSPProvider::providerGenericDialog

