/******************************************************************************/
//                                                                        
// SPDLL.CPP - Service Provider DLL shell.                                
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This module intercepts the TSPI calls and invokes the SP object        
// with the appropriate parameters.                                       
//
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
//                                                                        
/******************************************************************************/

#include "stdafx.h"
#include <ctype.h>
#include "spuser.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Globals

int g_iRefCount;        // Global providerInit/providerShutdown reference counter

#ifdef _DEBUG
int g_iShowAPITraceLevel = 2;
#endif

// Standard method usable in Win32 and Win16 to cast a handle.
#define CASTHANDLE(h) ((DWORD)(UINT)h)

/******************************************************************************/
//
// Special DLL entrypoints for thread application connection
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// DeviceNotify
//
// This callback is used by the companion application to communicate
// with the service provider.  If the companion application is 32-bit,
// then this function is "thunked" to.  All interaction with the companion
// application comes through this function.
//
extern "C"
VOID WINAPI _export DeviceNotify(WORD wCommand, DWORD dwConnId, DWORD dwData, 
                                 LPVOID lpBuff, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel > 1)
	    TRACE("DeviceNotify: wCommand=0x%x, ConnID=0x%lX, dwData=0x%lx, lpBuff=0x%lx, Size=%ld\r\n", 
	                wCommand, dwConnId, dwData, (DWORD)lpBuff, dwSize);
#endif
	                
    switch(wCommand)
    {
        // Companion application has started, mark the task and hwnd.
        case RESULT_INIT:
            TRACE("Registering HTASK %04x, HWND %04x as thread task\r\n", (UINT)GetCurrentTask(), (UINT)dwData);
            GetSP()->InitializeRequestThread((HWND)dwData);
            break;
        
        // This is an interval timer coming from the thread application.
        case RESULT_INTERVALTIMER:
            GetSP()->IntervalTimer();
            break;
        
        // Service provider requested thread switch - pass to recieve data.
        case RESULT_CONTEXTSWITCH:
            lpBuff = NULL;
            dwSize = 0L;               
            dwData = STARTING_COMMAND;
            
            // Fall through intentional

        // This is data coming from one of our devices.
        case RESULT_RCVDATA:
            {   
                DWORD dwDeviceID = ((dwConnId & 0xffff0000)>>16);
                CTSPIDevice* pDevice = GetSP()->GetDevice(dwDeviceID);
                ASSERT (pDevice != NULL);
                if (pDevice != NULL)
                {
                    ASSERT (pDevice->IsKindOf (RUNTIME_CLASS(CTSPIDevice)));
                    pDevice->ReceiveData(dwConnId, dwData, lpBuff, dwSize);
                }
            }
            break;

        // Pass unknown command types to the service provider directly.            
        default:                                                       
            GetSP()->UnknownDeviceNotify (wCommand, dwConnId, dwData, lpBuff, dwSize);
            break;
    }

}// DeviceNotify

/******************************************************************************/
//
// PUBLIC UTILITY FUNCTIONS
//
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////
// AllocMem
//
// This function is globally used to allocate memory.  Most memory
// allocations performed within the library come through this single
// function.
//
LPVOID AllocMem (DWORD dwSize)
{   
    // Don't allow over a segment of allocation 
    if (dwSize == 0 || dwSize > 65535L)
        return NULL;
    
    LPSTR lpszBuff = NULL;
    TRY
    {                 
        lpszBuff = new char [dwSize];
    }
    CATCH (CMemoryException, me)
    {
        TRACE("MEMORY ALLOCATION FAILURE: Bytes=%ld", dwSize);
    }
    END_CATCH

    return lpszBuff;

}// AllocMem

////////////////////////////////////////////////////////////////////////////
// FreeMem
//
// This function is globally used to free memory.  It should be matched
// to the above function to insure that the proper memory conventions.
//
VOID FreeMem (LPVOID lpBuff)
{
	delete [] (LPSTR) lpBuff;

}// FreeMem

////////////////////////////////////////////////////////////////////////////
// CopyBuffer
//
// This function is used globally to transfer a block of memory from
// one area to another.
//
VOID CopyBuffer (LPVOID lpDest, LPCVOID lpSource, DWORD dwSize)
{
    memcpy (lpDest, lpSource, (size_t) dwSize);

}// CopyBuffer

////////////////////////////////////////////////////////////////////////////
// FillBuffer
//
// Initialize a buffer with a known value.
//
VOID FillBuffer (LPVOID lpDest, BYTE bValue, DWORD dwSize)
{             
    memset (lpDest, bValue, (size_t) dwSize);

}// FillBuffer

////////////////////////////////////////////////////////////////////////////
// ReportError
//
// Return whether the return code is an error and output it through the
// TRACE stream.
//
BOOL ReportError (LONG lResult)
{
    if (lResult >= 0)
        return FALSE;

    TRACE ("TAPI ERROR: 0x%lx\r\n", lResult);
    return TRUE;

}// ReportError

///////////////////////////////////////////////////////////////////////////
// CopyVarString
//
// Copy a buffer into a variable string buffer
//
void CopyVarString (LPVARSTRING lpVarString, LPCSTR lpszBuff)
{                
    // Validate the buffer
    if (CTSPIRequest::IsAddressOk(lpVarString, sizeof(VARSTRING)) && lpszBuff != NULL)
    {
        lpVarString->dwStringSize = lstrlen (lpszBuff)+1;
        lpVarString->dwNeededSize = sizeof(VARSTRING)+lpVarString->dwStringSize;
        lpVarString->dwStringFormat = STRINGFORMAT_ASCII;
        if (lpVarString->dwTotalSize >= lpVarString->dwNeededSize)
        {
            lpVarString->dwStringOffset = sizeof(VARSTRING);
            LPSTR lpBuff = (LPSTR)lpVarString + lpVarString->dwStringOffset;
            CopyBuffer (lpBuff, lpszBuff, lpVarString->dwStringSize);
        }
    }           

}// CopyVarString

///////////////////////////////////////////////////////////////////////////
// CopyVarString
//
// Copy a buffer into a variable string buffer
//
void CopyVarString (LPVARSTRING lpVarString, LPVOID lpBuff, DWORD dwSize)
{                
    // Validate the buffer
    if (CTSPIRequest::IsAddressOk(lpVarString, sizeof(VARSTRING)))
    {
        lpVarString->dwStringSize = dwSize;
        lpVarString->dwNeededSize = sizeof(VARSTRING)+lpVarString->dwStringSize;
        lpVarString->dwStringFormat = STRINGFORMAT_BINARY;
        if (lpVarString->dwTotalSize >= lpVarString->dwNeededSize)
        {
            lpVarString->dwStringOffset = sizeof(VARSTRING);
            LPSTR lpVS = (LPSTR)lpVarString + lpVarString->dwStringOffset;
            CopyBuffer (lpVS, lpBuff, dwSize);
        }
    }           

}// CopyVarString

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// DumpMem
//
// Dump out a block of memory in HEX for the specified amount of bytes
//
void DumpMem (LPCSTR lpszTitle, LPVOID lpBuff, DWORD dwSize)
{   
    if (lpBuff == NULL || dwSize == 0)
        return;
        
    if (lpszTitle)
        TRACE (lpszTitle);

    LPSTR lpByte = (LPSTR) lpBuff;
    DWORD dwCount = 0, dwLine = 0;
    while (dwCount < dwSize)
    {
        BYTE b[17];
        static char szBuff[256];
        
        // Grab this portion of the buffer.
        for (int i = 0; i < 16; i++)
        {
            if (dwSize-dwCount > 0)
            {
                b[i] = *lpByte++;
                dwCount++;
            }
            else
                b[i] = 0;
        }

        // Fill in the HEX portion
        wsprintf (szBuff, "%0.8lX   %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X "\
                             "%0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X    ", dwLine,
                           (int)b[0]&0xff, (int)b[1]&0xff, (int)b[2]&0xff, (int)b[3]&0xff, 
						   (int)b[4]&0xff, (int)b[5]&0xff, (int)b[6]&0xff, (int)b[7]&0xff, 
						   (int)b[8]&0xff, (int)b[9]&0xff, (int)b[10]&0xff,(int)b[11]&0xff, 
						   (int)b[12]&0xff,(int)b[13]&0xff,(int)b[14]&0xff,(int)b[15]&0xff);
        dwLine = dwCount;
                      
        // Now do the ASCII portion.
        for (i = 0; i < 16; i++)
        {
            if (!isprint(b[i]))
                b[i] = '.';
        }
        
        b[16] = '\0';
        strcat (szBuff, (char*)b);
        TRACE ("%s\r\n", szBuff);
    }

}// DumpMem

///////////////////////////////////////////////////////////////////////////
// DumpVarString
//
// Dump a VARSTRING buffer
//
void DumpVarString (LPVARSTRING lpVarString)
{                
    if (lpVarString == NULL)
        TRACE ("<NULL>\r\n");
    else
    {
        TRACE ("  dwTotalSize  = 0x%lX\r\n", lpVarString->dwTotalSize);
        TRACE ("  dwNeededSize = 0x%lX\r\n", lpVarString->dwNeededSize);
        TRACE ("  dwUsedSize   = 0x%lX\r\n", lpVarString->dwUsedSize);
        
        switch (lpVarString->dwStringFormat)
        {
            case STRINGFORMAT_ASCII:
                TRACE ("  Format       = STRINGFORMAT_ASCII\r\n");
                break;
            case STRINGFORMAT_BINARY:
                TRACE ("  Format       = STRINGFORMAT_BINARY\r\n");
                break;
            case STRINGFORMAT_DBCS:
                TRACE ("  Format       = STRINGFORMAT_DBCS\r\n");
                break;
            case STRINGFORMAT_UNICODE:
                TRACE ("  Format       = STRINGFORMAT_UNICODE\r\n");
                break;
            default:
                TRACE ("  Format       = UNKNOWN!\r\n");
                break;
        }     
        
        TRACE ("  dwStringSize = 0x%lX\r\n", lpVarString->dwStringSize);
        TRACE ("  dwStringOffset = 0x%lX\r\n", lpVarString->dwStringOffset);

        if (lpVarString->dwStringOffset > 0 && lpVarString->dwStringSize > 0)
        {
            if (lpVarString->dwStringFormat == STRINGFORMAT_ASCII)
            {
                TRACE ("  Value        = <%s>\r\n", (LPCSTR)((LPCSTR)lpVarString + lpVarString->dwStringOffset));
            }
            else
            {
                DumpMem ("  Value\r\n", (LPVOID)((LPCSTR)lpVarString + lpVarString->dwStringOffset), lpVarString->dwStringSize);
            }
        }                            
    }

}// DumpVarString

#endif // _DEBUG

/******************************************************************************/
//
// TSPIAPI TSPI_line functions
//
/******************************************************************************/

///////////////////////////////////////////////////////////////////////////
// TSPI_lineAccept
//
// This function accepts the specified offering call.  It may optionally
// send the specified User->User information to the calling party.
//
extern "C"
LONG TSPIAPI TSPI_lineAccept (DRV_REQUESTID dwRequestId, HDRVCALL hdCall,
         LPCSTR lpsUserUserInfo, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{
	   TRACE("TSPI_lineAccept beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestId);
	   TRACE("  UserInfo=0x%lx, Size=%ld\r\n", (DWORD)lpsUserUserInfo, dwSize);
	}
	if (g_iShowAPITraceLevel > 1)
	   DumpMem ("UserInfo->\r\n", (LPVOID)lpsUserUserInfo, dwSize);
#endif	
 
   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineAccept(pCall, dwRequestId, lpsUserUserInfo, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
		TRACE("TSPI_lineAccept rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineAccept

///////////////////////////////////////////////////////////////////////////
// TSPI_lineAddToConference
//
// This function adds the specified call (hdConsultCall) to the
// conference (hdConfCall).
//
extern "C"
LONG TSPIAPI TSPI_lineAddToConference (DRV_REQUESTID dwRequestId, 
         HDRVCALL hdConfCall, HDRVCALL hdConsultCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineAddToConference beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx\r\n", (DWORD) dwRequestId);
	   TRACE("  Conference=0x%lx, Call=0x%lx\r\n", (DWORD) hdConfCall, (DWORD) hdConsultCall);
	}
#endif

   CTSPIConferenceCall* pConf = (CTSPIConferenceCall*) hdConfCall;
   ASSERT(pConf && pConf->IsKindOf(RUNTIME_CLASS(CTSPIConferenceCall)));
   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdConsultCall;
   ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall && pConf)
      lResult = GetSP()->lineAddToConference(pConf, pCall, dwRequestId);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineAddToConference rc=0x%lx\r\n", lResult);
#endif

   return lResult;

}// TSPI_lineAddToConference

///////////////////////////////////////////////////////////////////////////
// TSPI_lineAnswer
//
// This function answers the specified offering call.
//
extern "C"
LONG TSPIAPI TSPI_lineAnswer (DRV_REQUESTID dwRequestId, HDRVCALL hdCall,
         LPCSTR lpsUserUserInfo, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineAnswer beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestId);
	   TRACE("  UserInfo=0x%lx, Size=%ld\r\n", (DWORD)lpsUserUserInfo, dwSize);
	}
	if (g_iShowAPITraceLevel > 1)
   		DumpMem ("UserInfo->\r\n", (LPVOID)lpsUserUserInfo, dwSize);
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineAnswer(pCall, dwRequestId, lpsUserUserInfo, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineAnswer rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineAnswer

///////////////////////////////////////////////////////////////////////////
// TSPI_lineBlindTransfer
//
// This function performs a blind or single-step transfer of the
// specified call to the specified destination address.
//
extern "C"
LONG TSPIAPI TSPI_lineBlindTransfer (DRV_REQUESTID dwRequestId,
         HDRVCALL hdCall, LPCSTR lpszDestAddr, DWORD dwCountryCode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineBlindTransfer beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestId);
	   TRACE("  DestAddr=<%s>, Country=0x%lx\r\n", lpszDestAddr, dwCountryCode);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineBlindTransfer(pCall, dwRequestId, lpszDestAddr, dwCountryCode);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineBlindTransfer rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_lineBlindTransfer

////////////////////////////////////////////////////////////////////////////
// TSPI_lineClose
//
// This function closes the specified open line after stopping all
// asynchronous requests on the line.
//
extern "C"
LONG TSPIAPI TSPI_lineClose (HDRVLINE hdLine)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineClose beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx\r\n", (DWORD)hdLine);
	}
#endif

   CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
   ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pLine)
      lResult = GetSP()->lineClose(pLine);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineClose rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineClose

