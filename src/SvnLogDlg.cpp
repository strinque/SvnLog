#include "pch.h"
#include "framework.h"
#include "SvnLog.h"
#include "SvnLogDlg.h"
#include "afxdialogex.h"

// include flat design manifest
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// convert CString to string
const std::string to_string(const CString& str)
{
  return CStringA(str).GetString();
}

// convert string to CString
const CString to_cstring(const std::string& str)
{
  return CString(str.c_str());
}

// CListCtrl headers id
enum HEADER {
  HEADER_DATE = 0,
  HEADER_AUTHOR,
  HEADER_PROJECT,
  HEADER_REPOS,
  HEADER_REVISION
};

CSvnLogDlg::CSvnLogDlg(CWnd* pParent)
  : CDialog(IDD_SVNLOG_DIALOG, pParent),
  m_hIcon(),
  m_rcDlg(),
  m_editColor(),
  m_path_ctrl(WM_BROWSE_CTRL_UPDATE),
  m_path(),
  m_list_ctrl(),
  m_date_from(),
  m_date_to(),
  m_combo_project(),
  m_combo_author(),
  m_edit(),
  m_progress(),
  m_commits(),
  m_repos(),
  m_thread(),
  m_mutex(),
  m_cv(),
  m_task(Task::Undefined),
  m_locked(false)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  // start thread
  m_thread = std::thread(&CSvnLogDlg::Run, this);
}

CSvnLogDlg::~CSvnLogDlg()
{
  // wait for thread termination properly
  Notify(Task::Terminate);
  if (m_thread.joinable())
    m_thread.join();
}

void CSvnLogDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_MFCEDITBROWSE, m_path_ctrl);
  DDX_Control(pDX, IDC_LIST, m_list_ctrl);
  DDX_Control(pDX, IDC_DATETIMEPICKER_FROM, m_date_from);
  DDX_Control(pDX, IDC_DATETIMEPICKER_TO, m_date_to);
  DDX_Control(pDX, IDC_COMBO_PROJECT, m_combo_project);
  DDX_Control(pDX, IDC_COMBO_AUTHOR, m_combo_author);
  DDX_Text(pDX, IDC_EDIT, m_edit);
  DDX_Control(pDX, IDC_PROGRESS, m_progress);
}

#pragma warning(push)
#pragma warning(disable: 26454)
BEGIN_MESSAGE_MAP(CSvnLogDlg, CDialog)
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_WM_GETMINMAXINFO()
  ON_WM_CTLCOLOR()
  ON_WM_SETCURSOR()
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, &CSvnLogDlg::OnItemchangedList)
  ON_NOTIFY(NM_CLICK, IDC_LIST, &CSvnLogDlg::OnItemClickList)
  ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST, &CSvnLogDlg::OnGetInfoList)
  ON_BN_CLICKED(IDC_BUTTON_REFRESH, &CSvnLogDlg::OnBnClickedButtonRefresh)
  ON_BN_CLICKED(IDC_CHECK_FROM, &CSvnLogDlg::OnBnClickedCheckFrom)
  ON_BN_CLICKED(IDC_CHECK_TO, &CSvnLogDlg::OnBnClickedCheckTo)
  ON_BN_CLICKED(IDC_CHECK_PROJECT, &CSvnLogDlg::OnBnClickedCheckProject)
  ON_BN_CLICKED(IDC_CHECK_AUTHOR, &CSvnLogDlg::OnBnClickedCheckAuthor)
  ON_CBN_SELENDOK(IDC_COMBO_AUTHOR, &CSvnLogDlg::OnComboAuthorChanged)
  ON_CBN_SELENDOK(IDC_COMBO_PROJECT, &CSvnLogDlg::OnComboProjectChanged)
  ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_FROM, &CSvnLogDlg::OnDateFromChanged)
  ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_TO, &CSvnLogDlg::OnDateToChanged)
  ON_REGISTERED_MESSAGE(WM_BROWSE_CTRL_UPDATE, &CSvnLogDlg::OnPathChanged)
  ON_REGISTERED_MESSAGE(WM_UPDATE_PROGRESS_BAR, &CSvnLogDlg::OnUpdateProgress)
  ON_REGISTERED_MESSAGE(WM_POST_MESSAGE_QUEUE, &CSvnLogDlg::OnGetPostMessage)
END_MESSAGE_MAP()
#pragma warning(pop)

