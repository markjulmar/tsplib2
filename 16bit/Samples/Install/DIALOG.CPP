/******************************************************************************/
//
// DIALOG.CPP - Dialog to run the installation sample
//
// Copyright (C) 1997 JulMar Entertainment Technology, Inc.
// Copyright (C) 1997 Mark C. Smith
// All rights reserved.
//
// This source code is only intended as a supplement to the
// documentation provided with the TSPI C++ class library.
//
// WARNING: THIS CODE IS SPECIFIC TO 16-BIT TAPI!!
//          IT IS TO DEMONSTRATE LOADING V1.3 PROVIDERS UNDER WIN95 AND WIN31!
//
// DO NOT USE THIS FOR 32-BIT INSTALLATIONS!!
// 
/******************************************************************************/

#include "stdafx.h"
#include "dialog.h"
#include <windowsx.h>

/////////////////////////////////////////////////////////////////////////////
// Globals

CMainApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Definitions

typedef LONG (TSPIAPI * INITPROC) (HWND, DWORD);
typedef LONG (WINAPI * ADDPROC) (LPCSTR, HWND, LPDWORD);
typedef LONG (WINAPI * REMPROC) (DWORD, HWND);

#define COPYSIZE 4096
#define WF_WINNT 0x4000

 ///////////////////////////////////////////////////////////////////////////
// CopyFile
//
// Copies a file from source to destination.
//
BOOL CopyFile(const char* pszSource, const char* pszDest)
{       
    // Open the source and destination files.
    CFile srcFile, dstFile;
    if (!srcFile.Open (pszSource, CFile::modeRead | CFile::shareDenyNone))
        return(FALSE);
  
    if (!dstFile.Open (pszDest, CFile::modeCreate | CFile::modeReadWrite))
        return(FALSE);  

    // Allocate a block of memory to transfer the file
    // into.
    LPSTR pszBuff = (LPSTR) GlobalAllocPtr (GHND, COPYSIZE);
    if (pszBuff == NULL)
        return(FALSE);

	// Get the source data/time.
    CFileStatus srcTime;;
    srcFile.GetStatus (srcTime);
            
    // Run through the file reading blocks
    // and writing them to the destination
    // file.
    BOOL fDone = FALSE;
    TRY
    {
        while (!fDone)
        {   
            UINT iBytes = srcFile.Read(pszBuff, COPYSIZE);
            if (iBytes > 0)
                dstFile.Write(pszBuff, iBytes);
            if (iBytes < COPYSIZE)
                fDone = TRUE;
        } 
    }
    CATCH (CFileException, fe)
    {   
    }
    END_CATCH

    // Close the files and deallocate the memory.
    dstFile.Close();
    srcFile.Close();
    GlobalFreePtr (pszBuff);

    // If the copy did not succeed, then delete
    // the destination file.
    if (!fDone)
        CFile::Remove(pszDest);
    else
    {
		// Apply the time stamp to the destination file.
	    TRY
	    {
	        CFile::SetStatus (pszDest, srcTime);
	    }                               
	    CATCH (CFileException, fe)
	    {
	    }
	    END_CATCH
    }
    
    return(fDone);

}// CopyFile

