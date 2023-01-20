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

CSvnLogDlg::CSvnLogDlg(CWnd* pParent /*=nullptr*/)
  : CDialog(IDD_SVNLOG_DIALOG, pParent),
  m_hIcon(),
  m_rcDlg(),
  m_editColor(),
  m_path_ctrl(),
  m_path(),
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
}

#pragma warning(push)
#pragma warning(disable: 26454)
BEGIN_MESSAGE_MAP(CSvnLogDlg, CDialog)
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_WM_GETMINMAXINFO()
  ON_WM_CTLCOLOR()
  ON_EN_CHANGE(IDC_MFCEDITBROWSE, &CSvnLogDlg::OnPathChange)
  ON_MESSAGE(WM_POSTMESSAGE, &CSvnLogDlg::OnGetPostMessage)
END_MESSAGE_MAP()
#pragma warning(pop)

BOOL CSvnLogDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // set the icon for this dialog
  SetIcon(m_hIcon, TRUE);   // set big icon
  SetIcon(m_hIcon, FALSE);  // set small icon

  // set edit color
  m_editColor.CreateSolidBrush(RGB(255, 255, 255));

  // calculate minimum size
  GetWindowRect(&m_rcDlg);
  m_rcDlg -= m_rcDlg.TopLeft();

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
  m_path = str.GetString();

  // notify thread to begin scan
  Notify(SCAN);
}

LRESULT CSvnLogDlg::OnGetPostMessage(WPARAM wparam, LPARAM lparam)
{
  m_repos.get_logs();
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
    switch (m_task)
    {
    case SCAN:
      if(m_repos.scan_directory(m_path))
        ::PostMessage(m_hWnd, WM_POSTMESSAGE, 0, 0);
      break;

    case TERMINATE:
      terminate = true;
      break;

    default:
      break;
    }
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
