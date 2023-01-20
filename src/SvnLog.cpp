#include "pch.h"
#include "framework.h"
#include <memory>
#include "SvnLog.h"
#include "SvnLogDlg.h"

// global instanciation of the application
CSvnLogApp theApp;

CSvnLogApp::CSvnLogApp()
{
}

BOOL CSvnLogApp::InitInstance()
{
  std::unique_ptr<CShellManager> pShellManager(new CShellManager);
  CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
  
  CSvnLogDlg dlg;
  m_pMainWnd = &dlg;
  dlg.DoModal();

  CMFCVisualManager::DestroyInstance(true);
  return TRUE;
}