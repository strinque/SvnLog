#include "pch.h"
#include "framework.h"
#include "SvnLog.h"
#include "SvnLogDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// include flat design manifest
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' \
version='6.0.0.0' \
processorArchitecture='*' \
publicKeyToken='6595b64144ccf1df' \
language='*'\"") 

// size in pixel of list-control headers
#define LIST_CTRL_HEADER_DATE       135
#define LIST_CTRL_HEADER_AUTHOR     175
#define LIST_CTRL_HEADER_ID         70

void CAutoListCtrl::AddColumn(const CString& text, uint16_t flag, int size)
{
  // add empty column (to allow the center flag of the first header)
  if (m_size.empty())
    InsertColumn(0, _T(""), 0);

  // add the column without size and store it
  InsertColumn(static_cast<int>(m_size.size()+1), text, flag, size);
  m_size.push_back(size);
}

void CAutoListCtrl::Validate()
{
  if (m_size.empty())
    return;
  DeleteColumn(0);
  for (int i = 0; i < m_size.size(); ++i)
    SetColumnWidth(i, (m_size[i] == LVSCW_AUTOSIZE) ? GetWidth() : static_cast<int>(m_size[i]));
}

BEGIN_MESSAGE_MAP(CAutoListCtrl, CListCtrl)
  ON_WM_SIZE()
END_MESSAGE_MAP()

void CAutoListCtrl::OnSize(UINT nType, int cx, int cy)
{
  CListCtrl::OnSize(nType, cx, cy);
  for (int i = 0; i < m_size.size(); ++i)
    if (m_size[i] == LVSCW_AUTOSIZE)
      SetColumnWidth(i, GetWidth());
}

int CAutoListCtrl::GetWidth() const
{
  int width;
  CRect rc;
  GetClientRect(&rc);
  width = rc.Width();
  for (auto& s : m_size)
    width -= (s != LVSCW_AUTOSIZE) ? s : 0;
  return width;
}

CSvnLogDlg::CSvnLogDlg(CWnd* pParent /*=nullptr*/)
  : CDialog(IDD_SVNLOG_DIALOG, pParent),
  m_hIcon(),
  m_rcDlg(),
  m_editColor(),
  m_path_ctrl(),
  m_path(),
  m_list_ctrl(),
  m_edit(),
  m_progress(),
  m_repos(),
  m_thread(),
  m_mutex(),
  m_cv(),
  m_task(UNDEF)
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

  // start thread
  m_thread = std::thread(&CSvnLogDlg::Run, this);
}

CSvnLogDlg::~CSvnLogDlg()
{
  // wait for thread termination properly
  if (m_thread.joinable())
  {
    Notify(TERMINATE);
    m_thread.join();
  }
}

void CSvnLogDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_MFCEDITBROWSE, m_path_ctrl);
  DDX_Control(pDX, IDC_LIST, m_list_ctrl);
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
  ON_EN_CHANGE(IDC_MFCEDITBROWSE, &CSvnLogDlg::OnPathChange)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, &CSvnLogDlg::OnItemchangedList)
  ON_MESSAGE(WM_UPDATE_PROGRESS, &CSvnLogDlg::OnUpdateProgress)
  ON_MESSAGE(WM_POSTMESSAGE, &CSvnLogDlg::OnGetPostMessage)
END_MESSAGE_MAP()
#pragma warning(pop)

