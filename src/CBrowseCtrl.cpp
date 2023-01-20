#include "pch.h"
#include "framework.h"
#include "CBrowseCtrl.h"

// constructor
CBrowseCtrl::CBrowseCtrl(const UINT evt) :
  m_notification_enabled(true),
  m_evt(evt)
{
}

// destructor
CBrowseCtrl::~CBrowseCtrl()
{
}

#pragma warning(push)
#pragma warning(disable: 26454)
BEGIN_MESSAGE_MAP(CBrowseCtrl, CMFCEditBrowseCtrl)
  ON_CONTROL_REFLECT_EX(EN_CHANGE, &CBrowseCtrl::UpdateEnChangeStatus)
END_MESSAGE_MAP()
#pragma warning(pop)

BOOL CBrowseCtrl::UpdateEnChangeStatus()
{
  return !m_notification_enabled;
}

// set text without causing EN_CHANGE processing
void CBrowseCtrl::SetText(const CString& text)
{
  m_notification_enabled = false;
  SetWindowText(text);
  m_notification_enabled = true;
}

// called after the edit browse control is updated with the result of a browse action 
void CBrowseCtrl::OnAfterUpdate()
{
  GetParent()->PostMessage(m_evt, 0, 0);
}

// avoid the flickering
void CBrowseCtrl::OnDrawBrowseButton(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bIsButtonHot)
{
  CMemDC dc(*pDC, rect);
  __super::OnDrawBrowseButton(&dc.GetDC(), rect, bIsButtonPressed, bIsButtonHot);
}