/////////////////////////////////////////////////////////////////////////////
// CMainApp::InitInstance
//
// Initialize the main application
//
BOOL CMainApp::InitInstance()
{   
	// Gray dialogs.
	SetDialogBkColor();

    // Set our INI file to be the TELEPHON.INI
    AfxGetApp()->m_pszProfileName = "Telephon.ini";

	// If TAPI isn't available, gracefully error out.
	if (CInstallDlg::TAPI_Installed() == FALSE)
		AfxMessageBox (IDS_NOTAPI);

	CInstallDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	
	return FALSE;

}// CMainApp::InitInstance

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::CInstallDlg
//
// Constructor for the answer dialog
//
CInstallDlg::CInstallDlg(CWnd* pParent) : CDialog(CInstallDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInstallDlg)
	m_fATSP = FALSE;
	m_fDSSP = FALSE;
	m_foATSP = FALSE;
	m_foDSSP = FALSE;
	m_strATSPOk = "";
	m_strDSSPOk = "";
	m_strPath = "";
	m_strSystemDir = "";
	//}}AFX_DATA_INIT

}// CInstallDlg::CInstallDlg

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::~CInstallDlg
//
// Destructor for the answer dialog
//
CInstallDlg::~CInstallDlg()
{
}// CInstallDlg::~CInstallDlg

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::DoDataExchange
//
// Dialog data exchange for the answer dialog
//
void CInstallDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInstallDlg)
	DDX_Check(pDX, IDC_SP_ATSP, m_fATSP);
	DDX_Check(pDX, IDC_SP_DSSP, m_fDSSP);
	DDX_Text(pDX, IDC_TEXT1, m_strATSPOk);
	DDX_Text(pDX, IDC_TEXT2, m_strDSSPOk);
	//}}AFX_DATA_MAP

}// CInstallDlg::DoDataExchange

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg message map

