#pragma once

class CSvnLogDlg : public CDialog
{
// Construction
public:
  // constructor/destructor
  CSvnLogDlg(CWnd* pParent = nullptr);
  ~CSvnLogDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
  enum { IDD = IDD_SVNLOG_DIALOG };
#endif

protected:
  // generated message map functions
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support
  virtual BOOL OnInitDialog();
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  DECLARE_MESSAGE_MAP()

private:
  // mfc gui parameters
  HICON m_hIcon;
};