////////////////////////////////////////////////////////////////////////////
// TSPI_lineCloseCall
//
// This function closes the specified call.  The HDRVCALL handle will
// no longer be valid after this call.
//
extern "C"
LONG TSPIAPI TSPI_lineCloseCall (HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineCloseCall beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx\r\n", (DWORD)hdCall);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineCloseCall(pCall);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineCloseCall rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineCloseCall

///////////////////////////////////////////////////////////////////////////
// TSPI_lineCompleteCall
//
// This function is used to specify how a call that could not be
// connected normally should be completed instead.  The network or
// switch may not be able to complete a call because the network
// resources are busy, or the remote station is busy or doesn't answer.
//
extern "C"
LONG TSPIAPI TSPI_lineCompleteCall (DRV_REQUESTID dwRequestId,
         HDRVCALL hdCall, LPDWORD lpdwCompletionID, DWORD dwCompletionMode,
         DWORD dwMessageID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineCompleteCall beginning\r\n");
	    TRACE("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestId);
	    TRACE("  Completion Mode=0x%lx, MessageID=0x%lx\r\n", dwCompletionMode, dwMessageID);
	}
#endif

    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    LONG lResult = 0;
    
    // Validate the call completion mode.
    if (dwCompletionMode != LINECALLCOMPLMODE_CAMPON &&
        dwCompletionMode != LINECALLCOMPLMODE_CALLBACK &&
        dwCompletionMode != LINECALLCOMPLMODE_INTRUDE &&
        dwCompletionMode != LINECALLCOMPLMODE_MESSAGE)
        lResult = LINEERR_INVALCALLCOMPLMODE;        
    
    // Pass it through the Service provider.
    else if (pCall != NULL)
    {
        lResult = GetSP()->lineCompleteCall(pCall, dwRequestId, lpdwCompletionID, 
                        dwCompletionMode, dwMessageID);
#ifdef _DEBUG
		if (g_iShowAPITraceLevel > 1)                        
        	TRACE("  CompletionID=0x%lx\r\n", *lpdwCompletionID);
#endif
    }
    else
        lResult = LINEERR_INVALCALLHANDLE;

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineCompleteCall rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineCompleteCall

///////////////////////////////////////////////////////////////////////////
// TSPI_lineCompleteTransfer
//
// This function completes the transfer of the specified call to the
// party connected in the consultation call.  If 'dwTransferMode' is
// LINETRANSFERMODE_CONFERENCE, the original call handle is changed
// to a conference call.  Otherwise, the service provider should send
// callstate messages change all the calls to idle.
//
extern "C"
LONG TSPIAPI TSPI_lineCompleteTransfer (DRV_REQUESTID dwRequestId,
         HDRVCALL hdCall, HDRVCALL hdConsultCall, HTAPICALL htConfCall,
         LPHDRVCALL lphdConfCall, DWORD dwTransferMode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineCompleteTransfer beginning\r\n");
	    TRACE("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestId);
	    TRACE("  ConsultCall=0x%lx, TAPIConfCall=0x%lx, XferMode=0x%lx\r\n", (DWORD)hdConsultCall, (DWORD)htConfCall, dwTransferMode);
	}
#endif

    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    CTSPICallAppearance* pConsult = (CTSPICallAppearance*) hdConsultCall;
    ASSERT(pConsult && pConsult->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

    LONG lResult = 0L;
    if (dwTransferMode != LINETRANSFERMODE_TRANSFER &&
        dwTransferMode != LINETRANSFERMODE_CONFERENCE)
        lResult = LINEERR_OPERATIONFAILED;
    else
    {
        if (pCall && pConsult)
        {
            lResult = GetSP()->lineCompleteTransfer(pCall, dwRequestId, pConsult, 
                                    htConfCall, lphdConfCall, dwTransferMode);
#ifdef _DEBUG
			if (lResult == 0 && g_iShowAPITraceLevel > 1 && lphdConfCall)
                TRACE("  SP ConfHandle=0x%lx\r\n", (DWORD) *lphdConfCall);
#endif
        }
        else
            lResult = LINEERR_INVALCALLHANDLE;
    }
    
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineCompleteTransfer rc=0x%lx\r\n", lResult);
#endif
    return lResult;
   
}// TSPI_lineCompleteTransfer   

////////////////////////////////////////////////////////////////////////////
// TSPI_lineConditionalMediaDetection
//
// This function is invoked by TAPI.DLL when the application requests a
// line open using the LINEMAPPER.  This function will check the 
// requested media modes and return an acknowledgement based on whether 
// we can monitor all the requested modes.
//
extern "C"
LONG TSPIAPI TSPI_lineConditionalMediaDetection (HDRVLINE hdLine,
         DWORD dwMediaModes, LPLINECALLPARAMS const lpCallParams)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineConditionalMediaDetection beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx, MediaModes=0x%lx\r\n", (DWORD)hdLine, dwMediaModes);
	}                                                                                     
	if (g_iShowAPITraceLevel > 1)
	{
	   TRACE("Dumping LINECALLPARAMS at %08lx\r\n", (DWORD)lpCallParams);
	   TRACE("----------------------------------------------------------------\r\n");
	   TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpCallParams->dwTotalSize);
	   TRACE("  dwBearerMode\t\t=0x%lx\r\n", lpCallParams->dwBearerMode);
	   TRACE("  dwMinRate\t\t=0x%lx\r\n", lpCallParams->dwMinRate);
	   TRACE("  dwMaxRate\t\t=0x%lx\r\n", lpCallParams->dwMaxRate);
	   TRACE("  dwMediaMode\t\t=0x%lx\r\n", lpCallParams->dwMediaMode);
	   TRACE("  dwCallParamFlags\t\t=0x%lx\r\n", lpCallParams->dwCallParamFlags);
	   TRACE("  dwAddressMode\t\t=0x%lx\r\n", lpCallParams->dwAddressMode);
	   TRACE("  dwAddressID\t\t=0x%lx\r\n", lpCallParams->dwAddressID);
	   TRACE("  dwOrigAddressSize\t\t=0x%lx\r\n", lpCallParams->dwOrigAddressSize);
	   TRACE("  dwOrigAddressOffset\t\t=0x%lx\r\n", lpCallParams->dwOrigAddressOffset);
	   TRACE("  dwDisplayableAddressSize\t\t=0x%lx\r\n", lpCallParams->dwDisplayableAddressSize);
	   TRACE("  dwDisplayableAddressOffset\t\t=0x%lx\r\n", lpCallParams->dwDisplayableAddressOffset);
	   TRACE("  dwCalledPartySize\t\t=0x%lx\r\n", lpCallParams->dwCalledPartySize);
	   TRACE("  dwCalledPartyOffset\t\t=0x%lx\r\n", lpCallParams->dwCalledPartyOffset);
	   TRACE("  dwCommentSize\t\t=0x%lx\r\n", lpCallParams->dwCommentSize);
	   TRACE("  dwCommentOffset\t\t=0x%lx\r\n", lpCallParams->dwCommentOffset);
	   TRACE("  dwUserUserInfoSize\t\t=0x%lx\r\n", lpCallParams->dwUserUserInfoSize);
	   TRACE("  dwUserUserInfoOffset\t\t=0x%lx\r\n", lpCallParams->dwUserUserInfoOffset);
	   TRACE("  dwHighLevelCompSize\t\t=0x%lx\r\n", lpCallParams->dwHighLevelCompSize);
	   TRACE("  dwHighLevelCompOffset\t\t=0x%lx\r\n", lpCallParams->dwHighLevelCompOffset);
	   TRACE("  dwLowLevelCompSize\t\t=0x%lx\r\n", lpCallParams->dwLowLevelCompSize);
	   TRACE("  dwLowLevelCompOffset\t\t=0x%lx\r\n", lpCallParams->dwLowLevelCompOffset);
	   TRACE("  dwDevSpecificSize\t\t=0x%lx\r\n", lpCallParams->dwDevSpecificSize);
	   TRACE("  dwDevSpecificOffset\t\t=0x%lx\r\n", lpCallParams->dwDevSpecificOffset);
	   TRACE("----------------------------------------------------------------\r\n");	
	}
#endif

   CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
   ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pLine)
      lResult = GetSP()->lineConditionalMediaDetection(pLine, dwMediaModes, lpCallParams);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineConditionalMediaDetection rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineConditionalMediaDetection

////////////////////////////////////////////////////////////////////////////
// TSPI_lineConfigDialog
//
// This function is called to display the line configuration dialog
// when the user requests it through either the TAPI api or the control
// panel applet.
//
extern "C"
LONG TSPIAPI TSPI_lineConfigDialog (DWORD dwDeviceID, HWND hwndOwner, LPCSTR lpszDeviceClass)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineConfigDialog beginning\r\n");
	   TRACE("  DeviceId=0x%lx, hwndOwner=%08lx\r\n", dwDeviceID, CASTHANDLE(hwndOwner));
	}
#endif

   CTSPILineConnection* pLine = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
   ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
   LONG lResult = LINEERR_BADDEVICEID;

   if (pLine)
      lResult = GetSP()->lineConfigDialog(pLine, CWnd::FromHandle(hwndOwner), CString(lpszDeviceClass));

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineConfigDialog rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineConfigDialog

///////////////////////////////////////////////////////////////////////////
// TSPI_lineConfigDialogEdit (Win95)
//
// This function causes the provider of the specified line device to
// display a modal dialog to allow the user to configure parameters
// related to the line device.  The parameters editted are NOT the
// current device parameters, rather the set retrieved from the
// 'TSPI_lineGetDevConfig' function (lpDeviceConfigIn), and are returned
// in the lpDeviceConfigOut parameter.
//
extern "C"
LONG TSPIAPI TSPI_lineConfigDialogEdit (DWORD dwDeviceID, HWND hwndOwner,
         LPCSTR lpszDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize,
         LPVARSTRING lpDeviceConfigOut)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineConfigDialogEdit beginning\r\n");
	    TRACE("  DeviceId=0x%lx, hwndOwner=%08lx\r\n", dwDeviceID, CASTHANDLE(hwndOwner));
	    TRACE("  DeviceClass=<%s>\r\n", lpszDeviceClass);
	    TRACE("  ConfigIn=0x%lx, Size=%ld\r\n", (DWORD)lpDeviceConfigIn, dwSize);
	}
	if (g_iShowAPITraceLevel > 1)
	    DumpMem ("lpConfigIn ->\r\n", lpDeviceConfigIn, dwSize);
#endif

    CTSPILineConnection* pLine = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
    ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    LONG lResult = LINEERR_BADDEVICEID;

    if (pLine)
    {
        lResult = GetSP()->lineConfigDialogEdit(pLine, CWnd::FromHandle(hwndOwner),
                                 CString(lpszDeviceClass), lpDeviceConfigIn, dwSize,
                                 lpDeviceConfigOut);
#ifdef _DEBUG
		if (lResult == 0 && g_iShowAPITraceLevel > 1)
		{                                              
	        TRACE ("lpDeviceConfigOut ->\r\n");
	        DumpVarString (lpDeviceConfigOut);
		}
#endif
                                 
    }
   
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineConfigDialogEdit rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineConfigDialogEdit

