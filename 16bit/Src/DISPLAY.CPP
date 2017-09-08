/******************************************************************************/
//                                                                        
// DISPLAY.CPP - Source code for the CPhoneDisplay class.
//                                                                        
// Copyright (C) 1994-1997 Mark C. Smith
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains the code for driving a phone display.               
//
// This source code is intended only as a supplement to the
// TSP++ Class Library product documentation.  This source code cannot 
// be used in part or whole in any form outside the TSP++ library.
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

//////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::CPhoneDisplay
//
// Display constructor
//
CPhoneDisplay::CPhoneDisplay()
{
    m_lpsDisplay = NULL;
    m_sizDisplay = CSize(0,0);
    m_ptCursor = CPoint(0,0);
    m_cLF = '\n';

}// CPhoneDisplay::CPhoneDisplay

//////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::~CPhoneDisplay
//
// Display Destructor
//
CPhoneDisplay::~CPhoneDisplay()
{
    delete m_lpsDisplay;

}// CPhoneDisplay::~CPhoneDisplay

///////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::Init
//
// Initialize our display
//
void CPhoneDisplay::Init(int iColumnCount, int iRowCount, char cLF)
{
    delete m_lpsDisplay;
    m_lpsDisplay = NULL;
    m_cLF = cLF;
    
    // Allocate a buffer if we have coumns/rows
    if (iColumnCount > 0 && iRowCount > 0)
    {
        m_sizDisplay.cx = iColumnCount;
        m_sizDisplay.cy = iRowCount;
        m_lpsDisplay = new char [m_sizDisplay.cx*m_sizDisplay.cy+1];
    }
   
}// CPhoneDisplay::Init

///////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::GetCursorPosition
//
// Returns the maximum number of columns on display
//
CPoint CPhoneDisplay::GetCursorPosition() const
{
    return m_ptCursor;

}// CPhoneDisplay::GetCursorPosition

///////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::GetDisplaySize
//
// Returns the size of our display.
//
CSize CPhoneDisplay::GetDisplaySize() const
{
    return m_sizDisplay;

}// CPhoneDisplay::GetDisplaySize

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::GetTextBuffer
//
// Return the contents of the screen buffer
//
LPCSTR CPhoneDisplay::GetTextBuffer() const
{
    return m_lpsDisplay;

}// CPhoneDisplay::GetTextBuffer

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::AddCharacter
//
// Add a character at the cursor position.
//
void CPhoneDisplay::AddCharacter(char cChar)
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
// CPhoneDisplay::AddString
//
// Add a string to the display.
//
void CPhoneDisplay::AddString(LPCSTR pszText)
{
    while (*pszText != '\0')
        AddCharacter (*pszText++);

}// CPhoneDisplay::AddString

////////////////////////////////////////////////////////////////////////////////
// CPhoneDisplay::SetCharacterAtPosition
//
// Set a character into the display at a particular position.
//
void CPhoneDisplay::SetCharacterAtPosition (int iColumn, int iRow, char cChar)
{
    if (iColumn == -1)
        iColumn = m_ptCursor.x;
    if (iRow == -1)
        iRow = m_ptCursor.y;
    
    // Validate the character.
    if (cChar > '\0' && cChar != m_cLF &&
        iColumn >= 0 && iColumn < m_sizDisplay.cx &&
        iRow >= 0 && iRow < m_sizDisplay.cy &&
        m_lpsDisplay != NULL)
    {
        m_lpsDisplay [iRow*m_sizDisplay.cx+iColumn] = cChar;
    }

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
        FillBuffer (m_lpsDisplay, ' ', (m_sizDisplay.cx*m_sizDisplay.cy));
        m_lpsDisplay[m_sizDisplay.cx*m_sizDisplay.cy] = '\0';
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
    {
        for (int x = 0; x < m_sizDisplay.cx; x++)
            SetCharacterAtPosition(x, iRow, ' ');
    }

}// CPhoneDisplay::ClearRow
