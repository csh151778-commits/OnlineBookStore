// CColorHeaderCtrl.cpp
#include "pch.h"
#include "CColorHeaderCtrl.h"

IMPLEMENT_DYNAMIC(CColorHeaderCtrl, CHeaderCtrl)

CColorHeaderCtrl::CColorHeaderCtrl() {}
CColorHeaderCtrl::~CColorHeaderCtrl() {}

void CColorHeaderCtrl::SetColors(COLORREF bg, COLORREF text)
{
    m_clrBg   = bg;
    m_clrText = text;
}

BEGIN_MESSAGE_MAP(CColorHeaderCtrl, CHeaderCtrl)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CColorHeaderCtrl::OnPaint()
{
    CPaintDC dc(this);

    CRect rcClient;
    GetClientRect(&rcClient);
    dc.FillSolidRect(&rcClient, m_clrBg);

    int count = GetItemCount();
    for (int i = 0; i < count; i++) {
        CRect rcItem;
        GetItemRect(i, &rcItem);

        // 구분선
        CPen pen(PS_SOLID, 1, RGB(220, 190, 160));
        CPen* pOld = dc.SelectObject(&pen);
        dc.MoveTo(rcItem.right - 1, rcItem.top + 3);
        dc.LineTo(rcItem.right - 1, rcItem.bottom - 3);
        dc.SelectObject(pOld);

        // 텍스트
        HDITEM hdi = {};
        WCHAR buf[256] = {};
        hdi.mask       = HDI_TEXT | HDI_FORMAT;
        hdi.pszText    = buf;
        hdi.cchTextMax = 255;
        GetItem(i, &hdi);

        dc.SetBkMode(TRANSPARENT);
        dc.SetTextColor(m_clrText);

        HFONT hFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            HANGEUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"맑은 고딕");
        HFONT hOld = (HFONT)dc.SelectObject(hFont);

        CRect rcText = rcItem;
        rcText.left += 6;
        dc.DrawText(buf, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        dc.SelectObject(hOld);
        DeleteObject(hFont);
    }
}
