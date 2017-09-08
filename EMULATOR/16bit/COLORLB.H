//////////////////////////////////////////////////////////////////////////////
// Colored listbox (shows each item in different colors).
//
class CColorLB : public CListBox
{
// New methods
public:
   int AddString(char cType, LPCSTR lpsz);
// Overrides
protected:
   //{{AFX_MSG(CColorLB)
   virtual void DrawItem(LPDRAWITEMSTRUCT lpdi);
   virtual void MeasureItem(LPMEASUREITEMSTRUCT lpmi);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};