BOOL CSvnLogDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // set the icon for this dialog
  SetIcon(m_hIcon, TRUE);   // set big icon
  SetIcon(m_hIcon, FALSE);  // set small icon

  // configure list control
  m_list_ctrl.SetExtendedStyle(m_list_ctrl.GetExtendedStyle() | LVS_EX_FULLROWSELECT);
  m_list_ctrl.AddColumn(_T("Date"), LVCFMT_CENTER, LIST_CTRL_HEADER_DATE);
  m_list_ctrl.AddColumn(_T("Author"), LVCFMT_CENTER, LIST_CTRL_HEADER_AUTHOR);
  m_list_ctrl.AddColumn(_T("Svn Repository"), LVCFMT_LEFT, LVSCW_AUTOSIZE);
  m_list_ctrl.AddColumn(_T("Commit ID"), LVCFMT_CENTER, LIST_CTRL_HEADER_ID);
  m_list_ctrl.Validate();

  // set edit color
  m_editColor.CreateSolidBrush(RGB(255, 255, 255));

  // calculate minimum size
  GetWindowRect(&m_rcDlg);
  m_rcDlg -= m_rcDlg.TopLeft();

  // lock the mfc-controls and load the svn-repository
  LockCtrl(IDC_MFCEDITBROWSE);
  m_progress.ShowWindow(SW_SHOW);
  Notify(LOAD);

  // add min/close buttons
  SetWindowLong(this->m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
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

void CSvnLogDlg::OnPathChange()
{
  // retrieve path from control
  CString str;
  m_path_ctrl.GetWindowTextW(str);
  {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_path = str.GetString();
  }
  
  // notify thread to begin scan
  LockCtrl(IDC_MFCEDITBROWSE);
  Notify(SCAN);
}

void CSvnLogDlg::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
{
  LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
  *pResult = 0;
  if (pNMLV && (pNMLV->uNewState & LVIS_SELECTED))
  {
    USES_CONVERSION;
    m_edit = CString(m_repos.get_commit(W2CA(m_list_ctrl.GetItemText(pNMLV->iItem, 2)), 
                                        _ttoi(m_list_ctrl.GetItemText(pNMLV->iItem, 3)))->desc.c_str());
    UpdateData(FALSE);
  }
}

LRESULT CSvnLogDlg::OnUpdateProgress(WPARAM wparam, LPARAM lparam)
{
  m_progress.SetPos(static_cast<int>(wparam));
  return 0;
}

LRESULT CSvnLogDlg::OnGetPostMessage(WPARAM wparam, LPARAM lparam)
{
  const enum result res = static_cast<enum result>(wparam);
  switch (res)
  {
  case LOAD_FAILURE:
  case LOAD_SUCCESS:
    UnlockCtrl(IDC_MFCEDITBROWSE);
    UpdateListCtrl();
    break;
  case SAVE_FAILURE:
  case SAVE_SUCCESS:
    break;
  case SCAN_FAILURE:
    UnlockCtrl(IDC_MFCEDITBROWSE);
    break;
  case SCAN_SUCCESS:
    Notify(GET_LOGS);
    break;
  case GET_LOGS_FAILURE:
    UnlockCtrl(IDC_MFCEDITBROWSE);
    m_progress.ShowWindow(SW_HIDE);
    m_list_ctrl.DeleteAllItems();
    break;
  case GET_LOGS_SUCCESS:
    UnlockCtrl(IDC_MFCEDITBROWSE);
    m_progress.ShowWindow(SW_HIDE);
    UpdateListCtrl();
    Notify(SAVE);
    break;
  default:
    break;
  }
  return 0;
}

void CSvnLogDlg::Run()
{
  volatile bool terminate = false;
  enum result res = UNDEFINED;
  std::unique_lock<std::mutex> lck(m_mutex);
  while (!terminate)
  {
    // wait for task
    m_cv.wait(lck);

    // execute request
    switch (m_task)
    {
    case LOAD:
      if (!m_repos.load())
        res = LOAD_FAILURE;
      else
        res = LOAD_SUCCESS;
      break;

    case SAVE:
      if (!m_repos.save())
        res = SAVE_FAILURE;
      else
        res = SAVE_SUCCESS;
      break;

    case SCAN:
      if (!m_repos.scan_directory(m_path))
        res = SCAN_FAILURE;
      else
        res = SCAN_SUCCESS;
      break;
    
    case GET_LOGS:
    {
      // create callback to update progress-bar
      std::function<void(std::size_t, std::size_t)> update_cb = [this](std::size_t idx, std::size_t total)
      {
        static std::size_t old_val = -1;
        const std::size_t val = static_cast<std::size_t>(idx * 100 / total);
        if (val != old_val)
        {
          ::PostMessage(m_hWnd, WM_UPDATE_PROGRESS, static_cast<WPARAM>(val), 0);
          old_val = val;
        }
      };
      if (!m_repos.get_logs(update_cb))
        res = GET_LOGS_FAILURE;
      else
        res = GET_LOGS_SUCCESS;
      break;
    }

    case TERMINATE:
      terminate = true;
      break;

    default:
      break;
    }

    // update gui and state-machine
    if (!terminate)
      ::PostMessage(m_hWnd, WM_POSTMESSAGE, static_cast<WPARAM>(res), 0);
  }
}

void CSvnLogDlg::Notify(const enum tasks task)
{
  // update state
  {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_task = task;
  }

  // notify thread to execute task
  m_cv.notify_one();
}

void CSvnLogDlg::LockCtrl(const uint32_t id)
{
  if (!id)
  {
    GetDlgItem(IDC_LIST)->EnableWindow(FALSE);
    GetDlgItem(IDC_EDIT)->EnableWindow(FALSE);
    GetDlgItem(IDC_MFCEDITBROWSE)->EnableWindow(FALSE);
  }
  else
    GetDlgItem(id)->EnableWindow(FALSE);
  Invalidate(TRUE);
  UpdateWindow();
}

void CSvnLogDlg::UnlockCtrl(const uint32_t id)
{
  if (!id)
  {
    GetDlgItem(IDC_LIST)->EnableWindow(TRUE);
    GetDlgItem(IDC_EDIT)->EnableWindow(TRUE);
    GetDlgItem(IDC_MFCEDITBROWSE)->EnableWindow(TRUE);
  }
  else
    GetDlgItem(id)->EnableWindow(TRUE);
}

void CSvnLogDlg::UpdateListCtrl()
{
  m_list_ctrl.DeleteAllItems();
  {
    int idx = 0;
    const std::set<std::shared_ptr<struct svn_commit>>& commits = m_repos.get_commits();
    for (const auto& c : commits)
    {
      m_list_ctrl.InsertItem(idx, CString(c->date.c_str()));
      m_list_ctrl.SetItemText(idx, 1, CString(c->author.c_str()));
      m_list_ctrl.SetItemText(idx, 2, CString(c->repos.c_str()));
      m_list_ctrl.SetItemText(idx, 3, CString(std::to_string(c->id).c_str()));
      ++idx;
    }
  }
}
