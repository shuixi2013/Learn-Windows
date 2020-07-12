#include "pch.h"
#include "framework.h"
#include "MyPictrueCtrl.h"
BEGIN_MESSAGE_MAP(CMyPictrueCtrl, CStatic)
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


void CMyPictrueCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
    //变更图片
    HICON hIcon = ::LoadIcon(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON2));
    HDC hDc = ::GetDC(this->m_hWnd);
    ::DrawIcon(hDc, 0, 0, hIcon);
    ::ReleaseDC(this->m_hWnd, hDc);
    //变更光标
    HCURSOR hCursor = ::LoadCursor(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_CURSOR1));
    ::SetCursor(hCursor);
    m_bOnDraw = TRUE;
    //设置捕获鼠标
    ::SetCapture(this->m_hWnd);

    CStatic::OnLButtonDown(nFlags, point);
}


void CMyPictrueCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
    if (m_bOnDraw)
    {
        //还原图片
        HICON hIcon = ::LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
        HDC hDc = ::GetDC(this->m_hWnd);
        ::DrawIcon(hDc, 0, 0, hIcon);
        ::ReleaseDC(this->m_hWnd, hDc);
        //还原旧窗口边框
        DrawWnd(m_hOldWnd);
        m_hOldWnd = NULL;
        //还原光标
        m_bOnDraw = FALSE;
        //释放鼠标捕获
        ReleaseCapture();
    }
    CStatic::OnLButtonUp(nFlags, point);
}


void CMyPictrueCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_bOnDraw)
    {
        //设置光标
        //HCURSOR hCursor = ::LoadCursor(::GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_CURSOR1));
        //::SetCursor(hCursor);

        //获取鼠标所在的窗口
        POINT ptOfScreen = POINT{ point.x, point.y };
        ::ClientToScreen(this->m_hWnd, &ptOfScreen);
        HWND hWndOfPt = ::WindowFromPoint(ptOfScreen);
        //不捕捉Spy++自身程序的窗口
        HWND hWndSpyFind = ::FindWindow(NULL, _T("19MFCSpy++"));
        CRect rcSpyFindDlg;
        ::GetWindowRect(hWndSpyFind, &rcSpyFindDlg);
        HWND hWndpchunterDlg = AfxGetMainWnd()->GetSafeHwnd();
        if (hWndOfPt == this->GetSafeHwnd() ||
             hWndOfPt == hWndpchunterDlg ||
                rcSpyFindDlg.PtInRect(ptOfScreen))
        {
            return;
        }
        //绘制边框
        if (m_hOldWnd != hWndOfPt)
        {
            DrawWnd(m_hOldWnd);   //还原旧窗口边框
            DrawWnd(hWndOfPt);    //绘制新窗口边框
            //填充信息
            TCHAR szWndClass[MAXBYTE] = { 0 };
            TCHAR szWndText[MAXBYTE] = { 0 };
            ::GetClassName(hWndOfPt, szWndClass, sizeof(szWndClass) / sizeof(TCHAR));
            ::GetWindowText(hWndOfPt, szWndText, sizeof(szWndText) / sizeof(TCHAR));
            CString csHwnd; 
            csHwnd.Format(_T("%08X"), (DWORD)hWndOfPt);
            ::SetDlgItemText(hWndSpyFind, IDC_EDIT_SPYFIND_HANDLE, csHwnd.GetBuffer());
            ::SetDlgItemText(hWndSpyFind, IDC_EDIT_SPYFIND_TITLE, szWndText);
            ::SetDlgItemText(hWndSpyFind, IDC_EDIT_SPYFIND_CLASS, szWndClass);
            InvalidateRect(NULL, FALSE);
            m_hOldWnd = hWndOfPt;
        }
    }

    CStatic::OnMouseMove(nFlags, point);
}


void CMyPictrueCtrl::DrawWnd(HWND hWnd)
{
    if (hWnd == NULL)
    {
        return;
    }
    //绘制矩形：拿桌面，绘制一个矩形框
    HWND hWndDesk = ::GetDesktopWindow();
    HDC hDcDesk = ::GetDC(NULL);//注意：这里传NULL才能拿到整个屏幕的DC

    RECT rc;
    ::GetWindowRect(hWnd, &rc);

    HPEN hPen = ::CreatePen(PS_SOLID, 5, RGB(255, 255, 255));
    ::SelectObject(hDcDesk, hPen);
    ::SelectObject(hDcDesk, GetStockObject(NULL_BRUSH));
    ::SetROP2(hDcDesk, R2_XORPEN);//将画笔的颜色与一个值做异或
    ::Rectangle(hDcDesk, rc.left, rc.top, rc.right, rc.bottom);

    ::ReleaseDC(hWndDesk, hDcDesk);
}