/*****************************************************************************/
//
// DLL32.H
//
// Sample THUNK layer for 32-bit companion application linking
// to 16-bit TSP layer.  This is a header to be included by the
// 32-bit companion application so it links to this DeviceNotify rather
// than the 16-bit version.  "spuser.h" should still be included in
// the companion, but it should link against this project rather than
// the service provider directly.
//
// This code is a sample, and as such is not guarenteed to work as
// listed here.  It was tested under the golden release of Windows 95,
// and conforms to the standards defined in the Windows 95 SDK for
// universal thunks.
// 
// It may be modified to suite your needs.  Use at your own risk.
//
/*****************************************************************************/

// Prototype for function.
__declspec(dllimport) BOOL WINAPI DeviceNotify32(WORD wCommand, 
            DWORD dwConnID, DWORD dwData, LPVOID lpBuff, DWORD dwSize);
