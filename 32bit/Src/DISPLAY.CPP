/******************************************************************************/
//                                                                        
// DISPLAY.CPP - Source code for the CPhoneDisplay class.
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains the code for driving a phone display.               
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
// Debug memory diagnostics

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::Init
//
// Initialize our display
//
void CPhoneDisplay::Init(int iColumnCount, int iRowCount, TCHAR cLF)
{
    delete m_lpsDisplay;
    m_lpsDisplay = NULL;
    m_cLF = cLF;
    
    // Allocate a buffer if we have coumns/rows
    if (iColumnCount > 0 && iRowCount > 0)
    {
        m_sizDisplay.cx = iColumnCount;
        m_sizDisplay.cy = iRowCount;
        m_lpsDisplay = new TCHAR [m_sizDisplay.cx*m_sizDisplay.cy+1];
    }
   
}// CPhoneDisplay::Init

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::AddCharacter
//
// Add a character at the cursor position.
//
void CPhoneDisplay::AddCharacter(TCHAR cChar)
{
    // If the character is our LINEFEED, then adjust the cursor position.
    if (cChar == m_cLF)
    {
        if (++m_ptCursor.y >= m_sizDisplay.cy)
            m_ptCursor.y = 0;
        m_ptCursor.x = 0;
    }
    else // Normal character.
    {   
        // Place the character into the buffer.
        SetCharacterAtPosition(-1, -1, cChar);
        // Increment our cursor position.
        if (++m_ptCursor.x >= m_sizDisplay.cx)
        {
            m_ptCursor.x = 0;
            if (++m_ptCursor.y >= m_sizDisplay.cy)
                m_ptCursor.y = 0;
        }
    }

}// CPhoneDisplay::AddCharacter

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::SetCharacterAtPosition
//
// Set a character into the display at a particular position.
//
void CPhoneDisplay::SetCharacterAtPosition (int iColumn, int iRow, TCHAR cChar)
{
    if (iColumn == -1)
        iColumn = m_ptCursor.x;
    if (iRow == -1)
        iRow = m_ptCursor.y;
    
    // Validate the character.
    if (cChar > _T('\0') && cChar != m_cLF &&
        iColumn >= 0 && iColumn < m_sizDisplay.cx &&
        iRow >= 0 && iRow < m_sizDisplay.cy && m_lpsDisplay != NULL)
        m_lpsDisplay [iRow*m_sizDisplay.cx+iColumn] = cChar;

}// CPhoneDisplay::SetCharacterAtPosition

///////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::SetCursorPosition
//
// Set the current cursor position for the display
//
void CPhoneDisplay::SetCursorPosition(int iColumn, int iRow)
{
    if (iColumn >= 0 && iColumn < m_sizDisplay.cx)
        m_ptCursor.x = iColumn;
    if (iRow >= 0 && iRow < m_sizDisplay.cy)
        m_ptCursor.y = iRow;

}// CPhoneDisplay::SetCursorPosition

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::Reset
//
// Reset the contents of the display and clear the buffer.
// 
VOID CPhoneDisplay::Reset()
{
    m_ptCursor = CPoint(0,0);
    if (m_lpsDisplay)
    {
        FillBuffer (m_lpsDisplay, _T(' '), (m_sizDisplay.cx*m_sizDisplay.cy));
        m_lpsDisplay[m_sizDisplay.cx*m_sizDisplay.cy] = _T('\0');
    }        

}// CPhoneDisplay::Reset

/////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::ClearRow
//
// Clear a single row of our display.  Do NOT change the cursor
// positioning
//
void CPhoneDisplay::ClearRow(int iRow)
{
    if (iRow >= 0 && iRow < m_sizDisplay.cy)
        for (int x = 0; x < m_sizDisplay.cx; x++)
            SetCharacterAtPosition(x, iRow, _T(' '));

}// CPhoneDisplay::ClearRow
