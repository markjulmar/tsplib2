/******************************************************************************/
//                                                                        
// SPTHREAD.CPP - Service Provider Thread support
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the threading support for the library.
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
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::StartNextCommand
//
// Start a new command on the service provider.
//
VOID CServiceProvider::StartNextCommand(CTSPIConnection* pConn)
{                     
	// Ship it to the connection.  We will simply use the thread we are
	// on since we are guarenteed that it is TAPISRV.EXE.
	// Unless overriden, this will end up in ProcessData.
	pConn->ReceiveData (STARTING_COMMAND, NULL, 0L);

}// CServiceProvider::StartNextCommand

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::OpenDevice
//
// Open a device.
//
BOOL CServiceProvider::OpenDevice (CTSPIConnection* /*pConn*/)
{   
	return FALSE;

}// CServiceProvider::OpenDevice

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::CloseDevice
//
// Close the device in question
//
BOOL CServiceProvider::CloseDevice (CTSPIConnection* /*pConn*/)
{
	return FALSE;

}// CServiceProvider::CloseDevice

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::SendData
//
// This sends the buffer to the physical telephony device.  It is called
// by the CTSPIDevice class to send a buffer through the main class.
//
BOOL CServiceProvider::SendData(CTSPIConnection* /*pConn*/, LPCVOID /*lpBuff*/, DWORD /*dwSize*/)
{
	// If used, derived provider MUST provide.	
	ASSERT (FALSE);
    return FALSE;

}// CServiceProvider::SendData

///////////////////////////////////////////////////////////////////////////
// CServiceProvider::OnTimer
//
// Manage the interval timer
//
void CServiceProvider::OnTimer(CTSPIConnection* /*pConn*/)
{
	/* Do nothing */

}// CServiceProvider::OnTimer