BOOL CSvnLogDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // set the icon for this dialog
  SetIcon(m_hIcon, TRUE);   // set big icon
  SetIcon(m_hIcon, FALSE);  // set small icon

  // configure list control
  const std::vector<struct header_info> headers = {
    {HEADER_DATE, L"Date", LVCFMT_CENTER, 125},
    {HEADER_AUTHOR, L"Author", LVCFMT_CENTER, 120},
    {HEADER_PROJECT, L"Project Path", LVCFMT_LEFT, LVSCW_AUTOSIZE},
    {HEADER_REPOS, L"Repository Url", LVCFMT_LEFT, 500},
    {HEADER_REVISION, L"Revision", LVCFMT_RIGHT, 55}
  };
  m_list_ctrl.SetHeaders(headers);

  // set edit color
  m_editColor.CreateSolidBrush(RGB(255, 255, 255));

  // calculate minimum size
  GetWindowRect(&m_rcDlg);
  m_rcDlg -= m_rcDlg.TopLeft();

  // set refresh button icon
  static_cast<CButton*>(GetDlgItem(IDC_BUTTON_REFRESH))->SetIcon(static_cast<HICON>(
    LoadImage(AfxGetApp()->m_hInstance,
      MAKEINTRESOURCE(IDI_ICON_REFRESH),
      (WPARAM)IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR)));

  // lock the mfc-controls and load the svn-repository
  LockCtrl({ IDC_MFCEDITBROWSE, IDC_BUTTON_REFRESH });
  Notify(Task::Load);

  // add min/close buttons and resizable windows
  SetWindowLong(this->m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) | WS_OVERLAPPEDWINDOW);
  return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSvnLogDlg::OnPaint()
{
  if (IsIconic())
  {
    CPaintDC dc(this); // device context for painting

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // center icon in client rectangle
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // draw the icon
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialog::OnPaint();
  }
}

HCURSOR CSvnLogDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

void CSvnLogDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
  // limits the size to the frame
  lpMMI->ptMinTrackSize.x = m_rcDlg.Width();
  lpMMI->ptMinTrackSize.y = m_rcDlg.Height();
  CDialog::OnGetMinMaxInfo(lpMMI);
}

HBRUSH CSvnLogDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
  HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
  switch (pWnd->GetDlgCtrlID())
  {
  case IDC_EDIT:
    pDC->SetTextColor(RGB(0, 0, 0));
    pDC->SetBkColor(RGB(255, 255, 255));
    hbr = static_cast<HBRUSH>(m_editColor.GetSafeHandle());
    break;
  default:
    break;
  }
  return hbr;
}

BOOL CSvnLogDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
  if (m_locked)
  {
    ::SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
    return TRUE;
  }
  return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

void CSvnLogDlg::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  *pResult = 0;

  if (pNMLV && (pNMLV->uNewState & LVIS_SELECTED))
  {
    // set commit log infos on control
    const CString& project = m_list_ctrl.GetItemText(pNMLV->iItem, HEADER_PROJECT);
    const CString& revision = m_list_ctrl.GetItemText(pNMLV->iItem, HEADER_REVISION);
    const auto& commit = m_repos.get_commit(to_string(project),
                                            std::atoi(to_string(revision).c_str()));
    m_edit = commit->desc.c_str();
    UpdateData(FALSE);
  }
}

void CSvnLogDlg::OnItemClickList(NMHDR* pNMHDR, LRESULT* pResult)
{
  LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
  *pResult = 0;

  // display log infos when url is selected
  if ((pNMItemActivate->iItem != -1) &&
      (pNMItemActivate->iSubItem == HEADER_REPOS))
  {
    const CString& project = m_list_ctrl.GetItemText(pNMItemActivate->iItem, HEADER_PROJECT);
    const CString& revision = m_list_ctrl.GetItemText(pNMItemActivate->iItem, HEADER_REVISION);
    m_repos.show_logs(to_string(project),
                      std::atoi(to_string(revision).c_str()));
  }
}

// this function is called when the virtual list needs data
void CSvnLogDlg::OnGetInfoList(NMHDR* pNMHDR, LRESULT* pResult)
{
  LV_DISPINFO* pDispInfo = reinterpret_cast<LV_DISPINFO*>(pNMHDR);
  *pResult = 0;

  // update the list if needed
  LV_ITEM* item = &(pDispInfo)->item;
  if (item->mask & LVIF_TEXT)
  {
    const auto& commit = m_commits.get_commit(item->iItem);
    switch (item->iSubItem)
    {
    case HEADER_DATE:
      static_cast<void>(lstrcpyn(item->pszText, to_cstring(commit->date_str), item->cchTextMax));
      break;
    case HEADER_AUTHOR:
      static_cast<void>(lstrcpyn(item->pszText, to_cstring(commit->author), item->cchTextMax));
      break;
    case HEADER_PROJECT:
      static_cast<void>(lstrcpyn(item->pszText, to_cstring(commit->repos), item->cchTextMax));
      break;
    case HEADER_REPOS:
      static_cast<void>(lstrcpyn(item->pszText, to_cstring(commit->url), item->cchTextMax));
      break;
    case HEADER_REVISION:
      static_cast<void>(lstrcpyn(item->pszText, to_cstring(commit->id_str), item->cchTextMax));
      break;
    default:
      break;
    }
  }
}

