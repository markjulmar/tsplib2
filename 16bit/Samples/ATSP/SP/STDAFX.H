/************************************************************************/
//                                                                        
// STDAFX.H - Pre-compiled header support for ATSP.
//                                                                        
// Copyright (C) 1994, 1995, 1996 Mark C. Smith// All rights reserved                                                    
//         
// Include the necessary files for building the library.
//                                                                
/************************************************************************/

#define TAPI_CURRENT_VERSION 0x00010004
#define _USRDLL                     // TSPs are DLL files
#include <afxwin.h>                 // Include the MFC/AFX headers
#include <windowsx.h>               // Some useful MS macros
#include <tspi.h>                   // TAPI support for service providers.
#include <splib.h>                  // Our class library
#include <spuser.h>                 // Thread communication