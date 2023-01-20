#pragma once
#ifndef __AFXWIN_H__
  #error "include 'pch.h' before including this file for PCH"
#endif
#include "resource.h"    // main symbols

class CSvnLogApp : public CWinApp
{
public:
  CSvnLogApp();

public:
  virtual BOOL InitInstance();
  DECLARE_MESSAGE_MAP()
};
extern CSvnLogApp theApp;