///////////////////////////////////////////////////////////////////////////
// TSPI_lineDevSpecific
//
// This function is used as a general extension mechanims to allow
// service providers to provide access to features not described in
// other operations.
//
extern "C"
LONG TSPIAPI TSPI_lineDevSpecific (DRV_REQUESTID dwRequestId, HDRVLINE hdLine,
         DWORD dwAddressId, HDRVCALL hdCall, LPVOID lpParams, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineDevSpecific beginning\r\n");
	    TRACE("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestId);
	    TRACE("  SP Line Handle=0x%lx, AddressId=0x%lx\r\n", (DWORD)hdLine, dwAddressId);
	    TRACE("  LPARAMS=0x%lx, size=%d\r\n", (DWORD) lpParams, dwSize);
	}
	if (g_iShowAPITraceLevel > 1)
	    DumpMem ("LPARAMS ->\r\n", lpParams, dwSize);
#endif

    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    CTSPIAddressInfo* pAddr = NULL;
    
    if (pLine != NULL)
        pAddr = pLine->GetAddress(dwAddressId);

    LONG lResult = GetSP()->lineDevSpecific(pLine, pAddr, pCall, dwRequestId, lpParams, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineDevSpecific rc=0x%lx\r\n", lResult);
#endif
    return lResult;
   
}// TSPI_lineDevSpecific

///////////////////////////////////////////////////////////////////////////
// TSPI_lineDevSpecificFeature
//
// This function is used as an extension mechanism to enable service
// providers to provide access to features not described in other
// operations.
//
extern "C"
LONG TSPIAPI TSPI_lineDevSpecificFeature (DRV_REQUESTID dwRequestId, HDRVLINE hdLine,
         DWORD dwFeature, LPVOID lpParams, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineDevSpecificFeature beginning\r\n");
	    TRACE("  SP Line Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdLine, (DWORD) dwRequestId);
	    TRACE("  Feature=0x%lx, LPARAMS=0x%lx, size=%d\r\n", (DWORD) dwFeature, (DWORD) lpParams, dwSize);
	}
	if (g_iShowAPITraceLevel > 1)
	    DumpMem ("LPARAMS ->\r\n", lpParams, dwSize);
#endif
    
    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
   
    LONG lResult = LINEERR_INVALLINEHANDLE;
    if (pLine)
        lResult = GetSP()->lineDevSpecificFeature(pLine, dwFeature, dwRequestId, lpParams, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineDevSpecificFeature rc=0x%lx\r\n", lResult);
#endif
    return lResult;
   
}// TSPI_lineDevSpecificFeature

///////////////////////////////////////////////////////////////////////////
// TSPI_lineDial
//
// This function dials the specified dialable number on the specified
// call device.
//
extern "C"
LONG TSPIAPI TSPI_lineDial (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
         LPCSTR lpszDestAddress, DWORD dwCountryCode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineDial beginning\r\n");
	    TRACE("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n", dwRequestID, (DWORD)hdCall);
	    TRACE("  Destination=<%s>, Country=%d\r\n", lpszDestAddress, dwCountryCode);	
	}
#endif

    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    LONG lResult = LINEERR_INVALCALLHANDLE;

    if (pCall)
        lResult = GetSP()->lineDial(pCall, dwRequestID, CString(lpszDestAddress), dwCountryCode);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	    TRACE("TSPI_lineDial rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineDial

////////////////////////////////////////////////////////////////////////////
// TSPI_lineDrop
//
// This function disconnects the specified call.  The call is still
// valid and should be closed by the application following this API.
//
extern "C"
LONG TSPIAPI TSPI_lineDrop (DRV_REQUESTID dwRequestID, HDRVCALL hdCall, 
         LPCSTR lpsUserUserInfo, DWORD dwSize)
{        
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineDrop beginning\r\n");
	    TRACE("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n", dwRequestID, (DWORD)hdCall);
	    TRACE("  UserInfo=%08lx, Size=%d\r\n", (DWORD)lpsUserUserInfo, dwSize);
	}
	if (g_iShowAPITraceLevel > 1)
   		DumpMem ("UserInfo->\r\n", (LPVOID)lpsUserUserInfo, dwSize);
#endif
        
    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    LONG lResult = LINEERR_INVALCALLHANDLE;

    if (pCall)
        lResult = GetSP()->lineDrop(pCall, dwRequestID, lpsUserUserInfo, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineDrop rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineDrop

///////////////////////////////////////////////////////////////////////////
// TSPI_lineDropOnClose (Win95)
//
// This function is called for each call owned by an application
// that calls the TAPI 'lineClose' function if, at the time of the call,
// the application is the sole owner of the call.
//
extern "C"
LONG TSPIAPI TSPI_lineDropOnClose (HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineDropOnClose beginning\r\n");
	    TRACE("  Call Handle=0x%lx\r\n", (DWORD) hdCall);
	}
#endif

    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    LONG lResult = LINEERR_INVALCALLHANDLE;

    if (pCall)
        lResult = GetSP()->lineDropOnClose(pCall);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	    TRACE("TSPI_lineDropOnClose rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineDropOnClose

///////////////////////////////////////////////////////////////////////////
// TSPI_lineDropNoOwner (Win95)
//
// This function is called when a new call is delivered by the SP to
// TAPI (via a LINE_NEWCALL) if no application can be found to become
// the owner of the call.  This can happen when one or monitors exist
// for the line, and the call is in a state other than idle or offering.
// This prevents calls from being present in the system for which no 
// application has responsibility to drop the call.
//
extern "C"
LONG TSPIAPI TSPI_lineDropNoOwner (HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineDropNoOwner beginning\r\n");
	    TRACE("  Call Handle=0x%lx\r\n", (DWORD) hdCall);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineDropNoOwner(pCall);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineDropNoOwner rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineDropNoOwner

///////////////////////////////////////////////////////////////////////////
// TSPI_lineForward
//
// This function forwards calls destined for the specified address on
// the specified line, according to the specified forwarding instructions.
// When an origination address is forwarded, the incoming calls for that
// address are deflected to the other number by the switch.  This function
// provides a combination of forward and do-not-disturb features.  This
// function can also cancel specific forwarding currently in effect.
//
extern "C"
LONG TSPIAPI TSPI_lineForward (DRV_REQUESTID dwRequestId, HDRVLINE hdLine,
         DWORD bAllAddresses, DWORD dwAddressId, 
         LPLINEFORWARDLIST const lpForwardList, DWORD dwNumRingsAnswer,
         HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall,
         LPLINECALLPARAMS const lpCallParams)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineForward beginning\r\n");
	    TRACE("  SP Line Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdLine, (DWORD) dwRequestId);
	    TRACE("  TAPI Call handle=0x%lx\r\n", (DWORD) htConsultCall);
	    TRACE("  All=%d, Address=0x%lx, NumRings=%d", bAllAddresses, dwAddressId, dwNumRingsAnswer);
	}
#endif

    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    ASSERT(pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    CTSPIAddressInfo* pAddr = NULL;

    LONG lResult = 0;
    if (pLine)
    {
        if (bAllAddresses == FALSE)
        {
            pAddr = pLine->GetAddress(dwAddressId);
            if (pAddr == NULL)
                lResult = LINEERR_INVALADDRESSID;
        }

        if (lResult == 0)
        {
            lResult = GetSP()->lineForward(pLine, pAddr, dwRequestId, lpForwardList,
                                     dwNumRingsAnswer, htConsultCall, lphdConsultCall,
                                     lpCallParams);
#ifdef DEBUG
			if (g_iShowAPITraceLevel > 1)
	            TRACE("  Returned SP call handle=0x%lx", *lphdConsultCall);
#endif	         
        }
    }
    else
        lResult = LINEERR_INVALLINEHANDLE;

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineForward rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineForward

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGatherDigits
//
// This function initiates the buffered gathering of digits on the 
// specified call.  TAPI.DLL specifies a buffer in which to place the digits,
// and the maximum number of digits to be collected.
//
// Digit collection may be terminated in the following ways:
//
//  (1)  The requested number of digits is collected.
//
//  (2)  One of the digits detected matches a digit in 'szTerminationDigits'
//       before the specified number of digits is collected.  The detected
//       termination digit is added to the buffer and the buffer is returned.
// 
//  (3)  One of the timeouts expires.  The 'dwFirstDigitTimeout' expires if
//       the first digit is not received in this time period.  The 
//       'dwInterDigitTimeout' expires if the second, third (and so on) digit
//       is not received within that time period, and a partial buffer is 
//       returned.
//
//  (4)  Calling this function again while digit gathering is in process.
//       The old collection session is terminated, and the contents is
//       undefined.  The mechanism for canceling without restarting this
//       event is to invoke this function with 'lpszDigits' equal to NULL.
//
extern "C"
LONG TSPIAPI TSPI_lineGatherDigits (HDRVCALL hdCall, DWORD dwEndToEnd,
         DWORD dwDigitModes, LPSTR lpszDigits, DWORD dwNumDigits,
         LPCSTR lpszTerminationDigits, DWORD dwFirstDigitTimeout,
         DWORD dwInterDigitTimeout)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGatherDigits beginning\r\n");
	   TRACE("  Call Handle=0x%lx, EndId=0x%lx\r\n", (DWORD) hdCall, dwEndToEnd);
	   TRACE("  Mode=0x%lx, Buff=%08lx, Size=%ld\r\n", dwDigitModes, lpszDigits, dwNumDigits);
	   TRACE("  Termination=<%s>, First TO=%ld, Inter TO=%ld\r\n", lpszTerminationDigits, dwFirstDigitTimeout, dwInterDigitTimeout);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineGatherDigits(pCall, dwEndToEnd, dwDigitModes, lpszDigits,
                           dwNumDigits, lpszTerminationDigits, dwFirstDigitTimeout, dwInterDigitTimeout);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGatherDigits rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineGatherDigits

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGenerateDigits
//
// This function initiates the generation of the specified digits
// using the specified signal mode.
//
extern "C"
LONG TSPIAPI TSPI_lineGenerateDigits (HDRVCALL hdCall, DWORD dwEndToEndID,
         DWORD dwDigitMode, LPCSTR lpszDigits, DWORD dwDuration)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGenerateDigits beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, EndToEndId=0x%lx\r\n", (DWORD)hdCall, dwEndToEndID);
	   TRACE("  DigitMode=0x%lx, Digits=<%s>, Duration=%d\r\n", dwDigitMode, lpszDigits, dwDuration);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineGenerateDigits(pCall, dwEndToEndID, dwDigitMode, lpszDigits, dwDuration);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGenerateDigits rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineGenerateDigits

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGenerateTone
//
// This function generates the specified tone inband over the specified
// call.  Invoking this function with a zero for 'dwToneMode' aborts any
// tone generation currently in progress on the specified call.
// Invoking 'lineGenerateTone' or 'lineGenerateDigit' also aborts the
// current tone generation and initiates the generation of the newly
// specified tone or digits.
//
extern "C"
LONG TSPIAPI TSPI_lineGenerateTone (HDRVCALL hdCall, DWORD dwEndToEnd,
         DWORD dwToneMode, DWORD dwDuration, DWORD dwNumTones,
         LPLINEGENERATETONE const lpTones)
{       
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGenerateTone beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, EndToEndId=0x%lx\r\n", (DWORD)hdCall, dwEndToEnd);
	   TRACE("  ToneMode=0x%lx, Duration=%ld Count=%ld\r\n", dwToneMode, dwDuration, dwNumTones);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;
   if (pCall)
      lResult = GetSP()->lineGenerateTone(pCall, dwEndToEnd, dwToneMode, 
               dwDuration, dwNumTones, lpTones);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGenerateTone rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_lineGenerateTone

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetAddressCaps
//
// This function queries the specified address on the specified
// line device to determine its telephony capabilities.
//
extern "C"
LONG TSPIAPI TSPI_lineGetAddressCaps (DWORD dwDeviceID, DWORD dwAddressID,
         DWORD dwTSPIVersion, DWORD dwExtVersion, LPLINEADDRESSCAPS lpAddressCaps)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineGetAddressCaps beginning\r\n");
	    TRACE("  DeviceId=0x%lx, AddressId=0x%lx, SP Version=0x%lx\r\n", dwDeviceID, dwAddressID, dwTSPIVersion);
	    TRACE("  ExtVersion=0x%lx, AddressCaps=%08lx\r\n", dwExtVersion, (DWORD)lpAddressCaps);
	}
#endif

    CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
    LONG lResult = LINEERR_BADDEVICEID;

    if (pConn)
    {
        CTSPIAddressInfo* pAddr = pConn->GetAddress(dwAddressID);
        if (pAddr == NULL)
            lResult = LINEERR_INVALADDRESSID;

        else
        {
            lResult = GetSP()->lineGetAddressCaps(pAddr, dwTSPIVersion, dwExtVersion, lpAddressCaps);

#ifdef _DEBUG
			if (lResult == 0 && g_iShowAPITraceLevel > 1)
			{                                              
	            TRACE("Dumping LINEADDRESSCAPS at %08lx\r\n", (DWORD)lpAddressCaps);
	            TRACE("----------------------------------------------------------------\r\n");
	            TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpAddressCaps->dwTotalSize);
	            TRACE("  dwNeededSize\t\t=0x%lx\r\n", lpAddressCaps->dwNeededSize);
	            TRACE("  dwUsedSize\t\t=0x%lx\r\n", lpAddressCaps->dwUsedSize);
	            TRACE("  dwLineDeviceID\t\t=0x%lx\r\n", lpAddressCaps->dwLineDeviceID);
	            TRACE("  dwAddressSize\t\t=0x%lx\r\n", lpAddressCaps->dwAddressSize);
	            TRACE("  dwAddressOffset\t\t=0x%lx\r\n", lpAddressCaps->dwAddressOffset);
	            TRACE("  dwDevSpecificSize\t\t=0x%lx\r\n", lpAddressCaps->dwDevSpecificSize);
	            TRACE("  dwDevSpecificOffset\t\t=0x%lx\r\n", lpAddressCaps->dwDevSpecificOffset);
	            TRACE("  dwAddressSharing\t\t=0x%lx\r\n", lpAddressCaps->dwAddressSharing);
	            TRACE("  dwAddressStates\t\t=0x%lx\r\n", lpAddressCaps->dwAddressStates);
	            TRACE("  dwCallInfoStates\t\t=0x%lx\r\n", lpAddressCaps->dwCallInfoStates);
	            TRACE("  dwCallerIDFlags\t\t=0x%lx\r\n", lpAddressCaps->dwCallerIDFlags);
	            TRACE("  dwCalledIDFlags\t\t=0x%lx\r\n", lpAddressCaps->dwCalledIDFlags);
	            TRACE("  dwConnectedIDFlags\t\t=0x%lx\r\n", lpAddressCaps->dwConnectedIDFlags);
	            TRACE("  dwRedirectionIDFlags\t\t=0x%lx\r\n", lpAddressCaps->dwRedirectionIDFlags);
	            TRACE("  dwRedirectingIDFlags\t\t=0x%lx\r\n", lpAddressCaps->dwRedirectingIDFlags);
	            TRACE("  dwCallStates\t\t=0x%lx\r\n", lpAddressCaps->dwCallStates);
	            TRACE("  dwDialToneModes\t\t=0x%lx\r\n", lpAddressCaps->dwDialToneModes);
	            TRACE("  dwBusyModes\t\t=0x%lx\r\n", lpAddressCaps->dwBusyModes);
	            TRACE("  dwSpecialInfo\t\t=0x%lx\r\n", lpAddressCaps->dwSpecialInfo);
	            TRACE("  dwDisconnectModes\t\t=0x%lx\r\n", lpAddressCaps->dwDisconnectModes);
	            TRACE("  dwMaxNumActiveCalls\t\t=0x%lx\r\n", lpAddressCaps->dwMaxNumActiveCalls);
	            TRACE("  dwMaxNumOnHoldCalls\t\t=0x%lx\r\n", lpAddressCaps->dwMaxNumOnHoldCalls);
	            TRACE("  dwMaxNumOnHoldPendingCalls\t\t=0x%lx\r\n", lpAddressCaps->dwMaxNumOnHoldPendingCalls);
	            TRACE("  dwMaxNumConference\t\t=0x%lx\r\n", lpAddressCaps->dwMaxNumConference);
	            TRACE("  dwMaxNumTransConf\t\t=0x%lx\r\n", lpAddressCaps->dwMaxNumTransConf);
	            TRACE("  dwAddrCapFlags\t\t=0x%lx\r\n", lpAddressCaps->dwAddrCapFlags);
	            TRACE("  dwCallFeatures\t\t=0x%lx\r\n", lpAddressCaps->dwCallFeatures);
	            TRACE("  dwRemoveFromConfCaps\t\t=0x%lx\r\n", lpAddressCaps->dwRemoveFromConfCaps);
	            TRACE("  dwRemoveFromConfState\t\t=0x%lx\r\n", lpAddressCaps->dwRemoveFromConfState);
	            TRACE("  dwTransferModes\t\t=0x%lx\r\n", lpAddressCaps->dwTransferModes);
	            TRACE("  dwParkModes\t\t=0x%lx\r\n", lpAddressCaps->dwParkModes);
	            TRACE("  dwForwardModes\t\t=0x%lx\r\n", lpAddressCaps->dwForwardModes);
	            TRACE("  dwMaxForwardEntries\t\t=0x%lx\r\n", lpAddressCaps->dwMaxForwardEntries);
	            TRACE("  dwMaxSpecificEntries\t\t=0x%lx\r\n", lpAddressCaps->dwMaxSpecificEntries);
	            TRACE("  dwMinFwdNumRings\t\t=0x%lx\r\n", lpAddressCaps->dwMinFwdNumRings);
	            TRACE("  dwMaxFwdNumRings\t\t=0x%lx\r\n", lpAddressCaps->dwMaxFwdNumRings);
	            TRACE("  dwMaxCallCompletions\t\t=0x%lx\r\n", lpAddressCaps->dwMaxCallCompletions);
	            TRACE("  dwCallCompletionConds\t\t=0x%lx\r\n", lpAddressCaps->dwCallCompletionConds);
	            TRACE("  dwCallCompletionModes\t\t=0x%lx\r\n", lpAddressCaps->dwCallCompletionModes);
	            TRACE("  dwNumCompletionMessages\t\t=0x%lx\r\n", lpAddressCaps->dwNumCompletionMessages);
	            TRACE("  dwCompletionMsgTextEntrySize\t\t=0x%lx\r\n", lpAddressCaps->dwCompletionMsgTextEntrySize);
	            TRACE("  dwCompletionMsgTextSize\t\t=0x%lx\r\n", lpAddressCaps->dwCompletionMsgTextSize);
	            TRACE("  dwCompletionMsgTextOffset\t\t=0x%lx\r\n", lpAddressCaps->dwCompletionMsgTextOffset);
	            TRACE("----------------------------------------------------------------\r\n");
			}
#endif
        }
    }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineGetAddressCaps rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineGetAddressCaps

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetAddressID
//
// This function returns the specified address associated with the
// specified line in a specified format.
//
extern "C"
LONG TSPIAPI TSPI_lineGetAddressID (HDRVLINE hdLine, LPDWORD lpdwAddressID, 
         DWORD dwAddressMode, LPCSTR lpsAddress, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetAddressID beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx, AddressId Buffer=%08lx\r\n", (DWORD)hdLine, (DWORD)lpdwAddressID);
	   TRACE("  AddressMode=0x%lx, Address Name=<%s>, Size=%ld\r\n", dwAddressMode, lpsAddress, dwSize);
	}
#endif

   CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
   ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn)
      lResult = GetSP()->lineGetAddressID(pConn, lpdwAddressID, dwAddressMode, lpsAddress, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetAddressID Id=0x%lx, rc=0x%lx\r\n", *lpdwAddressID, lResult);
#endif
   return lResult;

}// TSPI_lineGetAddressID

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetAddressStatus
//
// This function queries the specified address for its current status.
//
extern "C"
LONG TSPIAPI TSPI_lineGetAddressStatus (HDRVLINE hdLine, DWORD dwAddressID,
         LPLINEADDRESSSTATUS lpAddressStatus)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineGetAddressStatus beginning\r\n");
	    TRACE("  SP Line Handle=0x%lx, AddressID=0x%lx, lpAddressStatus=%08lx\r\n", (DWORD)hdLine, dwAddressID, lpAddressStatus);
	}
#endif

    CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
    LONG lResult = LINEERR_INVALLINEHANDLE;
    if (pConn)
    {   
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
        CTSPIAddressInfo* pAddr = pConn->GetAddress(dwAddressID);
        if (pAddr == NULL)
            lResult = LINEERR_INVALADDRESSID;
        else
        {
            lResult = GetSP()->lineGetAddressStatus(pAddr, lpAddressStatus);
#ifdef _DEBUG
			if (lResult == 0 && g_iShowAPITraceLevel > 1)
			{                                              
	            TRACE("Dumping LINEADDRESSSTATUS at %08lx\r\n", (DWORD)lpAddressStatus);
	            TRACE("----------------------------------------------------------------\r\n");
	            TRACE("  dwTotalSize\t\t=0%lx\r\n", lpAddressStatus->dwTotalSize);
	            TRACE("  dwNeededSize\t\t=0%lx\r\n", lpAddressStatus->dwNeededSize);
	            TRACE("  dwUsedSize\t\t=0%lx\r\n", lpAddressStatus->dwUsedSize);
	            TRACE("  dwNumInUse\t\t=0%lx\r\n", lpAddressStatus->dwNumInUse);
	            TRACE("  dwNumActiveCalls\t\t=0%lx\r\n", lpAddressStatus->dwNumActiveCalls);
	            TRACE("  dwNumOnHoldCalls\t\t=0%lx\r\n", lpAddressStatus->dwNumOnHoldCalls);
	            TRACE("  dwNumOnHoldPendCalls\t\t=0%lx\r\n", lpAddressStatus->dwNumOnHoldPendCalls);
	            TRACE("  dwAddressFeatures\t\t=0%lx\r\n", lpAddressStatus->dwAddressFeatures);
	            TRACE("  dwNumRingsNoAnswer\t\t=0%lx\r\n", lpAddressStatus->dwNumRingsNoAnswer);
	            TRACE("  dwForwardNumEntries\t\t=0%lx\r\n", lpAddressStatus->dwForwardNumEntries);
	            TRACE("  dwForwardSize\t\t=0%lx\r\n", lpAddressStatus->dwForwardSize);
	            TRACE("  dwForwardOffset\t\t=0%lx\r\n", lpAddressStatus->dwForwardOffset);
	            TRACE("  dwTerminalModesSize\t\t=0%lx\r\n", lpAddressStatus->dwTerminalModesSize);
	            TRACE("  dwTerminalModesOffset\t\t=0%lx\r\n", lpAddressStatus->dwTerminalModesOffset);
	            TRACE("  dwDevSpecificSize\t\t=0%lx\r\n", lpAddressStatus->dwDevSpecificSize);
	            TRACE("  dwDevSpecificOffset\t\t=0%lx\r\n", lpAddressStatus->dwDevSpecificOffset);
	            TRACE("----------------------------------------------------------------\r\n");
			}
#endif
        }
    }
    
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineGetAddressStatus rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineGetAddressStatus

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetCallAddressID
//
// This function retrieves the address for the specified call.
//
extern "C"
LONG TSPIAPI TSPI_lineGetCallAddressID (HDRVCALL hdCall, LPDWORD lpdwAddressID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetCallAddressID beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, AddressID Buff=%08lx\r\n", (DWORD)hdCall, (DWORD)lpdwAddressID);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineGetCallAddressID(pCall, lpdwAddressID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetCallAddressID AddressID=0x%lx, rc=0x%lx\r\n", *lpdwAddressID, lResult);
#endif
   return lResult;

}// TSPI_lineGetCallAddressID

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetCallInfo
//
// This function retrieves the telephony information for the specified
// call handle.
//
extern "C"
LONG TSPIAPI TSPI_lineGetCallInfo (HDRVCALL hdCall, LPLINECALLINFO lpCallInfo)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetCallInfo beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, lpCallInfo=%08lx\r\n", (DWORD)hdCall, (DWORD)lpCallInfo);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
   {
      lResult = GetSP()->lineGetCallInfo(pCall, lpCallInfo);
#ifdef _DEBUG
		if (lResult == 0 && g_iShowAPITraceLevel > 1)
		{                                              
	      TRACE("Dumping LINECALLINFO at %08lx\r\n", (DWORD)lpCallInfo);
	      TRACE("----------------------------------------------------------------\r\n");
	      TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpCallInfo->dwTotalSize);
	      TRACE("  dwNeededSize\t\t=0x%lx\r\n", lpCallInfo->dwNeededSize);
	      TRACE("  dwUsedSize\t\t=0x%lx\r\n", lpCallInfo->dwUsedSize);
	      TRACE("  hLine\t\t=0x%lx\r\n", (DWORD)lpCallInfo->hLine);
	      TRACE("  dwLineDeviceID\t\t=0x%lx\r\n", lpCallInfo->dwLineDeviceID);
	      TRACE("  dwAddressID\t\t=0x%lx\r\n", lpCallInfo->dwAddressID);
	      TRACE("  dwBearerMode\t\t=0x%lx\r\n", lpCallInfo->dwBearerMode);
	      TRACE("  dwRate\t\t=0x%lx\r\n", lpCallInfo->dwRate);
	      TRACE("  dwMediaMode\t\t=0x%lx\r\n", lpCallInfo->dwMediaMode);
	      TRACE("  dwAppSpecific\t\t=0x%lx\r\n", lpCallInfo->dwAppSpecific);
	      TRACE("  dwCallID\t\t=0x%lx\r\n", lpCallInfo->dwCallID);
	      TRACE("  dwRelatedCallID\t\t=0x%lx\r\n", lpCallInfo->dwRelatedCallID);
	      TRACE("  dwCallParamFlags\t\t=0x%lx\r\n", lpCallInfo->dwCallParamFlags);
	      TRACE("  dwCallStates\t\t=0x%lx\r\n", lpCallInfo->dwCallStates);
	      TRACE("  dwMonitorDigitModes\t\t=0x%lx\r\n", lpCallInfo->dwMonitorDigitModes);
	      TRACE("  dwMonitorMediaModes\t\t=0x%lx\r\n", lpCallInfo->dwMonitorMediaModes);
	      TRACE("  dwOrigin\t\t=0x%lx\r\n", lpCallInfo->dwOrigin);
	      TRACE("  dwReason\t\t=0x%lx\r\n", lpCallInfo->dwReason);
	      TRACE("  dwCompletionID\t\t=0x%lx\r\n", lpCallInfo->dwCompletionID);
	      TRACE("  dwNumOwners\t\t=0x%lx\r\n", lpCallInfo->dwNumOwners);
	      TRACE("  dwNumMonitors\t\t=0x%lx\r\n", lpCallInfo->dwNumMonitors);
	      TRACE("  dwCountryCode\t\t=0x%lx\r\n", lpCallInfo->dwCountryCode);
	      TRACE("  dwTrunk\t\t=0x%lx\r\n", lpCallInfo->dwTrunk);
	      TRACE("  dwCallerIDFlags\t\t=0x%lx\r\n", lpCallInfo->dwCallerIDFlags);
	      TRACE("  dwCallerIDSize\t\t=0x%lx\r\n", lpCallInfo->dwCallerIDSize);
	      TRACE("  dwCallerIDOffset\t\t=0x%lx\r\n", lpCallInfo->dwCallerIDOffset);
	      TRACE("  dwCallerIDNameSize\t\t=0x%lx\r\n", lpCallInfo->dwCallerIDNameSize);
	      TRACE("  dwCallerIDNameOffset\t\t=0x%lx\r\n", lpCallInfo->dwCallerIDNameOffset);
	      TRACE("  dwCalledIDFlags\t\t=0x%lx\r\n", lpCallInfo->dwCalledIDFlags);
	      TRACE("  dwCalledIDSize\t\t=0x%lx\r\n", lpCallInfo->dwCalledIDSize);
	      TRACE("  dwCalledIDOffset\t\t=0x%lx\r\n", lpCallInfo->dwCalledIDOffset);
	      TRACE("  dwCalledIDNameSize\t\t=0x%lx\r\n", lpCallInfo->dwCalledIDNameSize);
	      TRACE("  dwCalledIDNameOffset\t\t=0x%lx\r\n", lpCallInfo->dwCalledIDNameOffset);
	      TRACE("  dwConnectedIDFlags\t\t=0x%lx\r\n", lpCallInfo->dwConnectedIDFlags);
	      TRACE("  dwConnectedIDSize\t\t=0x%lx\r\n", lpCallInfo->dwConnectedIDSize);
	      TRACE("  dwConnectedIDOffset\t\t=0x%lx\r\n", lpCallInfo->dwConnectedIDOffset);
	      TRACE("  dwConnectedIDNameSize\t\t=0x%lx\r\n", lpCallInfo->dwConnectedIDNameSize);
	      TRACE("  dwConnectedIDNameOffset\t\t=0x%lx\r\n", lpCallInfo->dwConnectedIDNameOffset);
	      TRACE("  dwRedirectionIDFlags\t\t=0x%lx\r\n", lpCallInfo->dwRedirectionIDFlags);
	      TRACE("  dwRedirectionIDSize\t\t=0x%lx\r\n", lpCallInfo->dwRedirectionIDSize);
	      TRACE("  dwRedirectionIDOffset\t\t=0x%lx\r\n", lpCallInfo->dwRedirectionIDOffset);
	      TRACE("  dwRedirectionIDNameSize\t\t=0x%lx\r\n", lpCallInfo->dwRedirectionIDNameSize);
	      TRACE("  dwRedirectionIDNameOffset\t\t=0x%lx\r\n", lpCallInfo->dwRedirectionIDNameOffset);
	      TRACE("  dwRedirectingIDFlags\t\t=0x%lx\r\n", lpCallInfo->dwRedirectingIDFlags);
	      TRACE("  dwRedirectingIDSize\t\t=0x%lx\r\n", lpCallInfo->dwRedirectingIDSize);
	      TRACE("  dwRedirectingIDOffset\t\t=0x%lx\r\n", lpCallInfo->dwRedirectingIDOffset);
	      TRACE("  dwRedirectingIDNameSize\t\t=0x%lx\r\n", lpCallInfo->dwRedirectingIDNameSize);
	      TRACE("  dwRedirectingIDNameOffset\t\t=0x%lx\r\n", lpCallInfo->dwRedirectingIDNameOffset);
	      TRACE("  dwAppNameSize\t\t=0x%lx\r\n", lpCallInfo->dwAppNameSize);
	      TRACE("  dwAppNameOffset\t\t=0x%lx\r\n", lpCallInfo->dwAppNameOffset);
	      TRACE("  dwDisplayableAddressSize\t\t=0x%lx\r\n", lpCallInfo->dwDisplayableAddressSize);
	      TRACE("  dwDisplayableAddressOffset\t\t=0x%lx\r\n", lpCallInfo->dwDisplayableAddressOffset);
	      TRACE("  dwCalledPartySize\t\t=0x%lx\r\n", lpCallInfo->dwCalledPartySize);
	      TRACE("  dwCalledPartyOffset\t\t=0x%lx\r\n", lpCallInfo->dwCalledPartyOffset);
	      TRACE("  dwCommentSize\t\t=0x%lx\r\n", lpCallInfo->dwCommentSize);
	      TRACE("  dwCommentOffset\t\t=0x%lx\r\n", lpCallInfo->dwCommentOffset);
	      TRACE("  dwDisplaySize\t\t=0x%lx\r\n", lpCallInfo->dwDisplaySize);
	      TRACE("  dwDisplayOffset\t\t=0x%lx\r\n", lpCallInfo->dwDisplayOffset);
	      TRACE("  dwUserUserInfoSize\t\t=0x%lx\r\n", lpCallInfo->dwUserUserInfoSize);
	      TRACE("  dwUserUserInfoOffset\t\t=0x%lx\r\n", lpCallInfo->dwUserUserInfoOffset);
	      TRACE("  dwHighLevelCompSize\t\t=0x%lx\r\n", lpCallInfo->dwHighLevelCompSize);
	      TRACE("  dwHighLevelCompOffset\t\t=0x%lx\r\n", lpCallInfo->dwHighLevelCompOffset);
	      TRACE("  dwLowLevelCompSize\t\t=0x%lx\r\n", lpCallInfo->dwLowLevelCompSize);
	      TRACE("  dwLowLevelCompOffset\t\t=0x%lx\r\n", lpCallInfo->dwLowLevelCompOffset);
	      TRACE("  dwChargingInfoSize\t\t=0x%lx\r\n", lpCallInfo->dwChargingInfoSize);
	      TRACE("  dwChargingInfoOffset\t\t=0x%lx\r\n", lpCallInfo->dwChargingInfoOffset);
	      TRACE("  dwTerminalModesSize\t\t=0x%lx\r\n", lpCallInfo->dwTerminalModesSize);
	      TRACE("  dwTerminalModesOffset\t\t=0x%lx\r\n", lpCallInfo->dwTerminalModesOffset);
	      TRACE("  dwDevSpecificSize\t\t=0x%lx\r\n", lpCallInfo->dwDevSpecificSize);
	      TRACE("  dwDevSpecificOffset\t\t=0x%lx\r\n", lpCallInfo->dwDevSpecificOffset);
	      TRACE("----------------------------------------------------------------\r\n");
		}
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetCallInfo rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineGetCallInfo

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetCallStatus
//
// This function retrieves the status for the specified call.
//
extern "C"
LONG TSPIAPI TSPI_lineGetCallStatus (HDRVCALL hdCall, LPLINECALLSTATUS lpCallStatus)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetCallStatus beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, lpCallStatus=%08lx\r\n", (DWORD) hdCall, (DWORD) lpCallStatus);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
   {
      lResult = GetSP()->lineGetCallStatus(pCall, lpCallStatus);
#ifdef _DEBUG
		if (lResult == 0 && g_iShowAPITraceLevel > 1)
		{                                              
	      TRACE("Dumping LINECALLSTATUS at %08lx\r\n", (DWORD)lpCallStatus);
	      TRACE("----------------------------------------------------------------\r\n");
	      TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpCallStatus->dwTotalSize);
	      TRACE("  dwNeededSize\t\t=0x%lx\r\n", lpCallStatus->dwNeededSize);
	      TRACE("  dwUsedSize\t\t=0x%lx\r\n", lpCallStatus->dwUsedSize);
	      TRACE("  dwCallState\t\t=0x%lx\r\n", lpCallStatus->dwCallState);
	      TRACE("  dwCallStateMode\t\t=0x%lx\r\n", lpCallStatus->dwCallStateMode);
	      TRACE("  dwCallPrivilege\t\t=0x%lx\r\n", lpCallStatus->dwCallPrivilege);
	      TRACE("  dwCallFeatures\t\t=0x%lx\r\n", lpCallStatus->dwCallFeatures);
	      TRACE("  dwDevSpecificSize\t\t=0x%lx\r\n", lpCallStatus->dwDevSpecificSize);
	      TRACE("  dwDevSpecificOffset\t\t=0x%lx\r\n", lpCallStatus->dwDevSpecificOffset);
	      TRACE("----------------------------------------------------------------\r\n");
		}
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetCallStatus rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineGetCallStatus

///////////////////////////////////////////////////////////////////////////
// TSPI_lineGetDevCaps
//
// This function retrieves the telephony device capabilties for the
// specified line.  This information is valid for all addresses on 
// the line.
//
extern "C"
LONG TSPIAPI TSPI_lineGetDevCaps (DWORD dwDeviceID, DWORD dwTSPIVersion, 
         DWORD dwExtVersion, LPLINEDEVCAPS lpLineDevCaps)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetDevCaps beginning\r\n");
	   TRACE("  DeviceId=0x%lx, SP Version=0x%lx, Ext Version=0x%lx\r\n", dwDeviceID, dwTSPIVersion, dwExtVersion);
	   TRACE("  lpLineDevCaps=%08lx\r\n", (DWORD) lpLineDevCaps);
	}
#endif

   CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);

   LONG lResult = LINEERR_BADDEVICEID;

   if (pConn)
   {
      lResult = GetSP()->lineGetDevCaps(pConn, dwTSPIVersion, dwExtVersion, lpLineDevCaps);
#ifdef _DEBUG
		if (lResult == 0 && g_iShowAPITraceLevel > 1)
		{                                              
	      TRACE("Dumping LINEDEVCAPS at %08lx\r\n", (DWORD) lpLineDevCaps);
	      TRACE("----------------------------------------------------------------\r\n");
	      TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpLineDevCaps->dwTotalSize);
	      TRACE("  dwNeededSize\t\t=0x%lx\r\n", lpLineDevCaps->dwNeededSize);
	      TRACE("  dwUsedSize\t\t=0x%lx\r\n", lpLineDevCaps->dwUsedSize);
	      TRACE("  dwProviderInfoSize\t\t=0x%lx\r\n", lpLineDevCaps->dwProviderInfoSize);
	      TRACE("  dwProviderInfoOffset\t\t=0x%lx\r\n", lpLineDevCaps->dwProviderInfoOffset);
	      TRACE("  dwSwitchInfoSize\t\t=0x%lx\r\n", lpLineDevCaps->dwSwitchInfoSize);
	      TRACE("  dwSwitchInfoOffset\t\t=0x%lx\r\n", lpLineDevCaps->dwSwitchInfoOffset);
	      TRACE("  dwPermanentLineID\t\t=0x%lx\r\n", lpLineDevCaps->dwPermanentLineID);
	      TRACE("  dwLineNameSize\t\t=0x%lx\r\n", lpLineDevCaps->dwLineNameSize);
	      TRACE("  dwLineNameOffset\t\t=0x%lx\r\n", lpLineDevCaps->dwLineNameOffset);
	      TRACE("  dwStringFormat\t\t=0x%lx\r\n", lpLineDevCaps->dwStringFormat);
	      TRACE("  dwAddressModes\t\t=0x%lx\r\n", lpLineDevCaps->dwAddressModes);
	      TRACE("  dwNumAddresses\t\t=0x%lx\r\n", lpLineDevCaps->dwNumAddresses);
	      TRACE("  dwBearerModes\t\t=0x%lx\r\n", lpLineDevCaps->dwBearerModes);
	      TRACE("  dwMaxRate\t\t=0x%lx\r\n", lpLineDevCaps->dwMaxRate);
	      TRACE("  dwMediaModes\t\t=0x%lx\r\n", lpLineDevCaps->dwMediaModes);
	      TRACE("  dwGenerateToneModes\t\t=0x%lx\r\n", lpLineDevCaps->dwGenerateToneModes);
	      TRACE("  dwGenerateToneMaxNumFreq\t\t=0x%lx\r\n", lpLineDevCaps->dwGenerateToneMaxNumFreq);
	      TRACE("  dwGenerateDigitModes\t\t=0x%lx\r\n", lpLineDevCaps->dwGenerateDigitModes);
	      TRACE("  dwMonitorToneMaxNumFreq\t\t=0x%lx\r\n", lpLineDevCaps->dwMonitorToneMaxNumFreq);
	      TRACE("  dwMonitorToneMaxNumEntries\t\t=0x%lx\r\n", lpLineDevCaps->dwMonitorToneMaxNumEntries);
	      TRACE("  dwMonitorDigitModes\t\t=0x%lx\r\n", lpLineDevCaps->dwMonitorDigitModes);
	      TRACE("  dwGatherDigitsMinTimeout\t\t=0x%lx\r\n", lpLineDevCaps->dwGatherDigitsMinTimeout);
	      TRACE("  dwGatherDigitsMaxTimeout\t\t=0x%lx\r\n", lpLineDevCaps->dwGatherDigitsMaxTimeout);
	      TRACE("  dwMedCtlDigitMaxListSize\t\t=0x%lx\r\n", lpLineDevCaps->dwMedCtlDigitMaxListSize);
	      TRACE("  dwMedCtlMediaMaxListSize\t\t=0x%lx\r\n", lpLineDevCaps->dwMedCtlMediaMaxListSize);
	      TRACE("  dwMedCtlToneMaxListSize\t\t=0x%lx\r\n", lpLineDevCaps->dwMedCtlToneMaxListSize);
	      TRACE("  dwMedCtlCallStateMaxListSize\t\t=0x%lx\r\n", lpLineDevCaps->dwMedCtlCallStateMaxListSize);
	      TRACE("  dwDevCapFlags\t\t=0x%lx\r\n", lpLineDevCaps->dwDevCapFlags);
	      TRACE("  dwMaxNumActiveCalls\t\t=0x%lx\r\n", lpLineDevCaps->dwMaxNumActiveCalls);
	      TRACE("  dwAnswerMode\t\t=0x%lx\r\n", lpLineDevCaps->dwAnswerMode);
	      TRACE("  dwRingModes\t\t=0x%lx\r\n", lpLineDevCaps->dwRingModes);
	      TRACE("  dwLineStates\t\t=0x%lx\r\n", lpLineDevCaps->dwLineStates);
	      TRACE("  dwUUIAcceptSize\t\t=0x%lx\r\n", lpLineDevCaps->dwUUIAcceptSize);
	      TRACE("  dwUUIAnswerSize\t\t=0x%lx\r\n", lpLineDevCaps->dwUUIAnswerSize);
	      TRACE("  dwUUIMakeCallSize\t\t=0x%lx\r\n", lpLineDevCaps->dwUUIMakeCallSize);
	      TRACE("  dwUUIDropSize\t\t=0x%lx\r\n", lpLineDevCaps->dwUUIDropSize);
	      TRACE("  dwUUISendUserUserInfoSize\t\t=0x%lx\r\n", lpLineDevCaps->dwUUISendUserUserInfoSize);
	      TRACE("  dwUUICallInfoSize\t\t=0x%lx\r\n", lpLineDevCaps->dwUUICallInfoSize);
	      TRACE("  dwNumTerminals\t\t=0x%lx\r\n", lpLineDevCaps->dwNumTerminals);
	      TRACE("  dwTerminalCapsSize\t\t=0x%lx\r\n", lpLineDevCaps->dwTerminalCapsSize);
	      TRACE("  dwTerminalCapsOffset\t\t=0x%lx\r\n", lpLineDevCaps->dwTerminalCapsOffset);
	      TRACE("  dwTerminalTextEntrySize\t\t=0x%lx\r\n", lpLineDevCaps->dwTerminalTextEntrySize);
	      TRACE("  dwTerminalTextSize\t\t=0x%lx\r\n", lpLineDevCaps->dwTerminalTextSize);
	      TRACE("  dwTerminalTextOffset\t\t=0x%lx\r\n", lpLineDevCaps->dwTerminalTextOffset);
	      TRACE("  dwDevSpecificSize\t\t=0x%lx\r\n", lpLineDevCaps->dwDevSpecificSize);
	      TRACE("  dwDevSpecificOffset\t\t=0x%lx\r\n", lpLineDevCaps->dwDevSpecificOffset);
	      TRACE("----------------------------------------------------------------\r\n");
		}
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetDevCaps rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineGetDevCaps

//////////////////////////////////////////////////////////////////////////
// TSPI_lineGetDevConfig
//
// This function returns a data structure object, the contents of which
// are specific to the line (SP) and device class, giving the current
// configuration of a device associated one-to-one with the line device.
//
extern "C"
LONG TSPIAPI TSPI_lineGetDevConfig (DWORD dwDeviceID, LPVARSTRING lpDeviceConfig,
         LPCSTR lpszDeviceClass)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetDevConfig beginning\r\n");
	   TRACE("  DeviceId=0x%lx, Class=<%s>\r\n", dwDeviceID, lpszDeviceClass);
	}