BEGIN_MESSAGE_MAP(CInstallDlg, CDialog)
	//{{AFX_MSG_MAP(CInstallDlg)
	ON_BN_CLICKED(IDC_SP_ATSP, OnChange)
	ON_BN_CLICKED(IDC_SP_DSSP, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::OnInitDialog
//
// Manage the creation of our dialog box.
//
BOOL CInstallDlg::OnInitDialog()
{
	// Find our current working directory based on the
	// path of this file.
	GetModuleFileName(AfxGetInstanceHandle(), m_strPath.GetBuffer(_MAX_PATH), _MAX_PATH);
	m_strPath.ReleaseBuffer();
	int iPos = m_strPath.ReverseFind('\\');
	if (iPos > 0)
		m_strPath = m_strPath.Left(iPos+1);

	// Locate the SYSTEM directory
	iPos = GetSystemDirectory(m_strSystemDir.GetBuffer(_MAX_PATH), _MAX_PATH);
	m_strSystemDir.ReleaseBuffer();
	if (iPos > 0)
   	{                              
   		if (m_strSystemDir.Right(1) != '\\')
   			m_strSystemDir += '\\';
   	}

    // Reset the font to all be ANSI var.
    CFont fntAnsi;
    fntAnsi.CreateStockObject (ANSI_VAR_FONT);
    CWnd* pwndChild = GetWindow (GW_CHILD);
    while (pwndChild != NULL && IsChild(pwndChild))
    {
        pwndChild->SetFont(&fntAnsi);
        pwndChild = pwndChild->GetWindow(GW_HWNDNEXT);
    }

	// Center the window
	CenterWindow();

	// Now process the current TELEPHON.INI settings and determine which providers
	// are loaded.
	m_foATSP = m_fATSP = TAPI_checkForDevice(IDS_ATSP_REQ1);
	m_foDSSP = m_fDSSP = TAPI_checkForDevice(IDS_DSSP_REQ1);

	// Verify that all the files are there.
	for (int i = IDS_ATSP_REQ1; i <= IDS_ATSP_REQ2; i++)
	{
		if (!CheckForFile(i))
		{
			m_strATSPOk = GetString(IDS_MISSINGFILES);
			GetDlgItem (IDC_SP_ATSP)->EnableWindow (FALSE);
			break;
		}
	}

	for (i = IDS_DSSP_REQ1; i <= IDS_DSSP_REQ2; i++)
	{
		if (!CheckForFile(i))
		{
			m_strDSSPOk = GetString(IDS_MISSINGFILES);
			GetDlgItem (IDC_SP_DSSP)->EnableWindow (FALSE);
			break;
		}
	}

	// Now make sure TAPI is installed and is the correct version.
	if (!TAPI_Installed())
	{
		AfxMessageBox (IDS_NOTAPI);
		GetDlgItem (IDC_SP_ATSP)->EnableWindow (FALSE);
		GetDlgItem (IDC_SP_DSSP)->EnableWindow (FALSE);
	}
	
	if (GetWinFlags() & WF_WINNT)
	{
		AfxMessageBox (IDS_WRONGVERSION);
		GetDlgItem (IDC_SP_ATSP)->EnableWindow (FALSE);
		GetDlgItem (IDC_SP_DSSP)->EnableWindow (FALSE);
	}
	
	// Connect all the controls via DDX
	CDialog::OnInitDialog();

	return TRUE;

}// CInstallDlg::OnInitDialog

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::OnChange
//
// Called when the user clicks either of the checkboxes.
//
void CInstallDlg::OnChange()
{
	UpdateData(TRUE);
	GetDlgItem(IDOK)->EnableWindow ((m_foATSP != m_fATSP || m_foDSSP != m_fDSSP));
		
}// CInstallDlg::OnChange

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::OnOK
//
// Process the OK button
//
void CInstallDlg::OnOK()
{                 
	UpdateData(TRUE);
	
	if (m_foATSP != m_fATSP)
	{
		if (m_fATSP)
			TAPI_addProvider (IDS_ATSP_REQ1);
		else
			TAPI_removeProvider(IDS_ATSP_REQ1);
	}
	
	if (m_foDSSP != m_fDSSP)
	{
		if (m_fDSSP)
			TAPI_addProvider (IDS_DSSP_REQ1);
		else
			TAPI_removeProvider(IDS_DSSP_REQ1);
	}                                             
	
	CDialog::OnOK();

}// CInstallDlg::OnOK

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::GetString
//
// Return a string from our resource table.
//
CString CInstallDlg::GetString(int iResID)
{                                         
	CString strBuff;
	strBuff.LoadString (iResID);
	return strBuff;
	
}// CInstallDlg::GetString

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::CheckForFile
//
// Verifies that all the required files are installed
//
BOOL CInstallDlg::CheckForFile(int iResID)
{   
	OFSTRUCT ofs;

	// Look in the WINDOWS/SYSTEM directory first.
	CString strFile = m_strSystemDir + GetString(iResID);
    if (OpenFile(strFile, &ofs, OF_EXIST) <= 0)
    {
    	// Or working path.
    	strFile = m_strPath + GetString(iResID);
    	if (OpenFile(strFile, &ofs, OF_EXIST) <= 0)
        	return FALSE;
	}
	return TRUE;

}// CInstallDlg::CheckForFile

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::TAPI_Installed
//
// Returns whether TAPI is installed or not (Win3.x or Win95).
//
BOOL CInstallDlg::TAPI_Installed()
{                                  
    // Check to make sure TAPI.DLL is there (16-bit)
	HMODULE hModTAPI  = LoadLibrary(GetString(IDS_TAPIDLL));
	if (hModTAPI)
		FreeLibrary (hModTAPI);	   
    return (hModTAPI != NULL);

}// CInstallDlg::TAPI_Installed

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::TAPI_addProvider
//
// Adds a provider to the system either using TAPI 1.4 APIs or by directly
// modifying the telehon.ini structures under TAPI 1.3.
//
DWORD CInstallDlg::TAPI_addProvider(int iResID)
{                                  
	DWORD dwPpid = (DWORD)-1;

	// Copy the required files into the WINDOWS/SYSTEM directory.
	for (int i = iResID; i < iResID+2; i++)
	{                                 
		// Look in the WINDOWS/SYSTEM directory first.
		OFSTRUCT ofs;
		CString strDestFile = m_strSystemDir + GetString(i);
	    if (OpenFile(strDestFile, &ofs, OF_EXIST) <= 0)
	    {
			CString strSrcFile = m_strPath + GetString(i);
		    if (!CopyFile (strSrcFile, strDestFile))
		    	return (DWORD)-1L;
		}
	}
	// Get a handle to the TAPI dll.  If an entrypoint exists which
	// is called "lineAddProvider", then use it per the new v1.4 (Win95)
	// specs.  It will automatically call the TSPI config entrypoint and
	// load the provider.
	CString strTSP = GetString(iResID);
	HMODULE hModTAPI  = LoadLibrary(GetString(IDS_TAPIDLL));
	if (hModTAPI)
	{
		ADDPROC fpAddProc = (ADDPROC) GetProcAddress(hModTAPI, "LINEADDPROVIDER");
      	if (fpAddProc) // New Win95 way.
      	{                    
        	if ((*fpAddProc)(strTSP, GetSafeHwnd(), &dwPpid) == 0)
         	{
            	if (dwPpid < 0x80000000 && TAPI_checkForDevice(iResID) == FALSE)
            	{
               		FreeLibrary(hModTAPI);
               		return dwPpid;
            	}
			}            	
        }
         
        // Special case the INI file corrupt error                          
		if (dwPpid == LINEERR_INIFILECORRUPT)
		{                                         
			AfxMessageBox (GetString(IDS_INIFILEERR));
       		FreeLibrary(hModTAPI);
       		return (DWORD) -1L;
		}
		else if (dwPpid != (DWORD)-1)
		{
			// Show message about problem dynamically adding device.
			char szErr[255];
			wsprintf (szErr, GetString(IDS_BADINSTALL1), dwPpid);
			AfxMessageBox(szErr);
			FreeLibrary(hModTAPI);
		}
    }

	// We are not running v1.4 or above of TAPI (ie: Win31)
	// Therefore, add the entry by hand -
	// Get the next provider number to use.  If it doesn't exist, exit.
	int iNextId = AfxGetApp()->GetProfileInt(GetString(IDS_PROVIDERS), GetString(IDS_NEXTPPID), -1);
	int iCount = AfxGetApp()->GetProfileInt(GetString(IDS_PROVIDERS), GetString(IDS_NUMPROV), -1);
   	if (iNextId == -1 || iCount == -1)
    	return (DWORD)-1L;

	// Add our new provider entry
	char szTopic[30];
	wsprintf(szTopic, GetString(IDS_PROVID), iCount);
	AfxGetApp()->WriteProfileInt (GetString(IDS_PROVIDERS), szTopic, iNextId);
	wsprintf(szTopic, GetString(IDS_PROVFILE), iCount);
	AfxGetApp()->WriteProfileString (GetString(IDS_PROVIDERS), szTopic, strTSP);

	// Increment the next available provider number and count.
	AfxGetApp()->WriteProfileInt (GetString(IDS_PROVIDERS), GetString(IDS_NEXTPPID), iNextId+1);
	AfxGetApp()->WriteProfileInt (GetString(IDS_PROVIDERS), GetString(IDS_NUMPROV), iCount+1);

	// Now run the configuration for the provider.
	dwPpid = (DWORD) iNextId;
	TAPI_runConfig (dwPpid, iResID);

	return dwPpid;

}// CInstallDlg::TAPI_addProvider

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::TAPI_removeProvider
//
// Remove a provider from the TAPI load list.
//
BOOL CInstallDlg::TAPI_removeProvider(int iResID)
{                                   
	// Locate the PPid of our device.
	char szTopic[30];
	DWORD dwPpid = -1L;                                                                          
	int iProvPos = 0;
	int iLen = AfxGetApp()->GetProfileInt(GetString(IDS_PROVIDERS), GetString(IDS_NEXTPPID), -1);
	if (iLen >= 0)
	{    
	    CString strLookingFor = GetString (iResID);
		for (int i = 0; i < iLen; i++)
		{
			wsprintf(szTopic, GetString(IDS_PROVFILE), i);
			CString strProvider = AfxGetApp()->GetProfileString(GetString(IDS_PROVIDERS), szTopic);
			if (!strProvider.CompareNoCase(strLookingFor))
			{                                                                          
				wsprintf(szTopic, GetString(IDS_PROVID), i);
				dwPpid = (DWORD) AfxGetApp()->GetProfileInt (GetString(IDS_PROVIDERS), szTopic, -1);
				iProvPos = i;
				break;
			}
		}
	}        
	
	// Not found?
	if (dwPpid == -1)
		return FALSE;

	// Get a handle to the TAPI dll.  If an entrypoint exists which
	// is called "lineRemoveProvider", then use it per the new v1.4 (Win95)
	// specs.
	HMODULE hModTAPI  = LoadLibrary(GetString(IDS_TAPIDLL));
	if (hModTAPI)
	{
		REMPROC fpRemProc = (REMPROC) GetProcAddress(hModTAPI, "LINEREMOVEPROVIDER");
      	if (fpRemProc) // New Win95 way.
      	{                    
        	if ((*fpRemProc)(dwPpid, GetSafeHwnd()) == 0)
         	{
           		FreeLibrary(hModTAPI);
           		return TRUE;
			}            	
        }
        
		// Show message about problem dynamically adding device.
		AfxMessageBox(IDS_BADREMOVE);
		FreeLibrary(hModTAPI);
    }

	// We are not running v1.4 or above of TAPI (ie: Win31)
	// Therefore, remove the entry by hand - decrement the count
	int iCount = AfxGetApp()->GetProfileInt(GetString(IDS_PROVIDERS), GetString(IDS_NUMPROV), -1);
	if (iCount != -1)
		AfxGetApp()->WriteProfileInt (GetString(IDS_PROVIDERS), GetString(IDS_NUMPROV), iCount-1);

	// Delete the strings associated with our provider
	wsprintf(szTopic, GetString(IDS_PROVFILE), iProvPos);
	AfxGetApp()->WriteProfileString (GetString(IDS_PROVIDERS), szTopic, NULL);
	wsprintf(szTopic, GetString(IDS_PROVID), iProvPos);
	AfxGetApp()->WriteProfileString (GetString(IDS_PROVIDERS), szTopic, NULL);

	// Give the providerRemove entrypoint a chance to run.
	HMODULE  hModule = LoadLibrary (GetString (iResID));
	if (hModule != NULL)
	{
   		INITPROC providerRemove = (INITPROC) GetProcAddress(hModule, MAKEINTRESOURCE(TSPI_PROVIDERREMOVE));
		if (providerRemove)
			(*providerRemove)(GetSafeHwnd(), dwPpid);
		FreeLibrary(hModule);
    }

	return TRUE;

}// CInstallDlg::TAPI_removeProvider

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::TAPI_checkForDevice
//
// Determines if a particular TSP will be loaded by TAPI by examining the
// telephon.ini.
//
BOOL CInstallDlg::TAPI_checkForDevice(int iResID)
{                                   
	// Locate our TSP filename in the telephon.ini
	int iLen = AfxGetApp()->GetProfileInt(GetString(IDS_PROVIDERS), GetString(IDS_NEXTPPID), -1);
	if (iLen >= 0)
	{    
	    CString strLookingFor = GetString (iResID);
		for (int i = 0; i < iLen; i++)
		{
			char szTopic[30];
			wsprintf(szTopic, GetString(IDS_PROVFILE), i);
			CString strProvider = AfxGetApp()->GetProfileString(GetString(IDS_PROVIDERS), szTopic);
			if (!strProvider.CompareNoCase(strLookingFor))
				return TRUE;
		}
	}
	return FALSE;		

}// CInstallDlg::TAPI_checkForDevice

/////////////////////////////////////////////////////////////////////////////
// CInstallDlg::TAPI_runConfig
//
// Runs the providerInstall entrypoint in the TSP.
//
BOOL CInstallDlg::TAPI_runConfig (DWORD dwPpid, int iResID)
{   
	// Load the TSP into memory                               
	HMODULE  hModule = LoadLibrary (GetString (iResID));
	if (hModule != NULL)
	{
   		INITPROC providerConfig = (INITPROC) GetProcAddress(hModule, MAKEINTRESOURCE(TSPI_PROVIDERINSTALL));
		if (providerConfig == NULL)
			providerConfig = (INITPROC) GetProcAddress(hModule, MAKEINTRESOURCE(TSPI_PROVIDERCONFIG));
		if (providerConfig)
			(*providerConfig)(GetSafeHwnd(), dwPpid);
		FreeLibrary(hModule);
		return TRUE;
    }

    AfxMessageBox(IDS_BADINSTALL2);
    return FALSE;
    
}// CInstallDlg::TAPI_runConfig
 
