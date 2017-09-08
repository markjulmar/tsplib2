/*****************************************************************************/
//
// DLLMAIN.C
//
// Sample THUNK layer for 32-bit companion application linking
// to 16-bit TSP layer.
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

// Prototype for the thunk connect function provided for both sides of the thunk.
BOOL _stdcall thk_ThunkConnect32(LPSTR pszDll16, LPSTR pszDll32, HINSTANCE hInst, DWORD dwReason);

// Prototype for function in 16-bit dll.
void PASCAL DeviceNotify(WORD wCommand, DWORD dwConnID, DWORD dwData, 
                         LPVOID lpBuff, DWORD dwSize);

//////////////////////////////////////////////////////////////////////////////
// DLLMain
//
// This function is called when the DLL is loaded by a process, or 
// when new threads are created by a process that has already loaded the
// DLL.  Also called when threads of a process that have loaded the
// DLL exit cleanly and when the process itself unloads the DLL.
//
BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    // Connect up the thunking layer.
    return thk_ThunkConnect32("PROVIDER.TSP", "DLL32.DLL", hDLLInst, fdwReason);
    
}// DLLMain

//////////////////////////////////////////////////////////////////////////////
// DeviceNotify32
//
// This is the 32-bit side of the notification.  It calls the 32->16
// bit thunk translation and will end up in the DeviceNotify function
// which is part of the spLIB++ product.
//     
__declspec(dllexport) void WINAPI DeviceNotify32(WORD wCommand, 
            DWORD dwConnID, DWORD dwData, LPVOID lpBuff, DWORD dwSize)
{
    // Call into the 16-bit service provider.
    DeviceNotify(wCommand, dwConnID, dwData, lpBuff, dwSize);
    
}// DeviceNotify32

     
