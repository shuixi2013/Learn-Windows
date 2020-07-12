#pragma once

#include "resource.h"
class CMyPictrueCtrl :
    public CStatic
{
private:
    BOOL m_bOnDraw = FALSE;
    HWND m_hOldWnd = NULL;
    void DrawWnd(HWND hWnd);
public:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};