#endif

   CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);

   LONG lResult = LINEERR_BADDEVICEID;

   if (pConn)
      lResult = GetSP()->lineGetDevConfig(pConn, CString(lpszDeviceClass), lpDeviceConfig);
   
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetDevConfig rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineGetDevConfig

//////////////////////////////////////////////////////////////////////////
// TSPI_lineGetExtensionID
//
// This function returns the extension ID that the service provider
// supports for the indicated line device.
//
extern "C"
LONG TSPIAPI TSPI_lineGetExtensionID (DWORD dwDeviceID, DWORD dwTSPIVersion,
         LPLINEEXTENSIONID lpExtensionID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetExtensionID beginning\r\n");
	   TRACE("  DeviceId=0x%lx, TSPI Ver=0x%lx\r\n", dwDeviceID, dwTSPIVersion);
	}
#endif

   CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);

   LONG lResult = LINEERR_BADDEVICEID;

   if (pConn)
   {
      lResult = GetSP()->lineGetExtensionID(pConn, dwTSPIVersion, lpExtensionID);
#ifdef _DEBUG
		if (lResult == 0 && g_iShowAPITraceLevel > 1)
		{                                              
	      TRACE("Dumping LINEEXTENSIONID at %08lx\r\n", lpExtensionID);
	      TRACE("----------------------------------------------------------------\r\n");
	      TRACE("  dwExtensionID0\t=0x%lx\r\n", lpExtensionID->dwExtensionID0);
	      TRACE("  dwExtensionID1\t=0x%lx\r\n", lpExtensionID->dwExtensionID1);
	      TRACE("  dwExtensionID2\t=0x%lx\r\n", lpExtensionID->dwExtensionID2);
	      TRACE("  dwExtensionID3\t=0x%lx\r\n", lpExtensionID->dwExtensionID3);
	      TRACE("----------------------------------------------------------------\r\n");
		}
#endif
   }
   
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetExtensionID rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_lineGetExtensionID

//////////////////////////////////////////////////////////////////////////
// TSPI_lineGetIcon
//
// This function retreives a service line device-specific icon for
// display to the user
//
extern "C"
LONG TSPIAPI TSPI_lineGetIcon (DWORD dwDeviceID, LPCSTR lpszDeviceClass,
      LPHICON lphIcon)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetIcon beginning\r\n");
	   TRACE("  DeviceId=0x%lx, Class=<%s>, Buff=%08lx\r\n", dwDeviceID, lpszDeviceClass, (DWORD)lphIcon);
	}
#endif

   CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);

   LONG lResult = LINEERR_BADDEVICEID;

   if (pConn)
      lResult = GetSP()->lineGetIcon(pConn, CString(lpszDeviceClass), lphIcon);
   
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetIcon hIcon=%08lx, rc=0x%lx\r\n", CASTHANDLE(*lphIcon), lResult);
#endif
   return lResult;
   
}// TSPI_lineGetIcon

//////////////////////////////////////////////////////////////////////////
// TSPI_lineGetID
//
// This function returns a device id for the specified
// device class associated with the specified line, address, or call
// handle.
//
extern "C"
LONG TSPIAPI TSPI_lineGetID (HDRVLINE hdLine, DWORD dwAddressID,
         HDRVCALL hdCall, DWORD dwSelect, LPVARSTRING lpVarString,
         LPCSTR lpszDeviceClass)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineGetID beginning\r\n");
	    TRACE("  SP Line Handle=0x%lx, AddressID=0x%lx\r\n", (DWORD) hdLine, dwAddressID);
	    TRACE("  SP Call Handle=0x%lx, Select=%ld, Buffer=%08lx\r\n", (DWORD)hdCall, dwSelect, (DWORD)lpVarString);
	    TRACE("  DeviceClass=<%s>\r\n", lpszDeviceClass);
	}
