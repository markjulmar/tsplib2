#define _USRDLL                     // TSPs are DLL files
#define TAPI_CURRENT_VERSION 0x00020001
#define WINVER 0x400
#include <afxwin.h>                 // Include the MFC/AFX headers
#include <afxmt.h>					// Multi-threading headers
#include <windowsx.h>               // Some useful MS macros
#include <tspi.h>                   // TAPI support for service providers.
#include <splib.h>                  // Our class library
#include <afxsock.h>				// Socket interface