void CSvnLogDlg::OnBnClickedButtonRefresh()
{
  OnPathChanged(0, 0);
}

void CSvnLogDlg::OnBnClickedCheckFrom()
{
  const bool is_checked = (IsDlgButtonChecked(IDC_CHECK_FROM) == BST_CHECKED);
  if (is_checked)
  {
    SYSTEMTIME lp;
    m_date_from.GetTime(&lp);
    m_commits.enable_filter_from(lp);
  }
  else
    m_commits.disable_filter_from();
  m_date_from.EnableWindow(is_checked);
  UpdateGui();
}

void CSvnLogDlg::OnBnClickedCheckTo()
{
  const bool is_checked = (IsDlgButtonChecked(IDC_CHECK_TO) == BST_CHECKED);
  if (is_checked)
  {
    SYSTEMTIME lp;
    m_date_to.GetTime(&lp);
    m_commits.enable_filter_to(lp);
  }
  else
    m_commits.disable_filter_to();
  m_date_to.EnableWindow(is_checked);
  UpdateGui();
}

void CSvnLogDlg::OnBnClickedCheckProject()
{
  const bool is_checked = (IsDlgButtonChecked(IDC_CHECK_PROJECT) == BST_CHECKED);
  if (!is_checked)
  {
    m_commits.disable_filter_project();
    m_combo_project.SetCurSel(-1);
  }
  m_combo_project.EnableWindow(is_checked);
  UpdateGui();
}

void CSvnLogDlg::OnBnClickedCheckAuthor()
{
  const bool is_checked = (IsDlgButtonChecked(IDC_CHECK_AUTHOR) == BST_CHECKED);
  if (!is_checked)
  {
    m_commits.disable_filter_author();
    m_combo_author.SetCurSel(-1);
  }
  m_combo_author.EnableWindow(is_checked);
  UpdateGui();
}

void CSvnLogDlg::OnComboAuthorChanged()
{
  if (m_combo_author.GetCurSel() != -1)
  {
    CString str;
    m_combo_author.GetLBText(m_combo_author.GetCurSel(), str);
    m_commits.enable_filter_author(to_string(str));
    UpdateGui();
  }
}

void CSvnLogDlg::OnComboProjectChanged()
{
  if (m_combo_project.GetCurSel() != -1)
  {
    CString str;
    m_combo_project.GetLBText(m_combo_project.GetCurSel(), str);
    m_commits.enable_filter_project(to_string(str));
    UpdateGui();
  }
}

void CSvnLogDlg::OnDateFromChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
  LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
  *pResult = 0;
  m_commits.enable_filter_from(pDTChange->st);
  UpdateGui();
}

void CSvnLogDlg::OnDateToChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
  LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
  *pResult = 0;
  m_commits.enable_filter_to(pDTChange->st);
  UpdateGui();
}

LRESULT CSvnLogDlg::OnPathChanged(WPARAM wparam, LPARAM lparam)
{
  // unused arguments
  static_cast<void>(wparam);
  static_cast<void>(lparam);

  // retrieve path from control
  m_path_ctrl.GetWindowText(m_path);

  // notify thread to begin scan
  LockCtrl();
  Notify(Task::Scan);
  return 0;
}

LRESULT CSvnLogDlg::OnUpdateProgress(WPARAM wparam, LPARAM lparam)
{
  m_progress.SetPos(static_cast<int>(wparam));
  return 0;
}

LRESULT CSvnLogDlg::OnGetPostMessage(WPARAM wparam, LPARAM lparam)
{
  const bool result = static_cast<bool>(wparam);
  switch (m_task)
  {
  case Task::Load:
    if (result)
    {
      UnlockCtrl();
      UpdateGui(true);
    }
    else
      UnlockCtrl({ IDC_MFCEDITBROWSE });
    break;

  case Task::Save:
    break;

  case Task::Scan:
    if (result)
      Notify(Task::GetLogs);
    else
      UnlockCtrl();
    break;

  case Task::GetLogs:
    if (result)
      Notify(Task::Save);
    UnlockCtrl();
    UpdateGui(true);
    break;

  default:
    break;
  }
  return 0;
}