#endif
    
    CTSPILineConnection* pConn = NULL;
    CTSPICallAppearance* pCall = NULL;
    CTSPIAddressInfo* pAddr = NULL;

    // Check how to find this connection info.  Based on the
    // information passed locate the proper connection info.
    LONG lResult = 0;

    switch(dwSelect)
    {
        case LINECALLSELECT_LINE:
            pConn = (CTSPILineConnection*) hdLine;
            ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
            break;

        case LINECALLSELECT_ADDRESS:
            pConn = (CTSPILineConnection*) hdLine;
            if (pConn != NULL)
            {
                ASSERT (pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
                pAddr = pConn->GetAddress(dwAddressID);
                if (pAddr != NULL)
                    ASSERT (pAddr->IsKindOf(RUNTIME_CLASS(CTSPIAddressInfo)));
                else
                    lResult = LINEERR_INVALADDRESSID;
            }       
            else
                lResult = LINEERR_INVALLINEHANDLE;
            break;

        case LINECALLSELECT_CALL:
            pCall = (CTSPICallAppearance*) hdCall;
            if (pCall != NULL)
            {
                ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
                pAddr = pCall->GetAddressInfo();
                pConn = pAddr->GetLineOwner();
            }
            else
                lResult = LINEERR_INVALCALLHANDLE;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    if (lResult == 0)
    {
        CString strName = lpszDeviceClass;
        strName.MakeLower();

        lResult = GetSP()->lineGetID(pConn, pAddr, pCall, strName, lpVarString);
#ifdef _DEBUG
		if (lResult == 0 && g_iShowAPITraceLevel > 1)
		{                                              
	        TRACE("Dumping VARSTRING at %08lx\r\n", (DWORD)lpVarString);
	        TRACE("----------------------------------------------------------------\r\n");
	        TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpVarString->dwTotalSize);
	        TRACE("  dwNeededSize\t\t=0x%lx\r\n", lpVarString->dwNeededSize);
	        TRACE("  dwUsedSize\t\t=0x%lx\r\n", lpVarString->dwUsedSize);
	        TRACE("  dwStringFormat\t\t=0x%lx\r\n", lpVarString->dwStringFormat);
	        TRACE("  dwStringSize\t\t=0x%lx\r\n", lpVarString->dwStringSize);
	        TRACE("  dwStringOffset\t\t=0x%lx\r\n", lpVarString->dwStringOffset);
	        TRACE("----------------------------------------------------------------\r\n");
		}
#endif
    }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineGetID rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineGetID

////////////////////////////////////////////////////////////////////////////
// TSPI_lineGetLineDevStatus
//
// This function queries the specified open line for its status.  The
// information is valid for all addresses on the line.
//
extern "C"
LONG TSPIAPI TSPI_lineGetLineDevStatus (HDRVLINE hdLine, LPLINEDEVSTATUS lpLineDevStatus)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetLineDevStatus beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx, lpLineDevStatus=%08lx\r\n", (DWORD)hdLine, (DWORD)lpLineDevStatus);
	}
#endif

   CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
   ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn)
   {
      lResult = GetSP()->lineGetLineDevStatus(pConn, lpLineDevStatus);
#ifdef _DEBUG
		if (lResult == 0 && g_iShowAPITraceLevel > 1)
		{                                              
	      TRACE("Dumping LINEDEVSTATUS at %08lx\r\n", (DWORD) lpLineDevStatus);
	      TRACE("----------------------------------------------------------------\r\n");
	      TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpLineDevStatus->dwTotalSize);
	      TRACE("  dwNeededSize\t\t=0x%lx\r\n", lpLineDevStatus->dwNeededSize);
	      TRACE("  dwUsedSize\t\t=0x%lx\r\n", lpLineDevStatus->dwUsedSize);
	      TRACE("  dwNumOpens\t\t=0x%lx\r\n", lpLineDevStatus->dwNumOpens);
	      TRACE("  dwOpenMediaModes\t\t=0x%lx\r\n", lpLineDevStatus->dwOpenMediaModes);
	      TRACE("  dwNumActiveCalls\t\t=0x%lx\r\n", lpLineDevStatus->dwNumActiveCalls);
	      TRACE("  dwNumOnHoldCalls\t\t=0x%lx\r\n", lpLineDevStatus->dwNumOnHoldCalls);
	      TRACE("  dwNumOnHoldPendCalls\t\t=0x%lx\r\n", lpLineDevStatus->dwNumOnHoldPendCalls);
	      TRACE("  dwLineFeatures\t\t=0x%lx\r\n", lpLineDevStatus->dwLineFeatures);
	      TRACE("  dwNumCallCompletions\t\t=0x%lx\r\n", lpLineDevStatus->dwNumCallCompletions);
	      TRACE("  dwRingMode\t\t=0x%lx\r\n", lpLineDevStatus->dwRingMode);
	      TRACE("  dwSignalLevel\t\t=0x%lx\r\n", lpLineDevStatus->dwSignalLevel);
	      TRACE("  dwBatteryLevel\t\t=0x%lx\r\n", lpLineDevStatus->dwBatteryLevel);
	      TRACE("  dwRoamMode\t\t=0x%lx\r\n", lpLineDevStatus->dwRoamMode);
	      TRACE("  dwDevStatusFlags\t\t=0x%lx\r\n", lpLineDevStatus->dwDevStatusFlags);
	      TRACE("  dwTerminalModesSize\t\t=0x%lx\r\n", lpLineDevStatus->dwTerminalModesSize);
	      TRACE("  dwTerminalModesOffset\t\t=0x%lx\r\n", lpLineDevStatus->dwTerminalModesOffset);
	      TRACE("  dwDevSpecificSize\t\t=0x%lx\r\n", lpLineDevStatus->dwDevSpecificSize);
	      TRACE("  dwDevSpecificOffset\t\t=0x%lx\r\n", lpLineDevStatus->dwDevSpecificOffset);
	      TRACE("----------------------------------------------------------------\r\n");
		}
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetLineDevStatus rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineGetLineDevStatus

////////////////////////////////////////////////////////////////////////////
// TSPI_lineGetNumAddressIDs
//
// This function returns the number of addresses availble on a line.
//
extern "C"
LONG TSPIAPI TSPI_lineGetNumAddressIDs (HDRVLINE hdLine, LPDWORD lpNumAddressIDs)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineGetNumAddressIDs beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx, lpNumAddress=%08lx\r\n", (DWORD)hdLine, (DWORD)lpNumAddressIDs);
	}
#endif

   CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
   ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn)
      lResult = GetSP()->lineGetNumAddressIDs(pConn, lpNumAddressIDs);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineGetNumAddressIDs NumAddresses=%ld, rc=0x%lx\r\n", *lpNumAddressIDs, lResult);
#endif
   return lResult;

}// TSPI_lineGetNumAddressIDs

////////////////////////////////////////////////////////////////////////////
// TSPI_lineHold
//
// This function places the specified call appearance on hold.
//
extern "C"
LONG TSPIAPI TSPI_lineHold (DRV_REQUESTID dwRequestID, HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineHold beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdCall);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineHold(pCall, dwRequestID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineHold rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineHold

////////////////////////////////////////////////////////////////////////////
// TSPI_lineMakeCall
//
// This function places a call on the specified line to the specified
// address.
//
extern "C"
LONG TSPIAPI TSPI_lineMakeCall (DRV_REQUESTID dwRequestID, HDRVLINE hdLine,
         HTAPICALL htCall, LPHDRVCALL lphdCall, LPCSTR lpszDestAddress,
         DWORD dwCountryCode, LPLINECALLPARAMS const lpCallParams)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineMakeCall beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Line Handle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdLine);
	   TRACE("  TAPI Call handle=0x%lx, Buffer for SP Call Handle=%08lx\r\n", (DWORD) htCall, (DWORD) lphdCall);
	   TRACE("  Address=<%s>, Country=%ld\r\n", lpszDestAddress, dwCountryCode);
	}
   if (lpCallParams && g_iShowAPITraceLevel > 1)
   {
      TRACE("Dumping LINECALLPARAMS at %08lx\r\n", (DWORD)lpCallParams);
      TRACE("----------------------------------------------------------------\r\n");
      TRACE("  dwTotalSize\t\t=0x%lx\r\n", lpCallParams->dwTotalSize);
      TRACE("  dwBearerMode\t\t=0x%lx\r\n", lpCallParams->dwBearerMode);
      TRACE("  dwMinRate\t\t=0x%lx\r\n", lpCallParams->dwMinRate);
      TRACE("  dwMaxRate\t\t=0x%lx\r\n", lpCallParams->dwMaxRate);
      TRACE("  dwMediaMode\t\t=0x%lx\r\n", lpCallParams->dwMediaMode);
      TRACE("  dwCallParamFlags\t\t=0x%lx\r\n", lpCallParams->dwCallParamFlags);
      TRACE("  dwAddressMode\t\t=0x%lx\r\n", lpCallParams->dwAddressMode);
      TRACE("  dwAddressID\t\t=0x%lx\r\n", lpCallParams->dwAddressID);
      TRACE("  dwOrigAddressSize\t\t=0x%lx\r\n", lpCallParams->dwOrigAddressSize);
      TRACE("  dwOrigAddressOffset\t\t=0x%lx\r\n", lpCallParams->dwOrigAddressOffset);
      TRACE("  dwDisplayableAddressSize\t\t=0x%lx\r\n", lpCallParams->dwDisplayableAddressSize);
      TRACE("  dwDisplayableAddressOffset\t\t=0x%lx\r\n", lpCallParams->dwDisplayableAddressOffset);
      TRACE("  dwCalledPartySize\t\t=0x%lx\r\n", lpCallParams->dwCalledPartySize);
      TRACE("  dwCalledPartyOffset\t\t=0x%lx\r\n", lpCallParams->dwCalledPartyOffset);
      TRACE("  dwCommentSize\t\t=0x%lx\r\n", lpCallParams->dwCommentSize);
      TRACE("  dwCommentOffset\t\t=0x%lx\r\n", lpCallParams->dwCommentOffset);
      TRACE("  dwUserUserInfoSize\t\t=0x%lx\r\n", lpCallParams->dwUserUserInfoSize);
      TRACE("  dwUserUserInfoOffset\t\t=0x%lx\r\n", lpCallParams->dwUserUserInfoOffset);
      TRACE("  dwHighLevelCompSize\t\t=0x%lx\r\n", lpCallParams->dwHighLevelCompSize);
      TRACE("  dwHighLevelCompOffset\t\t=0x%lx\r\n", lpCallParams->dwHighLevelCompOffset);
      TRACE("  dwLowLevelCompSize\t\t=0x%lx\r\n", lpCallParams->dwLowLevelCompSize);
      TRACE("  dwLowLevelCompOffset\t\t=0x%lx\r\n", lpCallParams->dwLowLevelCompOffset);
      TRACE("  dwDevSpecificSize\t\t=0x%lx\r\n", lpCallParams->dwDevSpecificSize);
      TRACE("  dwDevSpecificOffset\t\t=0x%lx\r\n", lpCallParams->dwDevSpecificOffset);
      TRACE("----------------------------------------------------------------\r\n");
   }
#endif

   CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
   ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn)
      lResult = GetSP()->lineMakeCall(pConn, dwRequestID, htCall, lphdCall, 
                           lpszDestAddress, dwCountryCode, lpCallParams);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineMakeCall SP Call Handle=0x%lx, rc=0x%lx\r\n", *lphdCall, lResult);
#endif
   return lResult;

}// TSPI_lineMakeCall

///////////////////////////////////////////////////////////////////////////
// TSPI_lineMonitorDigits
//
// This function enables and disables the unbuffered detection of digits
// received on the call.  Each time a digit of the specified digit mode(s)
// is detected, a LINE_MONITORDIGITS message is sent to the application by
// TAPI.DLL, indicating which digit was detected.
//
extern "C"
LONG TSPIAPI TSPI_lineMonitorDigits (HDRVCALL hdCall, DWORD dwDigitModes)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineMonitorDigits beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, DigitMode=0x%lx\r\n", (DWORD)hdCall, dwDigitModes);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineMonitorDigits(pCall, dwDigitModes);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineMonitorDigits rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineMonitorDigits

///////////////////////////////////////////////////////////////////////////
// TSPI_lineMonitorMedia
//
// This function enables and disables the detection of media modes on 
// the specified call.  When a media mode is detected, a LINE_MONITORMEDIA
// message is sent to TAPI.DLL.
//
extern "C"
LONG TSPIAPI TSPI_lineMonitorMedia (HDRVCALL hdCall, DWORD dwMediaModes)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineMonitorMedia beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, MediaMode=0x%lx\r\n", (DWORD)hdCall, dwMediaModes);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineMonitorMedia(pCall, dwMediaModes);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	   TRACE("TSPI_lineMonitorMedia rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_lineMonitorMedia

///////////////////////////////////////////////////////////////////////////
// TSPI_lineMonitorTones
// 
// This function enables and disables the detection of inband tones on
// the call.  Each time a specified tone is detected, a message is sent
// to the client application through TAPI.DLL
//
extern "C"
LONG TSPIAPI TSPI_lineMonitorTones (HDRVCALL hdCall, DWORD dwToneListID,
         LPLINEMONITORTONE const lpToneList, DWORD dwNumEntries)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineMonitorTones beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, Tone Id=0x%lx\r\n", (DWORD)hdCall, dwToneListID);
	   TRACE("  ToneList=%08lx, Count=%ld\r\n", (DWORD)lpToneList, dwNumEntries);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineMonitorTones(pCall, dwToneListID, lpToneList, dwNumEntries);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineMonitorTones rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_lineMonitorTones

///////////////////////////////////////////////////////////////////////////
// TSPI_lineNegotiateExtVersion
//
// This function returns the highest extension version number the SP is
// willing to operate under for the device given the range of possible
// extension versions.
//
extern "C"
LONG TSPIAPI TSPI_lineNegotiateExtVersion (DWORD dwDeviceID, DWORD dwTSPIVersion,
         DWORD dwLowVersion, DWORD dwHiVersion, LPDWORD lpdwExtVersion)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineNegotiateExtVersion beginning\r\n");
	   TRACE("  DeviceId=0x%lx, TSPI Ver=0x%lx, Hi=0x%lx, Lo=0x%lx\r\n", dwDeviceID, dwTSPIVersion, dwHiVersion, dwLowVersion);
	}
#endif
   LONG lResult = GetSP()->lineNegotiateExtVersion(dwDeviceID, dwTSPIVersion, 
                        dwLowVersion, dwHiVersion, lpdwExtVersion);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineNegotiateExtVersion Ver=0x%lx, rc=0x%lx\r\n", *lpdwExtVersion, lResult);
#endif
   return lResult;

}// TSPI_lineNegotiateExtVersion

///////////////////////////////////////////////////////////////////////////
// TSPI_lineNegotiateTSPIVersion
//
// This function is called to negotiate line versions for the TSP
// driver.
//
extern "C"
LONG TSPIAPI TSPI_lineNegotiateTSPIVersion(DWORD dwDeviceID,             
         DWORD dwLowVersion, DWORD dwHighVersion, LPDWORD lpdwTSPIVersion)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineNegotiateTSPIVersion beginning\r\n");
	   TRACE("  DeviceID=0x%lx, TAPI Version (0x%lx - 0x%lx)\r\n", dwDeviceID, dwLowVersion, dwHighVersion);
	}
#endif
   LONG lResult = GetSP()->lineNegotiateTSPIVersion(dwDeviceID, dwLowVersion,
                         dwHighVersion, lpdwTSPIVersion);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineNegotiateTSPIVersion Ver=0x%lx rc=0x%lx\r\n", *lpdwTSPIVersion, lResult);
#endif
   return lResult;

}// TSPI_lineNegotiateTSPIVersion

////////////////////////////////////////////////////////////////////////////
// TSPI_lineOpen
//
// This function opens the specified line device based on the device
// id passed and returns a handle for the line.  The TAPI.DLL line
// handle must also be retained for further interaction with this
// device.
//
extern "C"
LONG TSPIAPI TSPI_lineOpen (DWORD dwDeviceID, HTAPILINE htLine, 
         LPHDRVLINE lphdLine, DWORD dwTSPIVersion, LINEEVENT lpfnEventProc)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineOpen beginning\r\n");
	   TRACE("  DeviceId=0x%lx, TAPI line handle=0x%lx\r\n", dwDeviceID, (DWORD) htLine);
	   TRACE("  SPI Version=0x%lx, Event Callback=%08lx\r\n", dwTSPIVersion, (LONG) lpfnEventProc);
	}
#endif
   *lphdLine = 0;

   CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);

   LONG lResult = LINEERR_BADDEVICEID;

   if (pConn)
      lResult = GetSP()->lineOpen(pConn, htLine, lphdLine, dwTSPIVersion, 
                            lpfnEventProc);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	   TRACE("TSPI_lineOpen SP Line Handle=0x%lx, rc=0x%lx\r\n", *lphdLine, lResult);
#endif
   return lResult;

}// TSPI_lineOpen

//////////////////////////////////////////////////////////////////////////////
// TSPI_linePark
//
// This function parks the specified call according to the specified
// park mode.
//
extern "C"
LONG TSPIAPI TSPI_linePark (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
         DWORD dwParkMode, LPCSTR lpszDirAddr, LPVARSTRING lpNonDirAddress)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_linePark beginning\r\n");
	    TRACE("  AsynchReq=0x%lx, SP Call Handle=0x%lx, Park Mode=0x%lx\r\n", dwRequestID, (DWORD)hdCall, dwParkMode);
	    TRACE("  Dest Addr=<%s>\r\n", lpszDirAddr);
	}
#endif

    LONG lResult = 0L;

    if (dwParkMode != LINEPARKMODE_DIRECTED && dwParkMode != LINEPARKMODE_NONDIRECTED)
        lResult = LINEERR_INVALPARKMODE;
    else
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
        if (pCall)
        {
            ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
            lResult = GetSP()->linePark(pCall, dwRequestID, dwParkMode, lpszDirAddr, lpNonDirAddress);
        }
        else
            lResult = LINEERR_INVALCALLHANDLE;
    }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_linePark rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_linePark

