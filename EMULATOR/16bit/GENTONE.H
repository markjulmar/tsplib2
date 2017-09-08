//*****************************************************************************
//
// GENTONE.H
//
// Tone Generation dialog
//
// Change History:
// ----------------------------------------------------------------------
// 05/10/96 Mark Smith	(MCS)	Initial revision.
//
//*****************************************************************************

class CGenTone : public CDialog
{
// Construction
public:
	CGenTone(CWnd* pParent, CAddressAppearance* pAddr);

// Dialog Data
	//{{AFX_DATA(CGenTone)
	enum { IDD = IDD_GENTONE };
	CListBox	m_lbToneList;
	DWORD	m_dwFreq1;
	DWORD	m_dwFreq2;
	DWORD	m_dwFreq3;
	//}}AFX_DATA
	CAddressAppearance* m_pAddr;

// Overrides
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CGenTone)
	afx_msg void OnPredefined();
	afx_msg void OnUserdefined();
	virtual BOOL OnInitDialog();
	afx_msg void OnToneListSelchange();
	afx_msg void OnGenerate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
