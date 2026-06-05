#pragma once
#include <afxcmn.h>

class CColorHeaderCtrl : public CHeaderCtrl
{
    DECLARE_DYNAMIC(CColorHeaderCtrl)
public:
    CColorHeaderCtrl();
    virtual ~CColorHeaderCtrl();
    void SetColors(COLORREF bg, COLORREF text);

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    COLORREF m_clrBg   = RGB(193, 127, 74);
    COLORREF m_clrText = RGB(255, 255, 255);
};