///////////////////////////////////////////////////////////////////////////////
// TSPI_linePickup
//
// This function picks up a call alerting at the specified destination
// address and returns a call handle for the picked up call.  If invoked
// with a NULL for the 'lpszDestAddr' parameter, a group pickup is performed.
// If required by the device capabilities, 'lpszGroupID' specifies the
// group ID to which the alerting station belongs.
//
extern "C"
LONG TSPIAPI TSPI_linePickup (DRV_REQUESTID dwRequestID, HDRVLINE hdLine,
         DWORD dwAddressID, HTAPICALL htCall, LPHDRVCALL lphdCall,
         LPCSTR lpszDestAddr, LPCSTR lpszGroupID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_linePickup beginning\r\n");
	    TRACE("  AsynchReq=0x%lx, SP Line Handle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD)htCall);
	    TRACE("  TAPI Call Handle=0x%lx, AddrId=0x%lx\r\n", (DWORD) htCall, dwAddressID);
	    TRACE("  DestAddr=<%s>, Group=<%s>\r\n", lpszDestAddr, lpszGroupID);
	}
#endif
    LONG lResult = 0;
    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    if (pLine != NULL)
        ASSERT(pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    else
        lResult = LINEERR_INVALLINEHANDLE;  

    if (pLine)
    {
        CTSPIAddressInfo* pAddr = pLine->GetAddress(dwAddressID);
        if (pAddr != NULL)
            lResult = GetSP()->linePickup(pAddr, dwRequestID, htCall,
                                        lphdCall, lpszDestAddr, lpszGroupID);
        else
            lResult = LINEERR_INVALADDRESSID;
    }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_linePickup SP Call=0x%lx, rc=0x%lx\r\n", *lphdCall, lResult);
#endif
    return lResult;

}// TSPI_linePickup

////////////////////////////////////////////////////////////////////////////////
// TSPI_linePrepareAddToConference
//
// This function prepares an existing conference call for the addition of
// another party.  It creates a new temporary consultation call.  The new
// consultation call can subsequently be added to the conference call.
//
extern "C"
LONG TSPIAPI TSPI_linePrepareAddToConference (DRV_REQUESTID dwRequestID,
         HDRVCALL hdConfCall, HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall,
         LPLINECALLPARAMS const lpCallParams)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_linePrepareAddToConference beginning\r\n");
	   TRACE("  AsynchReq=0x%lx, SP Call=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdConfCall);
	   TRACE("  TAPI ConsultCall=0x%lx\r\n", (DWORD) htConsultCall);
	}
#endif
   CTSPIConferenceCall* pCall = (CTSPIConferenceCall*) hdConfCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPIConferenceCall)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->linePrepareAddToConference(pCall, dwRequestID,
                  htConsultCall, lphdConsultCall, lpCallParams);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_linePrepareAddToConference New Call=0x%lx, rc=0x%lx\r\n", *lphdConsultCall, lResult);
#endif
   return lResult;

}// TSPI_linePrepareAddToConference

/////////////////////////////////////////////////////////////////////////////////
// TSPI_lineRedirect
//
// This function redirects the specified offering call to the specified
// destination address.
//
extern "C"
LONG TSPIAPI TSPI_lineRedirect (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
         LPCSTR lpszDestAddr, DWORD dwCountryCode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineRedirect beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdCall);
	   TRACE("  DestAddr=<%s>, Country=0x%lx\r\n", lpszDestAddr, dwCountryCode);
	}
#endif
   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineRedirect(pCall, dwRequestID, lpszDestAddr, dwCountryCode);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineRedirect rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineRedirect

/////////////////////////////////////////////////////////////////////////////////
// TSPI_lineReleaseUserUserInfo
//
// This function releases a block of User->User information which is stored
// in the CALLINFO record.
//
extern "C"
LONG TSPIAPI TSPI_lineReleaseUserUserInfo(DRV_REQUESTID dwRequestID, HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineReleaseUserUserInfo beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdCall);
	}
#endif
   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineReleaseUserUserInfo(pCall, dwRequestID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineReleaseUserUserInfo rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineReleaseUserUserInfo

/////////////////////////////////////////////////////////////////////////////////
// TSPI_lineRemoveFromConference
//
// This function removes the specified call from the conference call to
// which it currently belongs.  The remaining calls in the conference call
// are unaffected.
//
extern "C"
LONG TSPIAPI TSPI_lineRemoveFromConference (DRV_REQUESTID dwRequestID, HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineRemoveFromConference beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdCall);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineRemoveFromConference(pCall, dwRequestID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineRemoveFromConference rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineRemoveFromConference

///////////////////////////////////////////////////////////////////////////////////
// TSPI_lineSecureCall
//
// This function secures the call from any interruptions or interference
// that may affect the call's media stream.
//
extern "C"
LONG TSPIAPI TSPI_lineSecureCall (DRV_REQUESTID dwRequestID, HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSecureCall beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdCall);
	}
#endif
   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineSecureCall(pCall, dwRequestID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSecureCall rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSecureCall

///////////////////////////////////////////////////////////////////////////////
// TSPI_lineSelectExtVersion
//
// This function selects the indicated extension version for the indicated
// line device.  Subsequent requests operate according to that extension
// version.
//
extern "C"
LONG TSPIAPI TSPI_lineSelectExtVersion (HDRVLINE hdLine, DWORD dwExtVersion)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSelectExtVersion beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx, Ext Ver=0x%lx\r\n", (DWORD) hdLine, dwExtVersion);
	}
#endif
   CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
   ASSERT(pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pLine)
      lResult = GetSP()->lineSelectExtVersion(pLine, dwExtVersion);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSelectExtVersion rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSelectExtVersion

//////////////////////////////////////////////////////////////////////////////
// TSPI_lineSendUserUserInfo
//
// This function sends user-to-user information to the remote party on the
// specified call.
//
extern "C"
LONG TSPIAPI TSPI_lineSendUserUserInfo (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
         LPCSTR lpsUserUserInfo, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSendUserUserInfo beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n", (DWORD)dwRequestID, (DWORD)hdCall);
	   TRACE("  UserToUser <%s>, Size=%ld\r\n", lpsUserUserInfo, dwSize);
	}
	if (g_iShowAPITraceLevel > 1)
   		DumpMem ("UserInfo->\r\n", (LPVOID)lpsUserUserInfo, dwSize);
#endif    

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineSendUserUserInfo(pCall, dwRequestID, lpsUserUserInfo, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSendUserUserInfo rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSendUserUserInfo
                                                          
//////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetAppSpecific
//
// This function sets the application specific portion of the 
// LINECALLINFO structure.  This is returned by the TSPI_lineGetCallInfo
// function.
//
extern "C"
LONG TSPIAPI TSPI_lineSetAppSpecific (HDRVCALL hdCall, DWORD dwAppSpecific)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetAppSpecific beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, dwAppSpecific=0x%lx\r\n", (DWORD) hdCall, dwAppSpecific);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineSetAppSpecific(pCall, dwAppSpecific);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSetAppSpecific rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetAppSpecific

/////////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetCallParams
//
// This function sets certain parameters for an existing call.
//
extern "C"
LONG TSPIAPI TSPI_lineSetCallParams (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
         DWORD dwBearerMode, DWORD dwMinRate, DWORD dwMaxRate, 
         LPLINEDIALPARAMS const lpDialParams)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetCallParams beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD)hdCall);
	   TRACE("  BearerMode=0x%lx, Rate=(%ld, %ld)\r\n", dwBearerMode, dwMinRate, dwMaxRate);
	}
	
	if (g_iShowAPITraceLevel > 1 && lpDialParams != NULL)
	{
	   TRACE("Dumping LINEDIALPARAMS at %08lx\r\n", lpDialParams);
	   TRACE("--------------------------------------------------------------\r\n");
	   TRACE("  dwDialPause\t=0x%lx\r\n", lpDialParams->dwDialPause);
	   TRACE("  dwDialSpeed\t=0x%lx\r\n", lpDialParams->dwDialSpeed);
	   TRACE("  dwDigitDuration\t=0x%lx\r\n", lpDialParams->dwDigitDuration);
	   TRACE("  dwWaitForDialtone\t=0x%lx\r\n", lpDialParams->dwWaitForDialtone);
	   TRACE("--------------------------------------------------------------\r\n");
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineSetCallParams(pCall, dwRequestID, dwBearerMode,
                  dwMinRate, dwMaxRate, lpDialParams);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSetCallParams rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetCallParams

////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetCurrentLocation (Win95)
//
// This function is called by TAPI whenever the address translation location
// is changed by the user (in the Dial Helper dialog or 
// 'lineSetCurrentLocation' function.  SPs which store parameters specific
// to a location (e.g. touch-tone sequences specific to invoke a particular
// PBX function) would use the location to select the set of parameters 
// applicable to the new location.
// 
extern "C"
LONG TSPIAPI TSPI_lineSetCurrentLocation (DWORD dwLocation)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetCurrentLocation beginning\r\n");
	   TRACE("  Location=0x%lx\r\n", dwLocation);
	}
#endif

   LONG lResult = GetSP()->lineSetCurrentLocation(dwLocation);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSetCurrentLocation rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetCurrentLocation

////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetDefaultMediaDetection
//
// This function tells us the new set of media modes to watch for on 
// this line (inbound or outbound).
//
extern "C"
LONG TSPIAPI TSPI_lineSetDefaultMediaDetection (HDRVLINE hdLine, DWORD dwMediaModes)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetDefaultMediaDetection beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx, dwMediaModes=0x%lx\r\n", (DWORD) hdLine, dwMediaModes);
	}
#endif

   CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
   ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn)
      lResult = GetSP()->lineSetDefaultMediaDetection(pConn, dwMediaModes);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSetDefaultMediaDetection rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetDefaultMediaDetection

/////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetDevConfig
//
// This function restores the configuration of a device associated one-to-one
// with the line device from a data structure obtained through TSPI_lineGetDevConfig.
// The contents of the data structure are specific to the service provider.
//
extern "C"
LONG TSPIAPI TSPI_lineSetDevConfig (DWORD dwDeviceID, LPVOID const lpDevConfig,
         DWORD dwSize, LPCSTR lpszDevClass)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetDevConfig beginning\r\n");
	   TRACE("  DeviceID=0x%lx, Buffer=%08lx, Size=%ld\r\n", dwDeviceID, (DWORD)lpDevConfig, dwSize);
	   TRACE("  DeviceClass <%s>\r\n", lpszDevClass);
	}
#endif
   CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);

   LONG lResult = LINEERR_BADDEVICEID;

   if (pConn)
      lResult = GetSP()->lineSetDevConfig(pConn, lpDevConfig, dwSize, CString(lpszDevClass));

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSetDevConfig rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetDevConfig

////////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetMediaControl
//
// This function enables and disables control actions on the media stream
// associated with the specified line, address, or call.  Media control actions
// can be triggered by the detection of specified digits, media modes,
// custom tones, and call states.  The new specified media controls replace all
// the ones that were in effect for this line, address, or call prior to this
// request.
//
extern "C"
LONG TSPIAPI TSPI_lineSetMediaControl (HDRVLINE hdLine, DWORD dwAddressID, 
         HDRVCALL hdCall, DWORD dwSelect, 
         LPLINEMEDIACONTROLDIGIT const lpDigitList, DWORD dwNumDigitEntries, 
         LPLINEMEDIACONTROLMEDIA const lpMediaList, DWORD dwNumMediaEntries, 
         LPLINEMEDIACONTROLTONE const lpToneList, DWORD dwNumToneEntries, 
         LPLINEMEDIACONTROLCALLSTATE const lpCallStateList, DWORD dwNumCallStateEntries)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineSetMediaControl beginning\r\n");
	    TRACE("  SP Line Handle=0x%lx, AddressID=0x%lx\r\n", (DWORD) hdLine, dwAddressID);
	    TRACE("  SP Call Handle=0x%lx, Select=%ld\r\n", (DWORD)hdCall, dwSelect);
	    TRACE("  Digits=%ld, Media=%ld, Tones=%ld, CallStates=%ld\r\n", dwNumDigitEntries, dwNumMediaEntries, dwNumToneEntries, dwNumCallStateEntries);
	}
#endif
    
    CTSPILineConnection* pConn = NULL;
    CTSPICallAppearance* pCall = NULL;
    CTSPIAddressInfo* pAddr = NULL;

    // Check how to find this connection info.  Based on the
    // information passed locate the proper connection info.
    LONG lResult = 0;

    switch(dwSelect)
    {
        case LINECALLSELECT_LINE:
            pConn = (CTSPILineConnection*) hdLine;
            if (pConn != NULL)
                ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
            else
                lResult = LINEERR_INVALLINEHANDLE;
            break;

        case LINECALLSELECT_ADDRESS:
            pConn = (CTSPILineConnection*) hdLine;
            if (pConn != NULL)
            {
                ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
                pAddr = pConn->GetAddress(dwAddressID);
                if (pAddr != NULL)
                    ASSERT (pAddr->IsKindOf(RUNTIME_CLASS(CTSPIAddressInfo)));
                else
                    lResult = LINEERR_INVALADDRESSID;
            }
            else
                lResult = LINEERR_INVALLINEHANDLE;
            break;

        case LINECALLSELECT_CALL:
            pCall = (CTSPICallAppearance*) hdCall;
            if (pCall != NULL)
            {
                ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
                pAddr = pCall->GetAddressInfo();
                pConn = pAddr->GetLineOwner();
            }
            else
                lResult = LINEERR_INVALCALLHANDLE;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    if (lResult == 0)
    {
        lResult = GetSP()->lineSetMediaControl(pConn, pAddr, pCall, 
                  lpDigitList, dwNumDigitEntries, 
                  lpMediaList, dwNumMediaEntries, 
                  lpToneList, dwNumToneEntries, 
                  lpCallStateList, dwNumCallStateEntries);
    }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineSetMediaControl rc=0x%lx\r\n", lResult);
#endif
    return lResult;
   
}// TSPI_lineSetMediaControl

////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetMediaMode
//
// This function changes the provided calls media in the LINECALLSTATE
// structure.
//
extern "C"
LONG TSPIAPI TSPI_lineSetMediaMode(HDRVCALL hdCall, DWORD dwMediaMode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetMediaMode beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, dwMediaModes=0x%lx\r\n", (DWORD) hdCall, dwMediaMode);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineSetMediaMode(pCall, dwMediaMode);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	   TRACE("TSPI_lineSetMediaMode rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetMediaMode

///////////////////////////////////////////////////////////////////////////
// TSPI_lineSetStatusMessages
//
// This function tells us which events to notify TAPI about when
// address or status changes about the specified line.
//
extern "C"
LONG TSPIAPI TSPI_lineSetStatusMessages (HDRVLINE hdLine, DWORD dwLineStates,
         DWORD dwAddressStates)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetStatusMessages beginning\r\n");
	   TRACE("  SP Line Handle=0x%lx, LineStates=0x%lx, Address States=0x%lx\r\n", (DWORD) hdLine, dwLineStates, dwAddressStates);
	}
#endif
   CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
   ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn)
      lResult = GetSP()->lineSetStatusMessages(pConn, dwLineStates, dwAddressStates);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSetStatusMessages rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetStatusMessages

/////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetTerminal
//
// This operation enables TAPI.DLL to specify to which terminal information
// related to a specified line, address, or call is to be routed.  This
// can be used while calls are in progress on the line, to allow events
// to be routed to different devices as required.
//
extern "C"
LONG TSPIAPI TSPI_lineSetTerminal (DRV_REQUESTID dwRequestID, HDRVLINE hdLine,
         DWORD dwAddressID, HDRVCALL hdCall, DWORD dwSelect, 
         DWORD dwTerminalModes, DWORD dwTerminalID, DWORD bEnable)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineSetTerminal beginning\r\n");
	    TRACE("  AsynchReqId=0x%lx\r\n", (DWORD) dwRequestID);
	    TRACE("  SP Line Handle=0x%lx, AddressID=0x%lx\r\n", (DWORD) hdLine, dwAddressID);
	    TRACE("  SP Call Handle=0x%lx, Select=%ld\r\n", (DWORD)hdCall, dwSelect);
	    TRACE("  Terminal Modes=0x%lx, Id=0x%lx, Enable=%ld\r\n", dwTerminalModes, dwTerminalID, bEnable);
	}
#endif
    CTSPILineConnection* pConn = NULL;
    CTSPICallAppearance* pCall = NULL;
    CTSPIAddressInfo* pAddr = NULL;

    // Check how to find this connection info.  Based on the
    // information passed locate the proper connection info.
    LONG lResult = 0;

    switch(dwSelect)
    {
        case LINECALLSELECT_LINE:
            pConn = (CTSPILineConnection*) hdLine;
            if (pConn != NULL)
                ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
            else
                lResult = LINEERR_INVALLINEHANDLE;
            break;

        case LINECALLSELECT_ADDRESS:
            pConn = (CTSPILineConnection*) hdLine;
            if (pConn != NULL)
            {
                ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
                pAddr = pConn->GetAddress(dwAddressID);
                if (pAddr != NULL)
                    ASSERT (pAddr->IsKindOf(RUNTIME_CLASS(CTSPIAddressInfo)));
                else
                    lResult = LINEERR_INVALADDRESSID;
            }
            else
                lResult = LINEERR_INVALLINEHANDLE;
            pConn = NULL;
            break;

        case LINECALLSELECT_CALL:
            pCall = (CTSPICallAppearance*) hdCall;
            if (pCall == NULL)
                lResult = LINEERR_INVALCALLHANDLE;
#ifdef _DEBUG
            else
                ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
#endif              
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    if (lResult == 0)
    {
        lResult = GetSP()->lineSetTerminal(pConn, pAddr, pCall, dwRequestID,
                                           dwTerminalModes, dwTerminalID, (BOOL)bEnable);
    }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineSetTerminal rc=0x%lx\r\n", lResult);
#endif
    return lResult;
   
}// TSPI_lineSetTerminal

////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetupConference
//
// This function sets up a conference call for the addition of a third 
// party.
//
extern "C"
LONG TSPIAPI TSPI_lineSetupConference (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
         HDRVLINE hdLine, HTAPICALL htConfCall, LPHDRVCALL lphdConfCall,
         HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall, DWORD dwNumParties,
         LPLINECALLPARAMS const lpLineCallParams)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetupConference beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Call Handle=0x%lx, SP LineHandle=0x%lx\r\n", (DWORD) dwRequestID, (DWORD) hdCall, (DWORD) hdLine);
	   TRACE("  TAPI ConfCall=0x%lx, TAPI ConsultCall=0x%lx, NumParties=%ld\r\n", htConfCall, htConsultCall, dwNumParties);
	}
