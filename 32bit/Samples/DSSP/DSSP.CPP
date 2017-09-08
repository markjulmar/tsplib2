/*****************************************************************************/
//
// DSSP.CPP - Digital Switch Service Provider Sample
//                                                                        
// This file contains the main service provider override class.
// 
// Copyright (C) 1997-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This service provider talks to a simulated digital switch emulator.  It
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
/*****************************************************************************/

#include "stdafx.h"
#include "dssp.h"

IMPLEMENT_DYNCREATE(CDSLine, CTSPILineConnection)
IMPLEMENT_DYNCREATE(CDSPhone, CTSPIPhoneConnection)
IMPLEMENT_DYNCREATE(CDSDevice, CTSPIDevice)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/*----------------------------------------------------------------------------
	MAIN TSP OBJECT
-----------------------------------------------------------------------------*/
CDSProvider theSP;

/*****************************************************************************
** Procedure:  CDSProvider::CDSProvider
**
** Arguments:  void
**
** Returns:    void
**
** Description:  This is the constructor for the provider object.  It
**               is responsible for initializing the UI.DLL name and the
**               provider manufacturer names.  In additional, any overrides
**               of class objects should be performed here.
**
*****************************************************************************/
CDSProvider::CDSProvider() : 
	CServiceProvider(_T("DSSP32.TSP"),								// Name of UI DLL
					 _T("Digital Switch Sample Service Provider"),	// Description
					TAPI_CURRENT_VERSION)							// Version of TAPI to negotiate to

{   
	// Setup our line device and phone device object override.
	SetRuntimeObjects (RUNTIME_CLASS(CDSDevice),					// Our device override
					   NULL,										// Request object
					   RUNTIME_CLASS(CDSLine),						// Line override
					   NULL, NULL, NULL,							// Address/Call/ConfCall objects
					   RUNTIME_CLASS(CDSPhone));					// Phone override	

}// CDSProvider::CDSProvider

