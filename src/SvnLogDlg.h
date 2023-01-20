#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "CAutoListCtrl.h"
#include "CBrowseCtrl.h"
#include "SvnRepos.h"
#include "FilteredCommits.h"

// windows message exchanged between thread and gui
const UINT WM_POST_MESSAGE_QUEUE = RegisterWindowMessage(L"MessagePosted");
const UINT WM_UPDATE_PROGRESS_BAR = RegisterWindowMessage(L"UpdateProgressBar");
const UINT WM_BROWSE_CTRL_UPDATE = RegisterWindowMessage(L"FolderSelectionChanged");

// task that can be executed in thread
enum class Task {
  Undefined,
  Load,
  Save,
  Scan,
  GetLogs,
  Terminate
};

class CSvnLogDlg : public CDialog
{
// Construction
public:
  // constructor/destructor
  CSvnLogDlg(CWnd* pParent = nullptr);
  ~CSvnLogDlg();

protected:
  // generated message map functions
  virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support
  virtual BOOL OnInitDialog();
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
  afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
  afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
  afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnItemClickList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnGetInfoList(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnBnClickedButtonRefresh();
  afx_msg void OnBnClickedCheckFrom();
  afx_msg void OnBnClickedCheckTo();
  afx_msg void OnBnClickedCheckProject();
  afx_msg void OnBnClickedCheckAuthor();
  afx_msg void OnComboAuthorChanged();
  afx_msg void OnComboProjectChanged();
  afx_msg void OnDateFromChanged(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg void OnDateToChanged(NMHDR* pNMHDR, LRESULT* pResult);
  afx_msg LRESULT OnPathChanged(WPARAM wparam, LPARAM lparam);
  afx_msg LRESULT OnUpdateProgress(WPARAM wparam, LPARAM lparam);
  afx_msg LRESULT OnGetPostMessage(WPARAM wparam, LPARAM lparam);
  DECLARE_MESSAGE_MAP()

private:
  // run the thread
  void Run();
  void Notify(const enum class Task task);

  // virtual functions
  void OnOK() override;

  // lock/unlock the interface
  void LockCtrl(const std::vector<uint32_t>& handles = {});
  void UnlockCtrl(const std::vector<uint32_t>& handles = {});

  // update mfc controls
  void UpdateGui(const bool update_commits = false);

private:
  // mfc gui parameters
  HICON m_hIcon;
  CRect m_rcDlg;
  CBrush m_editColor;

  // mfc controls
  CBrowseCtrl m_path_ctrl;
  CString m_path;
  CAutoListCtrl m_list_ctrl;
  CDateTimeCtrl m_date_from;
  CDateTimeCtrl m_date_to;
  CComboBox m_combo_project;
  CComboBox m_combo_author;
  CString m_edit;
  CProgressCtrl m_progress;

  // store, sort and filter commits
  FilteredCommits m_commits;

  // svn repository functions
  SvnRepos m_repos;

  // thread to not lock the gui
  std::thread m_thread;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  enum class Task m_task;
  bool m_locked;
};