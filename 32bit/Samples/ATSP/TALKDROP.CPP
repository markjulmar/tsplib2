/******************************************************************************/
//                                                                        
// TALKDROP.CPP - Talk/Drop dialog for the ATSP service provider
//                                                                        
// This file contains the dialog definitions and code for the Talk/Drop
// dialog which is used to facilitate action from the user when a
// call is connected using the modem.
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
#include "atsp.h"
#include "resource.h"
#include <windowsx.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

////////////////////////////////////////////////////////////////////////
// Constants

const UINT IDT_TIMER = 100;

////////////////////////////////////////////////////////////////////////
// Globals

static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static VOID OnCommand (HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static VOID OnTimer (HWND hwnd, UINT idEvent);
static VOID CenterWindow (HWND hwnd);
static VOID OnShowWindow (HWND hwnd, BOOL fShow, UINT status);
static VOID OnDestroy (HWND hwnd);

////////////////////////////////////////////////////////////////////////
// TalkDropDlgProc
//
// The Talk/Drop dialog window procedure.
//
BOOL CALLBACK TalkDropDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG (hDlg, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG (hDlg, WM_TIMER, OnTimer);
		HANDLE_MSG (hDlg, WM_COMMAND, OnCommand);
		HANDLE_MSG (hDlg, WM_SHOWWINDOW, OnShowWindow);
		HANDLE_MSG (hDlg, WM_DESTROY, OnDestroy);
		default: return FALSE;
	}

}// TalkDropDlgProc

////////////////////////////////////////////////////////////////////////
// OnInitDialog
//
// Initialize the Talk/Drop dialog for the ATSP sample.
//
static BOOL OnInitDialog(HWND hwnd, HWND /*hwndFocus*/, LPARAM lParam)
{
    // Set the font for all the controls to something
    // a little better than the default.
	HFONT hFont = (HFONT) GetStockObject (ANSI_VAR_FONT);
	HWND hwndChild = GetWindow (hwnd, GW_CHILD);
	while (hwndChild != NULL && IsChild (hwnd, hwndChild))
	{
		SetWindowFont (hwndChild, hFont, FALSE);
		hwndChild = GetNextSibling(hwndChild);
	}
   
    // Center the window on the screen.
    CenterWindow(hwnd);

	// Save our parameter in our USER area.
	SetWindowLong (hwnd, DWL_USER, lParam);

	// And let Windows set focus to our first control.
    return FALSE;

}// OnInitDialog

////////////////////////////////////////////////////////////////////////
// OnShowWindow
//
// This is called when the window is shown.
//
static VOID OnShowWindow (HWND hwnd, BOOL fShow, UINT /*status*/)
{
	// Reset elapsed time
	if (fShow == TRUE)
	{
		OnTimer(hwnd, 0);
		SetTimer (hwnd, IDT_TIMER, 1000L, NULL);
	}

}// OnShowWindow

////////////////////////////////////////////////////////////////////////
// CenterWindow
//
// Utility function to center a window onto the screen.
// WARNING: ASSUMES that the window is smaller than the screen!
//
static VOID CenterWindow (HWND hwnd)
{
	RECT rc;
	GetWindowRect (hwnd, &rc);
	SetWindowPos (hwnd, HWND_TOP, 
		          (GetSystemMetrics (SM_CXSCREEN)/2)-(rc.right-rc.left),
		          (GetSystemMetrics (SM_CYSCREEN)/2)-(rc.bottom-rc.top),
				  0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW);

}// CenterWindow

////////////////////////////////////////////////////////////////////////
// OnCommand
//
// Process a TALK or DROP command for the AT service provider.  
// This will end our dialog and send a message back to the service
// provider indicating user action.
//
static VOID OnCommand (HWND hwnd, int id, HWND /*hwndCtl*/, UINT codeNotify)
{
	if (codeNotify == BN_CLICKED && (id == IDOK || id == IDCANCEL))
	{
		EnableWindow (GetDlgItem (hwnd, IDOK), FALSE);
		EnableWindow (GetDlgItem (hwnd, IDCANCEL), FALSE);

		// Save off the result
		LPDWORD lpDword = (LPDWORD) GetWindowLong (hwnd, DWL_USER);
		*lpDword = (id == IDOK) ? 1L : 0L;

		DestroyWindow (hwnd);
	}

}// OnCommand

///////////////////////////////////////////////////////////////////////
// OnDestroy
//
// Window is closing - quit our message loop
//
static VOID OnDestroy (HWND hwnd)
{
	KillTimer(hwnd, IDT_TIMER);
	PostQuitMessage(0L);

}// OnDestroy

///////////////////////////////////////////////////////////////////////
// OnTimer
//
// Elapsed timer for the dialog - shows an elapsed time while
// the dialog is up.
//
static VOID OnTimer (HWND hwnd, UINT idEvent)
{       
	static DWORD dwStartMsec = 0L;
	static DWORD dwElapsedSec = 0L;

	if (idEvent != IDT_TIMER)
	{
		dwStartMsec = GetTickCount();
		dwElapsedSec = 0L;
	}

    DWORD dwElapsed = (GetTickCount() - dwStartMsec) / 1000;
	if (dwElapsed != dwElapsedSec)
	{
		TCHAR szBuff[80];
		_stprintf (szBuff, _T("%ld second%c"), dwElapsed, (dwElapsed == 1) ? ' ' : 's');
		SetWindowText (GetDlgItem (hwnd, IDC_ELAPSEDTIME), szBuff);
		dwElapsedSec = dwElapsed;
	}
    
}// OnTimer

