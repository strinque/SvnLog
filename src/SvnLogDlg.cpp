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
  m_hIcon()
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CSvnLogDlg::~CSvnLogDlg()
{
}

void CSvnLogDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
}

#pragma warning(push)
#pragma warning(disable: 26454)
BEGIN_MESSAGE_MAP(CSvnLogDlg, CDialog)
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()
#pragma warning(pop)

BOOL CSvnLogDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // set the icon for this dialog
  SetIcon(m_hIcon, TRUE);   // set big icon
  SetIcon(m_hIcon, FALSE);  // set small icon

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
