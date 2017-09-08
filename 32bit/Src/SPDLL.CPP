/******************************************************************************/
//                                                                        
// SPDLL.CPP - Service Provider DLL shell.                                
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This module intercepts the TSPI calls and invokes the SP object        
// with the appropriate parameters.                                       
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
#include <ctype.h>
#include <oleauto.h>
#include <atlconv.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// Standard method usable in Win32 and Win16 to cast a handle.
#define CASTHANDLE(h) ((DWORD)(UINT)h)
#define MB_SERVICE_NOTIFICATION 0x00200000L

/******************************************************************************/
//
// PUBLIC UTILITY FUNCTIONS
//
/******************************************************************************/

extern LPCTSTR gszTelephonyKey;

#ifdef _DEBUG
int g_iShowAPITraceLevel = 0;
#endif

///////////////////////////////////////////////////////////////////////////
// IsDeletedObject
//
// Simple utility function to determine if the passed object is deleted.
//
bool IsDeletedObject(CTSPILineConnection* pObject)
{
	__try
	{
		// Check to see if we can access the memory at this location.
		if (IsBadWritePtr(pObject, sizeof(CTSPILineConnection)) ||
			pObject == NULL || !pObject->IsLineDevice() || pObject->HasBeenDeleted())
			return true;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// Not an object!
		return true;
	}

	// Object looks ok
	return false;

}// IsDeletedObject

///////////////////////////////////////////////////////////////////////////
// IsDeletedObject
//
// Simple utility function to determine if the passed object is deleted.
//
bool IsDeletedObject(CTSPIPhoneConnection* pObject)
{
	__try
	{
		if (IsBadWritePtr(pObject, sizeof(CTSPIPhoneConnection)) ||
			pObject == NULL || !pObject->IsPhoneDevice() || pObject->HasBeenDeleted())
			return true;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		// Not an object!
		return true;
	}

	// No other checks.
	return false;

}// IsDeletedObject

///////////////////////////////////////////////////////////////////////////
// IsDeletedObject
//
// Utility function to determine if the passed object is deleted.
//
BOOL IsDeletedObject(void* pBuffer)
{
	return (IsBadWritePtr(pBuffer, sizeof(DWORD)));

}// IsDeletedObject

#ifndef _UNICODE
///////////////////////////////////////////////////////////////////////////
// ConvertWideToAnsi
//
// Utility function included with non-UNICODE build to convert the
// UNICODE strings to normal ANSI single-byte strings.
//
CString ConvertWideToAnsi (LPCWSTR lpszInput)
{
	CString strReturn;
	if (lpszInput != NULL)
	{
		int iSize = WideCharToMultiByte (CP_ACP, 0, lpszInput, -1, NULL, 0, NULL, NULL);
		if (iSize > 0)
		{
			LPSTR lpszBuff = strReturn.GetBuffer(iSize+1);
			if (lpszBuff != NULL)
				WideCharToMultiByte (CP_ACP, 0, lpszInput, -1, lpszBuff, iSize, NULL, NULL);
			strReturn.ReleaseBuffer();
		}
	}
	return strReturn;

}// ConvertWideToAnsi
#endif // _UNICODE

///////////////////////////////////////////////////////////////////////////
// CopyVarString
//
// Copy a string (UNICODE or not) into a variable string buffer
//
void CopyVarString(LPVARSTRING lpVarString, LPCTSTR lpszBuff)
{
	if (lpVarString->dwStringFormat != STRINGFORMAT_UNICODE &&
		lpVarString->dwStringFormat != STRINGFORMAT_ASCII)
	{
#ifdef _UNICODE
		lpVarString->dwStringFormat = STRINGFORMAT_UNICODE;
#else
		lpVarString->dwStringFormat = STRINGFORMAT_ASCII;
#endif
	}

	lpVarString->dwNeededSize = sizeof(VARSTRING);
	lpVarString->dwUsedSize = sizeof(VARSTRING);

	USES_CONVERSION;
	BOOL fSuccess = FALSE;
	if (lpVarString->dwStringFormat == STRINGFORMAT_UNICODE)
	{
		fSuccess = AddDataBlock(lpVarString, lpVarString->dwStringOffset, lpVarString->dwStringSize, T2W(const_cast<TCHAR*>(lpszBuff)));
	}
	else // dwStringFormat = STRINGFORMAT_ASCII
	{
		fSuccess = AddDataBlock(lpVarString, lpVarString->dwStringOffset, lpVarString->dwStringSize, T2A(const_cast<TCHAR*>(lpszBuff)));
	}

	if (fSuccess)
		lpVarString->dwNeededSize += lpVarString->dwStringSize;
	else
	{
#ifdef _UNICODE
		lpVarString->dwNeededSize += (lstrlen(lpszBuff)+1) * sizeof(TCHAR);
#else
		lpVarString->dwNeededSize += MultiByteToWideChar(CP_ACP, 0, lpszBuff, -1, NULL, 0) * sizeof(WCHAR);
#endif
	}

}// CopyVarString

///////////////////////////////////////////////////////////////////////////
// CopyVarString
//
// Copy a non-typed buffer into a variable string buffer
//
void CopyVarString (LPVARSTRING lpVarString, LPVOID lpBuff, DWORD dwSize)
{                
	lpVarString->dwNeededSize = sizeof(VARSTRING) + dwSize;
	lpVarString->dwStringFormat = STRINGFORMAT_BINARY;
	if (lpVarString->dwTotalSize >= lpVarString->dwNeededSize)
	{
		lpVarString->dwStringSize = dwSize;
		lpVarString->dwStringOffset = sizeof(VARSTRING);
		CopyBuffer ((LPSTR)lpVarString+sizeof(VARSTRING), lpBuff, dwSize);
	}   		

}// CopyVarString

///////////////////////////////////////////////////////////////////////////
// AddDataBlock
//
// Public function to add a string to a VARSTRING type buffer.
//
BOOL AddDataBlock (LPVOID lpVB, DWORD& dwOffset, DWORD& dwSize, LPCSTR lpszBuff)
{
	// Convert the ANSI to UNICODE
	BOOL fRC = FALSE;
	int iSize = MultiByteToWideChar(CP_ACP, 0, lpszBuff, -1, NULL, 0);
	if (iSize > 0)
	{
		// Adjust the NEEDED size for UNICODE strings when the passed
		// data was a normal string (and therefore calculated incorrectly).
		LPVARSTRING lpVarBuffer = (LPVARSTRING) lpVB;
		lpVarBuffer->dwNeededSize += (iSize*sizeof(wchar_t));

		// Convert the buffer
		LPWSTR lpMyString = (LPWSTR) AllocMem (iSize*sizeof(wchar_t));
		if (lpMyString != NULL)
		{
			MultiByteToWideChar(CP_ACP, 0, lpszBuff, -1, lpMyString, iSize);
			fRC = AddDataBlock (lpVB, dwOffset, dwSize, lpMyString, iSize*sizeof(wchar_t));
			FreeMem (lpMyString);
		}
	}
	return fRC;

}// AddDataBlock

///////////////////////////////////////////////////////////////////////////
// AddDataBlock
//
// Public function to adjust a variable length structure field for 
// the library.  This may be called multiple times to add the same 
// information into a structure.  The offset will only be adjusted if
// the information in the record is zero.
//
BOOL AddDataBlock (LPVOID lpVB, DWORD& dwOffset, DWORD& dwSize, 
				   LPCVOID lpBuffer, DWORD dwBufferSize)
{
	// If the buffer size is zero, ignore this.
	if (dwBufferSize == 0)
		return FALSE;
	
	// Cast our block to something which has the first three fields 
	// in the same order as all our variable structures defined by TAPI.
	LPVARSTRING lpVarBuffer = (LPVARSTRING) lpVB;

	DWORD dwUsedSize = lpVarBuffer->dwUsedSize;
#ifndef _X86_
	// Make sure the used size field is DWORD-aligned.  This is required
	// for most non-Intel machines.
	while ((dwUsedSize % 4) != 0)
	{
		dwUsedSize++;
		lpVarBuffer->dwNeededSize++;
	}
#endif

	// If we don't have enough room for the buffer here then exit.
	if (lpVarBuffer->dwTotalSize < dwUsedSize+dwBufferSize)
		return FALSE;

	// We have enough room, save the new used size.
	lpVarBuffer->dwUsedSize = dwUsedSize;

	// Fill in the buffer.
	CopyBuffer ((LPSTR)lpVarBuffer+lpVarBuffer->dwUsedSize, lpBuffer, dwBufferSize);

	// Adjust the offset and size.  We only adjust the offset if it is zero
	// since we may be APPENDING data to an existing block.
	if (dwOffset == 0)
		dwOffset = lpVarBuffer->dwUsedSize;
	dwSize += dwBufferSize;
	lpVarBuffer->dwUsedSize += dwBufferSize;

	// Added the block successfully.
	return TRUE;
	
}// AddDataBlock

