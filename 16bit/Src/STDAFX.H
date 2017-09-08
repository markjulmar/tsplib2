// Force TAPI 2.0 headers to be TAPI 1.4 complient if included.
#define TAPI_CURRENT_VERSION 0x00010004
#define _USRDLL
#include <afxwin.h>                 // Include the MFC/AFX headers
#include <windowsx.h>               // Some useful MS macros
#include "tspi.h"                   // TAPI support for service providers.
#include "splib.h"                  // Our class library
