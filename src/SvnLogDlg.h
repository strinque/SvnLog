#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "SvnRepos.h"

// windows message exchanged between thread and gui
#define WM_POSTMESSAGE      WM_USER + 1
#define WM_UPDATE_PROGRESS  WM_USER + 2

// result of task execution
enum result {
  UNDEFINED,
  LOAD_FAILURE,
  LOAD_SUCCESS,
  SAVE_FAILURE,
  SAVE_SUCCESS,
  SCAN_FAILURE,
  SCAN_SUCCESS,
  GET_LOGS_FAILURE,
  GET_LOGS_SUCCESS
};

// task that can be executed in thread
enum tasks {
  UNDEF,
  LOAD,
  SAVE,
  SCAN,
  GET_LOGS,
  TERMINATE
};

class CAutoListCtrl final : public CListCtrl
{
public:
  void AddColumn(const CString& text, uint16_t flag, int size=-1);
  void Validate();

protected:
  afx_msg void OnSize(UINT nType, int cx, int cy);
  DECLARE_MESSAGE_MAP()

private:
  int GetWidth() const;

private:
  std::vector<int> m_size;
};

class CSvnLogDlg : public CDialog
{
// Construction
public:
  // constructor/destructor
  CSvnLogDlg(CWnd* pParent = nullptr);
  virtual ~CSvnLogDlg();

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
  afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg LRESULT OnUpdateProgress(WPARAM wparam, LPARAM lparam);
  afx_msg LRESULT OnGetPostMessage(WPARAM wparam, LPARAM lparam);
  DECLARE_MESSAGE_MAP()

private:
  // run the thread
  void Run();
  void Notify(const enum tasks task);

  // lock/unlock the interface
  void LockCtrl(const uint32_t id=0);
  void UnlockCtrl(const uint32_t id=0);

  // update mfc controls
  void UpdateListCtrl();

private:
  // mfc gui parameters
  HICON m_hIcon;
  CRect m_rcDlg;
  CBrush m_editColor;

  // mfc controls
  CMFCEditBrowseCtrl m_path_ctrl;
  std::wstring m_path;
  CAutoListCtrl m_list_ctrl;
  CString m_edit;
  CProgressCtrl m_progress;

  // svn repository functions
  SvnRepos m_repos;

  // thread
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  enum tasks m_task;
};
