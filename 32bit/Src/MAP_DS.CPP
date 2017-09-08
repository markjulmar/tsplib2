/******************************************************************************/
//                                                                        
// MAP_DS.CPP - Source code for the CMapDWordToString object map
//                                                                        
// Copyright (C) 1994-1999 Mark C. Smith, JulMar Entertainment Technology, Inc.
// Copyright (C) 2000 JulMar Entertainment Technology, Inc.
// All rights reserved                                                    
//                                                                        
// This file contains all the source to implement the map which associates
// DWORD values to strings.
//
// Original Copyright © 1994-2004 JulMar Entertainment Technology, Inc. All rights reserved.
// Taken from MFC sources and altered to support different data key/type.
// Portions Copyright (C) 1995, 1996 Microsoft Corp.
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
#include <afxplex_.h>				// Internal data plex support.

IMPLEMENT_SERIAL(CMapDWordToString, CObject, 0)

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::CMapDWordToString
//
// Constructor for our map
//
CMapDWordToString::CMapDWordToString(int nBlockSize) :
	m_pHashTable(NULL), m_nHashTableSize(17), m_nCount(0),
	m_pFreeList(NULL), m_pBlocks(NULL)
{
	if (nBlockSize == 0)
		nBlockSize = 10;
	m_nBlockSize = nBlockSize;

}// CMapDWordToString::CMapDWordToString

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::~CMapDWordToString
//
// Destructor for the map - remove and delete all the keys and values.
//
CMapDWordToString::~CMapDWordToString()
{
	RemoveAll();

}// CMapDWordToString::~CMapDWordToString

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::InitHashTable
//
// This allocates the hash table for matching keys.
//
void CMapDWordToString::InitHashTable(UINT nHashSize, BOOL bAllocNow)
{
	ASSERT(m_nCount == 0);
	ASSERT(nHashSize > 0);

	// If there is already a hash table in existance, free it.
	if (m_pHashTable != NULL)
	{
		delete[] m_pHashTable;
		m_pHashTable = NULL;
	}

	// Allocate a new hash table of the specified size.
	if (bAllocNow)
	{
		m_pHashTable = new CAssoc* [nHashSize];
		memset(m_pHashTable, 0, sizeof(CAssoc*) * nHashSize);
	}

	m_nHashTableSize = nHashSize;

}// CMapDWordToString::InitHashTable

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::RemoveAll
//
// Remove all entries from our map.
//
void CMapDWordToString::RemoveAll()
{
	// If we have a hash table, delete it.
	if (m_pHashTable != NULL)
	{
		// free hash table
		delete[] m_pHashTable;
		m_pHashTable = NULL;
	}

	// Reset all the counts, freelist, and delete the data chain.
	m_nCount = 0;
	m_pFreeList = NULL;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;

}// CMapDWordToString::RemoveAll

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::NewAssoc
//
// Internal function to allocate a new association record.
//
CMapDWordToString::CAssoc* CMapDWordToString::NewAssoc()
{
	// If we don't have any blocks on the free list, then allocate a new block.
	if (m_pFreeList == NULL)
	{
		// Allocate the block and add it into our data block chain.
		CPlex* newBlock = CPlex::Create(m_pBlocks, m_nBlockSize, sizeof(CMapDWordToString::CAssoc));
		CMapDWordToString::CAssoc* pAssoc = (CMapDWordToString::CAssoc*) newBlock->data();
		pAssoc += m_nBlockSize - 1;

		// Move the block into the free list where it will be allocated from.
		for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
		{
			pAssoc->pNext = m_pFreeList;
			m_pFreeList = pAssoc;
		}
	}

	// Grab the association from the free list, increment the 
	// pointer to the free list and increment our count of used 
	// associations.
	CMapDWordToString::CAssoc* pAssoc = m_pFreeList;
	m_pFreeList = m_pFreeList->pNext;
	m_nCount++;
	ASSERT(m_nCount > 0);  // make sure we don't overflow
	
	// Initialize the association and return it.
	pAssoc->key = 0;
	pAssoc->value = "";
	return pAssoc;

}// CMapDWordToString::NewAssoc

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::FreeAssoc
//
// Internal function to deallocate an association record from our
// data chain.  The association is moved to our freelist, and when we
// have no associations left on the data chain, the whole data block
// is deallocated.
//
void CMapDWordToString::FreeAssoc(CMapDWordToString::CAssoc* pAssoc)
{
	// Move it to the head of the freelist
	pAssoc->pNext = m_pFreeList;
	m_pFreeList = pAssoc;
	m_nCount--;
	
	ASSERT(m_nCount >= 0);  // make sure we don't underflow
	if (m_nCount == 0)
		RemoveAll();

}// CMapDWordToString::FreeAssoc

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::GetAssocAt
//
// Locate an association record with a DWORD key.  If the key is not
// in-use, return a NULL record.
//
CMapDWordToString::CAssoc* CMapDWordToString::GetAssocAt(DWORD key, UINT& nHash) const
{
	// If we have no hash table, return a NULL
	if (m_pHashTable == NULL)
		return NULL;

	// Get the hash key value.
	nHash = HashKey(key) % m_nHashTableSize;

	// See if it exists
	CAssoc* pAssoc;
	for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (pAssoc->key == key)
			return pAssoc;
	}
	return NULL;

}// CMapDWordToString::GetAssocAt

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::Lookup
// 
// Public function to retrieve a string based on a key
//
BOOL CMapDWordToString::Lookup(DWORD key, CString& rValue) const
{
	UINT nHash;
	CAssoc* pAssoc = GetAssocAt(key, nHash);
	if (pAssoc == NULL)
		return FALSE;

	// Return the value found.
	rValue = pAssoc->value;
	return TRUE;

}// CMapDWordToString::Lookup

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToStringoperator[]
//
// Return the string associated with a specified key.  If no value exists,
// add a new key and return a reference to the data value.
//
CString& CMapDWordToString::operator[](DWORD key)
{
	UINT nHash;
	CAssoc* pAssoc;
	if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
	{
		if (m_pHashTable == NULL)
			InitHashTable(m_nHashTableSize);

		// it doesn't exist, add a new Association
		pAssoc = NewAssoc();
		pAssoc->nHashValue = nHash;
		pAssoc->key = key;

		// put into hash table
		pAssoc->pNext = m_pHashTable[nHash];
		m_pHashTable[nHash] = pAssoc;
	}
	
	// Return the reference
	return pAssoc->value;

}// CMapDWordToStringoperator[]

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::RemoveKey
//
// Remove a specific association from our list.
//
BOOL CMapDWordToString::RemoveKey(DWORD key)
{
	// No data in our table?
	if (m_pHashTable == NULL)
		return FALSE;

	// Locate the association BEFORE the one we are removing.
	// This will be cross-linked back.
	CAssoc** ppAssocPrev;
	ppAssocPrev = &m_pHashTable[HashKey(key) % m_nHashTableSize];

	// Now, find our association and remove it from the list.
	CAssoc* pAssoc;
	for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
	{
		if (pAssoc->key == key)
		{
			*ppAssocPrev = pAssoc->pNext;
			FreeAssoc(pAssoc);
			return TRUE;
		}
		ppAssocPrev = &pAssoc->pNext;
	}
	
	// Never found the association.
	return FALSE;

}// CMapDWordToString::RemoveKey

