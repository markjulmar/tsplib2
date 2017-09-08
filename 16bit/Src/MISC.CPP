/******************************************************************************/
//                                                                        
// MISC.CPP - Source file for the misc. class functions
//                                             
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//
// This file implements all the request object constructors and
// the various support classes required in the library which are
// considered internal to the spLIB++ product.
//
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
//                                                                        
/******************************************************************************/

#include "stdafx.h"

IMPLEMENT_DYNCREATE (CTSPIBaseObject, CObject)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CADObArray::~CADObArray
//
// Destructor for the auto-delete object array
//
CADObArray::~CADObArray()
{                          
    for (int i = 0; i < GetSize(); i++)
    {
        CObject* pObj = GetAt(i);
        delete pObj;
    }
    
}// CADObArray::~CADObArray

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEFORWARD::TSPILINEFORWARD
//
// Constructor for the LINEFORWARD object request
//
TSPILINEFORWARD::TSPILINEFORWARD()
{                                   
    dwNumRings = 0;
    pCall = NULL;
    lpCallParams = NULL;

}// TSPILINEFORWARD::TSPILINEFORWARD

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEFORWARD::~TSPILINEFORWARD
//
// Destructor for the LINEFORWARD object request
//
TSPILINEFORWARD::~TSPILINEFORWARD()
{                                    
    for (int i = 0; i < arrForwardInfo.GetSize(); i++)
    {
        TSPIFORWARDINFO* lpInfo = (TSPIFORWARDINFO*) arrForwardInfo[i];
        lpInfo->DecUsage();
    }
    FreeMem (lpCallParams);

}// TSPILINEFORWARD::~TSPILINEFORWARD

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIFORWARDINFO::TSPIFORWARDINFO
//
// Constructor for the TSPIFORWARDINFO object request
//
TSPIFORWARDINFO::TSPIFORWARDINFO()
{                                   
    dwForwardMode = 0;
    dwDestCountry = 0;
    iRefCount = 1;

}// TSPIFORWARDINFO::TSPIFORWARDINFO

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIFORWARDINFO::IncUsage
//
// Increment the reference counter
//
VOID TSPIFORWARDINFO::IncUsage()
{                               
    iRefCount++;
    
}// TSPIFORWARDINFO::IncUsage

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIFORWARDINFO::DecUsage
//
// Increment the reference counter
//
VOID TSPIFORWARDINFO::DecUsage()
{                               
    if (--iRefCount <= 0)
        delete this;
    
}// TSPIFORWARDINFO::DecUsage

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICALLPARAMS::TSPICALLPARAMS
//
// Constructor for the TSPICALLPARAMS object request
//
TSPICALLPARAMS::TSPICALLPARAMS()
{                                 
    dwBearerMode = 0;
    dwMinRate = 0;
    dwMaxRate = 0;
    FillBuffer (&DialParams, 0, sizeof(LINEDIALPARAMS));

}// TSPICALLPARAMS::TSPICALLPARAMS

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEPARK::TSPILINEPARK
//
// Constructor for the TSPILINEPARK object request
//
TSPILINEPARK::TSPILINEPARK()
{                             
    dwParkMode = 0;
    lpNonDirAddress = NULL;

}// TSPILINEPARK::TSPILINEPARK              

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINEPICKUP::TSPILINEPICKUP
//
// Constructor for the TSPILINEPICKUP object request
//
TSPILINEPICKUP::TSPILINEPICKUP()
{                                 
    strGroupID = "";
    
}// TSPILINEPICKUP::TSPILINEPICKUP              

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITRANSFER::TSPITRANSFER
//
// Constructor for the TSPITRANSFER object request
//
TSPITRANSFER::TSPITRANSFER()
{                             
    pCall = NULL;
    pConsult = NULL;
    pConf = NULL;
    dwTransferMode = 0L;
    lpCallParams = NULL;

}// TSPITRANSFER::TSPITRANSFER

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITRANSFER::~TSPITRANSFER
//
// Destructor for the TSPITRANSFER object request
//
TSPITRANSFER::~TSPITRANSFER()
{                             
    FreeMem (lpCallParams);

}// TSPITRANSFER::~TSPITRANSFER

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICONFERENCE::TSPICONFERENCE
//
// Constructor for the TSPICONFERENCE object request
//
TSPICONFERENCE::TSPICONFERENCE()
{                                 
    pConfCall = NULL;
    pCall = NULL;
    pConsult = NULL;
    dwPartyCount = 0L;
    lpCallParams = NULL;

}// TSPICONFERENCE::TSPICONFERENCE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICONFERENCE::~TSPICONFERENCE
//
// Destructor for the TSPICONFERENCE object request
//
TSPICONFERENCE::~TSPICONFERENCE()
{                                 
    FreeMem (lpCallParams);

}// TSPICONFERENCE::~TSPICONFERENCE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIDIGITGATHER::TSPIDIGITGATHER
//
// Constructor for the TSPIDIGITGATHER object request
//
TSPIDIGITGATHER::TSPIDIGITGATHER()
{                                   
    dwEndToEndID = 0L;
    dwDigitModes = 0L;
    lpBuffer = NULL;
    dwSize = 0L;
    dwCount = 0L;
    strTerminationDigits = "";
    dwFirstDigitTimeout = 0L;
    dwInterDigitTimeout = 0L;
    dwLastTime = 0L;

}// TSPIDIGITGATHER::TSPIDIGITGATHER

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIGENERATE::TSPIGENERATE
//
// Constructor for the TSPIGENERATE object request
//
TSPIGENERATE::TSPIGENERATE()
{                             
    dwEndToEndID = 0L;
    dwMode = 0L;
    dwDuration = 0L;
    strDigits = "";

}// TSPIGENERATE::TSPIGENERATE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIGENERATE::~TSPIGENERATE
//
// Constructor for the TSPIGENERATE object request
//
TSPIGENERATE::~TSPIGENERATE()
{                             
    for (int i = 0; i < arrTones.GetSize(); i++)
    {                          
        LPLINEGENERATETONE lpTone = (LPLINEGENERATETONE) arrTones[i];
        delete lpTone;
    }
    
}// TSPIGENERATE::~TSPIGENERATE

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICOMPLETECALL::TSPICOMPLETECALL
//
// Constructor for the TSPICOMPLETECALL object request
//
TSPICOMPLETECALL::TSPICOMPLETECALL()
{                                     
    dwCompletionMode = 0L;
    dwMessageId = 0L;

}// TSPICOMPLETECALL::TSPICOMPLETECALL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPICOMPLETECALL::TSPICOMPLETECALL
// 
// Copy constructor
//
TSPICOMPLETECALL::TSPICOMPLETECALL(const TSPICOMPLETECALL* pComplete)
{
    dwCompletionID = pComplete->dwCompletionID;
    dwCompletionMode = pComplete->dwCompletionMode;
    dwMessageId = pComplete->dwMessageId;
    pCall = pComplete->pCall;
    dwSwitchInfo = pComplete->dwSwitchInfo;
    strSwitchInfo = pComplete->strSwitchInfo;
    
}// TSPICOMPLETECALL::TSPICOMPLETECALL
   
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMAKECALL::TSPIMAKECALL
//
// Constructor for the TSPIMAKECALL object request
//
TSPIMAKECALL::TSPIMAKECALL()
{                             
    dwCountryCode = 0L;
    lpCallParams = NULL;

}// TSPIMAKECALL::TSPIMAKECALL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMAKECALL::~TSPIMAKECALL
//
// Destructor for the TSPIMAKECALL object request
//
TSPIMAKECALL::~TSPIMAKECALL()
{                             
    FreeMem (lpCallParams);
    
}// TSPIMAKECALL::~TSPIMAKECALL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITONEMONITOR::TSPITONEMONITOR
//
// Constructor for the tone monitor request object
//
TSPITONEMONITOR::TSPITONEMONITOR()
{
    dwToneListID = 0;
    
}// TSPITONEMONITOR::TSPITONEMONITOR

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPITONEMONITOR::~TSPITONEMONITOR
//
// Destructor for the tone monitor request object
//
TSPITONEMONITOR::~TSPITONEMONITOR()
{
    for (int i = 0; i < arrTones.GetSize(); i++)
    {
        LPLINEMONITORTONE lpTone = (LPLINEMONITORTONE) arrTones[i];
        delete lpTone;
    }
    arrTones.RemoveAll();
    
}// TSPITONEMONITOR::~TSPITONEMONITOR

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPILINESETTERMINAL::TSPILINESETTERMINAL
//
// Constructor for the TSPILINESETTERMINAL object request
//
TSPILINESETTERMINAL::TSPILINESETTERMINAL()
{                                           
    pLine = NULL;
    pAddress = NULL;
    pCall = NULL;
    dwTerminalModes = 0L;
    dwTerminalID = 0L;
    bEnable = FALSE;

}// TSPILINESETTERMINAL::TSPILINESETTERMINAL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::TSPIMEDIACONTROL
//
// Constructor for the TSPIMEDIACONTROL object request
//
TSPIMEDIACONTROL::TSPIMEDIACONTROL()
{                                     
    m_iRefCount = 0;
    
}// TSPIMEDIACONTROL::TSPIMEDIACONTROL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::~TSPIMEDIACONTROL
//
// Destructor for the TSPIMEDIACONTROL object request
//
TSPIMEDIACONTROL::~TSPIMEDIACONTROL()
{   
    ASSERT (m_iRefCount == 0);
    
    for (int i = 0; i < arrDigits.GetSize(); i++)
    {
        LPLINEMEDIACONTROLDIGIT lpMedia = (LPLINEMEDIACONTROLDIGIT) arrDigits[i];
        delete lpMedia;
    }

    for (i = 0; i < arrMedia.GetSize(); i++)
    {
        LPLINEMEDIACONTROLMEDIA lpMedia = (LPLINEMEDIACONTROLMEDIA) arrMedia[i];
        delete lpMedia;
    }

    for (i = 0; i < arrTones.GetSize(); i++)
    {
        LPLINEMEDIACONTROLTONE lpMedia = (LPLINEMEDIACONTROLTONE) arrTones[i];
        delete lpMedia;
    }

    for (i = 0; i < arrCallStates.GetSize(); i++)
    {
        LPLINEMEDIACONTROLCALLSTATE lpMedia = (LPLINEMEDIACONTROLCALLSTATE) arrCallStates[i];
        delete lpMedia;
    }
                                  
}// TSPIMEDIACONTROL::~TSPIMEDIACONTROL

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::IncUsage
//
// Increment our usage count
//
VOID TSPIMEDIACONTROL::IncUsage()
{                             
    m_iRefCount++;

}// TSPIMEDIACONTROL::IncUsage

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIMEDIACONTROL::DecUsage
//
// Decrement our usage count and delete when it hits zero
//
// WARNING: THIS OBJECT MUST BE ALLOCATED WITH "new"!!!
//
VOID TSPIMEDIACONTROL::DecUsage()
{                             
    m_iRefCount--;
    if (m_iRefCount <= 0)
        delete this;

}// TSPIMEDIACONTROL::DecUsage
  
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIHOOKSWITCHPARAM::TSPIHOOKSWITCHPARAM
//
// Constructor for the TSPIHOOKSWITCHPARAM object request
//
TSPIHOOKSWITCHPARAM::TSPIHOOKSWITCHPARAM()
{                                           
    dwHookSwitchDevice = 0L;
    dwParam = 0L;

}// TSPIHOOKSWITCHPARAM::TSPIHOOKSWITCHPARAM

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIRINGPATTERN::TSPIRINGPATTERN
//
// Constructor for the TSPIRINGPATTERN object request
//
TSPIRINGPATTERN::TSPIRINGPATTERN()
{                                   
    dwRingMode = 0L;
    dwVolume = 0L;

}// TSPIRINGPATTERN::TSPIRINGPATTERN

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPISETBUTTONINFO::TSPISETBUTTONINFO
//
// Constructor for the TSPISETBUTTONINFO object request
//
TSPISETBUTTONINFO::TSPISETBUTTONINFO()
{   
    dwButtonLampId = 0L;
    dwFunction = 0L;
    dwMode = 0L;
    strText = "";

}// TSPISETBUTTONINFO::TSPISETBUTTONINFO

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONESETDISPLAY::TSPIPHONESETDISPLAY
//
// Constructor for the TSPIPHONESETDISPLAY object request
//
TSPIPHONESETDISPLAY::TSPIPHONESETDISPLAY()
{                                           
    dwRow = 0;
    dwColumn = 0;
    lpvDisplay = NULL;
    dwSize = 0;

}// TSPIPHONESETDISPLAY::TSPIPHONESETDISPLAY
                      
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONESETDISPLAY::~TSPIPHONESETDISPLAY
//
// Destructor for the TSPIPHONESETDISPLAY object request
//
TSPIPHONESETDISPLAY::~TSPIPHONESETDISPLAY()
{                                           
    FreeMem (lpvDisplay);

}// TSPIPHONESETDISPLAY::~TSPIPHONESETDISPLAY
                      
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONEDATA::TSPIPHONEDATA
//
// Constructor for the TSPIPHONEDATA object request
//
TSPIPHONEDATA::TSPIPHONEDATA()
{                               
    dwDataID = 0L;
    lpBuffer = NULL;
    dwSize = 0L;

}// TSPIPHONEDATA::TSPIPHONEDATA

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// TSPIPHONEDATA::~TSPIPHONEDATA
//
// Destructor for the TSPIPHONEDATA object request
//
TSPIPHONEDATA::~TSPIPHONEDATA()
{                                
    if (lpBuffer)
        FreeMem (lpBuffer);

}// TSPIPHONEDATA::~TSPIPHONEDATA

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::CFlagArray
//
// Flag array constructor
//
CFlagArray::CFlagArray()
{ 
    /* Do nothing*/
    
}// CFlagArray::CFlagArray

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::operator[]
//
// Retrieve a bit from a specified position.
//
BOOL CFlagArray::operator[] (int nIndex) const
{
    return GetAt (nIndex);
    
}// CFlagArray::operator[]

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::GetAt
//
// Retrieve a bit from a specified position.
//
BOOL CFlagArray::GetAt (int nIndex) const
{    
    int nByteIndex = nIndex / 8;          
    nIndex -= (nByteIndex*8);
	
	if (nByteIndex >= m_arrValues.GetSize())
		return FALSE;

    BYTE bValue = m_arrValues[nByteIndex];
    return (BOOL) (bValue>>nIndex) & 0x1;
    
}// CFlagArray::GetAt

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CFlagArray::SetAt
//
// Set a bit in a particular position - add allocation bytes to the array
// if necessary.
//
void CFlagArray::SetAt (int nIndex, BOOL fFlag)
{
    int nByteIndex = nIndex / 8;          
    nIndex -= (nByteIndex*8);
    BYTE bValue = 0;
    
    if (nByteIndex < m_arrValues.GetSize())
        bValue = m_arrValues[nByteIndex];
    else
        m_arrValues.SetAtGrow (nByteIndex, 0);
        
    BYTE bMask = 0;     
    if (nIndex > 0)
        bMask |= (1 << nIndex);
    else
        bMask = 1;
    if (fFlag)
        bValue |= bMask;
    else
        bValue &= ~bMask;
    m_arrValues.SetAt (nByteIndex, bValue);                
    
}// CFlagArray::SetAt

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::CTSPIBaseObject
//
// Constructor
//
CTSPIBaseObject::CTSPIBaseObject()
{
	m_dwItemData = (DWORD) -1L;

}// CTSPIBaseObject::GetSyncObject

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::GetItemData
//
// Return the DWORD item data setup by the developer
//
DWORD CTSPIBaseObject::GetItemData() const
{
	return m_dwItemData;

}// CTSPIBaseObject::GetItemData

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::GetItemDataPtr
//
// Return the DWORD item data setup by the developer
//
void* CTSPIBaseObject::GetItemDataPtr() const
{
	return (void*) m_dwItemData;

}// CTSPIBaseObject::GetItemDataPtr

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::SetItemData
//
// Set the DWORD item data 
//
void CTSPIBaseObject::SetItemData(DWORD dwItem)
{
	m_dwItemData = dwItem;

}// CTSPIBaseObject::SetItemData

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// CTSPIBaseObject::SetItemDataPtr
//
// Set the DWORD item data
//
void CTSPIBaseObject::SetItemDataPtr(void* pItem)
{
	m_dwItemData = (DWORD) pItem;

}// CTSPIBaseObject::SetItemDataPtr

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// USERUSERINFO
// 
// User to User information structure
// 
USERUSERINFO::USERUSERINFO(LPVOID lpBuff, DWORD dwInSize) :
	dwSize(dwInSize)
{
	lpvBuff = AllocMem(dwSize);
	CopyBuffer(lpvBuff, lpBuff, dwSize);

}// USERUSERINFO::USERUSERINFO

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// ~USERUSERINFO
// 
// User to User information structure
// 
USERUSERINFO::~USERUSERINFO()
{
	FreeMem(lpvBuff);

}// USERUSERINFO::USERUSERINFO

