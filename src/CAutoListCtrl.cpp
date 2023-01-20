#include "pch.h"
#include "framework.h"
#include "CAutoListCtrl.h"
#include <regex>

// constructor
CAutoListCtrl::CAutoListCtrl() :
  m_link(),
  m_default(),
  m_size()
{
}

// destructor
CAutoListCtrl::~CAutoListCtrl()
{
}

// set the headers of the control
void CAutoListCtrl::SetHeaders(const std::vector<struct header_info>& headers)
{
  // set extended style
  SetExtendedStyle(GetExtendedStyle() | LVS_EX_FULLROWSELECT);

  // create default and link font (for url)
  LOGFONT lf = {};
  CFont* currentFont = GetFont();
  currentFont->GetLogFont(&lf);
  m_default.CreateFontIndirect(&lf);
  lf.lfUnderline = true;
  m_link.CreateFontIndirect(&lf);

  // add empty column (bug-fix for the first header with center-flag)
  InsertColumn(0, L"", 0);

  // add all headers informations
  m_size.clear();
  for (const auto& h : headers)
  {
    m_size.push_back(h.size);
    InsertColumn(h.id + 1, h.name, h.style, h.size);
  }

  // remove empty column (bug-fix)
  DeleteColumn(0);

  // compute the headers size automaticaly
  for (int i = 0; i < m_size.size(); ++i)
    if(m_size[i] == LVSCW_AUTOSIZE)
      SetColumnWidth(i, GetRemainingWidth());
}

// clear the item selection
void CAutoListCtrl::ClearItemSelection()
{
  const int row = GetSelectionMark();
  if (row != -1)
    SetItemState(row, GetItemState(row, LVIS_SELECTED) & ~LVIS_SELECTED, LVIS_SELECTED);
}

#pragma warning(push)
#pragma warning(disable: 26454)
BEGIN_MESSAGE_MAP(CAutoListCtrl, CListCtrl)
  ON_WM_SIZE()
  ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()
#pragma warning(pop)

void CAutoListCtrl::OnSize(UINT nType, int cx, int cy)
{
  CListCtrl::OnSize(nType, cx, cy);
  for (int i = 0; i < m_size.size(); ++i)
    if (m_size[i] == LVSCW_AUTOSIZE)
      SetColumnWidth(i, GetRemainingWidth());
}

void CAutoListCtrl::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
  NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
  *pResult = 0;

  // check if the item is an url
  const CString& url = GetItemText(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem);
  bool is_url = std::regex_search(url.GetString(), std::wregex(LR"(http(s)*:\/\/)"));

  // draw control
  switch (pLVCD->nmcd.dwDrawStage)
  {
    case CDDS_PREPAINT:
      *pResult = CDRF_NOTIFYITEMDRAW;
      break;

    case CDDS_ITEMPREPAINT:
      *pResult = CDRF_NOTIFYSUBITEMDRAW;
      break;

    case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
      if(is_url)
      {
        pLVCD->clrText = RGB(0, 0, 255);
        ::SelectObject(pLVCD->nmcd.hdc, m_link.m_hObject);
      }
      else
      {
        pLVCD->clrText = RGB(0, 0, 0);
        ::SelectObject(pLVCD->nmcd.hdc, m_default.m_hObject);
      }
      *pResult = CDRF_DODEFAULT;
      break;

    default:
      break;
  }
}

// compute the remaining width in the CListCtrl
int CAutoListCtrl::GetRemainingWidth() const
{
  CRect rc;
  GetClientRect(&rc);
  int width = rc.Width();
  for (const auto& s : m_size)
    width -= (s == LVSCW_AUTOSIZE) ? 0 : s;
  return width;
}