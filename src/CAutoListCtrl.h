#pragma once
#include <vector>
#include <string>

// header informations
struct header_info {
  int id;
  CString name;
  int style;
  int size;
};

// mfc CListCtrl that resize column automaticaly
class CAutoListCtrl final : public CListCtrl
{
public:
  // constructor/destructor
  CAutoListCtrl();
  ~CAutoListCtrl();

  // set the headers of the control
  void SetHeaders(const std::vector<struct header_info>& headers);

  // clear the item selection
  void ClearItemSelection();

protected:
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
  DECLARE_MESSAGE_MAP()

private:
  // compute the remaining width in the CListCtrl
  int GetRemainingWidth() const;

private:
  // attributes for url column
  CFont m_link;
  CFont m_default;

  // store the size of all headers
  std::vector<int> m_size;
};