/////////////////////////////////////////////////////////////////////////////
// CMapDWordToString::GetNextAssoc
//
// Return the next association in our list.
//
void CMapDWordToString::GetNextAssoc(POSITION& rNextPosition, DWORD& rKey, CString& rValue) const
{
	ASSERT(m_pHashTable != NULL);

	CAssoc* pAssocRet = (CAssoc*)rNextPosition;
	ASSERT(pAssocRet != NULL);

	// If we are to locate the FIRST association.
	if (pAssocRet == (CAssoc*) BEFORE_START_POSITION)
	{
		for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
		{
			if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
				break;
		}
		ASSERT(pAssocRet != NULL);
	}

	// Otherwise, locate the NEXT association.
	ASSERT(AfxIsValidAddress(pAssocRet, sizeof(CAssoc)));
	CAssoc* pAssocNext;
	if ((pAssocNext = pAssocRet->pNext) == NULL)
	{
		for (UINT nBucket = pAssocRet->nHashValue + 1; nBucket < m_nHashTableSize; nBucket++)
		{
			if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
				break;
		}
	}

	// Replace the value in our position.
	rNextPosition = (POSITION) pAssocNext;

	// Fill in return data
	rKey = pAssocRet->key;
	rValue = pAssocRet->value;

}// CMapDWordToString::GetNextAssoc
