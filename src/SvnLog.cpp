#include "pch.h"
#include "framework.h"
#include <memory>
#include "SvnLog.h"
#include "SvnLogDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CSvnLogApp, CWinApp)
  ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// global instanciation of the application
CSvnLogApp theApp;

CSvnLogApp::CSvnLogApp()
{
}

BOOL CSvnLogApp::InitInstance()
{
  CWinApp::InitInstance();

	std::unique_ptr<CShellManager> pShellManager(new CShellManager);
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

  CSvnLogDlg dlg;
  m_pMainWnd = &dlg;
  dlg.DoModal();
  
#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
  ControlBarCleanUp();
#endif

  return FALSE;
}