#endif
   CTSPILineConnection* pConn = NULL;
   CTSPICallAppearance* pCall = NULL;

   // If the call handle is non-NULL, look it up.
   if (hdCall)
   {
      pCall = (CTSPICallAppearance*) hdCall;
      ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
      pConn = pCall->GetLineConnectionInfo();
      ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
   }
   else if (hdLine)
   {
      pConn = (CTSPILineConnection*) hdLine;
      ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
   }

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn != NULL || pCall != NULL)
   {
      lResult = GetSP()->lineSetupConference(pConn, pCall, dwRequestID, htConfCall,
                  lphdConfCall, htConsultCall, lphdConsultCall, dwNumParties,
                  lpLineCallParams);
#ifdef _DEBUG
	  if (lResult == 0 && g_iShowAPITraceLevel > 1)
	      TRACE("Returned SP ConfCall=0x%lx, SP ConsultCall=0x%lx\r\n", *lphdConfCall, *lphdConsultCall);
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSetupConference rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetupConference

////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetupTransfer
//
// This function sets up a call for transfer to a destination address.
// A new call handle is created which represents the destination
// address.
//
extern "C"
LONG TSPIAPI TSPI_lineSetupTransfer (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
       HTAPICALL htConsultCall, LPHDRVCALL lphdConsultCall,
       LPLINECALLPARAMS const lpCallParams)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSetupTransfer beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, Asynch ReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestID);
	   TRACE("  New TAPI CH=0x%lx\r\n", (DWORD) htConsultCall);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
   ASSERT(lphdConsultCall != NULL);

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
   {
      lResult = GetSP()->lineSetupTransfer(pCall, dwRequestID, htConsultCall, 
                     lphdConsultCall, lpCallParams);

#ifdef _DEBUG
	   if (lResult == 0 && g_iShowAPITraceLevel)
	      TRACE("  Returned SP Call handle=0x%lx\r\n", *lphdConsultCall);
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	   TRACE("TSPI_lineSetupTransfer rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSetupTransfer

//////////////////////////////////////////////////////////////////////////////
// TSPI_lineSwapHold
//
// This function swaps the specified active call with the specified
// call on hold.
//
extern "C"
LONG TSPIAPI TSPI_lineSwapHold (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
       HDRVCALL hdHeldCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineSwapHold beginning\r\n");
	   TRACE("  SP Call Handle=0x%lx, Asynch ReqId=0x%lx\r\n", (DWORD) hdCall, (DWORD) dwRequestID);
	   TRACE("  Held SP Call Handle=0x%lx\r\n", (DWORD) hdHeldCall);
	}
#endif

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
   CTSPICallAppearance* pHeldCall = (CTSPICallAppearance*) hdHeldCall;

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall && pHeldCall)
      lResult = GetSP()->lineSwapHold(pCall, dwRequestID, pHeldCall);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineSwapHold rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineSwapHold

////////////////////////////////////////////////////////////////////////////
// TSPI_lineUncompleteCall
//
// This function is used to cancel the specified call completion request
// on the specified line.
//
extern "C"
LONG TSPIAPI TSPI_lineUncompleteCall (DRV_REQUESTID dwRequestID,
         HDRVLINE hdLine, DWORD dwCompletionID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineUncompleteCall beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Line Handle=0x%lx, CompleteId=0x%lx\r\n", (DWORD)dwRequestID, (DWORD)hdLine, dwCompletionID);
	}
#endif
   CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
   ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

   LONG lResult = LINEERR_INVALLINEHANDLE;

   if (pConn)
      lResult = GetSP()->lineUncompleteCall(pConn, dwRequestID, dwCompletionID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineUncompleteCall rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_lineUncompleteCall

////////////////////////////////////////////////////////////////////////////
// TSPI_lineUnhold
//
// This function retrieves the specified held call
//
extern "C"
LONG TSPIAPI TSPI_lineUnhold (DRV_REQUESTID dwRequestId, HDRVCALL hdCall)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_lineUnhold beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n", (DWORD) dwRequestId, (DWORD) hdCall);
	}
#endif
   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall)
      lResult = GetSP()->lineUnhold(pCall, dwRequestId);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_lineUnhold rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_lineUnhold

/////////////////////////////////////////////////////////////////////////////
// TSPI_lineUnpark
//
// This function retrieves the call parked at the specified
// address and returns a call handle for it.
//
extern "C"
LONG TSPIAPI TSPI_lineUnpark (DRV_REQUESTID dwRequestID, HDRVLINE hdLine,
         DWORD dwAddressID, HTAPICALL htCall, LPHDRVCALL lphdCall, 
         LPCSTR lpszDestAddr)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_lineUnpark beginning\r\n");
	    TRACE("  AsynchReqId=0x%lx, SP Line Handle=0x%lx, AddressId=0x%lx\r\n", (DWORD)dwRequestID, (DWORD)hdLine, dwAddressID);
	    TRACE("  TAPI Call Handle=0x%lx, Address=<%s>\r\n", (DWORD)htCall, lpszDestAddr);
	}
#endif
    
    LONG lResult = LINEERR_INVALLINEHANDLE;
    CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
    if (pConn != NULL)
    {
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
        CTSPIAddressInfo* pAddr = pConn->GetAddress(dwAddressID);
        if (pAddr == NULL)
            lResult = LINEERR_INVALADDRESSID;
        else
            lResult = GetSP()->lineUnpark(pAddr, dwRequestID, htCall, lphdCall, lpszDestAddr);
    }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_lineUnpark rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_lineUnpark

/******************************************************************************/
//
// TSPIAPI TSPI_phone functions
//
/******************************************************************************/

//////////////////////////////////////////////////////////////////////////
// TSPI_phoneClose
//
// This function closes the specified open phone device after completing
// or aborting all outstanding asynchronous requests on the device.
//
extern "C"
LONG TSPIAPI TSPI_phoneClose (HDRVPHONE hdPhone)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneClose beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD)hdPhone);
	}
#endif
   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneClose(pPhone);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneClose rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneClose

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneConfigDialog
//
// This function invokes the parameter configuration dialog for the
// phone device.
//
extern "C"
LONG TSPIAPI TSPI_phoneConfigDialog (DWORD dwDeviceId, HWND hwndOwner, LPCSTR lpszDeviceClass)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneConfigDialog beginning\r\n");
	   TRACE("  Device Id=0x%lx, Owner=%08lx\r\n", dwDeviceId, CASTHANDLE(hwndOwner));
	   TRACE("  Device Class=<%s>\r\n", lpszDeviceClass);
	}
