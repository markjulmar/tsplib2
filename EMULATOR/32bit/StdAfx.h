// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__C4B58209_5DD2_11D1_9C3D_006097D5E97D__INCLUDED_)
#define AFX_STDAFX_H__C4B58209_5DD2_11D1_9C3D_006097D5E97D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define WINVER 0x501

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxsock.h>		// MFC socket extensions
#include <ctype.h>
#include "controls.h"
#include "emintf.h"

// Define our timer events.
#define TIMER_RING        100     // This is used for RING events
#define TIMER_PAINT_LAMPS 101     // This is used to paint lamps
#define TIMER_APP 		  102     // This is used as the interval timer to the TSP.
                                
// Button define info.
#define FIRST_FUNCTION_INDEX    12
#define LAST_FUNCTION_INDEX     27
#define BTN_VOLUP               28
#define BTN_VOLDOWN             29
#define BTN_HOLD                30
#define BTN_DROP                31
#define BTN_MSGWAITING          32

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C4B58209_5DD2_11D1_9C3D_006097D5E97D__INCLUDED_)
