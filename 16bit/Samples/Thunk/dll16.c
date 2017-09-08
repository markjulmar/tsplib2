/*****************************************************************************/
//
// DLL16.C
//
// Sample THUNK layer for 32-bit companion application linking
// to 16-bit TSP layer.  This implements the 16-bit side of the thunk
//
// This code is a sample, and as such is not guarenteed to work as
// listed here.  It was tested under the golden release of Windows 95,
// and conforms to the standards defined in the Windows 95 SDK for
// universal thunks.
// 
// It may be modified to suite your needs.  Use at your own risk.
//
/*****************************************************************************/

#include <windows.h>

////////////////////////////////////////////////////////////////////////////////////
// Prototypes

BOOL FAR PASCAL __export DllEntryPoint (DWORD dwReason, WORD hInst, WORD wDS,
                               WORD wHeapSize, DWORD dwReserved1, WORD wReserved2);

BOOL FAR PASCAL thk_ThunkConnect16(LPSTR pszDll16, LPSTR pszDll32, WORD hInst,
                                   DWORD dwReason);

////////////////////////////////////////////////////////////////////////////////////
// DLLEntryPoint
//
// Required entrypoint for the thunk connection code.
//
BOOL FAR PASCAL __export DllEntryPoint (DWORD dwReason, WORD hInst, WORD wDS,
                               WORD wHeapSize, DWORD dwReserved1, WORD wReserved2);
{
    // Connect up to the 32-bit thunk
    return thk_ThunkConnect16("PROVIDER.TSP", "DLL32.DLL", hInst, dwReason);
    
}// DllEntryPoint