#endif
   CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneConfigDialog(pPhone, CWnd::FromHandle(hwndOwner), 
                                    CString(lpszDeviceClass));

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneConfigDialog rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneConfigDialog

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneDevSpecific
//
// This function is used as a general extension mechanism to enable
// a TAPI implementation to provide features not generally available
// to the specification.
//
extern "C"
LONG TSPIAPI TSPI_phoneDevSpecific (DRV_REQUESTID dwRequestID, HDRVPHONE hdPhone,
               LPVOID lpParams, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneDevSpecific beginning\r\n");
	   TRACE("  AsynchReqId=0x%lx, SP Phone Handle=0x%lx\r\n", (DWORD)dwRequestID, (DWORD)hdPhone);
	   TRACE("  lpParams=%08lx, Size=%ld\r\n", (DWORD)lpParams, dwSize);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneDevSpecific(pPhone, dwRequestID, lpParams, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneDevSpecific rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneDevSpecific

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetButtonInfo
//
// This function returns information about the specified phone 
// button.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetButtonInfo (HDRVPHONE hdPhone, DWORD dwButtonId,
               LPPHONEBUTTONINFO lpPhoneInfo)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetButtonInfo beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD) hdPhone);
	   TRACE("  ButtonId=%ld, PhoneInfo=%08lx\r\n", dwButtonId, (DWORD) lpPhoneInfo);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
   {
      lResult = GetSP()->phoneGetButtonInfo(pPhone, dwButtonId, lpPhoneInfo);
#ifdef _DEBUG
	  if (lResult == 0 && g_iShowAPITraceLevel)
	  {                                              
	      TRACE("Dumping PHONEBUTTONINFO at %08lx\r\n", (DWORD) lpPhoneInfo);
	      TRACE("----------------------------------------------------------------\r\n");
	      TRACE("  dwTotalSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwTotalSize);
	      TRACE("  dwNeededSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwNeededSize);
	      TRACE("  dwUsedSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwUsedSize);
	      TRACE("  dwButtonMode\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonMode);
	      TRACE("  dwButtonFunction\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonFunction);
	      TRACE("  dwButtonTextSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonTextSize);
	      TRACE("  dwButtonTextOffset\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonTextOffset);
	      TRACE("  dwDevSpecificSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwDevSpecificSize);
	      TRACE("  dwDevSpecificOffset\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwDevSpecificOffset);
	      TRACE("----------------------------------------------------------------\r\n");
	  }
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetButtonInfo rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetButtonInfo

////////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetData
//
// This function uploads the information from the specified location
// in the open phone device to the specified buffer.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetData (HDRVPHONE hdPhone, DWORD dwDataId,
               LPVOID lpData, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetData beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD) hdPhone);
	   TRACE("  DataId=%ld, Buffer=%08lx, Size=%ld\r\n", dwDataId, (DWORD)lpData, dwSize);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneGetData(pPhone, dwDataId, lpData, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetData rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetData

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetDevCaps
//
// This function queries a specified phone device to determine its
// telephony capabilities
//
extern "C"
LONG TSPIAPI TSPI_phoneGetDevCaps (DWORD dwDeviceId, DWORD dwTSPIVersion,
               DWORD dwExtVersion, LPPHONECAPS lpPhoneCaps)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetDevCaps beginning\r\n");
	   TRACE("  Phone Deviceid=0x%lx\r\n", dwDeviceId);
	   TRACE("  TSPIVersion=0x%lx, ExtVer=0x%lx, PhoneCaps=%08lx\r\n", dwTSPIVersion, dwExtVersion, (DWORD)lpPhoneCaps);
	}
#endif

   CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneGetDevCaps(pPhone, dwTSPIVersion, dwExtVersion, lpPhoneCaps);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetDevCaps rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_phoneGetDevCaps

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetDisplay
//
// This function returns the current contents of the specified phone
// display.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetDisplay (HDRVPHONE hdPhone, LPVARSTRING lpString)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetDisplay beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD) hdPhone);
	   TRACE("  VarString=%08lx\r\n", (DWORD)lpString);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneGetDisplay(pPhone, lpString);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetDisplay rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetDisplay

//////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetExtensionID
//
// This function retrieves the extension ID that the service provider
// supports for the indicated device.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetExtensionID (DWORD dwDeviceId, DWORD dwTSPIVersion,
               LPPHONEEXTENSIONID lpExtensionId)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetExtensionID beginning\r\n");
	   TRACE("  DeviceId=0x%lx, TSPIVersion=0x%lx, lpPhoneExtId=%08lx\r\n", dwDeviceId, dwTSPIVersion, (DWORD)lpExtensionId);
	}
#endif

   CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneGetExtensionID(pPhone, dwTSPIVersion, lpExtensionId);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetExtensionID rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetExtensionID

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetGain
//
// This function returns the gain setting of the microphone of the
// specified phone's hookswitch device.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetGain (HDRVPHONE hdPhone, DWORD dwHookSwitchDev,
               LPDWORD lpdwGain)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetGain beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, HookSwitchDev=%x, GainBuffer=%08lx\r\n", (DWORD)hdPhone, dwHookSwitchDev, (DWORD)lpdwGain);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneGetGain(pPhone, dwHookSwitchDev, lpdwGain);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetGain rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetGain

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetHookSwitch
//
// This function retrieves the current hook switch setting of the
// specified open phone device
//
extern "C"
LONG TSPIAPI TSPI_phoneGetHookSwitch (HDRVPHONE hdPhone, LPDWORD lpdwHookSwitchDevs)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetHookSwitch beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, Buffer=%08lx\r\n", (DWORD)hdPhone, (DWORD)lpdwHookSwitchDevs);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
   {
      lResult = GetSP()->phoneGetHookSwitch(pPhone, lpdwHookSwitchDevs);
#ifdef _DEBUG
	  if (g_iShowAPITraceLevel)
         TRACE("  Hookswitch settings are 0x%lx\r\n", *lpdwHookSwitchDevs);
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetHookSwitch rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetHookSwitch

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetIcon
//
// This function retrieves a specific icon for display from an
// application.  This icon will represent the phone device.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetIcon (DWORD dwDeviceId, LPCSTR lpszDevClass, 
               LPHICON lphIcon)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetIcon beginning\r\n");
	   TRACE("  DeviceId=0x%lx, lpszDevClass=<%s>\r\n", dwDeviceId, lpszDevClass);
	}
#endif

   CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneGetIcon(pPhone, CString(lpszDevClass), lphIcon);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetIcon rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_phoneGetIcon

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetID
//
// This function retrieves the device id of the specified open phone
// handle (or some other media handle if available).
//
extern "C"
LONG TSPIAPI TSPI_phoneGetID (HDRVPHONE hdPhone, LPVARSTRING lpDeviceId, 
               LPCSTR lpszDevClass)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetID beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, DevClass=<%s>\r\n", (DWORD)hdPhone, lpszDevClass);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneGetID(pPhone, CString(lpszDevClass), lpDeviceId);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetID rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetID

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetLamp
//
// This function returns the current lamp mode of the specified
// lamp.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetLamp (HDRVPHONE hdPhone, DWORD dwButtonLampId,
               LPDWORD lpdwLampMode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetLamp beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, LampId=0x%lx\r\n", (DWORD)hdPhone, dwButtonLampId);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
   {
      lResult = GetSP()->phoneGetLamp(pPhone, dwButtonLampId, lpdwLampMode);
#ifdef _DEBUG
	  if (lResult == 0 && g_iShowAPITraceLevel)
      	TRACE("  Returned lamp mode = 0x%lx\r\n", *lpdwLampMode);
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetLamp rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_phoneGetLamp

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetRing
//
// This function enables an application to query the specified open
// phone device as to its current ring mode.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetRing (HDRVPHONE hdPhone, LPDWORD lpdwRingMode,
               LPDWORD lpdwVolume)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetRing beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD)hdPhone);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
   {
      lResult = GetSP()->phoneGetRing(pPhone, lpdwRingMode, lpdwVolume);
#ifdef _DEBUG
	  if (g_iShowAPITraceLevel)
      	TRACE("  Returned Ringmode=0x%lx, RingVolume=0x%lx\r\n", *lpdwRingMode, *lpdwVolume);
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	   TRACE("TSPI_phoneGetRing rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_phoneGetRing

//////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetStatus
//
// This function queries the specified open phone device for its
// overall status.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetStatus (HDRVPHONE hdPhone, LPPHONESTATUS lpPhoneStatus)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetStatus beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD)hdPhone);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
   {
      lResult = GetSP()->phoneGetStatus(pPhone, lpPhoneStatus);
#ifdef _DEBUG
	  if (lResult == 0 && g_iShowAPITraceLevel > 1)
	  {                                              
	      TRACE("Dumping PHONESTATUS at %08lx\r\n", (DWORD) lpPhoneStatus);
	      TRACE("----------------------------------------------------------------\r\n");
	      TRACE("  dwTotalSize\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwTotalSize);
	      TRACE("  dwNeededSize\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwNeededSize);
	      TRACE("  dwUsedSize\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwUsedSize);
	      TRACE("  dwStatusFlags;\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwStatusFlags);
	      TRACE("  dwNumOwners\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwNumOwners);
	      TRACE("  dwNumMonitors\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwNumMonitors);
	      TRACE("  dwRingMode\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwRingMode);
	      TRACE("  dwRingVolume\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwRingVolume);
	      TRACE("  dwHandsetHookSwitchMode\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwHandsetHookSwitchMode);
	      TRACE("  dwHandsetVolume\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwHandsetVolume);
	      TRACE("  dwHandsetGain\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwHandsetGain);
	      TRACE("  dwSpeakerHookSwitchMode\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwSpeakerHookSwitchMode);
	      TRACE("  dwSpeakerVolume\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwSpeakerVolume);
	      TRACE("  dwSpeakerGain\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwSpeakerGain);
	      TRACE("  dwHeadsetHookSwitchMode\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwHeadsetHookSwitchMode);
	      TRACE("  dwHeadsetVolume\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwHeadsetVolume);
	      TRACE("  dwHeadsetGain\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwHeadsetGain);
	      TRACE("  dwDisplaySize\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwDisplaySize);
	      TRACE("  dwDisplayOffset\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwDisplayOffset);
	      TRACE("  dwLampModesSize\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwLampModesSize);
	      TRACE("  dwLampModesOffset\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwLampModesOffset);
	      TRACE("  dwOwnerNameSize\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwOwnerNameSize);
	      TRACE("  dwOwnerNameOffset\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwOwnerNameOffset);
	      TRACE("  dwDevSpecificSize\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwDevSpecificSize);
	      TRACE("  dwDevSpecificOffset\t\t\t= 0x%lx\r\n", lpPhoneStatus->dwDevSpecificOffset);
	      TRACE("----------------------------------------------------------------\r\n");
	 }
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	}
#endif
   TRACE("TSPI_phoneGetStatus rc=0x%lx\r\n", lResult);
   return lResult;
  
}// TSPI_phoneGetStatus

////////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetVolume
//
// This function returns the volume setting of the phone device.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetVolume (HDRVPHONE hdPhone, DWORD dwHookSwitchDev,
               LPDWORD lpdwVolume)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneGetVolume beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD)hdPhone);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
   {
      lResult = GetSP()->phoneGetVolume(pPhone, dwHookSwitchDev, lpdwVolume);
#ifdef _DEBUG
	  if (g_iShowAPITraceLevel)
			TRACE("  Return hookswitch volume is 0x%lx\r\n", *lpdwVolume);
#endif
   }

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneGetVolume rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneGetVolume

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneNegotiateTSPIVersion
//
// This function returns the highest SP version number the
// service provider is willing to operate under for this device,
// given the range of possible values.
//
extern "C"
LONG TSPIAPI TSPI_phoneNegotiateTSPIVersion (DWORD dwDeviceID,
               DWORD dwLowVersion, DWORD dwHighVersion,
               LPDWORD lpdwVersion)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneNegotiateTSPIVersion beginning\r\n");
	   TRACE("  DeviceID=0x%lx, TAPI Version (0x%lx - 0x%lx)\r\n", dwDeviceID, dwLowVersion, dwHighVersion);
	}
#endif
   LONG lResult = GetSP()->phoneNegotiateTSPIVersion(dwDeviceID, dwLowVersion,
                         dwHighVersion, lpdwVersion);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneNegotiateTSPIVersion Ver=0x%lx, rc=0x%lx\r\n", *lpdwVersion, lResult);
#endif
   return lResult;

}// TSPI_phoneNegotiateTSPIVersion   

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneNegotiateExtVersion
//
// This function returns the highest extension version number the
// service provider is willing to operate under for this device,
// given the range of possible extension values.
//
extern "C"
LONG TSPIAPI TSPI_phoneNegotiateExtVersion (DWORD dwDeviceID,
               DWORD dwTSPIVersion, DWORD dwLowVersion, DWORD dwHighVersion,
               LPDWORD lpdwExtVersion)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneNegotiateExtVersion beginning\r\n");
	   TRACE("  DeviceID=0x%lx, TAPI Version (0x%lx - 0x%lx)\r\n", dwDeviceID, dwLowVersion, dwHighVersion);
	}
#endif

   LONG lResult = GetSP()->phoneNegotiateExtVersion(dwDeviceID, 
                         dwTSPIVersion, dwLowVersion,
                         dwHighVersion, lpdwExtVersion);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("  Return version=0x%lx\r\n", *lpdwExtVersion);
	   TRACE("TSPI_phoneNegotiateExtVersion rc=0x%lx\r\n", lResult);
	}
#endif
   return lResult;

}// TSPI_phoneNegotiateExtVersion   

////////////////////////////////////////////////////////////////////////////
// TSPI_phoneOpen
//
// This function opens the phone device whose device ID is given,
// returning the service provider's opaque handle for the device and
// retaining the TAPI opaque handle.
//
extern "C"
LONG TSPIAPI TSPI_phoneOpen (DWORD dwDeviceId, HTAPIPHONE htPhone,
               LPHDRVPHONE lphdPhone, DWORD dwTSPIVersion, PHONEEVENT lpfnEventProc)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneOpen beginning\r\n");
	   TRACE("  DeviceID=0x%lx, TAPI Handle=0x%lx, TSPIVersion=0x%lx, Event=%08lx\r\n", dwDeviceId, (DWORD)htPhone, (DWORD)lphdPhone, (DWORD)lpfnEventProc);
	}
#endif
   CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneOpen(pPhone, htPhone, lphdPhone, dwTSPIVersion, lpfnEventProc);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("  SP handle for phone is 0x%lx\r\n", *lphdPhone);
	   TRACE("TSPI_phoneOpen rc=0x%lx\r\n", lResult);
	}
#endif
   return lResult;

}// TSPI_phoneOpen

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneSelectExtVersion
//
// This function selects the indicated extension version for the
// indicated phone device.  Subsequent requests operate according to
// that extension version.
//
extern "C"
LONG TSPIAPI TSPI_phoneSelectExtVersion (HDRVPHONE hdPhone, DWORD dwExtVersion)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSelectExtVersion beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, dwExtVersion=0x%lx\r\n", (DWORD)hdPhone, dwExtVersion);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneSelectExtVersion(pPhone, dwExtVersion);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSelectExtVersion rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSelectExtVersion

//////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetButtonInfo
//
// This function sets information about the specified button on the
// phone device.
//
extern "C"
LONG TSPIAPI TSPI_phoneSetButtonInfo (DRV_REQUESTID dwRequestId, HDRVPHONE hdPhone, DWORD dwButtonId,
               LPPHONEBUTTONINFO const lpPhoneInfo)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetButtonInfo beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD) hdPhone, (DWORD) dwRequestId);
	   TRACE("  ButtonId=%ld\r\n", dwButtonId);
	}
	if (g_iShowAPITraceLevel > 1)
	{
	   TRACE("Dumping PHONEBUTTONINFO at %08lx\r\n", (DWORD) lpPhoneInfo);
	   TRACE("----------------------------------------------------------------\r\n");
	   TRACE("  dwTotalSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwTotalSize);
	   TRACE("  dwNeededSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwNeededSize);
	   TRACE("  dwUsedSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwUsedSize);
	   TRACE("  dwButtonMode\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonMode);
	   TRACE("  dwButtonFunction\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonFunction);
	   TRACE("  dwButtonTextSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonTextSize);
	   TRACE("  dwButtonTextOffset\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwButtonTextOffset);
	   TRACE("  dwDevSpecificSize\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwDevSpecificSize);
	   TRACE("  dwDevSpecificOffset\t\t\t= 0x%lx\r\n", lpPhoneInfo->dwDevSpecificOffset);
	   TRACE("----------------------------------------------------------------\r\n");
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneSetButtonInfo(pPhone, dwRequestId, dwButtonId, lpPhoneInfo);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetButtonInfo rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSetButtonInfo

//////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetData
//
// This function downloads the information in the specified buffer
// to the opened phone device at the selected data id.
//
extern "C"
LONG TSPIAPI TSPI_phoneSetData (DRV_REQUESTID dwRequestId, HDRVPHONE hdPhone, 
               DWORD dwDataId, LPVOID const lpData, DWORD dwSize)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetData beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx RequestId=0x%lx\r\n", (DWORD) hdPhone, (DWORD) dwRequestId);
	   TRACE("  DataId=%ld, Buffer=%08lx, Size=%ld\r\n", dwDataId, (DWORD)lpData, dwSize);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneSetData(pPhone, dwRequestId, dwDataId, lpData, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetData rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSetData

//////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetDisplay
//
// This function causes the specified string to be displayed on the
// phone device.
//
extern "C"
LONG TSPIAPI TSPI_phoneSetDisplay (DRV_REQUESTID dwRequestID, 
         HDRVPHONE hdPhone, DWORD dwRow, DWORD dwCol, LPCSTR lpszDisplay,
         DWORD dwSize)   
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetDisplay beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx\r\n", (DWORD) hdPhone);
	   TRACE("  AsynchReqId=0x%lx, Row=%ld, Col=%ld\r\n", (DWORD)dwRequestID, dwRow, dwCol);
	   TRACE("  Display=<%s>, size=%ld\r\n", lpszDisplay, dwSize);
	}
#endif
   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone)
      lResult = GetSP()->phoneSetDisplay(pPhone, dwRequestID, dwRow, dwCol, lpszDisplay, dwSize);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetDisplay rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSetDisplay

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetGain
//
// This function sets the gain of the microphone of the specified hook
// switch device.
//
extern "C"
LONG TSPIAPI TSPI_phoneSetGain (DRV_REQUESTID dwRequestId, HDRVPHONE hdPhone, DWORD dwHookSwitchDev,
               DWORD dwGain)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{
	   TRACE("TSPI_phoneSetGain beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, RequestId=0x%lx, HookSwitchDev=%x, Gain=%lx\r\n", (DWORD)hdPhone, (DWORD)dwRequestId, dwHookSwitchDev, dwGain);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneSetGain(pPhone, dwRequestId, dwHookSwitchDev, dwGain);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetGain rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSetGain

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetHookSwitch
//
// This function sets the hook state of the specified open phone's
// hookswitch device to the specified mode.  Only the hookswitch
// state of the hookswitch devices listed is affected.
//
extern "C"
LONG TSPIAPI TSPI_phoneSetHookSwitch (DRV_REQUESTID dwRequestId, 
      HDRVPHONE hdPhone, DWORD dwHookSwitchDevs, DWORD dwHookSwitchMode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetHookSwitch beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD)dwRequestId);
	   TRACE("  HookswitchDevs=0x%lx, Mode=0x%lx\r\n", dwHookSwitchDevs, dwHookSwitchMode);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneSetHookSwitch(pPhone, dwRequestId, dwHookSwitchDevs, dwHookSwitchMode);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetHookSwitch rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSetHookSwitch

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetLamp
//
// This function causes the specified lamp to be set on the phone
// device to the specified mode.
//  
extern "C"
LONG TSPIAPI TSPI_phoneSetLamp (DRV_REQUESTID dwRequestId, HDRVPHONE hdPhone, 
               DWORD dwButtonLampId, DWORD dwLampMode)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetLamp beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD)hdPhone, (DWORD)dwRequestId);
	   TRACE("  LampId=0x%lx, Mode=0x%lx\r\n", dwButtonLampId, dwLampMode);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneSetLamp(pPhone, dwRequestId, dwButtonLampId, dwLampMode);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetLamp rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_phoneSetLamp

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetRing
//
// This function rings the specified open phone device using the
// specified ring mode and volume.
//
extern "C"
LONG TSPIAPI TSPI_phoneSetRing (DRV_REQUESTID dwRequestId, HDRVPHONE hdPhone, 
               DWORD dwRingMode, DWORD dwVolume)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetRing beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD)hdPhone, (DWORD)dwRequestId);
	   TRACE("  Ring mode=%ld, Volume=%ld\r\n", dwRingMode, dwVolume);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneSetRing(pPhone, dwRequestId, dwRingMode, dwVolume);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetRing rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_phoneSetRing

//////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetStatusMessages
//
// This function causes the service provider to filter status messages
// which are not currently of interest to any application.
//
extern "C"
LONG TSPIAPI TSPI_phoneSetStatusMessages (HDRVPHONE hdPhone, DWORD dwPhoneStates,
            DWORD dwButtonModes, DWORD dwButtonStates)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetStatusMessages beginning\r\n");
	   TRACE("  SP Phone handle=0x%lx, Modes=0x%lx, States=0x%lx\r\n", (DWORD)hdPhone, dwButtonModes, dwButtonStates);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneSetStatusMessages(pPhone, dwPhoneStates, dwButtonModes, dwButtonStates);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetStatusMessages rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSetStatusMessages

/////////////////////////////////////////////////////////////////////////
// TSPI_phoneSetVolume
//
// This function either sets the volume of the speaker or the 
// specified hookswitch device on the phone
//
extern "C"
LONG TSPIAPI TSPI_phoneSetVolume (DRV_REQUESTID dwRequestId, HDRVPHONE hdPhone, 
               DWORD dwHookSwitchDev, DWORD dwVolume)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_phoneSetVolume beginning\r\n");
	   TRACE("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n", (DWORD)hdPhone, (DWORD)dwRequestId);
	   TRACE("  HookSwitchDev=0x%lx, Volume=0x%lx\r\n", dwHookSwitchDev, dwVolume);
	}
#endif

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone)
      lResult = GetSP()->phoneSetVolume(pPhone, dwRequestId, dwHookSwitchDev, dwVolume);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_phoneSetVolume rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_phoneSetVolume

/******************************************************************************/
//
// TSPIAPI TSPI_provider functions
//
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////
// TSPI_providerConfig
//
// This function is invoked from the control panel and allows the user
// to configure our service provider.
//        
extern "C"
LONG TSPIAPI TSPI_providerConfig (HWND hwndOwner, DWORD dwPermanentProviderID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_providerConfig beginning\r\n");
	   TRACE("  hwndOwner=%08lx, ProviderId=0x%lx\r\n", CASTHANDLE(hwndOwner), dwPermanentProviderID);
	}
#endif

   LONG lResult = GetSP()->providerConfig(dwPermanentProviderID, CWnd::FromHandle(hwndOwner));

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_providerConfig rc=0x%lx\r\n", lResult);
#endif
   return lResult;
   
}// TSPI_providerConfig

///////////////////////////////////////////////////////////////////////////
// TSPI_providerInit
//
// This function is called when TAPI.DLL wants to initialize
// our service provider.
//
extern "C"
LONG TSPIAPI TSPI_providerInit (DWORD dwTSPIVersion,
         DWORD dwPermanentProviderID, DWORD dwLineDeviceIDBase,
         DWORD dwPhoneDeviceIDBase, DWORD dwNumLines, DWORD dwNumPhones,
         ASYNC_COMPLETION lpfnCompletionProc)
{       
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_providerInit beginning\r\n");
	    TRACE("  SPI Version=0x%lx, ProviderID=0x%lx\r\n", dwTSPIVersion, dwPermanentProviderID);
	    TRACE("  LineBase=%ld, Count=%ld, PhoneBase=%ld, Count=%ld\r\n", dwLineDeviceIDBase, dwNumLines, dwPhoneDeviceIDBase, dwNumPhones);
	    TRACE("  AsynchCompletionProc=%08lx\r\n", lpfnCompletionProc);
	    TRACE("  Reference Count=%d\r\n", g_iRefCount);
	}
#endif

    LONG lResult = GetSP()->providerInit(dwTSPIVersion, dwPermanentProviderID,
                        dwLineDeviceIDBase, dwPhoneDeviceIDBase, dwNumLines,
                        dwNumPhones, lpfnCompletionProc);
    if (lResult == 0)
        g_iRefCount++;
        
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_providerInit rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_providerInit

////////////////////////////////////////////////////////////////////////////
// TSPI_providerInstall
//
// This function is invoked to install the service provider onto
// the system.
//        
extern "C"
LONG TSPIAPI TSPI_providerInstall(HWND hwndOwner, DWORD dwPermanentProviderID)
{  
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_providerInstall beginning\r\n");
	   TRACE("  hwndOwner=%08lx, ProviderId=0x%lx\r\n", CASTHANDLE(hwndOwner), dwPermanentProviderID);
	}
#endif

   LONG lResult = GetSP()->providerInstall(dwPermanentProviderID, CWnd::FromHandle(hwndOwner));

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_providerInstall rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_providerInstall

////////////////////////////////////////////////////////////////////////////
// TSPI_providerRemove
//
// This function removes the service provider
//
extern "C"
LONG TSPIAPI TSPI_providerRemove(HWND hwndOwner, DWORD dwPermanentProviderID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_providerRemove beginning\r\n");
	   TRACE("  hwndOwner=%08lx, ProviderId=0x%lx\r\n", CASTHANDLE(hwndOwner), dwPermanentProviderID);
	}
#endif

   LONG lResult = GetSP()->providerRemove(dwPermanentProviderID, CWnd::FromHandle(hwndOwner));

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_providerRemove rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_providerRemove

///////////////////////////////////////////////////////////////////////////
// TSPI_providerShutdown
//
// This function is called when the TAPI.DLL is shutting down our
// service provider.
//
extern "C"
LONG TSPIAPI TSPI_providerShutdown (DWORD dwTSPIVersion)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	    TRACE("TSPI_providerShutdown beginning\r\n");
	    TRACE("  SPI Version=0x%lx\r\n", dwTSPIVersion);
	    TRACE("  Reference Count=%d\r\n", g_iRefCount);
	}
#endif
    
    // Decrement our reference count and call the shutdown event.
    g_iRefCount--;
    LONG lResult = GetSP()->providerShutdown(dwTSPIVersion);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
    	TRACE("TSPI_providerShutdown rc=0x%lx\r\n", lResult);
#endif
    return lResult;

}// TSPI_providerShutdown

////////////////////////////////////////////////////////////////////////////
// TSPI_providerEnumDevices (Win95)
//
// This function is called before the TSPI_providerInit to determine
// the number of line and phone devices supported by the service provider.
// If the function is not available, then TAPI will read the information
// out of the TELEPHON.INI file per TAPI 1.0.
//
extern "C"
LONG TSPIAPI TSPI_providerEnumDevices (DWORD dwPermanentProviderID, LPDWORD lpdwNumLines,
         LPDWORD lpdwNumPhones, HPROVIDER hProvider, LINEEVENT lpfnLineCreateProc,
         PHONEEVENT lpfnPhoneCreateProc)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_providerEnumDevices beginning\r\n");
	   TRACE("  ProviderId=0x%lx, hProvider=0x%lx\r\n", dwPermanentProviderID, (DWORD)hProvider);
	}
#endif

   LONG lResult = GetSP()->providerEnumDevices(dwPermanentProviderID, lpdwNumLines,
            lpdwNumPhones, hProvider, lpfnLineCreateProc, lpfnPhoneCreateProc);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_providerEnumDevices NumLines=%ld, NumPhones=%ld, rc=0x%lx\r\n", *lpdwNumLines, *lpdwNumPhones, lResult);
#endif
   return lResult;

}// TSPI_providerEnumDevices

/////////////////////////////////////////////////////////////////////////////
// TSPI_providerCreateLineDevice  (Win95)
//
// This function is called by TAPI in response to the receipt of a 
// LINE_CREATE message from the service provider which allows the dynamic
// creation of a new line device.  The passed deviceId identifies this
// line from TAPIs perspective.
//
extern "C"
LONG TSPIAPI TSPI_providerCreateLineDevice (DWORD dwTempID, DWORD dwDeviceID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_providerCreateLineDevice beginning\r\n");
	   TRACE("  TempId=0x%lx, DeviceId=0x%lx\r\n", dwTempID, dwDeviceID);
	}
#endif

   LONG lResult = GetSP()->providerCreateLineDevice(dwTempID, dwDeviceID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	   TRACE("TSPI_providerCreateLineDevice rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_providerCreateLineDevice

/////////////////////////////////////////////////////////////////////////////
// TSPI_providerCreatePhoneDevice (Win95)
//
// This function is called by TAPI in response to the receipt of a
// PHONE_CREATE message from the service provider which allows the dynamic
// creation of a new phone device.  The passed deviceId identifies this
// phone from TAPIs perspective.
//
extern "C"
LONG TSPIAPI TSPI_providerCreatePhoneDevice (DWORD dwTempID, DWORD dwDeviceID)
{
#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
	{                                              
	   TRACE("TSPI_providerCreatePhoneDevice beginning\r\n");
	   TRACE("  TempId=0x%lx, DeviceId=0x%lx\r\n", dwTempID, dwDeviceID);
	}
#endif

   LONG lResult = GetSP()->providerCreatePhoneDevice(dwTempID, dwDeviceID);

#ifdef _DEBUG
	if (g_iShowAPITraceLevel)
   		TRACE("TSPI_providerCreatePhoneDevice rc=0x%lx\r\n", lResult);
#endif
   return lResult;

}// TSPI_providerCreatePhoneDevice