void CSvnLogDlg::Run()
{
  volatile bool terminate = false;
  std::unique_lock<std::mutex> lck(m_mutex);
  while (!terminate)
  {
    // wait for task
    m_cv.wait(lck);

    // execute request
    bool result = false;
    switch (m_task)
    {
    case Task::Load:
      {
        std::string path;
        result = m_repos.load(path);
        m_path = to_cstring(path);
      }
      break;

    case Task::Save:
      result = m_repos.save(to_string(m_path));
      break;

    case Task::Scan:
      result = m_repos.scan_directory(to_string(m_path));
      break;
    
    case Task::GetLogs:
    {
      // create callback to update progress-bar
      std::function<void(std::size_t, std::size_t)> update_cb = [this](std::size_t idx, std::size_t total)
      {
        static std::size_t old_val = -1;
        const std::size_t val = total ? static_cast<std::size_t>(idx * 100 / total) : 0;
        if (val != old_val)
        {
          ::PostMessage(m_hWnd, WM_UPDATE_PROGRESS_BAR, static_cast<WPARAM>(val), 0);
          old_val = val;
        }
      };
      result = m_repos.get_logs(update_cb);
      break;
    }

    case Task::Terminate:
      terminate = true;
      break;

    default:
      break;
    }

    // update gui and state-machine
    if (!terminate)
      ::PostMessage(m_hWnd, WM_POST_MESSAGE_QUEUE, static_cast<WPARAM>(result), 0);
  }
}

void CSvnLogDlg::Notify(const enum class Task task)
{
  // update state
  {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_task = task;
  }

  // notify thread to execute task
  m_cv.notify_one();
}

void CSvnLogDlg::OnOK()
{
  OnPathChanged(0, 0);
}

void CSvnLogDlg::LockCtrl(const std::vector<uint32_t>& handles)
{
  m_locked = true;
  if (handles.empty())
  {
    GetDlgItem(IDC_MFCEDITBROWSE)->EnableWindow(FALSE);
    GetDlgItem(IDC_BUTTON_REFRESH)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK_FROM)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK_TO)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK_PROJECT)->EnableWindow(FALSE);
    GetDlgItem(IDC_CHECK_AUTHOR)->EnableWindow(FALSE);
    GetDlgItem(IDC_LIST)->EnableWindow(FALSE);
    GetDlgItem(IDC_EDIT)->EnableWindow(FALSE);
    GetDlgItem(IDC_PROGRESS)->EnableWindow(FALSE);
  }
  else
  {
    for (const auto& i : handles)
      GetDlgItem(i)->EnableWindow(FALSE);
  }
}

void CSvnLogDlg::UnlockCtrl(const std::vector<uint32_t>& handles)
{
  m_locked = false;
  if (handles.empty())
  {
    GetDlgItem(IDC_MFCEDITBROWSE)->EnableWindow(TRUE);
    GetDlgItem(IDC_BUTTON_REFRESH)->EnableWindow(TRUE);
    GetDlgItem(IDC_CHECK_FROM)->EnableWindow(TRUE);
    GetDlgItem(IDC_CHECK_TO)->EnableWindow(TRUE);
    GetDlgItem(IDC_CHECK_PROJECT)->EnableWindow(TRUE);
    GetDlgItem(IDC_CHECK_AUTHOR)->EnableWindow(TRUE);
    GetDlgItem(IDC_LIST)->EnableWindow(TRUE);
    GetDlgItem(IDC_EDIT)->EnableWindow(TRUE);
    GetDlgItem(IDC_PROGRESS)->EnableWindow(TRUE);
  }
  else
  {
    for (const auto& i : handles)
      GetDlgItem(i)->EnableWindow(TRUE);
  }
}

void CSvnLogDlg::UpdateGui(const bool update_commits)
{
  // sort and filter commits - disable redraw CListCtrl between updates
  m_list_ctrl.SetRedraw(false);
  m_list_ctrl.DeleteAllItems();
  if (update_commits)
  {
    // sort commits
    m_commits.set_commits(m_repos.get_commits());

    // reset project combobox with all available projects
    m_combo_project.ResetContent();
    for (const auto& p : m_commits.get_projects())
      m_combo_project.AddString(to_cstring(p));

    // reset author combobox with all available authors
    m_combo_author.ResetContent();
    for (const auto& a : m_commits.get_authors())
      m_combo_author.AddString(to_cstring(a));
  }
  m_commits.filter_commits();
  m_list_ctrl.SetItemCount(static_cast<int>(m_commits.get_commits().size()));
  m_list_ctrl.ClearItemSelection();
  m_list_ctrl.SetRedraw(true);

  // update gui
  m_edit.Empty();
  m_path_ctrl.SetText(m_path);
  m_progress.SetPos(0);
  UpdateData(FALSE);
}