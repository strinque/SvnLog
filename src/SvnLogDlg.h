#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "SvnRepos.h"

// windows message exchanged between thread and gui
#define WM_POSTMESSAGE      WM_USER + 1

// task that can be executed in thread
enum tasks {
  UNDEF,
  SCAN,
  TERMINATE
};

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
  afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg void OnPathChange();
  afx_msg LRESULT OnGetPostMessage(WPARAM wparam, LPARAM lparam);
  DECLARE_MESSAGE_MAP()

private:
  // run the thread
  void Run();
  void Notify(const enum tasks task);

private:
  // mfc gui parameters
  HICON m_hIcon;
  CRect m_rcDlg;
  CBrush m_editColor;

  // mfc controls
  CMFCEditBrowseCtrl m_path_ctrl;
  std::wstring m_path;

  // svn repository functions
  SvnRepos m_repos;

  // thread
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  enum tasks m_task;
};
