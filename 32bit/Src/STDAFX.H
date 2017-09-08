/************************************************************************/
//                                                                        
// STDAFX.H - Pre-compiled header support for SPLIB retail bulild
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//         
// Include the necessary files for building the library.
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
/************************************************************************/

#define TAPI_CURRENT_VERSION 0x00020000	// Only support TAPI 2
#define WINVER 0x400

#define _USRDLL						// MFC statically linked DLL	
#include <afxwin.h>                 // Include the MFC/AFX headers
#include <afxmt.h>					// Multi-threaded classes
#include <windowsx.h>               // Some useful MS macros
#include <tchar.h>					// Unicode support
#include <tspi.h>                   // TAPI support for service providers.
#include "splib.h"                  // Our class library
