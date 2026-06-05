// CColorListCtrl.cpp
#include "pch.h"
#include "CColorListCtrl.h"

IMPLEMENT_DYNAMIC(CColorListCtrl, CListCtrl)

CColorListCtrl::CColorListCtrl() {}
CColorListCtrl::~CColorListCtrl() {}

void CColorListCtrl::SetHeaderColors(COLORREF bg, COLORREF text)
{
    m_clrHdrBg   = bg;
    m_clrHdrText = text;
}

BEGIN_MESSAGE_MAP(CColorListCtrl, CListCtrl)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CColorListCtrl::OnCustomDraw)
END_MESSAGE_MAP()

void CColorListCtrl::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pCD = (LPNMLVCUSTOMDRAW)pNMHDR;
    *pResult = CDRF_DODEFAULT;

    switch (pCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;

    case CDDS_ITEMPREPAINT:
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        break;

    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
        int row = (int)pCD->nmcd.dwItemSpec;
        if (row >= 0) {
            pCD->clrTextBk = (row % 2 == 0) ? m_clrRowOdd : m_clrRowEven;
            pCD->clrText   = m_clrRowText;
        }
        *pResult = CDRF_NEWFONT;
        break;
    }
    }
}
