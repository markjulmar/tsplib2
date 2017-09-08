// STDAFX.H - Pre-compiled header support for ATSP.

#define TAPI_CURRENT_VERSION 0x00020001
#define WINVER 0x400
#define _USRDLL                     // TSPs are DLL files

#include <afxwin.h>                 // Include the MFC/AFX headers
#include <afxmt.h>                  // Include the multi-threaded libraries
#include <tspi.h>                   // TAPI support for service providers.
#include "splib.h"                  // Our class library
