#pragma once
#include <stdbool.h>

// customize CMFCEditBrowseCtrl
//  - allow control's text value to be set without causing EN_CHANGE processing
//  - triggers event after the update has been processed
//  -  remove the flickering of buttons
class CBrowseCtrl final : public CMFCEditBrowseCtrl
{
public:
  // constructor/destructor
  CBrowseCtrl(const UINT evt);
  ~CBrowseCtrl();

  // set text without causing EN_CHANGE processing
  void SetText(const CString& text);

protected:
  afx_msg BOOL UpdateEnChangeStatus();
  DECLARE_MESSAGE_MAP()

private:
  // called after the edit browse control is updated with the result of a browse action 
  void OnAfterUpdate() override;

  // avoid the flickering
  void OnDrawBrowseButton(CDC* pDC, CRect rect, BOOL bIsButtonPressed, BOOL bIsButtonHot) override;

private:
  bool m_notification_enabled;
  UINT m_evt;
};