#ifdef _DEBUG
///////////////////////////////////////////////////////////////////////////
// DumpMem
//
// Dump out a block of memory in HEX for the specified amount of bytes
//
void DumpMem (LPCTSTR lpszTitle, LPVOID lpBuff, DWORD dwSize)
{   
    if (lpBuff == NULL || dwSize == 0 || DbgTraceLevel() < TRC_DUMP)
        return;

    if (lpszTitle)
        DTRACE (TRC_DUMP, lpszTitle);

	if (IsBadReadPtr (lpBuff, dwSize))
	{
		DTRACE(TRC_DUMP, _T("Invalid pointer/size 0x%lx, %d\r\n"), (DWORD)lpBuff, dwSize);
		return;
	}

    LPCTSTR lpByte = (LPCTSTR) lpBuff;
    DWORD dwCount = 0, dwLine = 0;
    while (dwCount < dwSize)
    {
        TCHAR b[17];
        static TCHAR szBuff[256];
        
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
        _stprintf (szBuff, _T("%0.8lX   %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X ")\
                           _T("%0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X %0.2X    "), 
						   dwLine,
                           (int)b[0]&0xff, (int)b[1]&0xff, (int)b[2]&0xff, (int)b[3]&0xff, 
						   (int)b[4]&0xff, (int)b[5]&0xff, (int)b[6]&0xff, (int)b[7]&0xff, 
						   (int)b[8]&0xff, (int)b[9]&0xff, (int)b[10]&0xff,(int)b[11]&0xff, 
						   (int)b[12]&0xff,(int)b[13]&0xff,(int)b[14]&0xff,(int)b[15]&0xff);
        dwLine = dwCount;
                      
        // Now do the ASCII portion.
        for (i = 0; i < 16; i++)
        {
            if (!_istprint(b[i]))
                b[i] = '.';
        }
        
        b[16] = _T('\0');
        _tcscat (szBuff, (LPCTSTR)b);
        DTRACE (TRC_DUMP, _T("%s\r\n"), szBuff);
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
        DTRACE (TRC_STRUCT, _T("<NULL>\r\n"));
    else
    {
		DTRACE (TRC_STRUCT, _T("VarString 0x%lx\r\n"), (DWORD) lpVarString);
        DTRACE (TRC_STRUCT, _T("  dwTotalSize = 0x%lX\r\n"), lpVarString->dwTotalSize);
        DTRACE (TRC_STRUCT, _T("  dwNeededSize = 0x%lX\r\n"), lpVarString->dwNeededSize);
        DTRACE (TRC_STRUCT, _T("  dwUsedSize = 0x%lX\r\n"), lpVarString->dwUsedSize);
        switch (lpVarString->dwStringFormat)
        {
            case STRINGFORMAT_ASCII:
                DTRACE (TRC_STRUCT, _T("  Format = STRINGFORMAT_ASCII\r\n"));
                break;
            case STRINGFORMAT_BINARY:
                DTRACE (TRC_STRUCT, _T("  Format = STRINGFORMAT_BINARY\r\n"));
                break;
            case STRINGFORMAT_DBCS:
                DTRACE (TRC_STRUCT, _T("  Format = STRINGFORMAT_DBCS\r\n"));
                break;
            case STRINGFORMAT_UNICODE:
                DTRACE (TRC_STRUCT, _T("  Format = STRINGFORMAT_UNICODE\r\n"));
                break;
            default:
                DTRACE (TRC_STRUCT, _T("  Format = UNKNOWN!\r\n"));
                break;
        }     
        
        DTRACE (TRC_STRUCT, _T("  dwStringSize = 0x%lX\r\n"), lpVarString->dwStringSize);
        DTRACE (TRC_STRUCT, _T("  dwStringOffset = 0x%lX\r\n"), lpVarString->dwStringOffset);

        if (lpVarString->dwStringOffset > 0 && lpVarString->dwStringSize > 0)
        {
			if (IsBadReadPtr ((LPCTSTR)lpVarString + lpVarString->dwStringOffset, lpVarString->dwStringSize))
			{
				DTRACE(TRC_STRUCT, _T("** Size/Offset results in bad pointer **\r\n"));
			}
			else
			{
				if (lpVarString->dwStringFormat == STRINGFORMAT_ASCII)
					DTRACE (TRC_STRUCT, _T("  Value = <%s>\r\n"), (LPCTSTR)((LPCTSTR)lpVarString + lpVarString->dwStringOffset));
				else
					DUMPMEM (_T("  Value\r\n"), (LPVOID)((LPCTSTR)lpVarString + lpVarString->dwStringOffset), lpVarString->dwStringSize);
            }
        }                            
    }

}// DumpVarString

///////////////////////////////////////////////////////////////////////////
// _AssertFailedLine
//
// Replacement for the ASSERT functionallity of MFC.  Under NT,
// the TAPI context is actually a seperate subsystem run in its own
// screen group, and therefore cannot do any kind of Win32 UI.
//
void _stdcall _AssertFailedLine(LPCSTR lpszFileName, int nLine, LPCSTR pszExpr)
{
	TCHAR chBuff[_MAX_PATH * 2];

	if (pszExpr != NULL)
		wsprintf(chBuff, _T("ASSERT(%hs)\r\n failed in %hs at %d\r\n"), pszExpr, lpszFileName, nLine);
	else
		wsprintf(chBuff, _T("ASSERT failed in %hs at %d\r\n"), lpszFileName, nLine);

	_TspTrace2(chBuff);

	if (::MessageBox (NULL, chBuff, _T("Assertion Failed"), 
			MB_ABORTRETRYIGNORE | MB_SETFOREGROUND | MB_SERVICE_NOTIFICATION) == IDABORT)
			_asm int 3;

	// You should stop here if abort is pressed.
	return;

}// _AssertFailedLine

///////////////////////////////////////////////////////////////////////////
// DbgTraceLevel
//
// Returns the current trace level from the registry.
//
int DbgTraceLevel()
{
	return max((int)GetSP()->ReadProfileDWord(0, _T("DebugLevel"), 0), 
				g_iShowAPITraceLevel);

}// DbgTraceLevel

///////////////////////////////////////////////////////////////////////////
// _TspTrace
//
// Internal trace function which is used to output information from
// the class library.
//
void _cdecl _TspTrace(int iTraceLevel, LPCTSTR pszFormat, ...)
{
	// Don't do the trace if we aren't at the right level.
	if (DbgTraceLevel() < iTraceLevel)
		return;

	va_list args;
	va_start(args, pszFormat);

	TCHAR szBuffer[512];
	int nBuf = _vsntprintf(szBuffer, sizeof(szBuffer)/sizeof(TCHAR), pszFormat, args);
	ASSERT (nBuf < 512);
	va_end(args);

	if (nBuf > 0)
		GetSP()->TraceOut(szBuffer);

}// _TspTrace

///////////////////////////////////////////////////////////////////////////
// _TspTrace2
//
// Internal trace function which is used to output information from
// the class library.
//
void _cdecl _TspTrace2(LPCTSTR pszFormat, ...)
{
	va_list args;
	va_start(args, pszFormat);

	TCHAR szBuffer[512];
	int nBuf = _vsntprintf(szBuffer, sizeof(szBuffer)/sizeof(TCHAR), pszFormat, args);
	ASSERT (nBuf < 512);
	va_end(args);

	if (nBuf > 0)
		GetSP()->TraceOut(szBuffer);

}// _TspTrace2

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
	DTRACE(TRC_API, _T("TSPI_lineAccept beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  UserInfo=0x%lx, Size=%ld\r\n"), (DWORD)lpsUserUserInfo, dwSize);
	DUMPMEM(_T("UserInfo->\r\n"), (LPVOID)lpsUserUserInfo, dwSize);
 
	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineAccept(pCall, dwRequestId, lpsUserUserInfo, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineAccept rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineAddToConference beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx\r\n"), (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  Conference=0x%lx, Call=0x%lx\r\n"), (DWORD) hdConfCall, (DWORD) hdConsultCall);

	CTSPIConferenceCall* pConf = (CTSPIConferenceCall*) hdConfCall;
	ASSERT(pConf && pConf->IsKindOf(RUNTIME_CLASS(CTSPIConferenceCall)));
	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdConsultCall;
	ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && pConf && !IsDeletedObject(pCall) && !IsDeletedObject(pConf))
		lResult = GetSP()->lineAddToConference(pConf, pCall, dwRequestId);

	DTRACE(TRC_API, _T("TSPI_lineAddToConference rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineAnswer beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  UserInfo=0x%lx, Size=%ld\r\n"), (DWORD)lpsUserUserInfo, dwSize);
	DUMPMEM (_T("UserInfo->\r\n"), (LPVOID)lpsUserUserInfo, dwSize);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineAnswer(pCall, dwRequestId, lpsUserUserInfo, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineAnswer rc=0x%lx\r\n"), lResult);
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
         HDRVCALL hdCall, LPCWSTR lpszDestAddr, DWORD dwCountryCode)
{
#ifdef _UNICODE
	CString strDestAddr = lpszDestAddr;
#else
	CString strDestAddr = ConvertWideToAnsi (lpszDestAddr);
#endif

	DTRACE(TRC_API, _T("TSPI_lineBlindTransfer beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  DestAddr=<%s>, Country=0x%lx\r\n"), strDestAddr, dwCountryCode);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineBlindTransfer(pCall, dwRequestId, strDestAddr, dwCountryCode);

	DTRACE(TRC_API, _T("TSPI_lineBlindTransfer rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineClose beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx\r\n"), (DWORD)hdLine);

	CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
	ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pLine && !IsDeletedObject(pLine))
		lResult = GetSP()->lineClose(pLine);

   DTRACE(TRC_API, _T("TSPI_lineClose rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineCloseCall beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx\r\n"), (DWORD)hdCall);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineCloseCall(pCall);

	DTRACE(TRC_API, _T("TSPI_lineCloseCall rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineCompleteCall beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  Completion Mode=0x%lx, MessageID=0x%lx\r\n"), dwCompletionMode, dwMessageID);

    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall && pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    LONG lResult = LINEERR_INVALCALLHANDLE;
    
	// Fill in with zero in case of error.
	*lpdwCompletionID = 0;

    // Validate the call completion mode.
    if (dwCompletionMode != LINECALLCOMPLMODE_CAMPON &&
        dwCompletionMode != LINECALLCOMPLMODE_CALLBACK &&
        dwCompletionMode != LINECALLCOMPLMODE_INTRUDE &&
        dwCompletionMode != LINECALLCOMPLMODE_MESSAGE)
        lResult = LINEERR_INVALCALLCOMPLMODE;        
    
    // Pass it through the Service provider.
    else if (pCall && !IsDeletedObject(pCall))
    {
        lResult = GetSP()->lineCompleteCall(pCall, dwRequestId, lpdwCompletionID, 
                        dwCompletionMode, dwMessageID);
    }

	DTRACE(TRC_API, _T("TSPI_lineCompleteCall CompletionID=0x%lx, rc=0x%lx\r\n"), *lpdwCompletionID, lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineCompleteTransfer beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  ConsultCall=0x%lx, TAPIConfCall=0x%lx, XferMode=0x%lx\r\n"), (DWORD)hdConsultCall, (DWORD)htConfCall, dwTransferMode);

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
        if (pCall && pConsult && !IsDeletedObject(pCall) && !IsDeletedObject(pConsult))
        {
            lResult = GetSP()->lineCompleteTransfer(pCall, dwRequestId, pConsult, 
                                    htConfCall, lphdConfCall, dwTransferMode);
#ifdef _DEBUG
            if (lphdConfCall != NULL)
                DTRACE(TRC_API, _T("  SP ConfHandle=0x%lx\r\n"), (DWORD) *lphdConfCall);
			else
				DTRACE(TRC_API, _T("  SP ConfHandle=NULL\r\n"));
#endif
        }
        else
            lResult = LINEERR_INVALCALLHANDLE;
    }
    
	DTRACE(TRC_API, _T("TSPI_lineCompleteTransfer rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineConditionalMediaDetection beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, MediaModes=0x%lx\r\n"), (DWORD)hdLine, dwMediaModes);
	DTRACE(TRC_STRUCT, _T("Dumping LINECALLPARAMS at %08lx\r\n"), (DWORD)lpCallParams);
	DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
	DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpCallParams->dwTotalSize);
	DTRACE(TRC_STRUCT, _T("  dwBearerMode\t\t=0x%lx\r\n"), lpCallParams->dwBearerMode);
	DTRACE(TRC_STRUCT, _T("  dwMinRate\t\t=0x%lx\r\n"), lpCallParams->dwMinRate);
	DTRACE(TRC_STRUCT, _T("  dwMaxRate\t\t=0x%lx\r\n"), lpCallParams->dwMaxRate);
	DTRACE(TRC_STRUCT, _T("  dwMediaMode\t\t=0x%lx\r\n"), lpCallParams->dwMediaMode);
	DTRACE(TRC_STRUCT, _T("  dwCallParamFlags\t\t=0x%lx\r\n"), lpCallParams->dwCallParamFlags);
	DTRACE(TRC_STRUCT, _T("  dwAddressMode\t\t=0x%lx\r\n"), lpCallParams->dwAddressMode);
	DTRACE(TRC_STRUCT, _T("  dwAddressID\t\t=0x%lx\r\n"), lpCallParams->dwAddressID);
	DTRACE(TRC_STRUCT, _T("  dwOrigAddressSize\t\t=0x%lx\r\n"), lpCallParams->dwOrigAddressSize);
	DTRACE(TRC_STRUCT, _T("  dwOrigAddressOffset\t\t=0x%lx\r\n"), lpCallParams->dwOrigAddressOffset);
	DTRACE(TRC_STRUCT, _T("  dwDisplayableAddressSize\t\t=0x%lx\r\n"), lpCallParams->dwDisplayableAddressSize);
	DTRACE(TRC_STRUCT, _T("  dwDisplayableAddressOffset\t\t=0x%lx\r\n"), lpCallParams->dwDisplayableAddressOffset);
	DTRACE(TRC_STRUCT, _T("  dwCalledPartySize\t\t=0x%lx\r\n"), lpCallParams->dwCalledPartySize);
	DTRACE(TRC_STRUCT, _T("  dwCalledPartyOffset\t\t=0x%lx\r\n"), lpCallParams->dwCalledPartyOffset);
	DTRACE(TRC_STRUCT, _T("  dwCommentSize\t\t=0x%lx\r\n"), lpCallParams->dwCommentSize);
	DTRACE(TRC_STRUCT, _T("  dwCommentOffset\t\t=0x%lx\r\n"), lpCallParams->dwCommentOffset);
	DTRACE(TRC_STRUCT, _T("  dwUserUserInfoSize\t\t=0x%lx\r\n"), lpCallParams->dwUserUserInfoSize);
	DTRACE(TRC_STRUCT, _T("  dwUserUserInfoOffset\t\t=0x%lx\r\n"), lpCallParams->dwUserUserInfoOffset);
	DTRACE(TRC_STRUCT, _T("  dwHighLevelCompSize\t\t=0x%lx\r\n"), lpCallParams->dwHighLevelCompSize);
	DTRACE(TRC_STRUCT, _T("  dwHighLevelCompOffset\t\t=0x%lx\r\n"), lpCallParams->dwHighLevelCompOffset);
	DTRACE(TRC_STRUCT, _T("  dwLowLevelCompSize\t\t=0x%lx\r\n"), lpCallParams->dwLowLevelCompSize);
	DTRACE(TRC_STRUCT, _T("  dwLowLevelCompOffset\t\t=0x%lx\r\n"), lpCallParams->dwLowLevelCompOffset);
	DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpCallParams->dwDevSpecificSize);
	DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpCallParams->dwDevSpecificOffset);
	DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));

	CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
	ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pLine && !IsDeletedObject(pLine))
		lResult = GetSP()->lineConditionalMediaDetection(pLine, dwMediaModes, lpCallParams);

	DTRACE(TRC_API, _T("TSPI_lineConditionalMediaDetection rc=0x%lx\r\n"), lResult);
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
LONG TSPIAPI TSPI_lineConfigDialog (
DWORD /*dwDeviceID*/, 
HWND /*hwndOwner*/, 
LPCSTR /*lpszDeviceClass*/)
{
	return LINEERR_OPERATIONUNAVAIL;

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
LONG TSPIAPI TSPI_lineConfigDialogEdit (
DWORD /*dwDeviceID*/, 
HWND /*hwndOwner*/,
LPCSTR /*lpszDeviceClass*/, 
LPVOID const /*lpDeviceConfigIn*/, 
DWORD /*dwSize*/,
LPVARSTRING /*lpDeviceConfigOut*/)
{
	return LINEERR_OPERATIONUNAVAIL;

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
	DTRACE(TRC_API, _T("TSPI_lineDevSpecific beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AddressId=0x%lx\r\n"), (DWORD)hdLine, dwAddressId);
	DTRACE(TRC_API, _T("  LPARAMS=0x%lx, size=%d\r\n"), (DWORD) lpParams, dwSize);
	DUMPMEM (_T("LPARAMS ->\r\n"), lpParams, dwSize);

    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    CTSPIAddressInfo* pAddr = NULL;
    
    if (pLine != NULL)
        pAddr = pLine->GetAddress(dwAddressId);

    LONG lResult = GetSP()->lineDevSpecific(pLine, pAddr, pCall, dwRequestId, lpParams, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineDevSpecific rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineDevSpecificFeature beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdLine, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  Feature=0x%lx, LPARAMS=0x%lx, size=%d\r\n"), (DWORD) dwFeature, (DWORD) lpParams, dwSize);
	DUMPMEM (_T("LPARAMS ->\r\n"), lpParams, dwSize);

    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    ASSERT(pLine && pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
   
    LONG lResult = LINEERR_INVALLINEHANDLE;
    if (pLine && !IsDeletedObject(pLine))
        lResult = GetSP()->lineDevSpecificFeature(pLine, dwFeature, dwRequestId, lpParams, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineDevSpecificFeature rc=0x%lx\r\n"), lResult);
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
         LPCWSTR lpszDestAddress, DWORD dwCountryCode)
{
#ifdef _UNICODE	
	CString strDestAddr = lpszDestAddress;
#else
	CString strDestAddr = ConvertWideToAnsi (lpszDestAddress);
#endif

	DTRACE(TRC_API, _T("TSPI_lineDial beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), dwRequestID, (DWORD)hdCall);
	DTRACE(TRC_API, _T("  Destination=<%s>, Country=%d\r\n"), (LPCTSTR) strDestAddr, dwCountryCode);

    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    LONG lResult = LINEERR_INVALCALLHANDLE;

    if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineDial(pCall, dwRequestID, strDestAddr, dwCountryCode);

    DTRACE(TRC_API, _T("TSPI_lineDial rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineDrop beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), dwRequestID, (DWORD)hdCall);
	DTRACE(TRC_API, _T("  UserInfo=%08lx, Size=%d\r\n"), (DWORD)lpsUserUserInfo, dwSize);
	DUMPMEM (_T("UserInfo->\r\n"), (LPVOID)lpsUserUserInfo, dwSize);
        
    CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
    ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
    LONG lResult = LINEERR_INVALCALLHANDLE;

    if (pCall && !IsDeletedObject(pCall))
        lResult = GetSP()->lineDrop(pCall, dwRequestID, lpsUserUserInfo, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineDrop rc=0x%lx\r\n"), lResult);
    return lResult;

}// TSPI_lineDrop

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
	DTRACE(TRC_API, _T("TSPI_lineForward beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdLine, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  TAPI Call handle=0x%lx\r\n"), (DWORD) htConsultCall);
	DTRACE(TRC_API, _T("  All=%d, Address=0x%lx, NumRings=%d\r\n"), bAllAddresses, dwAddressId, dwNumRingsAnswer);

    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    ASSERT(pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    CTSPIAddressInfo* pAddr = NULL;

    LONG lResult = 0;
    if (pLine && !IsDeletedObject(pLine))
    {
        if (bAllAddresses == FALSE)
        {
            pAddr = pLine->GetAddress(dwAddressId);
            if (pAddr == NULL || IsDeletedObject(pAddr))
                lResult = LINEERR_INVALADDRESSID;
        }

        if (lResult == 0)
        {
            lResult = GetSP()->lineForward(pLine, pAddr, dwRequestId, lpForwardList,
                                     dwNumRingsAnswer, htConsultCall, lphdConsultCall,
                                     lpCallParams);
			DTRACE(TRC_API, _T("  Returned SP call handle=0x%lx"), *lphdConsultCall);
        }
    }
    else
        lResult = LINEERR_INVALLINEHANDLE;

	DTRACE(TRC_API, _T("TSPI_lineForward rc=0x%lx\r\n"), lResult);
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
         DWORD dwDigitModes, LPWSTR lpszDigits, DWORD dwNumDigits,
         LPCWSTR lpszTerminationDigits, DWORD dwFirstDigitTimeout,
         DWORD dwInterDigitTimeout)
{
#ifdef _UNICODE
	CString strTermDigits = lpszTerminationDigits;
#else
	CString strTermDigits = ConvertWideToAnsi (lpszTerminationDigits);
#endif
	
	DTRACE(TRC_API, _T("TSPI_lineGatherDigits beginning\r\n"));
	DTRACE(TRC_API, _T("  Call Handle=0x%lx, EndId=0x%lx\r\n"), (DWORD) hdCall, dwEndToEnd);
	DTRACE(TRC_API, _T("  Mode=0x%lx, Buff=%08lx, Size=%ld\r\n"), dwDigitModes, (DWORD) lpszDigits, dwNumDigits);
	DTRACE(TRC_API, _T("  Termination=<%s>, First TO=%ld, Inter TO=%ld\r\n"), (LPCTSTR)strTermDigits, dwFirstDigitTimeout, dwInterDigitTimeout);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

	LONG lResult = LINEERR_INVALCALLHANDLE;
	if (pCall && !IsDeletedObject(pCall))
	{
		lResult = GetSP()->lineGatherDigits(pCall, dwEndToEnd, dwDigitModes, 
						   lpszDigits, dwNumDigits, strTermDigits, 
						   dwFirstDigitTimeout, dwInterDigitTimeout);
	}

	DTRACE(TRC_API, _T("TSPI_lineGatherDigits rc=0x%lx\r\n"), lResult);
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
         DWORD dwDigitMode, LPCWSTR lpszDigits, DWORD dwDuration)
{
#ifdef _UNICODE
	CString strDigits = lpszDigits;
#else
	CString strDigits = ConvertWideToAnsi (lpszDigits);
#endif

	DTRACE(TRC_API, _T("TSPI_lineGenerateDigits beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, EndToEndId=0x%lx\r\n"), (DWORD)hdCall, dwEndToEndID);
	DTRACE(TRC_API, _T("  DigitMode=0x%lx, Digits=<%s>, Duration=%d\r\n"), dwDigitMode, (LPCTSTR)strDigits, dwDuration);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineGenerateDigits(pCall, dwEndToEndID, dwDigitMode, 
								strDigits, dwDuration);

	DTRACE(TRC_API, _T("TSPI_lineGenerateDigits rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGenerateTone beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, EndToEndId=0x%lx\r\n"), (DWORD)hdCall, dwEndToEnd);
	DTRACE(TRC_API, _T("  ToneMode=0x%lx, Duration=%ld Count=%ld\r\n"), dwToneMode, dwDuration, dwNumTones);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineGenerateTone(pCall, dwEndToEnd, dwToneMode, dwDuration, dwNumTones, lpTones);

	DTRACE(TRC_API, _T("TSPI_lineGenerateTone rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetAddressCaps beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, AddressId=0x%lx, SP Version=0x%lx\r\n"), dwDeviceID, dwAddressID, dwTSPIVersion);
	DTRACE(TRC_API, _T("  ExtVersion=0x%lx, AddressCaps=%08lx\r\n"), dwExtVersion, (DWORD)lpAddressCaps);

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
			if (lResult == 0)
			{
				DTRACE(TRC_STRUCT, _T("Dumping LINEADDRESSCAPS at %08lx\r\n"), (DWORD)lpAddressCaps);
				DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
				DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpAddressCaps->dwTotalSize);
				DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t=0x%lx\r\n"), lpAddressCaps->dwNeededSize);
				DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t=0x%lx\r\n"), lpAddressCaps->dwUsedSize);
				DTRACE(TRC_STRUCT, _T("  dwLineDeviceID\t\t=0x%lx\r\n"), lpAddressCaps->dwLineDeviceID);
				DTRACE(TRC_STRUCT, _T("  dwAddressSize\t\t=0x%lx\r\n"), lpAddressCaps->dwAddressSize);
				DTRACE(TRC_STRUCT, _T("  dwAddressOffset\t\t=0x%lx\r\n"), lpAddressCaps->dwAddressOffset);
				DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpAddressCaps->dwDevSpecificSize);
				DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpAddressCaps->dwDevSpecificOffset);
				DTRACE(TRC_STRUCT, _T("  dwAddressSharing\t\t=0x%lx\r\n"), lpAddressCaps->dwAddressSharing);
				DTRACE(TRC_STRUCT, _T("  dwAddressStates\t\t=0x%lx\r\n"), lpAddressCaps->dwAddressStates);
				DTRACE(TRC_STRUCT, _T("  dwCallInfoStates\t\t=0x%lx\r\n"), lpAddressCaps->dwCallInfoStates);
				DTRACE(TRC_STRUCT, _T("  dwCallerIDFlags\t\t=0x%lx\r\n"), lpAddressCaps->dwCallerIDFlags);
				DTRACE(TRC_STRUCT, _T("  dwCalledIDFlags\t\t=0x%lx\r\n"), lpAddressCaps->dwCalledIDFlags);
				DTRACE(TRC_STRUCT, _T("  dwConnectedIDFlags\t\t=0x%lx\r\n"), lpAddressCaps->dwConnectedIDFlags);
				DTRACE(TRC_STRUCT, _T("  dwRedirectionIDFlags\t\t=0x%lx\r\n"), lpAddressCaps->dwRedirectionIDFlags);
				DTRACE(TRC_STRUCT, _T("  dwRedirectingIDFlags\t\t=0x%lx\r\n"), lpAddressCaps->dwRedirectingIDFlags);
				DTRACE(TRC_STRUCT, _T("  dwCallStates\t\t=0x%lx\r\n"), lpAddressCaps->dwCallStates);
				DTRACE(TRC_STRUCT, _T("  dwDialToneModes\t\t=0x%lx\r\n"), lpAddressCaps->dwDialToneModes);
				DTRACE(TRC_STRUCT, _T("  dwBusyModes\t\t=0x%lx\r\n"), lpAddressCaps->dwBusyModes);
				DTRACE(TRC_STRUCT, _T("  dwSpecialInfo\t\t=0x%lx\r\n"), lpAddressCaps->dwSpecialInfo);
				DTRACE(TRC_STRUCT, _T("  dwDisconnectModes\t\t=0x%lx\r\n"), lpAddressCaps->dwDisconnectModes);
				DTRACE(TRC_STRUCT, _T("  dwMaxNumActiveCalls\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxNumActiveCalls);
				DTRACE(TRC_STRUCT, _T("  dwMaxNumOnHoldCalls\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxNumOnHoldCalls);
				DTRACE(TRC_STRUCT, _T("  dwMaxNumOnHoldPendingCalls\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxNumOnHoldPendingCalls);
				DTRACE(TRC_STRUCT, _T("  dwMaxNumConference\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxNumConference);
				DTRACE(TRC_STRUCT, _T("  dwMaxNumTransConf\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxNumTransConf);
				DTRACE(TRC_STRUCT, _T("  dwAddrCapFlags\t\t=0x%lx\r\n"), lpAddressCaps->dwAddrCapFlags);
				DTRACE(TRC_STRUCT, _T("  dwCallFeatures\t\t=0x%lx\r\n"), lpAddressCaps->dwCallFeatures);
				DTRACE(TRC_STRUCT, _T("  dwRemoveFromConfCaps\t\t=0x%lx\r\n"), lpAddressCaps->dwRemoveFromConfCaps);
				DTRACE(TRC_STRUCT, _T("  dwRemoveFromConfState\t\t=0x%lx\r\n"), lpAddressCaps->dwRemoveFromConfState);
				DTRACE(TRC_STRUCT, _T("  dwTransferModes\t\t=0x%lx\r\n"), lpAddressCaps->dwTransferModes);
				DTRACE(TRC_STRUCT, _T("  dwParkModes\t\t=0x%lx\r\n"), lpAddressCaps->dwParkModes);
				DTRACE(TRC_STRUCT, _T("  dwForwardModes\t\t=0x%lx\r\n"), lpAddressCaps->dwForwardModes);
				DTRACE(TRC_STRUCT, _T("  dwMaxForwardEntries\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxForwardEntries);
				DTRACE(TRC_STRUCT, _T("  dwMaxSpecificEntries\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxSpecificEntries);
				DTRACE(TRC_STRUCT, _T("  dwMinFwdNumRings\t\t=0x%lx\r\n"), lpAddressCaps->dwMinFwdNumRings);
				DTRACE(TRC_STRUCT, _T("  dwMaxFwdNumRings\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxFwdNumRings);
				DTRACE(TRC_STRUCT, _T("  dwMaxCallCompletions\t\t=0x%lx\r\n"), lpAddressCaps->dwMaxCallCompletions);
				DTRACE(TRC_STRUCT, _T("  dwCallCompletionConds\t\t=0x%lx\r\n"), lpAddressCaps->dwCallCompletionConds);
				DTRACE(TRC_STRUCT, _T("  dwCallCompletionModes\t\t=0x%lx\r\n"), lpAddressCaps->dwCallCompletionModes);
				DTRACE(TRC_STRUCT, _T("  dwNumCompletionMessages\t\t=0x%lx\r\n"), lpAddressCaps->dwNumCompletionMessages);
				DTRACE(TRC_STRUCT, _T("  dwCompletionMsgTextEntrySize\t\t=0x%lx\r\n"), lpAddressCaps->dwCompletionMsgTextEntrySize);
				DTRACE(TRC_STRUCT, _T("  dwCompletionMsgTextSize\t\t=0x%lx\r\n"), lpAddressCaps->dwCompletionMsgTextSize);
				DTRACE(TRC_STRUCT, _T("  dwCompletionMsgTextOffset\t\t=0x%lx\r\n"), lpAddressCaps->dwCompletionMsgTextOffset);
				DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			}
#endif
        }
    }

	DTRACE(TRC_API, _T("TSPI_lineGetAddressCaps rc=0x%lx\r\n"), lResult);
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
         DWORD dwAddressMode, LPCWSTR lpszAddress, DWORD dwSize)
{
#ifdef _UNICODE	
	CString strAddress = lpszAddress;
#else
	CString strAddress = ConvertWideToAnsi (lpszAddress);
#endif

	DTRACE(TRC_API, _T("TSPI_lineGetAddressID beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AddressId Buffer=%08lx\r\n"), (DWORD)hdLine, (DWORD)lpdwAddressID);
	DTRACE(TRC_API, _T("  AddressMode=0x%lx, Address Name=<%s>, Size=%ld\r\n"), dwAddressMode, (LPCTSTR)strAddress, dwSize);

	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
	ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pConn && !IsDeletedObject(pConn))
		lResult = GetSP()->lineGetAddressID(pConn, lpdwAddressID, dwAddressMode, 
											strAddress, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineGetAddressID Id=0x%lx, rc=0x%lx\r\n"), *lpdwAddressID, lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetAddressStatus beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AddressID=0x%lx, lpAddressStatus=%08lx\r\n"), (DWORD)hdLine, dwAddressID, lpAddressStatus);
	
	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
    LONG lResult = LINEERR_INVALLINEHANDLE;

    if (pConn)
    {   
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
        CTSPIAddressInfo* pAddr = pConn->GetAddress(dwAddressID);
        if (pAddr == NULL || IsDeletedObject(pAddr))
            lResult = LINEERR_INVALADDRESSID;
        else
        {
            lResult = GetSP()->lineGetAddressStatus(pAddr, lpAddressStatus);
#ifdef _DEBUG
			if (lResult == 0)
			{
				DTRACE(TRC_STRUCT, _T("Dumping LINEADDRESSSTATUS at %08lx\r\n"), (DWORD)lpAddressStatus);
				DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
				DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0%lx\r\n"), lpAddressStatus->dwTotalSize);
				DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t=0%lx\r\n"), lpAddressStatus->dwNeededSize);
				DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t=0%lx\r\n"), lpAddressStatus->dwUsedSize);
				DTRACE(TRC_STRUCT, _T("  dwNumInUse\t\t=0%lx\r\n"), lpAddressStatus->dwNumInUse);
				DTRACE(TRC_STRUCT, _T("  dwNumActiveCalls\t\t=0%lx\r\n"), lpAddressStatus->dwNumActiveCalls);
				DTRACE(TRC_STRUCT, _T("  dwNumOnHoldCalls\t\t=0%lx\r\n"), lpAddressStatus->dwNumOnHoldCalls);
				DTRACE(TRC_STRUCT, _T("  dwNumOnHoldPendCalls\t\t=0%lx\r\n"), lpAddressStatus->dwNumOnHoldPendCalls);
				DTRACE(TRC_STRUCT, _T("  dwAddressFeatures\t\t=0%lx\r\n"), lpAddressStatus->dwAddressFeatures);
				DTRACE(TRC_STRUCT, _T("  dwNumRingsNoAnswer\t\t=0%lx\r\n"), lpAddressStatus->dwNumRingsNoAnswer);
				DTRACE(TRC_STRUCT, _T("  dwForwardNumEntries\t\t=0%lx\r\n"), lpAddressStatus->dwForwardNumEntries);
				DTRACE(TRC_STRUCT, _T("  dwForwardSize\t\t=0%lx\r\n"), lpAddressStatus->dwForwardSize);
				DTRACE(TRC_STRUCT, _T("  dwForwardOffset\t\t=0%lx\r\n"), lpAddressStatus->dwForwardOffset);
				DTRACE(TRC_STRUCT, _T("  dwTerminalModesSize\t\t=0%lx\r\n"), lpAddressStatus->dwTerminalModesSize);
				DTRACE(TRC_STRUCT, _T("  dwTerminalModesOffset\t\t=0%lx\r\n"), lpAddressStatus->dwTerminalModesOffset);
				DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0%lx\r\n"), lpAddressStatus->dwDevSpecificSize);
				DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0%lx\r\n"), lpAddressStatus->dwDevSpecificOffset);
				DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			}
#endif
        }
    }
    
	DTRACE(TRC_API, _T("TSPI_lineGetAddressStatus rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetCallAddressID beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, AddressID Buff=%08lx\r\n"), (DWORD)hdCall, (DWORD)lpdwAddressID);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineGetCallAddressID(pCall, lpdwAddressID);

	DTRACE(TRC_API, _T("TSPI_lineGetCallAddressID AddressID=0x%lx, rc=0x%lx\r\n"), *lpdwAddressID, lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetCallInfo beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, lpCallInfo=%08lx\r\n"), (DWORD)hdCall, (DWORD)lpCallInfo);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))

	{
		lResult = GetSP()->lineGetCallInfo(pCall, lpCallInfo);
#ifdef _DEBUG
		if (lResult == 0)
		{
			DTRACE(TRC_STRUCT, _T("Dumping LINECALLINFO at %08lx\r\n"), (DWORD)lpCallInfo);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpCallInfo->dwTotalSize);
			DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t=0x%lx\r\n"), lpCallInfo->dwNeededSize);
			DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t=0x%lx\r\n"), lpCallInfo->dwUsedSize);
			DTRACE(TRC_STRUCT, _T("  hLine\t\t=0x%lx\r\n"), (DWORD)lpCallInfo->hLine);
			DTRACE(TRC_STRUCT, _T("  dwLineDeviceID\t\t=0x%lx\r\n"), lpCallInfo->dwLineDeviceID);
			DTRACE(TRC_STRUCT, _T("  dwAddressID\t\t=0x%lx\r\n"), lpCallInfo->dwAddressID);
			DTRACE(TRC_STRUCT, _T("  dwBearerMode\t\t=0x%lx\r\n"), lpCallInfo->dwBearerMode);
			DTRACE(TRC_STRUCT, _T("  dwRate\t\t=0x%lx\r\n"), lpCallInfo->dwRate);
			DTRACE(TRC_STRUCT, _T("  dwMediaMode\t\t=0x%lx\r\n"), lpCallInfo->dwMediaMode);
			DTRACE(TRC_STRUCT, _T("  dwAppSpecific\t\t=0x%lx\r\n"), lpCallInfo->dwAppSpecific);
			DTRACE(TRC_STRUCT, _T("  dwCallID\t\t=0x%lx\r\n"), lpCallInfo->dwCallID);
			DTRACE(TRC_STRUCT, _T("  dwRelatedCallID\t\t=0x%lx\r\n"), lpCallInfo->dwRelatedCallID);
			DTRACE(TRC_STRUCT, _T("  dwCallParamFlags\t\t=0x%lx\r\n"), lpCallInfo->dwCallParamFlags);
			DTRACE(TRC_STRUCT, _T("  dwCallStates\t\t=0x%lx\r\n"), lpCallInfo->dwCallStates);
			DTRACE(TRC_STRUCT, _T("  dwMonitorDigitModes\t\t=0x%lx\r\n"), lpCallInfo->dwMonitorDigitModes);
			DTRACE(TRC_STRUCT, _T("  dwMonitorMediaModes\t\t=0x%lx\r\n"), lpCallInfo->dwMonitorMediaModes);
			DTRACE(TRC_STRUCT, _T("  dwOrigin\t\t=0x%lx\r\n"), lpCallInfo->dwOrigin);
			DTRACE(TRC_STRUCT, _T("  dwReason\t\t=0x%lx\r\n"), lpCallInfo->dwReason);
			DTRACE(TRC_STRUCT, _T("  dwCompletionID\t\t=0x%lx\r\n"), lpCallInfo->dwCompletionID);
			DTRACE(TRC_STRUCT, _T("  dwNumOwners\t\t=0x%lx\r\n"), lpCallInfo->dwNumOwners);
			DTRACE(TRC_STRUCT, _T("  dwNumMonitors\t\t=0x%lx\r\n"), lpCallInfo->dwNumMonitors);
			DTRACE(TRC_STRUCT, _T("  dwCountryCode\t\t=0x%lx\r\n"), lpCallInfo->dwCountryCode);
			DTRACE(TRC_STRUCT, _T("  dwTrunk\t\t=0x%lx\r\n"), lpCallInfo->dwTrunk);
			DTRACE(TRC_STRUCT, _T("  dwCallerIDFlags\t\t=0x%lx\r\n"), lpCallInfo->dwCallerIDFlags);
			DTRACE(TRC_STRUCT, _T("  dwCallerIDSize\t\t=0x%lx\r\n"), lpCallInfo->dwCallerIDSize);
			DTRACE(TRC_STRUCT, _T("  dwCallerIDOffset\t\t=0x%lx\r\n"), lpCallInfo->dwCallerIDOffset);
			DTRACE(TRC_STRUCT, _T("  dwCallerIDNameSize\t\t=0x%lx\r\n"), lpCallInfo->dwCallerIDNameSize);
			DTRACE(TRC_STRUCT, _T("  dwCallerIDNameOffset\t\t=0x%lx\r\n"), lpCallInfo->dwCallerIDNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwCalledIDFlags\t\t=0x%lx\r\n"), lpCallInfo->dwCalledIDFlags);
			DTRACE(TRC_STRUCT, _T("  dwCalledIDSize\t\t=0x%lx\r\n"), lpCallInfo->dwCalledIDSize);
			DTRACE(TRC_STRUCT, _T("  dwCalledIDOffset\t\t=0x%lx\r\n"), lpCallInfo->dwCalledIDOffset);
			DTRACE(TRC_STRUCT, _T("  dwCalledIDNameSize\t\t=0x%lx\r\n"), lpCallInfo->dwCalledIDNameSize);
			DTRACE(TRC_STRUCT, _T("  dwCalledIDNameOffset\t\t=0x%lx\r\n"), lpCallInfo->dwCalledIDNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwConnectedIDFlags\t\t=0x%lx\r\n"), lpCallInfo->dwConnectedIDFlags);
			DTRACE(TRC_STRUCT, _T("  dwConnectedIDSize\t\t=0x%lx\r\n"), lpCallInfo->dwConnectedIDSize);
			DTRACE(TRC_STRUCT, _T("  dwConnectedIDOffset\t\t=0x%lx\r\n"), lpCallInfo->dwConnectedIDOffset);
			DTRACE(TRC_STRUCT, _T("  dwConnectedIDNameSize\t\t=0x%lx\r\n"), lpCallInfo->dwConnectedIDNameSize);
			DTRACE(TRC_STRUCT, _T("  dwConnectedIDNameOffset\t\t=0x%lx\r\n"), lpCallInfo->dwConnectedIDNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwRedirectionIDFlags\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectionIDFlags);
			DTRACE(TRC_STRUCT, _T("  dwRedirectionIDSize\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectionIDSize);
			DTRACE(TRC_STRUCT, _T("  dwRedirectionIDOffset\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectionIDOffset);
			DTRACE(TRC_STRUCT, _T("  dwRedirectionIDNameSize\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectionIDNameSize);
			DTRACE(TRC_STRUCT, _T("  dwRedirectionIDNameOffset\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectionIDNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwRedirectingIDFlags\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectingIDFlags);
			DTRACE(TRC_STRUCT, _T("  dwRedirectingIDSize\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectingIDSize);
			DTRACE(TRC_STRUCT, _T("  dwRedirectingIDOffset\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectingIDOffset);
			DTRACE(TRC_STRUCT, _T("  dwRedirectingIDNameSize\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectingIDNameSize);
			DTRACE(TRC_STRUCT, _T("  dwRedirectingIDNameOffset\t\t=0x%lx\r\n"), lpCallInfo->dwRedirectingIDNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwAppNameSize\t\t=0x%lx\r\n"), lpCallInfo->dwAppNameSize);
			DTRACE(TRC_STRUCT, _T("  dwAppNameOffset\t\t=0x%lx\r\n"), lpCallInfo->dwAppNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwDisplayableAddressSize\t\t=0x%lx\r\n"), lpCallInfo->dwDisplayableAddressSize);
			DTRACE(TRC_STRUCT, _T("  dwDisplayableAddressOffset\t\t=0x%lx\r\n"), lpCallInfo->dwDisplayableAddressOffset);
			DTRACE(TRC_STRUCT, _T("  dwCalledPartySize\t\t=0x%lx\r\n"), lpCallInfo->dwCalledPartySize);
			DTRACE(TRC_STRUCT, _T("  dwCalledPartyOffset\t\t=0x%lx\r\n"), lpCallInfo->dwCalledPartyOffset);
			DTRACE(TRC_STRUCT, _T("  dwCommentSize\t\t=0x%lx\r\n"), lpCallInfo->dwCommentSize);
			DTRACE(TRC_STRUCT, _T("  dwCommentOffset\t\t=0x%lx\r\n"), lpCallInfo->dwCommentOffset);
			DTRACE(TRC_STRUCT, _T("  dwDisplaySize\t\t=0x%lx\r\n"), lpCallInfo->dwDisplaySize);
			DTRACE(TRC_STRUCT, _T("  dwDisplayOffset\t\t=0x%lx\r\n"), lpCallInfo->dwDisplayOffset);
			DTRACE(TRC_STRUCT, _T("  dwUserUserInfoSize\t\t=0x%lx\r\n"), lpCallInfo->dwUserUserInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwUserUserInfoOffset\t\t=0x%lx\r\n"), lpCallInfo->dwUserUserInfoOffset);
			DTRACE(TRC_STRUCT, _T("  dwHighLevelCompSize\t\t=0x%lx\r\n"), lpCallInfo->dwHighLevelCompSize);
			DTRACE(TRC_STRUCT, _T("  dwHighLevelCompOffset\t\t=0x%lx\r\n"), lpCallInfo->dwHighLevelCompOffset);
			DTRACE(TRC_STRUCT, _T("  dwLowLevelCompSize\t\t=0x%lx\r\n"), lpCallInfo->dwLowLevelCompSize);
			DTRACE(TRC_STRUCT, _T("  dwLowLevelCompOffset\t\t=0x%lx\r\n"), lpCallInfo->dwLowLevelCompOffset);
			DTRACE(TRC_STRUCT, _T("  dwChargingInfoSize\t\t=0x%lx\r\n"), lpCallInfo->dwChargingInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwChargingInfoOffset\t\t=0x%lx\r\n"), lpCallInfo->dwChargingInfoOffset);
			DTRACE(TRC_STRUCT, _T("  dwTerminalModesSize\t\t=0x%lx\r\n"), lpCallInfo->dwTerminalModesSize);
			DTRACE(TRC_STRUCT, _T("  dwTerminalModesOffset\t\t=0x%lx\r\n"), lpCallInfo->dwTerminalModesOffset);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpCallInfo->dwDevSpecificSize);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpCallInfo->dwDevSpecificOffset);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		}
#endif
	}

	DTRACE(TRC_API, _T("TSPI_lineGetCallInfo rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetCallStatus beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, lpCallStatus=%08lx\r\n"), (DWORD) hdCall, (DWORD) lpCallStatus);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
	{
		lResult = GetSP()->lineGetCallStatus(pCall, lpCallStatus);
#ifdef _DEBUG
		if (lResult == 0)
		{
			DTRACE(TRC_STRUCT, _T("Dumping LINECALLSTATUS at %08lx\r\n"), (DWORD)lpCallStatus);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpCallStatus->dwTotalSize);
			DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t=0x%lx\r\n"), lpCallStatus->dwNeededSize);
			DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t=0x%lx\r\n"), lpCallStatus->dwUsedSize);
			DTRACE(TRC_STRUCT, _T("  dwCallState\t\t=0x%lx\r\n"), lpCallStatus->dwCallState);
			DTRACE(TRC_STRUCT, _T("  dwCallStateMode\t\t=0x%lx\r\n"), lpCallStatus->dwCallStateMode);
			DTRACE(TRC_STRUCT, _T("  dwCallPrivilege\t\t=0x%lx\r\n"), lpCallStatus->dwCallPrivilege);
			DTRACE(TRC_STRUCT, _T("  dwCallFeatures\t\t=0x%lx\r\n"), lpCallStatus->dwCallFeatures);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpCallStatus->dwDevSpecificSize);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpCallStatus->dwDevSpecificOffset);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		}
#endif
   }

	DTRACE(TRC_API, _T("TSPI_lineGetCallStatus rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetDevCaps beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, SP Version=0x%lx, Ext Version=0x%lx\r\n"), dwDeviceID, dwTSPIVersion, dwExtVersion);
	DTRACE(TRC_API, _T("  lpLineDevCaps=%08lx\r\n"), (DWORD) lpLineDevCaps);

	CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
	LONG lResult = LINEERR_BADDEVICEID;

	if (pConn)
	{
		lResult = GetSP()->lineGetDevCaps(pConn, dwTSPIVersion, dwExtVersion, lpLineDevCaps);
#ifdef _DEBUG
		if (lResult == 0)
		{
			DTRACE(TRC_STRUCT, _T("Dumping LINEDEVCAPS at %08lx\r\n"), (DWORD) lpLineDevCaps);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwTotalSize);
			DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwNeededSize);
			DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwUsedSize);
			DTRACE(TRC_STRUCT, _T("  dwProviderInfoSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwProviderInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwProviderInfoOffset\t\t=0x%lx\r\n"), lpLineDevCaps->dwProviderInfoOffset);
			DTRACE(TRC_STRUCT, _T("  dwSwitchInfoSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwSwitchInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwSwitchInfoOffset\t\t=0x%lx\r\n"), lpLineDevCaps->dwSwitchInfoOffset);
			DTRACE(TRC_STRUCT, _T("  dwPermanentLineID\t\t=0x%lx\r\n"), lpLineDevCaps->dwPermanentLineID);
			DTRACE(TRC_STRUCT, _T("  dwLineNameSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwLineNameSize);
			DTRACE(TRC_STRUCT, _T("  dwLineNameOffset\t\t=0x%lx\r\n"), lpLineDevCaps->dwLineNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwStringFormat\t\t=0x%lx\r\n"), lpLineDevCaps->dwStringFormat);
			DTRACE(TRC_STRUCT, _T("  dwAddressModes\t\t=0x%lx\r\n"), lpLineDevCaps->dwAddressModes);
			DTRACE(TRC_STRUCT, _T("  dwNumAddresses\t\t=0x%lx\r\n"), lpLineDevCaps->dwNumAddresses);
			DTRACE(TRC_STRUCT, _T("  dwBearerModes\t\t=0x%lx\r\n"), lpLineDevCaps->dwBearerModes);
			DTRACE(TRC_STRUCT, _T("  dwMaxRate\t\t=0x%lx\r\n"), lpLineDevCaps->dwMaxRate);
			DTRACE(TRC_STRUCT, _T("  dwMediaModes\t\t=0x%lx\r\n"), lpLineDevCaps->dwMediaModes);
			DTRACE(TRC_STRUCT, _T("  dwGenerateToneModes\t\t=0x%lx\r\n"), lpLineDevCaps->dwGenerateToneModes);
			DTRACE(TRC_STRUCT, _T("  dwGenerateToneMaxNumFreq\t\t=0x%lx\r\n"), lpLineDevCaps->dwGenerateToneMaxNumFreq);
			DTRACE(TRC_STRUCT, _T("  dwGenerateDigitModes\t\t=0x%lx\r\n"), lpLineDevCaps->dwGenerateDigitModes);
			DTRACE(TRC_STRUCT, _T("  dwMonitorToneMaxNumFreq\t\t=0x%lx\r\n"), lpLineDevCaps->dwMonitorToneMaxNumFreq);
			DTRACE(TRC_STRUCT, _T("  dwMonitorToneMaxNumEntries\t\t=0x%lx\r\n"), lpLineDevCaps->dwMonitorToneMaxNumEntries);
			DTRACE(TRC_STRUCT, _T("  dwMonitorDigitModes\t\t=0x%lx\r\n"), lpLineDevCaps->dwMonitorDigitModes);
			DTRACE(TRC_STRUCT, _T("  dwGatherDigitsMinTimeout\t\t=0x%lx\r\n"), lpLineDevCaps->dwGatherDigitsMinTimeout);
			DTRACE(TRC_STRUCT, _T("  dwGatherDigitsMaxTimeout\t\t=0x%lx\r\n"), lpLineDevCaps->dwGatherDigitsMaxTimeout);
			DTRACE(TRC_STRUCT, _T("  dwMedCtlDigitMaxListSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwMedCtlDigitMaxListSize);
			DTRACE(TRC_STRUCT, _T("  dwMedCtlMediaMaxListSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwMedCtlMediaMaxListSize);
			DTRACE(TRC_STRUCT, _T("  dwMedCtlToneMaxListSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwMedCtlToneMaxListSize);
			DTRACE(TRC_STRUCT, _T("  dwMedCtlCallStateMaxListSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwMedCtlCallStateMaxListSize);
			DTRACE(TRC_STRUCT, _T("  dwDevCapFlags\t\t=0x%lx\r\n"), lpLineDevCaps->dwDevCapFlags);
			DTRACE(TRC_STRUCT, _T("  dwMaxNumActiveCalls\t\t=0x%lx\r\n"), lpLineDevCaps->dwMaxNumActiveCalls);
			DTRACE(TRC_STRUCT, _T("  dwAnswerMode\t\t=0x%lx\r\n"), lpLineDevCaps->dwAnswerMode);
			DTRACE(TRC_STRUCT, _T("  dwRingModes\t\t=0x%lx\r\n"), lpLineDevCaps->dwRingModes);
			DTRACE(TRC_STRUCT, _T("  dwLineStates\t\t=0x%lx\r\n"), lpLineDevCaps->dwLineStates);
			DTRACE(TRC_STRUCT, _T("  dwUUIAcceptSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwUUIAcceptSize);
			DTRACE(TRC_STRUCT, _T("  dwUUIAnswerSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwUUIAnswerSize);
			DTRACE(TRC_STRUCT, _T("  dwUUIMakeCallSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwUUIMakeCallSize);
			DTRACE(TRC_STRUCT, _T("  dwUUIDropSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwUUIDropSize);
			DTRACE(TRC_STRUCT, _T("  dwUUISendUserUserInfoSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwUUISendUserUserInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwUUICallInfoSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwUUICallInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwNumTerminals\t\t=0x%lx\r\n"), lpLineDevCaps->dwNumTerminals);
			DTRACE(TRC_STRUCT, _T("  dwTerminalCapsSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwTerminalCapsSize);
			DTRACE(TRC_STRUCT, _T("  dwTerminalCapsOffset\t\t=0x%lx\r\n"), lpLineDevCaps->dwTerminalCapsOffset);
			DTRACE(TRC_STRUCT, _T("  dwTerminalTextEntrySize\t\t=0x%lx\r\n"), lpLineDevCaps->dwTerminalTextEntrySize);
			DTRACE(TRC_STRUCT, _T("  dwTerminalTextSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwTerminalTextSize);
			DTRACE(TRC_STRUCT, _T("  dwTerminalTextOffset\t\t=0x%lx\r\n"), lpLineDevCaps->dwTerminalTextOffset);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpLineDevCaps->dwDevSpecificSize);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpLineDevCaps->dwDevSpecificOffset);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		}
#endif
	}

	DTRACE(TRC_API, _T("TSPI_lineGetDevCaps rc=0x%lx\r\n"), lResult);
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
         LPCWSTR lpszDeviceClass)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();
	
	DTRACE(TRC_API, _T("TSPI_lineGetDevConfig beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, Class=<%s>\r\n"), dwDeviceID, (LPCTSTR)strDevClass);

	CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
	LONG lResult = LINEERR_BADDEVICEID;
	if (pConn)
	{
		lResult = GetSP()->lineGetDevConfig(pConn, strDevClass, lpDeviceConfig);
#ifdef _DEBUG
		if (lResult == 0)
			DumpVarString (lpDeviceConfig);
#endif		
	}
   
	DTRACE(TRC_API, _T("TSPI_lineGetDevConfig rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetExtensionID beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, TSPI Ver=0x%lx\r\n"), dwDeviceID, dwTSPIVersion);

	CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
	LONG lResult = LINEERR_BADDEVICEID;
	if (pConn)
	{
		lResult = GetSP()->lineGetExtensionID(pConn, dwTSPIVersion, lpExtensionID);
#ifdef _DEBUG
		if (lResult == 0)
		{
			DTRACE(TRC_STRUCT, _T("Dumping LINEEXTENSIONID at %08lx\r\n"), lpExtensionID);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			DTRACE(TRC_STRUCT, _T("  dwExtensionID0\t=0x%lx\r\n"), lpExtensionID->dwExtensionID0);
			DTRACE(TRC_STRUCT, _T("  dwExtensionID1\t=0x%lx\r\n"), lpExtensionID->dwExtensionID1);
			DTRACE(TRC_STRUCT, _T("  dwExtensionID2\t=0x%lx\r\n"), lpExtensionID->dwExtensionID2);
			DTRACE(TRC_STRUCT, _T("  dwExtensionID3\t=0x%lx\r\n"), lpExtensionID->dwExtensionID3);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		}
#endif
	}
   
	DTRACE(TRC_API, _T("TSPI_lineGetExtensionID rc=0x%lx\r\n"), lResult);
	return lResult;
   
}// TSPI_lineGetExtensionID

//////////////////////////////////////////////////////////////////////////
// TSPI_lineGetIcon
//
// This function retreives a service line device-specific icon for
// display to the user
//
extern "C"
LONG TSPIAPI TSPI_lineGetIcon (DWORD dwDeviceID, LPCWSTR lpszDeviceClass,
      LPHICON lphIcon)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();

	DTRACE(TRC_API, _T("TSPI_lineGetIcon beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, Class=<%s>, Buff=%08lx\r\n"), dwDeviceID, (LPCTSTR)strDevClass, (DWORD)lphIcon);

	LONG lResult = LINEERR_BADDEVICEID;
	CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
	if (pConn)
		lResult = GetSP()->lineGetIcon(pConn, strDevClass, lphIcon);
   
	DTRACE(TRC_API, _T("TSPI_lineGetIcon hIcon=%08lx, rc=0x%lx\r\n"), CASTHANDLE(*lphIcon), lResult);
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
         LPCWSTR lpszDeviceClass, HANDLE hTargetProcess)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();

	DTRACE(TRC_API, _T("TSPI_lineGetID beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AddressID=0x%lx\r\n"), (DWORD) hdLine, dwAddressID);
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, Select=%ld, Buffer=%08lx\r\n"), (DWORD)hdCall, dwSelect, (DWORD)lpVarString);
	DTRACE(TRC_API, _T("  DeviceClass=<%s>\r\n"), (LPCTSTR) strDevClass);
	DTRACE(TRC_API, _T("  Process=0x%lx\r\n"), (DWORD) hTargetProcess);

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
                pAddr = pCall->GetAddressOwner();
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
        lResult = GetSP()->lineGetID(pConn, pAddr, pCall, strDevClass, lpVarString, hTargetProcess);
#ifdef _DEBUG
		if (lResult == 0)
			DumpVarString (lpVarString);
#endif		
    }

	DTRACE(TRC_API, _T("TSPI_lineGetID rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetLineDevStatus beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, lpLineDevStatus=%08lx\r\n"), (DWORD)hdLine, (DWORD)lpLineDevStatus);

	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
	ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));

	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pConn && !IsDeletedObject(pConn))
	{
		lResult = GetSP()->lineGetLineDevStatus(pConn, lpLineDevStatus);
#ifdef _DEBUG
		if (lResult == 0)
		{
			DTRACE(TRC_STRUCT, _T("Dumping LINEDEVSTATUS at %08lx\r\n"), (DWORD) lpLineDevStatus);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpLineDevStatus->dwTotalSize);
			DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t=0x%lx\r\n"), lpLineDevStatus->dwNeededSize);
			DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t=0x%lx\r\n"), lpLineDevStatus->dwUsedSize);
			DTRACE(TRC_STRUCT, _T("  dwNumOpens\t\t=0x%lx\r\n"), lpLineDevStatus->dwNumOpens);
			DTRACE(TRC_STRUCT, _T("  dwOpenMediaModes\t\t=0x%lx\r\n"), lpLineDevStatus->dwOpenMediaModes);
			DTRACE(TRC_STRUCT, _T("  dwNumActiveCalls\t\t=0x%lx\r\n"), lpLineDevStatus->dwNumActiveCalls);
			DTRACE(TRC_STRUCT, _T("  dwNumOnHoldCalls\t\t=0x%lx\r\n"), lpLineDevStatus->dwNumOnHoldCalls);
			DTRACE(TRC_STRUCT, _T("  dwNumOnHoldPendCalls\t\t=0x%lx\r\n"), lpLineDevStatus->dwNumOnHoldPendCalls);
			DTRACE(TRC_STRUCT, _T("  dwLineFeatures\t\t=0x%lx\r\n"), lpLineDevStatus->dwLineFeatures);
			DTRACE(TRC_STRUCT, _T("  dwNumCallCompletions\t\t=0x%lx\r\n"), lpLineDevStatus->dwNumCallCompletions);
			DTRACE(TRC_STRUCT, _T("  dwRingMode\t\t=0x%lx\r\n"), lpLineDevStatus->dwRingMode);
			DTRACE(TRC_STRUCT, _T("  dwSignalLevel\t\t=0x%lx\r\n"), lpLineDevStatus->dwSignalLevel);
			DTRACE(TRC_STRUCT, _T("  dwBatteryLevel\t\t=0x%lx\r\n"), lpLineDevStatus->dwBatteryLevel);
			DTRACE(TRC_STRUCT, _T("  dwRoamMode\t\t=0x%lx\r\n"), lpLineDevStatus->dwRoamMode);
			DTRACE(TRC_STRUCT, _T("  dwDevStatusFlags\t\t=0x%lx\r\n"), lpLineDevStatus->dwDevStatusFlags);
			DTRACE(TRC_STRUCT, _T("  dwTerminalModesSize\t\t=0x%lx\r\n"), lpLineDevStatus->dwTerminalModesSize);
			DTRACE(TRC_STRUCT, _T("  dwTerminalModesOffset\t\t=0x%lx\r\n"), lpLineDevStatus->dwTerminalModesOffset);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpLineDevStatus->dwDevSpecificSize);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpLineDevStatus->dwDevSpecificOffset);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		}
#endif
	}

	DTRACE(TRC_API, _T("TSPI_lineGetLineDevStatus rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineGetNumAddressIDs beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, lpNumAddress=%08lx\r\n"), (DWORD)hdLine, (DWORD)lpNumAddressIDs);

	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
	ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pConn && !IsDeletedObject(pConn))
		lResult = GetSP()->lineGetNumAddressIDs(pConn, lpNumAddressIDs);

	DTRACE(TRC_API, _T("TSPI_lineGetNumAddressIDs NumAddresses=%ld, rc=0x%lx\r\n"), *lpNumAddressIDs, lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineHold beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdCall);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineHold(pCall, dwRequestID);

	DTRACE(TRC_API, _T("TSPI_lineHold rc=0x%lx\r\n"), lResult);
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
         HTAPICALL htCall, LPHDRVCALL lphdCall, LPCWSTR lpszDestAddress,
         DWORD dwCountryCode, LPLINECALLPARAMS const lpCallParams)
{
#ifdef _UNICODE
	CString strDestAddr = lpszDestAddress;
#else
	CString strDestAddr = ConvertWideToAnsi (lpszDestAddress);
#endif

	DTRACE(TRC_API, _T("TSPI_lineMakeCall beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Line Handle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdLine);
	DTRACE(TRC_API, _T("  TAPI Call handle=0x%lx, Buffer for SP Call Handle=%08lx\r\n"), (DWORD) htCall, (DWORD) lphdCall);
	DTRACE(TRC_API, _T("  Address=<%s>, Country=%ld\r\n"), (LPCTSTR)strDestAddr,  dwCountryCode);
#ifdef _DEBUG
	if (lpCallParams)
	{
		DTRACE(TRC_STRUCT, _T("Dumping LINECALLPARAMS at %08lx\r\n"), (DWORD)lpCallParams);
		DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpCallParams->dwTotalSize);
		DTRACE(TRC_STRUCT, _T("  dwBearerMode\t\t=0x%lx\r\n"), lpCallParams->dwBearerMode);
		DTRACE(TRC_STRUCT, _T("  dwMinRate\t\t=0x%lx\r\n"), lpCallParams->dwMinRate);
		DTRACE(TRC_STRUCT, _T("  dwMaxRate\t\t=0x%lx\r\n"), lpCallParams->dwMaxRate);
		DTRACE(TRC_STRUCT, _T("  dwMediaMode\t\t=0x%lx\r\n"), lpCallParams->dwMediaMode);
		DTRACE(TRC_STRUCT, _T("  dwCallParamFlags\t\t=0x%lx\r\n"), lpCallParams->dwCallParamFlags);
		DTRACE(TRC_STRUCT, _T("  dwAddressMode\t\t=0x%lx\r\n"), lpCallParams->dwAddressMode);
		DTRACE(TRC_STRUCT, _T("  dwAddressID\t\t=0x%lx\r\n"), lpCallParams->dwAddressID);
		DTRACE(TRC_STRUCT, _T("  dwOrigAddressSize\t\t=0x%lx\r\n"), lpCallParams->dwOrigAddressSize);
		DTRACE(TRC_STRUCT, _T("  dwOrigAddressOffset\t\t=0x%lx\r\n"), lpCallParams->dwOrigAddressOffset);
		DTRACE(TRC_STRUCT, _T("  dwDisplayableAddressSize\t\t=0x%lx\r\n"), lpCallParams->dwDisplayableAddressSize);
		DTRACE(TRC_STRUCT, _T("  dwDisplayableAddressOffset\t\t=0x%lx\r\n"), lpCallParams->dwDisplayableAddressOffset);
		DTRACE(TRC_STRUCT, _T("  dwCalledPartySize\t\t=0x%lx\r\n"), lpCallParams->dwCalledPartySize);
		DTRACE(TRC_STRUCT, _T("  dwCalledPartyOffset\t\t=0x%lx\r\n"), lpCallParams->dwCalledPartyOffset);
		DTRACE(TRC_STRUCT, _T("  dwCommentSize\t\t=0x%lx\r\n"), lpCallParams->dwCommentSize);
		DTRACE(TRC_STRUCT, _T("  dwCommentOffset\t\t=0x%lx\r\n"), lpCallParams->dwCommentOffset);
		DTRACE(TRC_STRUCT, _T("  dwUserUserInfoSize\t\t=0x%lx\r\n"), lpCallParams->dwUserUserInfoSize);
		DTRACE(TRC_STRUCT, _T("  dwUserUserInfoOffset\t\t=0x%lx\r\n"), lpCallParams->dwUserUserInfoOffset);
		DTRACE(TRC_STRUCT, _T("  dwHighLevelCompSize\t\t=0x%lx\r\n"), lpCallParams->dwHighLevelCompSize);
		DTRACE(TRC_STRUCT, _T("  dwHighLevelCompOffset\t\t=0x%lx\r\n"), lpCallParams->dwHighLevelCompOffset);
		DTRACE(TRC_STRUCT, _T("  dwLowLevelCompSize\t\t=0x%lx\r\n"), lpCallParams->dwLowLevelCompSize);
		DTRACE(TRC_STRUCT, _T("  dwLowLevelCompOffset\t\t=0x%lx\r\n"), lpCallParams->dwLowLevelCompOffset);
		DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpCallParams->dwDevSpecificSize);
		DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpCallParams->dwDevSpecificOffset);
		DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
	}
#endif

	LONG lResult = LINEERR_INVALLINEHANDLE;
	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
	ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	if (pConn && !IsDeletedObject(pConn))
		lResult = GetSP()->lineMakeCall(pConn, dwRequestID, htCall, lphdCall, 
                           strDestAddr, dwCountryCode, lpCallParams);

	DTRACE(TRC_API, _T("TSPI_lineMakeCall SP Call Handle=0x%lx, rc=0x%lx\r\n"), *lphdCall, lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineMonitorDigits beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, DigitMode=0x%lx\r\n"), (DWORD)hdCall, dwDigitModes);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

	LONG lResult = LINEERR_INVALCALLHANDLE;
	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineMonitorDigits(pCall, dwDigitModes);

	DTRACE(TRC_API, _T("TSPI_lineMonitorDigits rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineMonitorMedia beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, MediaMode=0x%lx\r\n"), (DWORD)hdCall, dwMediaModes);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineMonitorMedia(pCall, dwMediaModes);

	DTRACE(TRC_API, _T("TSPI_lineMonitorMedia rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineMonitorTones beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, Tone Id=0x%lx\r\n"), (DWORD)hdCall, dwToneListID);
	DTRACE(TRC_API, _T("  ToneList=%08lx, Count=%ld\r\n"), (DWORD)lpToneList, dwNumEntries);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineMonitorTones(pCall, dwToneListID, lpToneList, dwNumEntries);

	DTRACE(TRC_API, _T("TSPI_lineMonitorTones rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineNegotiateExtVersion beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, TSPI Ver=0x%lx, Hi=0x%lx, Lo=0x%lx\r\n"), dwDeviceID, dwTSPIVersion, dwHiVersion, dwLowVersion);

	LONG lResult = GetSP()->lineNegotiateExtVersion(dwDeviceID, dwTSPIVersion, 
                        dwLowVersion, dwHiVersion, lpdwExtVersion);

	DTRACE(TRC_API, _T("TSPI_lineNegotiateExtVersion Ver=0x%lx, rc=0x%lx\r\n"), *lpdwExtVersion, lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineNegotiateTSPIVersion beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceID=0x%lx, TAPI Version (0x%lx - 0x%lx)\r\n"), dwDeviceID, dwLowVersion, dwHighVersion);

	LONG lResult = GetSP()->lineNegotiateTSPIVersion(dwDeviceID, dwLowVersion,
                         dwHighVersion, lpdwTSPIVersion);

	DTRACE(TRC_API, _T("TSPI_lineNegotiateTSPIVersion Ver=0x%lx rc=0x%lx\r\n"), *lpdwTSPIVersion, lResult);
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
   DTRACE(TRC_API, _T("TSPI_lineOpen beginning\r\n"));
   DTRACE(TRC_API, _T("  DeviceId=0x%lx, TAPI line handle=0x%lx\r\n"), dwDeviceID, (DWORD) htLine);
   DTRACE(TRC_API, _T("  SPI Version=0x%lx, Event Callback=%08lx\r\n"), dwTSPIVersion, (LONG) lpfnEventProc);

	*lphdLine = 0;

	CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);

	LONG lResult = LINEERR_BADDEVICEID;
	if (pConn)
		lResult = GetSP()->lineOpen(pConn, htLine, lphdLine, dwTSPIVersion, 
						lpfnEventProc);

	DTRACE(TRC_API, _T("TSPI_lineOpen SP Line Handle=0x%lx, rc=0x%lx\r\n"), *lphdLine, lResult);

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
         DWORD dwParkMode, LPCWSTR lpszDirAddr, LPVARSTRING lpNonDirAddress)
{
#ifdef _UNICODE
	CString strDestAddr = lpszDirAddr;
#else
	CString strDestAddr = ConvertWideToAnsi (lpszDirAddr);
#endif

	DTRACE(TRC_API, _T("TSPI_linePark beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReq=0x%lx, SP Call Handle=0x%lx, Park Mode=0x%lx\r\n"), dwRequestID, (DWORD)hdCall, dwParkMode);
	DTRACE(TRC_API, _T("  Dest Addr=<%s>\r\n"), (LPCTSTR) strDestAddr);

    LONG lResult = 0L;
    if (dwParkMode != LINEPARKMODE_DIRECTED && dwParkMode != LINEPARKMODE_NONDIRECTED)
        lResult = LINEERR_INVALPARKMODE;
    else
    {
        CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
        if (pCall && !IsDeletedObject(pCall))
        {
            ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
            lResult = GetSP()->linePark(pCall, dwRequestID, dwParkMode, 
										strDestAddr, lpNonDirAddress);
        }
        else
            lResult = LINEERR_INVALCALLHANDLE;
    }
    
    DTRACE(TRC_API, _T("TSPI_linePark rc=0x%lx\r\n"), lResult);
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
         LPCWSTR lpszDestAddr, LPCWSTR lpszGroupID)
{
#ifdef _UNICODE
	CString strDestAddr = lpszDestAddr;
	CString strGroupID = lpszGroupID;
#else
	CString strDestAddr = ConvertWideToAnsi (lpszDestAddr);
	CString strGroupID = ConvertWideToAnsi (lpszGroupID);
#endif

	DTRACE(TRC_API, _T("TSPI_linePickup beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReq=0x%lx, SP Line Handle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD)htCall);
	DTRACE(TRC_API, _T("  TAPI Call Handle=0x%lx, AddrId=0x%lx\r\n"), (DWORD) htCall, dwAddressID);
	DTRACE(TRC_API, _T("  DestAddr=<%s>, Group=<%s>\r\n"), (LPCTSTR) strDestAddr, (LPCTSTR) strGroupID);

    LONG lResult = 0;
    CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
    if (pLine != NULL)
        ASSERT(pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
    else
        lResult = LINEERR_INVALLINEHANDLE;  

    if (pLine && !IsDeletedObject(pLine))
    {
        CTSPIAddressInfo* pAddr = pLine->GetAddress(dwAddressID);
        if (pAddr != NULL)
            lResult = GetSP()->linePickup(pAddr, dwRequestID, htCall,
                                        lphdCall, strDestAddr, strGroupID);
        else
            lResult = LINEERR_INVALADDRESSID;
    }

	DTRACE(TRC_API, _T("TSPI_linePickup SP Call=0x%lx, rc=0x%lx\r\n"), *lphdCall, lResult);
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
	DTRACE(TRC_API, _T("TSPI_linePrepareAddToConference beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReq=0x%lx, SP Call=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdConfCall);
	DTRACE(TRC_API, _T("  TAPI ConsultCall=0x%lx\r\n"), (DWORD) htConsultCall);

	CTSPIConferenceCall* pCall = (CTSPIConferenceCall*) hdConfCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPIConferenceCall)));

	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->linePrepareAddToConference(pCall, dwRequestID,
			  htConsultCall, lphdConsultCall, lpCallParams);

	DTRACE(TRC_API, _T("TSPI_linePrepareAddToConference New Call=0x%lx, rc=0x%lx\r\n"), *lphdConsultCall, lResult);
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
         LPCWSTR lpszDestAddr, DWORD dwCountryCode)
{
#ifdef _UNICODE
	CString strDestAddr = lpszDestAddr;
#else
	CString strDestAddr = ConvertWideToAnsi (lpszDestAddr);
#endif

	DTRACE(TRC_API, _T("TSPI_lineRedirect beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdCall);
	DTRACE(TRC_API, _T("  DestAddr=<%s>, Country=0x%lx\r\n"), (LPCTSTR) strDestAddr, dwCountryCode);

	LONG lResult = LINEERR_INVALCALLHANDLE;
	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineRedirect(pCall, dwRequestID, strDestAddr, dwCountryCode);

	DTRACE(TRC_API, _T("TSPI_lineRedirect rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineReleaseUserUserInfo beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdCall);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineReleaseUserUserInfo(pCall, dwRequestID);

	DTRACE(TRC_API, _T("TSPI_lineReleaseUserUserInfo rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineRemoveFromConference beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdCall);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineRemoveFromConference(pCall, dwRequestID);

	DTRACE(TRC_API, _T("TSPI_lineRemoveFromConference rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineSecureCall beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP CallHandle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdCall);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineSecureCall(pCall, dwRequestID);

	DTRACE(TRC_API, _T("TSPI_lineSecureCall rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineSelectExtVersion beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, Ext Ver=0x%lx\r\n"), (DWORD) hdLine, dwExtVersion);

	CTSPILineConnection* pLine = (CTSPILineConnection*) hdLine;
	ASSERT(pLine->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pLine && !IsDeletedObject(pLine))
		lResult = GetSP()->lineSelectExtVersion(pLine, dwExtVersion);

	DTRACE(TRC_API, _T("TSPI_lineSelectExtVersion rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineSendUserUserInfo beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), (DWORD)dwRequestID, (DWORD)hdCall);
	DTRACE(TRC_API, _T("  UserToUser <%s>, Size=%ld\r\n"), lpsUserUserInfo, dwSize);
	DUMPMEM (_T("UserInfo->\r\n"), (LPVOID)lpsUserUserInfo, dwSize);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineSendUserUserInfo(pCall, dwRequestID, lpsUserUserInfo, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineSendUserUserInfo rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineSetAppSpecific beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, dwAppSpecific=0x%lx\r\n"), (DWORD) hdCall, dwAppSpecific);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineSetAppSpecific(pCall, dwAppSpecific);

	DTRACE(TRC_API, _T("TSPI_lineSetAppSpecific rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_lineSetAppSpecific

/////////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetCallData
//
// This function sets CALLDATA into a calls CALLINFO record.
//
// Added for v2.0
//
extern "C"
LONG TSPIAPI TSPI_lineSetCallData (DRV_REQUESTID dwRequestID, HDRVCALL hdCall,
								   LPVOID lpCallData, DWORD dwSize)
{
	DTRACE(TRC_API, _T("TSPI_lineSetCallData beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD)hdCall);
	DTRACE(TRC_API, _T("  CallData=0x%lx, Size=%ld\r\n"), (DWORD)lpCallData, dwSize);
	DUMPMEM (_T("CallData->\r\n"), lpCallData, dwSize);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineSetCallData(pCall, dwRequestID, lpCallData, dwSize);

	DTRACE(TRC_API, _T("TSPI_lineSetCallData rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_lineSetCallData

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
	DTRACE(TRC_API, _T("TSPI_lineSetCallParams beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD)hdCall);
	DTRACE(TRC_API, _T("  BearerMode=0x%lx, Rate=(%ld, %ld)\r\n"), dwBearerMode, dwMinRate, dwMaxRate);
	DTRACE(TRC_STRUCT, _T("Dumping LINEDIALPARAMS at %08lx\r\n"), lpDialParams);
	if (lpDialParams != NULL)
	{
		DTRACE(TRC_STRUCT, _T("--------------------------------------------------------------\r\n"));
		DTRACE(TRC_STRUCT, _T("  dwDialPause\t=0x%lx\r\n"), lpDialParams->dwDialPause);
		DTRACE(TRC_STRUCT, _T("  dwDialSpeed\t=0x%lx\r\n"), lpDialParams->dwDialSpeed);
		DTRACE(TRC_STRUCT, _T("  dwDigitDuration\t=0x%lx\r\n"), lpDialParams->dwDigitDuration);
		DTRACE(TRC_STRUCT, _T("  dwWaitForDialtone\t=0x%lx\r\n"), lpDialParams->dwWaitForDialtone);
		DTRACE(TRC_STRUCT, _T("--------------------------------------------------------------\r\n"));
	}

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineSetCallParams(pCall, dwRequestID, dwBearerMode,
				dwMinRate, dwMaxRate, lpDialParams);

	DTRACE(TRC_API, _T("TSPI_lineSetCallParams rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_lineSetCallParams

///////////////////////////////////////////////////////////////////////////
// TSPI_lineSetCallTreatment
//
// Sets the call treatment for the specified call.  If the call
// treatment can go into effect then it happens immediately,
// otherwise it is set into place the next time the call enters
// a state where the call treatment is valid.
//
extern "C"
LONG TSPIAPI TSPI_lineSetCallTreatment (DRV_REQUESTID dwRequestID,
					HDRVCALL hdCall, DWORD dwCallTreatment)
{
	DTRACE(TRC_API, _T("TSPI_lineSetTreatment beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD)hdCall);
	DTRACE(TRC_API, _T("  Treatment=0x%lx\r\n"), dwCallTreatment);

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall && !IsDeletedObject(pCall))
      lResult = GetSP()->lineSetCallTreatment(pCall, dwRequestID, dwCallTreatment);

	DTRACE(TRC_API, _T("TSPI_lineSetCallTreatment rc=0x%lx\r\n"), lResult);

   return lResult;

}// TSPI_lineSetCallTreatment

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
	DTRACE(TRC_API, _T("TSPI_lineSetCurrentLocation beginning\r\n"));
	DTRACE(TRC_API, _T("  Location=0x%lx\r\n"), dwLocation);

	LONG lResult = GetSP()->lineSetCurrentLocation(dwLocation);

	DTRACE(TRC_API, _T("TSPI_lineSetCurrentLocation rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineSetDefaultMediaDetection beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, dwMediaModes=0x%lx\r\n"), (DWORD) hdLine, dwMediaModes);

	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
	ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pConn && !IsDeletedObject(pConn))
		lResult = GetSP()->lineSetDefaultMediaDetection(pConn, dwMediaModes);

	DTRACE(TRC_API, _T("TSPI_lineSetDefaultMediaDetection rc=0x%lx\r\n"), lResult);

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
         DWORD dwSize, LPCWSTR lpszDeviceClass)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();

	DTRACE(TRC_API, _T("TSPI_lineSetDevConfig beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceID=0x%lx, Buffer=%08lx, Size=%ld\r\n"), dwDeviceID, (DWORD)lpDevConfig, dwSize);
	DTRACE(TRC_API, _T("  DeviceClass <%s>\r\n"), (LPCTSTR) strDevClass);

	LONG lResult = LINEERR_BADDEVICEID;
	CTSPILineConnection* pConn = GetSP()->GetConnInfoFromLineDeviceID(dwDeviceID);
	if (pConn)
		lResult = GetSP()->lineSetDevConfig(pConn, lpDevConfig, dwSize, strDevClass);

	DTRACE(TRC_API, _T("TSPI_lineSetDevConfig rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineSetMediaControl beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AddressID=0x%lx\r\n"), (DWORD) hdLine, dwAddressID);
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, Select=%ld\r\n"), (DWORD)hdCall, dwSelect);
	DTRACE(TRC_API, _T("  Digits=%ld, Media=%ld, Tones=%ld, CallStates=%ld\r\n"), dwNumDigitEntries, dwNumMediaEntries, dwNumToneEntries, dwNumCallStateEntries);
    
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
                pAddr = pCall->GetAddressOwner();
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

	DTRACE(TRC_API, _T("TSPI_lineSetMediaControl rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineSetMediaMode beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, dwMediaModes=0x%lx\r\n"), (DWORD) hdCall, dwMediaMode);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineSetMediaMode(pCall, dwMediaMode);

	DTRACE(TRC_API, _T("TSPI_lineSetMediaMode rc=0x%lx\r\n"), lResult);

	return lResult;

}// TSPI_lineSetMediaMode

///////////////////////////////////////////////////////////////////////////
// TSPI_lineSetCallQualityOfService
//
// This function attempts to negotiate a level of QOS on the call with
// the switch.  If the desired QOS is not available, then it returns an
// error and remains at the current level of QOS.
//
extern "C"
LONG TSPIAPI TSPI_lineSetCallQualityOfService(DRV_REQUESTID dwRequestID,
					HDRVCALL hdCall, LPVOID lpSendingFlowSpec,
					DWORD dwSendingFlowSpecSize, LPVOID lpReceivingFlowSpec,
					DWORD dwReceivingFlowSpecSize)
{
	DTRACE(TRC_API, _T("TSPI_lineSetCallQualityOfService beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD)hdCall);
	DTRACE(TRC_API, _T("  SendingFlowSpec=0x%lx, Size=%ld\r\n"), (DWORD)lpSendingFlowSpec, dwSendingFlowSpecSize);
	DTRACE(TRC_API, _T("  ReceivingFlowSpec=0x%lx, Size=%ld\r\n"), (DWORD)lpReceivingFlowSpec, dwReceivingFlowSpecSize);
	DUMPMEM (_T("SendingFlowSpec->\r\n"), lpSendingFlowSpec, dwSendingFlowSpecSize);
	DUMPMEM (_T("ReceivingFlowSpec->\r\n"), lpReceivingFlowSpec, dwReceivingFlowSpecSize);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT (pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

	LONG lResult = LINEERR_INVALCALLHANDLE;
	if (pCall && !IsDeletedObject(pCall))
		lResult = GetSP()->lineSetQualityOfService(pCall, dwRequestID, lpSendingFlowSpec,
							dwSendingFlowSpecSize, lpReceivingFlowSpec, dwReceivingFlowSpecSize);

	DTRACE(TRC_API, _T("TSPI_lineSetCallQualityOfService rc=0x%lx\r\n"), lResult);

	return lResult;

}// TSPI_lineSetCallQualityOfService

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
	DTRACE(TRC_API, _T("TSPI_lineSetStatusMessages beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, LineStates=0x%lx, Address States=0x%lx\r\n"), (DWORD) hdLine, dwLineStates, dwAddressStates);

	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
	ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pConn && !IsDeletedObject(pConn))
		lResult = GetSP()->lineSetStatusMessages(pConn, dwLineStates, dwAddressStates);

	DTRACE(TRC_API, _T("TSPI_lineSetStatusMessages rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineSetTerminal beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx\r\n"), (DWORD) dwRequestID);
	DTRACE(TRC_API, _T("  SP Line Handle=0x%lx, AddressID=0x%lx\r\n"), (DWORD) hdLine, dwAddressID);
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, Select=%ld\r\n"), (DWORD)hdCall, dwSelect);
	DTRACE(TRC_API, _T("  Terminal Modes=0x%lx, Id=0x%lx, Enable=%ld\r\n"), dwTerminalModes, dwTerminalID, bEnable);

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

	DTRACE(TRC_API, _T("TSPI_lineSetTerminal rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineSetupConference beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx, SP LineHandle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdCall, (DWORD) hdLine);
	DTRACE(TRC_API, _T("  TAPI ConfCall=0x%lx, TAPI ConsultCall=0x%lx, NumParties=%ld\r\n"), htConfCall, htConsultCall, dwNumParties);

	CTSPILineConnection* pConn = NULL;
	CTSPICallAppearance* pCall = NULL;

	// If the call handle is non-NULL, look it up.
	if (hdCall)
	{
		pCall = (CTSPICallAppearance*) hdCall;
		ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
		pConn = pCall->GetLineOwner();
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
		DTRACE(TRC_API, _T("Returned SP ConfCall=0x%lx, SP ConsultCall=0x%lx\r\n"), *lphdConfCall, *lphdConsultCall);
	}

	DTRACE(TRC_API, _T("TSPI_lineSetupConference rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineSetupTransfer beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, Asynch ReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestID);
	DTRACE(TRC_API, _T("  New TAPI CH=0x%lx\r\n"), (DWORD) htConsultCall);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	ASSERT(lphdConsultCall != NULL);

	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && !IsDeletedObject(pCall))
	{
		lResult = GetSP()->lineSetupTransfer(pCall, dwRequestID, htConsultCall, 
                     lphdConsultCall, lpCallParams);

		DTRACE(TRC_API, _T("  Returned SP Call handle=0x%lx\r\n"), *lphdConsultCall);
	}

	DTRACE(TRC_API, _T("TSPI_lineSetupTransfer rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_lineSetupTransfer

//////////////////////////////////////////////////////////////////////////////
// TSPI_lineSetLineDevStatus
//
// The service provider sets the device status as indicated,
// sending the appropriate LINEDEVSTATE messages to indicate the
// new status.
//
extern "C"
LONG TSPIAPI TSPI_lineSetLineDevStatus (DRV_REQUESTID dwRequestID,
					HDRVLINE hdLine, DWORD dwStatusToChange,
					DWORD fStatus)
{
	DTRACE(TRC_API, _T("TSPI_lineSetLineDevStatus beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP LineHandle=0x%lx\r\n"), (DWORD) dwRequestID, (DWORD) hdLine);
	DTRACE(TRC_API, _T("  StatusToChange=0x%lx, State=0x%lx\r\n"), dwStatusToChange, fStatus);

	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
    ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pConn && !IsDeletedObject(pConn))
		lResult = GetSP()->lineSetLineDevStatus (pConn, dwRequestID,
									dwStatusToChange, fStatus);

	DTRACE(TRC_API, _T("TSPI_lineSetLineDevStatus rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_lineSetLineDevStatus

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
	DTRACE(TRC_API, _T("TSPI_lineSwapHold beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Call Handle=0x%lx, Asynch ReqId=0x%lx\r\n"), (DWORD) hdCall, (DWORD) dwRequestID);
	DTRACE(TRC_API, _T("  Held SP Call Handle=0x%lx\r\n"), (DWORD) hdHeldCall);

	CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
	ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));
	CTSPICallAppearance* pHeldCall = (CTSPICallAppearance*) hdHeldCall;

	LONG lResult = LINEERR_INVALCALLHANDLE;

	if (pCall && pHeldCall && !IsDeletedObject(pCall) && !IsDeletedObject(pHeldCall))
		lResult = GetSP()->lineSwapHold(pCall, dwRequestID, pHeldCall);

	DTRACE(TRC_API, _T("TSPI_lineSwapHold rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_lineUncompleteCall beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Line Handle=0x%lx, CompleteId=0x%lx\r\n"), (DWORD)dwRequestID, (DWORD)hdLine, dwCompletionID);

	CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
	ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
	LONG lResult = LINEERR_INVALLINEHANDLE;

	if (pConn && !IsDeletedObject(pConn))
		lResult = GetSP()->lineUncompleteCall(pConn, dwRequestID, dwCompletionID);

	DTRACE(TRC_API, _T("TSPI_lineUncompleteCall rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_lineUnhold beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Call Handle=0x%lx\r\n"), (DWORD) dwRequestId, (DWORD) hdCall);

   CTSPICallAppearance* pCall = (CTSPICallAppearance*) hdCall;
   ASSERT(pCall->IsKindOf(RUNTIME_CLASS(CTSPICallAppearance)));

   LONG lResult = LINEERR_INVALCALLHANDLE;

   if (pCall && !IsDeletedObject(pCall))
      lResult = GetSP()->lineUnhold(pCall, dwRequestId);

	DTRACE(TRC_API, _T("TSPI_lineUnhold rc=0x%lx\r\n"), lResult);

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
         LPCWSTR lpszDestAddr)
{
#ifdef _UNICODE
	CString strDestAddr = lpszDestAddr;
#else
	CString strDestAddr = ConvertWideToAnsi (lpszDestAddr);
#endif

	DTRACE(TRC_API, _T("TSPI_lineUnpark beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Line Handle=0x%lx, AddressId=0x%lx\r\n"), (DWORD)dwRequestID, (DWORD)hdLine, dwAddressID);
	DTRACE(TRC_API, _T("  TAPI Call Handle=0x%lx, Address=<%s>\r\n"), (DWORD)htCall, (LPCTSTR) strDestAddr);
    
    LONG lResult = LINEERR_INVALLINEHANDLE;
    CTSPILineConnection* pConn = (CTSPILineConnection*) hdLine;
    if (pConn != NULL)
    {
        ASSERT(pConn->IsKindOf(RUNTIME_CLASS(CTSPILineConnection)));
        CTSPIAddressInfo* pAddr = pConn->GetAddress(dwAddressID);
        if (pAddr && !IsDeletedObject(pAddr))
            lResult = GetSP()->lineUnpark(pAddr, dwRequestID, htCall, 
										lphdCall, strDestAddr);
		else
            lResult = LINEERR_INVALADDRESSID;

    }

	DTRACE(TRC_API, _T("TSPI_lineUnpark rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneClose beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD)hdPhone);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_INVALPHONEHANDLE;

	if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneClose(pPhone);

	DTRACE(TRC_API, _T("TSPI_phoneClose rc=0x%lx\r\n"), lResult);

	return lResult;

}// TSPI_phoneClose

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneConfigDialog
//
// This function invokes the parameter configuration dialog for the
// phone device.
//
extern "C"
LONG TSPIAPI TSPI_phoneConfigDialog (DWORD /*dwDeviceId*/, HWND /*hwndOwner*/, 
									 LPCSTR /*lpszDeviceClass*/)
{
	return PHONEERR_OPERATIONUNAVAIL;

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
	DTRACE(TRC_API, _T("TSPI_phoneDevSpecific beginning\r\n"));
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, SP Phone Handle=0x%lx\r\n"), (DWORD)dwRequestID, (DWORD)hdPhone);
	DTRACE(TRC_API, _T("  lpParams=%08lx, Size=%ld\r\n"), (DWORD)lpParams, dwSize);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

	LONG lResult = PHONEERR_INVALPHONEHANDLE;

	if (pPhone && !IsDeletedObject(pPhone))
		lResult = GetSP()->phoneDevSpecific(pPhone, dwRequestID, lpParams, dwSize);

	DTRACE(TRC_API, _T("TSPI_phoneDevSpecific rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneGetButtonInfo beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD) hdPhone);
	DTRACE(TRC_API, _T("  ButtonId=%ld, PhoneInfo=%08lx\r\n"), dwButtonId, (DWORD) lpPhoneInfo);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_INVALPHONEHANDLE;

	if (pPhone && !IsDeletedObject(pPhone))
	{
		lResult = GetSP()->phoneGetButtonInfo(pPhone, dwButtonId, lpPhoneInfo);
#ifdef _DEBUG
		if (lResult == 0)
		{
			DTRACE(TRC_STRUCT, _T("Dumping PHONEBUTTONINFO at %08lx\r\n"), (DWORD) lpPhoneInfo);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwTotalSize);
			DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwNeededSize);
			DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwUsedSize);
			DTRACE(TRC_STRUCT, _T("  dwButtonMode\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonMode);
			DTRACE(TRC_STRUCT, _T("  dwButtonFunction\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonFunction);
			DTRACE(TRC_STRUCT, _T("  dwButtonTextSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonTextSize);
			DTRACE(TRC_STRUCT, _T("  dwButtonTextOffset\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonTextOffset);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwDevSpecificSize);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwDevSpecificOffset);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		}
#endif
	}

	DTRACE(TRC_API, _T("TSPI_phoneGetButtonInfo rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneGetData beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD) hdPhone);
	DTRACE(TRC_API, _T("  DataId=%ld, Buffer=%08lx, Size=%ld\r\n"), dwDataId, (DWORD)lpData, dwSize);

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_INVALPHONEHANDLE;

   if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneGetData(pPhone, dwDataId, lpData, dwSize);

	DTRACE(TRC_API, _T("TSPI_phoneGetData rc=0x%lx\r\n"), lResult);

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
   DTRACE(TRC_API, _T("TSPI_phoneGetDevCaps beginning\r\n"));
   DTRACE(TRC_API, _T("  Phone Deviceid=0x%lx\r\n"), dwDeviceId);
   DTRACE(TRC_API, _T("  TSPIVersion=0x%lx, ExtVer=0x%lx, PhoneCaps=%08lx\r\n"), dwTSPIVersion, dwExtVersion, (DWORD)lpPhoneCaps);

   CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);
   LONG lResult = PHONEERR_BADDEVICEID;
   if (pPhone)
   {
		lResult = GetSP()->phoneGetDevCaps(pPhone, dwTSPIVersion, dwExtVersion, lpPhoneCaps);
#ifdef _DEBUG
		if (lResult == 0)
		{
			DTRACE(TRC_STRUCT, _T("Dumping PHONECAPS at %08lx\r\n"), (DWORD) lpPhoneCaps);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
			DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwTotalSize);
			DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwNeededSize);
			DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwUsedSize);
			DTRACE(TRC_STRUCT, _T("  dwProviderInfoSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwProviderInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwProviderInfoOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwProviderInfoOffset);
			DTRACE(TRC_STRUCT, _T("  dwPhoneInfoSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwPhoneInfoSize);
			DTRACE(TRC_STRUCT, _T("  dwPhoneInfoOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwPhoneInfoOffset);
			DTRACE(TRC_STRUCT, _T("  dwPermanentPhoneID\t\t=0x%lx\r\n"), lpPhoneCaps->dwPermanentPhoneID);
			DTRACE(TRC_STRUCT, _T("  dwPhoneNameSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwPhoneNameSize);
			DTRACE(TRC_STRUCT, _T("  dwPhoneNameOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwPhoneNameOffset);
			DTRACE(TRC_STRUCT, _T("  dwStringFormat\t\t=0x%lx\r\n"), lpPhoneCaps->dwStringFormat);
			DTRACE(TRC_STRUCT, _T("  dwPhoneStates\t\t=0x%lx\r\n"), lpPhoneCaps->dwPhoneStates);
			DTRACE(TRC_STRUCT, _T("  dwHookSwitchDevs\t\t=0x%lx\r\n"), lpPhoneCaps->dwHookSwitchDevs);
			DTRACE(TRC_STRUCT, _T("  dwHandsetHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwHandsetHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwSpeakerHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwSpeakerHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwHeadsetHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwHeadsetHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwVolumeFlags\t\t=0x%lx\r\n"), lpPhoneCaps->dwVolumeFlags);
			DTRACE(TRC_STRUCT, _T("  dwGainFlags\t\t=0x%lx\r\n"), lpPhoneCaps->dwGainFlags);
			DTRACE(TRC_STRUCT, _T("  dwDisplayNumRows\t\t=0x%lx\r\n"), lpPhoneCaps->dwDisplayNumRows);
			DTRACE(TRC_STRUCT, _T("  dwDisplayNumColumns\t\t=0x%lx\r\n"), lpPhoneCaps->dwDisplayNumColumns);
			DTRACE(TRC_STRUCT, _T("  dwNumRingModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwNumRingModes);
			DTRACE(TRC_STRUCT, _T("  dwNumButtonLamps\t\t=0x%lx\r\n"), lpPhoneCaps->dwNumButtonLamps);
			DTRACE(TRC_STRUCT, _T("  dwButtonModesSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwButtonModesSize);
			DTRACE(TRC_STRUCT, _T("  dwButtonModesOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwButtonModesOffset);
			DTRACE(TRC_STRUCT, _T("  dwButtonFunctionsSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwButtonFunctionsSize);
			DTRACE(TRC_STRUCT, _T("  dwButtonFunctionsOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwButtonFunctionsOffset);
			DTRACE(TRC_STRUCT, _T("  dwLampModesSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwLampModesSize);
			DTRACE(TRC_STRUCT, _T("  dwLampModesOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwLampModesOffset);
			DTRACE(TRC_STRUCT, _T("  dwNumSetData\t\t=0x%lx\r\n"), lpPhoneCaps->dwNumSetData);
			DTRACE(TRC_STRUCT, _T("  dwSetDataSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwSetDataSize);
			DTRACE(TRC_STRUCT, _T("  dwSetDataOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwSetDataOffset);
			DTRACE(TRC_STRUCT, _T("  dwNumGetData\t\t=0x%lx\r\n"), lpPhoneCaps->dwNumGetData);
			DTRACE(TRC_STRUCT, _T("  dwGetDataSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwGetDataSize);
			DTRACE(TRC_STRUCT, _T("  dwGetDataOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwGetDataOffset);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwDevSpecificSize);
			DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwDevSpecificOffset);
			DTRACE(TRC_STRUCT, _T("  dwDeviceClassesSize\t\t=0x%lx\r\n"), lpPhoneCaps->dwDeviceClassesSize);
			DTRACE(TRC_STRUCT, _T("  dwDeviceClassesOffset\t\t=0x%lx\r\n"), lpPhoneCaps->dwDeviceClassesOffset);
			DTRACE(TRC_STRUCT, _T("  dwPhoneFeatures\t\t=0x%lx\r\n"), lpPhoneCaps->dwPhoneFeatures);
			DTRACE(TRC_STRUCT, _T("  dwSettableHandsetHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwSettableHandsetHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwSettableSpeakerHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwSettableSpeakerHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwSettableHeadsetHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwSettableHeadsetHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwMonitoredHandsetHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwMonitoredHandsetHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwMonitoredSpeakerHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwMonitoredSpeakerHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("  dwMonitoredHeadsetHookSwitchModes\t\t=0x%lx\r\n"), lpPhoneCaps->dwMonitoredHeadsetHookSwitchModes);
			DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		}
#endif
   }


	DTRACE(TRC_API, _T("TSPI_phoneGetDevCaps rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneGetDisplay beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD) hdPhone);
	DTRACE(TRC_API, _T("  VarString=%08lx\r\n"), (DWORD)lpString);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_INVALPHONEHANDLE;

	if (pPhone && !IsDeletedObject(pPhone))
	{
	    lResult = GetSP()->phoneGetDisplay(pPhone, lpString);
#ifdef _DEBUG
		DumpVarString (lpString);
#endif		
	}

	DTRACE(TRC_API, _T("TSPI_phoneGetDisplay rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneGetExtensionID beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, TSPIVersion=0x%lx, lpPhoneExtId=%08lx\r\n"), dwDeviceId, dwTSPIVersion, (DWORD)lpExtensionId);

	CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);
	LONG lResult = PHONEERR_BADDEVICEID;
	if (pPhone)
		lResult = GetSP()->phoneGetExtensionID(pPhone, dwTSPIVersion, lpExtensionId);

	DTRACE(TRC_API, _T("TSPI_phoneGetExtensionID rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneGetGain beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, HookSwitchDev=%x, GainBuffer=%08lx\r\n"), (DWORD)hdPhone, dwHookSwitchDev, (DWORD)lpdwGain);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_BADDEVICEID;

	if (pPhone && !IsDeletedObject(pPhone))
		lResult = GetSP()->phoneGetGain(pPhone, dwHookSwitchDev, lpdwGain);

	DTRACE(TRC_API, _T("TSPI_phoneGetGain rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneGetHookSwitch beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, Buffer=%08lx\r\n"), (DWORD)hdPhone, (DWORD)lpdwHookSwitchDevs);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

	LONG lResult = PHONEERR_BADDEVICEID;

	if (pPhone && !IsDeletedObject(pPhone))
	{
		lResult = GetSP()->phoneGetHookSwitch(pPhone, lpdwHookSwitchDevs);
		DTRACE(TRC_API, _T("  Hookswitch settings are 0x%lx\r\n"), *lpdwHookSwitchDevs);
	}

	DTRACE(TRC_API, _T("TSPI_phoneGetHookSwitch rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_phoneGetHookSwitch

///////////////////////////////////////////////////////////////////////////
// TSPI_phoneGetIcon
//
// This function retrieves a specific icon for display from an
// application.  This icon will represent the phone device.
//
extern "C"
LONG TSPIAPI TSPI_phoneGetIcon (DWORD dwDeviceId, LPCWSTR lpszDeviceClass, 
               LPHICON lphIcon)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();
	DTRACE(TRC_API, _T("TSPI_phoneGetIcon beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, lpszDevClass=<%s>\r\n"), dwDeviceId, (LPCTSTR)strDevClass);

	LONG lResult = PHONEERR_BADDEVICEID;
	CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);
	if (pPhone)
		lResult = GetSP()->phoneGetIcon(pPhone, strDevClass, lphIcon);

	DTRACE(TRC_API, _T("TSPI_phoneGetIcon rc=0x%lx\r\n"), lResult);

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
               LPCWSTR lpszDeviceClass, HANDLE hTargetProcess)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();
	DTRACE(TRC_API, _T("TSPI_phoneGetID beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, DevClass=<%s>\r\n"), (DWORD)hdPhone, (LPCTSTR)strDevClass);
	DTRACE(TRC_API, _T("  Process=0x%lx\r\n"), (DWORD) hTargetProcess);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

	LONG lResult = PHONEERR_BADDEVICEID;
	if (pPhone && !IsDeletedObject(pPhone))
	{
		lResult = GetSP()->phoneGetID(pPhone, strDevClass, lpDeviceId, hTargetProcess);
#ifdef _DEBUG
		DumpVarString (lpDeviceId);
#endif
	}

	DTRACE(TRC_API, _T("TSPI_phoneGetID rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneGetLamp beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, LampId=0x%lx\r\n"), (DWORD)hdPhone, dwButtonLampId);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_BADDEVICEID;

	if (pPhone && !IsDeletedObject(pPhone))
	{
		lResult = GetSP()->phoneGetLamp(pPhone, dwButtonLampId, lpdwLampMode);
		DTRACE(TRC_API, _T("  Returned lamp mode = 0x%lx\r\n"), *lpdwLampMode);
	}

	DTRACE(TRC_API, _T("TSPI_phoneGetLamp rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneGetRing beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD)hdPhone);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_BADDEVICEID;

	if (pPhone && !IsDeletedObject(pPhone))
	{
		lResult = GetSP()->phoneGetRing(pPhone, lpdwRingMode, lpdwVolume);
		DTRACE(TRC_API, _T("  Returned Ringmode=0x%lx, RingVolume=0x%lx\r\n"), *lpdwRingMode, *lpdwVolume);
	}

	DTRACE(TRC_API, _T("TSPI_phoneGetRing rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneGetStatus beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD)hdPhone);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone && !IsDeletedObject(pPhone))
   {
      lResult = GetSP()->phoneGetStatus(pPhone, lpPhoneStatus);
#ifdef _DEBUG
	  if (lResult == 0)
	  {
		  DTRACE(TRC_STRUCT, _T("Dumping PHONESTATUS at %08lx\r\n"), (DWORD) lpPhoneStatus);
		  DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
		  DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwTotalSize);
		  DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwNeededSize);
		  DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwUsedSize);
		  DTRACE(TRC_STRUCT, _T("  dwStatusFlags;\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwStatusFlags);
		  DTRACE(TRC_STRUCT, _T("  dwNumOwners\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwNumOwners);
		  DTRACE(TRC_STRUCT, _T("  dwNumMonitors\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwNumMonitors);
		  DTRACE(TRC_STRUCT, _T("  dwRingMode\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwRingMode);
		  DTRACE(TRC_STRUCT, _T("  dwRingVolume\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwRingVolume);
		  DTRACE(TRC_STRUCT, _T("  dwHandsetHookSwitchMode\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwHandsetHookSwitchMode);
		  DTRACE(TRC_STRUCT, _T("  dwHandsetVolume\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwHandsetVolume);
		  DTRACE(TRC_STRUCT, _T("  dwHandsetGain\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwHandsetGain);
		  DTRACE(TRC_STRUCT, _T("  dwSpeakerHookSwitchMode\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwSpeakerHookSwitchMode);
		  DTRACE(TRC_STRUCT, _T("  dwSpeakerVolume\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwSpeakerVolume);
		  DTRACE(TRC_STRUCT, _T("  dwSpeakerGain\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwSpeakerGain);
		  DTRACE(TRC_STRUCT, _T("  dwHeadsetHookSwitchMode\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwHeadsetHookSwitchMode);
		  DTRACE(TRC_STRUCT, _T("  dwHeadsetVolume\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwHeadsetVolume);
		  DTRACE(TRC_STRUCT, _T("  dwHeadsetGain\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwHeadsetGain);
		  DTRACE(TRC_STRUCT, _T("  dwDisplaySize\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwDisplaySize);
		  DTRACE(TRC_STRUCT, _T("  dwDisplayOffset\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwDisplayOffset);
		  DTRACE(TRC_STRUCT, _T("  dwLampModesSize\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwLampModesSize);
		  DTRACE(TRC_STRUCT, _T("  dwLampModesOffset\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwLampModesOffset);
		  DTRACE(TRC_STRUCT, _T("  dwOwnerNameSize\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwOwnerNameSize);
		  DTRACE(TRC_STRUCT, _T("  dwOwnerNameOffset\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwOwnerNameOffset);
		  DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwDevSpecificSize);
		  DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t\t= 0x%lx\r\n"), lpPhoneStatus->dwDevSpecificOffset);
		  DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
	  }
#endif
   }

	DTRACE(TRC_API, _T("TSPI_phoneGetStatus rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneGetVolume beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD)hdPhone);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_BADDEVICEID;

	if (pPhone && !IsDeletedObject(pPhone))
	{
		lResult = GetSP()->phoneGetVolume(pPhone, dwHookSwitchDev, lpdwVolume);
		DTRACE(TRC_API, _T("  Return hookswitch volume is 0x%lx\r\n"), *lpdwVolume);
	}

	DTRACE(TRC_API, _T("TSPI_phoneGetVolume rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneNegotiateTSPIVersion beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceID=0x%lx, TAPI Version (0x%lx - 0x%lx)\r\n"), dwDeviceID, dwLowVersion, dwHighVersion);

	LONG lResult = GetSP()->phoneNegotiateTSPIVersion(dwDeviceID, dwLowVersion,
                         dwHighVersion, lpdwVersion);

	DTRACE(TRC_API, _T("TSPI_phoneNegotiateTSPIVersion Ver=0x%lx, rc=0x%lx\r\n"), *lpdwVersion, lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneNegotiateExtVersion beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceID=0x%lx, TAPI Version (0x%lx - 0x%lx)\r\n"), dwDeviceID, dwLowVersion, dwHighVersion);

	LONG lResult = GetSP()->phoneNegotiateExtVersion(dwDeviceID, 
                         dwTSPIVersion, dwLowVersion,
                         dwHighVersion, lpdwExtVersion);

	DTRACE(TRC_API, _T("  Return version=0x%lx\r\n"), *lpdwExtVersion);
	DTRACE(TRC_API, _T("TSPI_phoneNegotiateExtVersion rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneOpen beginning\r\n"));
	DTRACE(TRC_API, _T("  DeviceID=0x%lx, TAPI Handle=0x%lx, TSPIVersion=0x%lx, Event=%08lx\r\n"), dwDeviceId, (DWORD)htPhone, (DWORD)lphdPhone, (DWORD)lpfnEventProc);

	CTSPIPhoneConnection* pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwDeviceId);
	LONG lResult = PHONEERR_BADDEVICEID;
	if (pPhone)
		lResult = GetSP()->phoneOpen(pPhone, htPhone, lphdPhone, dwTSPIVersion, lpfnEventProc);

	DTRACE(TRC_API, _T("  SP handle for phone is 0x%lx\r\n"), *lphdPhone);
	DTRACE(TRC_API, _T("TSPI_phoneOpen rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneSelectExtVersion beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, dwExtVersion=0x%lx\r\n"), (DWORD)hdPhone, dwExtVersion);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_INVALPHONEHANDLE;

	if (pPhone && !IsDeletedObject(pPhone))
		lResult = GetSP()->phoneSelectExtVersion(pPhone, dwExtVersion);

	DTRACE(TRC_API, _T("TSPI_phoneSelectExtVersion rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneSetButtonInfo beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD) hdPhone, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  ButtonId=%ld\r\n"), dwButtonId);
	DTRACE(TRC_STRUCT, _T("Dumping PHONEBUTTONINFO at %08lx\r\n"), (DWORD) lpPhoneInfo);
	DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));
	DTRACE(TRC_STRUCT, _T("  dwTotalSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwTotalSize);
	DTRACE(TRC_STRUCT, _T("  dwNeededSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwNeededSize);
	DTRACE(TRC_STRUCT, _T("  dwUsedSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwUsedSize);
	DTRACE(TRC_STRUCT, _T("  dwButtonMode\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonMode);
	DTRACE(TRC_STRUCT, _T("  dwButtonFunction\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonFunction);
	DTRACE(TRC_STRUCT, _T("  dwButtonTextSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonTextSize);
	DTRACE(TRC_STRUCT, _T("  dwButtonTextOffset\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwButtonTextOffset);
	DTRACE(TRC_STRUCT, _T("  dwDevSpecificSize\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwDevSpecificSize);
	DTRACE(TRC_STRUCT, _T("  dwDevSpecificOffset\t\t\t= 0x%lx\r\n"), lpPhoneInfo->dwDevSpecificOffset);
	DTRACE(TRC_STRUCT, _T("----------------------------------------------------------------\r\n"));

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_INVALPHONEHANDLE;

	if (pPhone && !IsDeletedObject(pPhone))
		lResult = GetSP()->phoneSetButtonInfo(pPhone, dwRequestId, dwButtonId, lpPhoneInfo);

	DTRACE(TRC_API, _T("TSPI_phoneSetButtonInfo rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_phoneSetData beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx RequestId=0x%lx\r\n"), (DWORD) hdPhone, (DWORD) dwRequestId);
	DTRACE(TRC_API, _T("  DataId=%ld, Buffer=%08lx, Size=%ld\r\n"), dwDataId, (DWORD)lpData, dwSize);

	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	LONG lResult = PHONEERR_INVALPHONEHANDLE;

	if (pPhone && !IsDeletedObject(pPhone))
		lResult = GetSP()->phoneSetData(pPhone, dwRequestId, dwDataId, lpData, dwSize);

	DTRACE(TRC_API, _T("TSPI_phoneSetData rc=0x%lx\r\n"), lResult);
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
         HDRVPHONE hdPhone, DWORD dwRow, DWORD dwCol, LPCWSTR lpszDisplay,
         DWORD dwSize)   
{
#ifdef _UNICODE
	CString strDisplay = lpszDisplay;
#else
	CString strDisplay = ConvertWideToAnsi (lpszDisplay);
#endif

	DTRACE(TRC_API, _T("TSPI_phoneSetDisplay beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx\r\n"), (DWORD) hdPhone);
	DTRACE(TRC_API, _T("  AsynchReqId=0x%lx, Row=%ld, Col=%ld\r\n"), (DWORD)dwRequestID, dwRow, dwCol);
	DTRACE(TRC_API, _T("  Display=<%s>, size=%ld\r\n"), (LPCTSTR) strDisplay, dwSize);

	LONG lResult = PHONEERR_INVALPHONEHANDLE;
	CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
	ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
	if (pPhone && !IsDeletedObject(pPhone))
		lResult = GetSP()->phoneSetDisplay(pPhone, dwRequestID, dwRow, dwCol, strDisplay, dwSize);

	DTRACE(TRC_API, _T("TSPI_phoneSetDisplay rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneSetGain beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, RequestId=0x%lx, HookSwitchDev=%x, Gain=%lx\r\n"), (DWORD)hdPhone, (DWORD)dwRequestId, dwHookSwitchDev, dwGain);

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneSetGain(pPhone, dwRequestId, dwHookSwitchDev, dwGain);

	DTRACE(TRC_API, _T("TSPI_phoneSetGain rc=0x%lx\r\n"), lResult);

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
   DTRACE(TRC_API, _T("TSPI_phoneSetHookSwitch beginning\r\n"));
   DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD)dwRequestId);
   DTRACE(TRC_API, _T("  HookswitchDevs=0x%lx, Mode=0x%lx\r\n"), dwHookSwitchDevs, dwHookSwitchMode);

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));
   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneSetHookSwitch(pPhone, dwRequestId, dwHookSwitchDevs, dwHookSwitchMode);

	DTRACE(TRC_API, _T("TSPI_phoneSetHookSwitch rc=0x%lx\r\n"), lResult);

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
   DTRACE(TRC_API, _T("TSPI_phoneSetLamp beginning\r\n"));
   DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD)hdPhone, (DWORD)dwRequestId);
   DTRACE(TRC_API, _T("  LampId=0x%lx, Mode=0x%lx\r\n"), dwButtonLampId, dwLampMode);

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneSetLamp(pPhone, dwRequestId, dwButtonLampId, dwLampMode);

	DTRACE(TRC_API, _T("TSPI_phoneSetLamp rc=0x%lx\r\n"), lResult);

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
	DTRACE(TRC_API, _T("TSPI_phoneSetRing beginning\r\n"));
	DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD)hdPhone, (DWORD)dwRequestId);
	DTRACE(TRC_API, _T("  Ring mode=%ld, Volume=%ld\r\n"), dwRingMode, dwVolume);

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneSetRing(pPhone, dwRequestId, dwRingMode, dwVolume);

	DTRACE(TRC_API, _T("TSPI_phoneSetRing rc=0x%lx\r\n"), lResult);
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
   DTRACE(TRC_API, _T("TSPI_phoneSetStatusMessages beginning\r\n"));
   DTRACE(TRC_API, _T("  SP Phone handle=0x%lx, Modes=0x%lx, States=0x%lx\r\n"), (DWORD)hdPhone, dwButtonModes, dwButtonStates);

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneSetStatusMessages(pPhone, dwPhoneStates, dwButtonModes, dwButtonStates);

	DTRACE(TRC_API, _T("TSPI_phoneSetStatusMessages rc=0x%lx\r\n"), lResult);
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
   DTRACE(TRC_API, _T("TSPI_phoneSetVolume beginning\r\n"));
   DTRACE(TRC_API, _T("  SP Phone Handle=0x%lx, AsynchReqId=0x%lx\r\n"), (DWORD)hdPhone, (DWORD)dwRequestId);
   DTRACE(TRC_API, _T("  HookSwitchDev=0x%lx, Volume=0x%lx\r\n"), dwHookSwitchDev, dwVolume);

   CTSPIPhoneConnection* pPhone = (CTSPIPhoneConnection*) hdPhone;
   ASSERT(pPhone->IsKindOf(RUNTIME_CLASS(CTSPIPhoneConnection)));

   LONG lResult = PHONEERR_BADDEVICEID;

   if (pPhone && !IsDeletedObject(pPhone))
      lResult = GetSP()->phoneSetVolume(pPhone, dwRequestId, dwHookSwitchDev, dwVolume);

	DTRACE(TRC_API, _T("TSPI_phoneSetVolume rc=0x%lx\r\n"), lResult);

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
LONG TSPIAPI TSPI_providerConfig (HWND, DWORD)
{
	return LINEERR_OPERATIONUNAVAIL;
   
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
         ASYNC_COMPLETION lpfnCompletionProc, LPDWORD lpdwTSPIOptions)
{
	DTRACE(TRC_API, _T("TSPI_providerInit beginning\r\n"));
	DTRACE(TRC_API, _T("  SPI Version=0x%lx, ProviderID=0x%lx\r\n"), dwTSPIVersion, dwPermanentProviderID);
	DTRACE(TRC_API, _T("  LineBase=%ld, Count=%ld, PhoneBase=%ld, Count=%ld\r\n"), dwLineDeviceIDBase, dwNumLines, dwPhoneDeviceIDBase, dwNumPhones);
	DTRACE(TRC_API, _T("  AsynchCompletionProc=%08lx\r\n"), lpfnCompletionProc);

	LONG lResult = GetSP()->providerInit(dwTSPIVersion, dwPermanentProviderID,
                        dwLineDeviceIDBase, dwPhoneDeviceIDBase, dwNumLines,
                        dwNumPhones, lpfnCompletionProc, lpdwTSPIOptions);

	DTRACE(TRC_API, _T("TSPI_providerInit Options=0x%lx, rc=0x%lx\r\n"), *lpdwTSPIOptions, lResult);
	return lResult;

}// TSPI_providerInit

////////////////////////////////////////////////////////////////////////////
// TSPI_providerInstall
//
// This function is invoked to install the service provider onto
// the system.
//        
extern "C"
LONG TSPIAPI TSPI_providerInstall(HWND, DWORD)
{
	return LINEERR_OPERATIONUNAVAIL;

}// TSPI_providerInstall

////////////////////////////////////////////////////////////////////////////
// TSPI_providerRemove
//
// This function removes the service provider
//
extern "C"
LONG TSPIAPI TSPI_providerRemove(HWND, DWORD)
{
	return LINEERR_OPERATIONUNAVAIL;

}// TSPI_providerRemove

///////////////////////////////////////////////////////////////////////////
// TSPI_providerShutdown
//
// This function is called when the TAPI.DLL is shutting down our
// service provider.
//
extern "C"
LONG TSPIAPI TSPI_providerShutdown (DWORD dwTSPIVersion, DWORD dwProviderID)
{
	DTRACE(TRC_API, _T("TSPI_providerShutdown beginning\r\n"));
	DTRACE(TRC_API, _T("  SPI Version=0x%lx\r\n"), dwTSPIVersion);
	DTRACE(TRC_API, _T("  Provider ID=0x%lx\r\n"), dwProviderID);

	// Locate the proper device.
	CTSPIDevice* pDevice = GetSP()->GetDevice(dwProviderID);
	ASSERT (pDevice && pDevice->IsKindOf(RUNTIME_CLASS(CTSPIDevice)));
	LONG lResult = GetSP()->providerShutdown(dwTSPIVersion, pDevice);

	DTRACE(TRC_API, _T("TSPI_providerShutdown rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_providerEnumDevices beginning\r\n"));
	DTRACE(TRC_API, _T("  ProviderId=0x%lx, hProvider=0x%lx\r\n"), dwPermanentProviderID, (DWORD)hProvider);

	LONG lResult = GetSP()->providerEnumDevices(dwPermanentProviderID, lpdwNumLines,
			lpdwNumPhones, hProvider, lpfnLineCreateProc, lpfnPhoneCreateProc);

	DTRACE(TRC_API, _T("TSPI_providerEnumDevices NumLines=%ld, NumPhones=%ld, rc=0x%lx\r\n"), *lpdwNumLines, *lpdwNumPhones, lResult);
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
	DTRACE(TRC_API, _T("TSPI_providerCreateLineDevice beginning\r\n"));
	DTRACE(TRC_API, _T("  TempId=0x%lx, DeviceId=0x%lx\r\n"), dwTempID, dwDeviceID);
	LONG lResult = GetSP()->providerCreateLineDevice(dwTempID, dwDeviceID);

	DTRACE(TRC_API, _T("TSPI_providerCreateLineDevice rc=0x%lx\r\n"), lResult);
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
	DTRACE(TRC_API, _T("TSPI_providerCreatePhoneDevice beginning\r\n"));
	DTRACE(TRC_API, _T("  TempId=0x%lx, DeviceId=0x%lx\r\n"), dwTempID, dwDeviceID);
	LONG lResult = GetSP()->providerCreatePhoneDevice(dwTempID, dwDeviceID);

	DTRACE(TRC_API, _T("TSPI_providerCreatePhoneDevice rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_providerCreatePhoneDevice

/////////////////////////////////////////////////////////////////////////////
// TSPI_providerFreeDialogInstance (Tapi 2.0)
//
// This function is called to inform the service provider that
// the dialog associated with the "hdDlgInstance" has exited.
// After this function is called, the service provider should no
// longer send data to the dialog using the LINE_SENDDIALOGINSTANCEDATA
// message.
//
extern "C"
LONG TSPIAPI TSPI_providerFreeDialogInstance (HDRVDIALOGINSTANCE hdDlgInstance)
{
	DTRACE(TRC_API, _T("TSPI_providerFreeDialogInstance beginning\r\n"));
	DTRACE(TRC_API, _T("  Dialog=0x%lx\r\n"), (DWORD) hdDlgInstance);

	LONG lResult = GetSP()->providerFreeDialogInstance (hdDlgInstance);

	DTRACE(TRC_API, _T("TSPI_providerFreeDialogInstance rc=0x%lx\r\n"), lResult);
	return lResult;

}// TSPI_providerFreeDialogInstance

/////////////////////////////////////////////////////////////////////////////
// TSPI_providerGenericDialogData (Tapi 2.0)
//
// This function delivers to the service provider data that was
// sent from the UI DLL running in an application context via
// the TSISPIDLLCALLBACK function.  The contents of the memory
// block pointed to be lpParams is defined by the service provider
// and UI DLL.  The service provider can modify the contents of the
// memory block; when this function returns, TAPI will copy the
// new modified data back to the original UI DLL parameter block.
//
extern "C"
LONG TSPIAPI TSPI_providerGenericDialogData (DWORD dwObjectID,
			DWORD dwObjectType, LPVOID lpParams, DWORD dwSize)
{
	DTRACE(TRC_API, _T("TSPI_providerGenericDialogData beginning\r\n"));
	DTRACE(TRC_API, _T("  Object ID=0x%lx, Object Type=0x%lx\r\n"), dwObjectID, dwObjectType);
	DTRACE(TRC_API, _T("  Params=0x%lx, Size=%ld\r\n"), lpParams, dwSize);
	DUMPMEM (_T("Generic Data Block->\r\n"), lpParams, dwSize);

	CTSPIDevice* pDevice = NULL;
	CTSPILineConnection* pLine = NULL;
	CTSPIPhoneConnection* pPhone = NULL;
	HDRVDIALOGINSTANCE hdDlgInst = NULL;

	LONG lResult = 0L;

	if (dwObjectType == TUISPIDLL_OBJECT_LINEID)
	{
		pLine = GetSP()->GetConnInfoFromLineDeviceID(dwObjectID);
		if (pLine == NULL)
			lResult = LINEERR_BADDEVICEID;

		// Internal mechanism to convert line/phone ids into provider
		if (dwSize == sizeof(DWORD)*2 && *((LPDWORD)lpParams) == GDD_LINEPHONETOPROVIDER)
		{
			LPDWORD pdwInfo = (LPDWORD) lpParams;
			*pdwInfo++ = GDD_LINEPHONETOPROVIDEROK;
			*pdwInfo = pLine->GetDeviceInfo()->GetProviderID();
			return 0;
		}
		// or permanent line/phone id
		else if (dwSize == sizeof(DWORD)*2 && *((LPDWORD)lpParams) == GDD_LINEPHONETOPERMANENT)
		{
			LPDWORD pdwInfo = (LPDWORD) lpParams;
			*pdwInfo++ = GDD_LINEPHONETOPERMANENTOK;
			*pdwInfo = pLine->GetPermanentDeviceID();
			return 0;
		}
	}

	else if (dwObjectType == TUISPIDLL_OBJECT_PHONEID)
	{
		pPhone = GetSP()->GetConnInfoFromPhoneDeviceID(dwObjectID);
		if (pPhone == NULL)
			lResult = PHONEERR_BADDEVICEID;

		// Internal mechanism to convert line/phone ids into provider
		if (dwSize == sizeof(DWORD)*2 && *((LPDWORD)lpParams) == GDD_LINEPHONETOPROVIDER)
		{
			LPDWORD pdwInfo = (LPDWORD) lpParams;
			*pdwInfo++ = GDD_LINEPHONETOPROVIDEROK;
			*pdwInfo = pPhone->GetDeviceInfo()->GetProviderID();
			return 0;
		}
		// or permanent line/phone id
		else if (dwSize == sizeof(DWORD)*2 && *((LPDWORD)lpParams) == GDD_LINEPHONETOPERMANENT)
		{
			LPDWORD pdwInfo = (LPDWORD) lpParams;
			*pdwInfo++ = GDD_LINEPHONETOPERMANENTOK;
			*pdwInfo = pPhone->GetPermanentDeviceID();
			return 0;
		}
	}

	else if (dwObjectType == TUISPIDLL_OBJECT_PROVIDERID)
	{
		pDevice = GetSP()->GetDevice(dwObjectID);
		if (pDevice == NULL)
			lResult = TAPIERR_INVALDEVICEID;
	}

	else if (dwObjectType == TUISPIDLL_OBJECT_DIALOGINSTANCE)
		hdDlgInst = (HDRVDIALOGINSTANCE) dwObjectID;

	else
		lResult = LINEERR_INVALPARAM;

	if (lResult == 0)
		lResult = GetSP()->providerGenericDialogData (pDevice, pLine, 
					pPhone, hdDlgInst,lpParams, dwSize);
	
	DTRACE(TRC_API, _T("TSPI_providerGenericDialogData rc=0x%lx\r\n"), lResult);
#ifdef _DEBUG
	if (lResult == 0)
		DUMPMEM (_T("Returning Data Block->\r\n"), lpParams, dwSize);
#endif
	return lResult;

}// TSPI_providerGenericDialogData

/////////////////////////////////////////////////////////////////////////////
// TSPI_providerUIIdentify (Tapi 2.0)
//
// This function returns the name of the UI DLL for this 
// service provider.
//
extern "C"
LONG TSPIAPI TSPI_providerUIIdentify (LPWSTR lpszUIDLLName)
{
	DTRACE(TRC_API, _T("TSPI_providerUIIdentify beginning\r\n"));
	DTRACE(TRC_API, _T("  Buffer=0x%lx\r\n"), (DWORD)lpszUIDLLName);

	LONG lResult = GetSP()->providerUIIdentify(lpszUIDLLName);

	DTRACE(TRC_API, _T("TSPI_providerUIIdentify rc=0x%lx\r\n"), lResult);
#ifdef _DEBUG
	if (lResult == 0)
	DUMPMEM(_T("UIDLL Name->\r\n"), lpszUIDLLName, MAX_PATH-1);
#endif

	return lResult;

}// TSPI_providerUIIdentify

/******************************************************************************/
//
// TSPIAPI TUISPI user-interface functions for 32-bit TAPI
//
/******************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// TUISPI_lineConfigDialog
//
// This function is called to display the line configuration dialog
// when the user requests it through either the TAPI api or the control
// panel applet.
//
extern "C"
LONG TSPIAPI TUISPI_lineConfigDialog (TUISPIDLLCALLBACK lpfnDLLCallback,
				DWORD dwDeviceID, HWND hwndOwner, LPCWSTR lpszDeviceClass)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();

	DTRACE(TRC_API, _T("TUISPI_lineConfigDialog beginning\r\n"));
	DTRACE(TRC_API, _T("  DLL Callback=0x%lx\r\n"), (DWORD) lpfnDLLCallback);
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, hwndOwner=%08lx\r\n"), dwDeviceID, CASTHANDLE(hwndOwner));
	DTRACE(TRC_API, _T("  DeviceClass=<%s>\r\n"), (LPCTSTR) strDevClass);

	LONG lResult = GetSP()->lineConfigDialog(dwDeviceID, CWnd::FromHandle(hwndOwner), 
							strDevClass, lpfnDLLCallback);

	DTRACE(TRC_API, _T("TUISPI_lineConfigDialog rc=0x%lx\r\n"), lResult);

	return lResult;

}// TUISPI_lineConfigDialog

///////////////////////////////////////////////////////////////////////////
// TUISPI_lineConfigDialogEdit
//
// This function causes the provider of the specified line device to
// display a modal dialog to allow the user to configure parameters
// related to the line device.  The parameters editted are NOT the
// current device parameters, rather the set retrieved from the
// 'TSPI_lineGetDevConfig' function (lpDeviceConfigIn), and are returned
// in the lpDeviceConfigOut parameter.
//
extern "C"
LONG TSPIAPI TUISPI_lineConfigDialogEdit (TUISPIDLLCALLBACK lpfnDLLCallback,
		 DWORD dwDeviceID, HWND hwndOwner,
         LPCWSTR lpszDeviceClass, LPVOID const lpDeviceConfigIn, DWORD dwSize,
         LPVARSTRING lpDeviceConfigOut)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();

	DTRACE(TRC_API, _T("TUISPI_lineConfigDialogEdit beginning\r\n"));
	DTRACE(TRC_API, _T("  DLL Callback=0x%lx\r\n"), (DWORD) lpfnDLLCallback);
	DTRACE(TRC_API, _T("  DeviceId=0x%lx, hwndOwner=%08lx\r\n"), dwDeviceID, CASTHANDLE(hwndOwner));
	DTRACE(TRC_API, _T("  DeviceClass=<%s>\r\n"), (LPCTSTR) strDevClass);
	DTRACE(TRC_API, _T("  ConfigIn=0x%lx, Size=%ld\r\n"), (DWORD)lpDeviceConfigIn, dwSize);
	DUMPMEM (_T("lpConfigIn ->\r\n"), lpDeviceConfigIn, dwSize);

    LONG lResult = GetSP()->lineConfigDialogEdit(dwDeviceID, CWnd::FromHandle(hwndOwner),
                        strDevClass, lpDeviceConfigIn, dwSize,
                        lpDeviceConfigOut, lpfnDLLCallback);
#ifdef _DEBUG
	DumpVarString (lpDeviceConfigOut);
#endif
	DTRACE(TRC_API, _T("TUISPI_lineConfigDialogEdit rc=0x%lx\r\n"), lResult);
    return lResult;

}// TUISPI_lineConfigDialogEdit

///////////////////////////////////////////////////////////////////////////
// TUISPI_phoneConfigDialog
//
// This function invokes the parameter configuration dialog for the
// phone device.
//
extern "C"
LONG TSPIAPI TUISPI_phoneConfigDialog (TUISPIDLLCALLBACK lpfnDLLCallback,
			DWORD dwDeviceId, HWND hwndOwner, LPCWSTR lpszDeviceClass)
{
#ifdef _UNICODE
	CString strDevClass = lpszDeviceClass;
#else
	CString strDevClass = ConvertWideToAnsi (lpszDeviceClass);
#endif

	strDevClass.MakeLower();

	DTRACE(TRC_API, _T("TUISPI_phoneConfigDialog beginning\r\n"));
	DTRACE(TRC_API, _T("  DLL Callback=0x%lx\r\n"), (DWORD) lpfnDLLCallback);
	DTRACE(TRC_API, _T("  Device Id=0x%lx, Owner=%08lx\r\n"), dwDeviceId, CASTHANDLE(hwndOwner));
	DTRACE(TRC_API, _T("  Device Class=<%s>\r\n"), (LPCTSTR) strDevClass);

    LONG lResult = GetSP()->phoneConfigDialog(dwDeviceId, CWnd::FromHandle(hwndOwner), 
                           strDevClass, lpfnDLLCallback);

	DTRACE(TRC_API, _T("TUISPI_phoneConfigDialog rc=0x%lx\r\n"), lResult);
	return lResult;

}// TUISPI_phoneConfigDialog

////////////////////////////////////////////////////////////////////////////
// TUISPI_providerConfig
//
// This function is invoked from the control panel and allows the user
// to configure our service provider.
//        
extern "C"
LONG TSPIAPI TUISPI_providerConfig (TUISPIDLLCALLBACK lpfnDLLCallback,
		HWND hwndOwner, DWORD dwPermanentProviderID)
{
	DTRACE(TRC_API, _T("TUISPI_providerConfig beginning\r\n"));
	DTRACE(TRC_API, _T("  DLL Callback=0x%lx\r\n"), (DWORD) lpfnDLLCallback);
	DTRACE(TRC_API, _T("  hwndOwner=%08lx, ProviderId=0x%lx\r\n"), CASTHANDLE(hwndOwner), dwPermanentProviderID);

	LONG lResult = GetSP()->providerConfig(dwPermanentProviderID, 
						CWnd::FromHandle(hwndOwner), lpfnDLLCallback);

	DTRACE(TRC_API, _T("TUISPI_providerConfig rc=0x%lx\r\n"), lResult);
	return lResult;
   
}// TUISPI_providerConfig

////////////////////////////////////////////////////////////////////////////
// TUISPI_providerGenericDialog
//
// This function is called when the service provider requests
// a spontaneous dialog not known by TAPI itself.
//
extern "C"
LONG TSPIAPI TUISPI_providerGenericDialog (TUISPIDLLCALLBACK lpfnDLLCallback,
		HTAPIDIALOGINSTANCE htDlgInst, LPVOID lpParams, DWORD dwSize,
		HANDLE hEvent)
{
	DTRACE(TRC_API, _T("TUISPI_providerGenericDialog beginning\r\n"));
	DTRACE(TRC_API, _T("  DLL Callback=0x%lx\r\n"), (DWORD) lpfnDLLCallback);
	DTRACE(TRC_API, _T("  TAPI Dialog Handle=0x%lx, Event=0x%lx\r\n"), (DWORD)htDlgInst, CASTHANDLE(hEvent));
	DTRACE(TRC_API, _T("  Parameters=0x%lx, Size=%ld\r\n"), lpParams, dwSize);
	DUMPMEM(_T("Param Block->\r\n"), lpParams, dwSize);

	LONG lResult = GetSP()->providerGenericDialog (htDlgInst,
					lpParams, dwSize, hEvent, lpfnDLLCallback);

	DTRACE(TRC_API, _T("TUISPI_providerGenericDialog rc=0x%lx\r\n"), lResult);

	return lResult;

}// TUISPI_providerGenericDialog

/////////////////////////////////////////////////////////////////////////////
// TSPI_providerGenericDialogData
//
// This function delivers to the UI DLL data that was
// sent from the service provider via the LINE_SENDDIALOGINSTANCEDATA
// message.  The contents of the memory block pointed to be 
// lpParams is defined by the service provider and UI DLL.  
//
extern "C"
LONG TSPIAPI TUISPI_providerGenericDialogData (
		HTAPIDIALOGINSTANCE htDlgInst, LPVOID lpParams, DWORD dwSize)
{
	DTRACE(TRC_API, _T("TUISPI_providerGenericDialogData beginning\r\n"));
	DTRACE(TRC_API, _T("  TAPI Dialog Handle=0x%lx\r\n"), (DWORD)htDlgInst);
	DTRACE(TRC_API, _T("  Parameters=0x%lx, Size=%ld\r\n"), lpParams, dwSize);
	DUMPMEM(_T("Param Block->\r\n"), lpParams, dwSize);

	LONG lResult = GetSP()->providerGenericDialogData (htDlgInst,
					lpParams, dwSize);

	DTRACE(TRC_API, _T("TUISPI_providerGenericDialogData rc=0x%lx\r\n"), lResult);

	return lResult;

}// TSPI_providerGenericDialogData

////////////////////////////////////////////////////////////////////////////
// TUISPI_providerInstall
//
// This function is invoked to install the service provider onto
// the system.
//        
extern "C"
LONG TSPIAPI TUISPI_providerInstall(TUISPIDLLCALLBACK lpfnDLLCallback,
			HWND hwndOwner, DWORD dwPermanentProviderID)
{
	DTRACE(TRC_API, _T("TUISPI_providerInstall beginning\r\n"));
	DTRACE(TRC_API, _T("  DLL Callback=0x%lx\r\n"), (DWORD) lpfnDLLCallback);
	DTRACE(TRC_API, _T("  hwndOwner=%08lx, ProviderId=0x%lx\r\n"), CASTHANDLE(hwndOwner), dwPermanentProviderID);

	LONG lResult = GetSP()->providerInstall(dwPermanentProviderID, CWnd::FromHandle(hwndOwner),
							lpfnDLLCallback);

	DTRACE(TRC_API, _T("TUISPI_providerInstall rc=0x%lx\r\n"), lResult);

	return lResult;

}// TUISPI_providerInstall

////////////////////////////////////////////////////////////////////////////
// TUISPI_providerRemove
//
// This function removes the service provider
//
extern "C"
LONG TSPIAPI TUISPI_providerRemove(TUISPIDLLCALLBACK lpfnDLLCallback,
			HWND hwndOwner, DWORD dwPermanentProviderID)
{
	DTRACE(TRC_API, _T("TUISPI_providerRemove beginning\r\n"));
	DTRACE(TRC_API, _T("  DLL Callback=0x%lx\r\n"), (DWORD) lpfnDLLCallback);
	DTRACE(TRC_API, _T("  hwndOwner=%08lx, ProviderId=0x%lx\r\n"), CASTHANDLE(hwndOwner), dwPermanentProviderID);

	LONG lResult = GetSP()->providerRemove(dwPermanentProviderID, CWnd::FromHandle(hwndOwner),
							lpfnDLLCallback);

	DTRACE(TRC_API, _T("TUISPI_providerRemove rc=0x%lx\r\n"), lResult);

	return lResult;

}// TUISPI_providerRemove
