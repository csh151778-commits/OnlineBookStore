#pragma once
#include <afxcmn.h>

// 헤더 색상을 적용할 수 있는 커스텀 ListCtrl
class CColorListCtrl : public CListCtrl
{
    DECLARE_DYNAMIC(CColorListCtrl)
public:
    CColorListCtrl();
    virtual ~CColorListCtrl();

    void SetHeaderColors(COLORREF bg, COLORREF text);

protected:
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    COLORREF m_clrHdrBg   = RGB(193, 127, 74);
    COLORREF m_clrHdrText = RGB(255, 255, 255);
    COLORREF m_clrRowOdd  = RGB(255, 255, 255);
    COLORREF m_clrRowEven = RGB(253, 245, 236);
    COLORREF m_clrRowText = RGB( 92,  68,  51);
};
