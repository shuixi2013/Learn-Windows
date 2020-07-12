#include "pch.h"
#include "framework.h"
#include "19MFCSpyDll.h"
#include <tlhelp32.h>
#include <windows.h>

struct AFX_MSGMAP_ENTRY
{
    UINT nMessage;   // windows message
    UINT nCode;      // control code or WM_NOTIFY code
    UINT nID;        // control ID (or 0 for windows messages)
    UINT nLastID;    // used for entries specifying a range of control id's
    UINT_PTR nSig;       // signature type (action) or pointer to message #
    DWORD pfn;    // routine to call (or special value)
};

struct AFX_MSGMAP
{
    const AFX_MSGMAP* (PASCAL* pfnGetBaseMap)();
    const AFX_MSGMAP_ENTRY* lpEntries;
};

typedef DWORD* (__stdcall* PFN_FROMHANDLEPERMANENT)(HWND hWnd);
typedef DWORD* (__stdcall* PFN_GETRUNTIMECLASS)();
typedef AFX_MSGMAP* (__stdcall* PFN_GETMESSAGEMAP)();

struct MY_MFCFUNC
{
    PFN_GETRUNTIMECLASS pfn_GetRuntimeClass;
    PFN_GETMESSAGEMAP pfn_GetMessageMap;
};

void MySetWindowLong(WNDPROC wndProc);
const char* GetMsgName(WORD wID);
MY_MFCFUNC GetMFCFunc(DWORD dwAfxWndProcVA);
void SetMFCSpyData(MY_MFCFUNC mfcFunc);
BOOL IsMFCDll(DWORD* dwPfnFromHandleVA);


#pragma data_seg("MFCSPY_DATA") //定义共享区域的开始
HWND g_hMFCWndSelected = NULL;
CHAR g_szBuffMFCSpyData[0x1000] = { 0 };
#pragma data_seg("")            //定义共享区域的结束
#pragma comment(linker, "/SECTION:MFCSPY_DATA,RWS")

WNDPROC g_pfnAfxWndProc = NULL;


LRESULT CALLBACK MyWindowProc(HWND hwnd,      // handle to window
    UINT uMsg,      // message identifier
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    //获取GetRuntimeClass和GetMessageMap的函数地址
    MY_MFCFUNC mfcFunc = GetMFCFunc((DWORD)g_pfnAfxWndProc);
    if (mfcFunc.pfn_GetMessageMap != NULL && mfcFunc.pfn_GetRuntimeClass != NULL)
    {
        //拼接显示的数据
        SetMFCSpyData(mfcFunc);
    }
    else
    {
        g_szBuffMFCSpyData[0] = '\0';
    }

    //将窗口过程函数重置为原始值
    LRESULT lRet = g_pfnAfxWndProc(hwnd, uMsg, wParam, lParam);
    MySetWindowLong(g_pfnAfxWndProc);

    //设置信号，通知19MFCSpy这里处理结束了
    HANDLE hEvent = NULL;
    hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "19MFCSpy");
    if (hEvent != NULL)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    return lRet;
}


void MySetWindowLong(WNDPROC wndProc)
{
    if (IsWindowUnicode(g_hMFCWndSelected))
    {
        g_pfnAfxWndProc = (WNDPROC)SetWindowLongPtrW(g_hMFCWndSelected, GWLP_WNDPROC, (LONG)wndProc);
    }
    else
    {
        g_pfnAfxWndProc = (WNDPROC)SetWindowLongPtrA(g_hMFCWndSelected, GWLP_WNDPROC, (LONG)wndProc);
    }
}


/*
    识别MFC的不同编译方式，获取正确的窗口过程处理函数
*/
MY_MFCFUNC GetMFCFunc(DWORD dwAfxWndProcVA)
{
    MY_MFCFUNC mfcFunc = { 0 };

    PFN_FROMHANDLEPERMANENT pfn_FromHandlePermanent = NULL;
    BOOL bIsDll = IsMFCDll((DWORD*)&pfn_FromHandlePermanent);
    if (bIsDll)
    {
        DWORD* pCWnd = pfn_FromHandlePermanent(g_hMFCWndSelected);
        mfcFunc.pfn_GetRuntimeClass = (PFN_GETRUNTIMECLASS) * (DWORD*)(*pCWnd);
        mfcFunc.pfn_GetMessageMap = (PFN_GETMESSAGEMAP) * (DWORD*)(*pCWnd + 0x30);  // GetMessageMap是虚表第11项
    }
    else
    {
        //静态编译 Debug版 有jmp
        if (*(BYTE*)dwAfxWndProcVA == 0xE9)
        {
            DWORD dwJmpDif = *(DWORD*)(dwAfxWndProcVA + 1);
            dwAfxWndProcVA = dwAfxWndProcVA + 5 + dwJmpDif;

            //获取CWnd::FromHandlePermanent的地址
            if (*(BYTE*)(dwAfxWndProcVA + 35) == 0xE8)
            {
                DWORD dwJmpDif2 = *(DWORD*)(dwAfxWndProcVA + 36);
                PFN_FROMHANDLEPERMANENT pfn_FromHandlePermanent =
                    (PFN_FROMHANDLEPERMANENT)(dwAfxWndProcVA + 40 + dwJmpDif2);

                //获取到CWnd基类指针
                DWORD* pCWnd = pfn_FromHandlePermanent(g_hMFCWndSelected);

                //获取GetRuntimClass函数地址
                mfcFunc.pfn_GetRuntimeClass = (PFN_GETRUNTIMECLASS) * (DWORD*)(*pCWnd);

                //获取对象的MessageMap, GetMessageMap函数地址是虚表第13项
                mfcFunc.pfn_GetMessageMap = (PFN_GETMESSAGEMAP) * (DWORD*)(*pCWnd + 0x30);
            }
        }
        //静态编译 Release版
        else
        {
            if (*(BYTE*)(dwAfxWndProcVA + 22) == 0xE8)
            {
                DWORD dwJmpDif2 = *(DWORD*)(dwAfxWndProcVA + 23);
                PFN_FROMHANDLEPERMANENT pfn_FromHandlePermanent =
                    (PFN_FROMHANDLEPERMANENT)(dwAfxWndProcVA + 27 + dwJmpDif2);
                DWORD* pCWnd = pfn_FromHandlePermanent(g_hMFCWndSelected);
                mfcFunc.pfn_GetRuntimeClass = (PFN_GETRUNTIMECLASS) * (DWORD*)(*pCWnd);
                mfcFunc.pfn_GetMessageMap = (PFN_GETMESSAGEMAP) * (DWORD*)(*pCWnd + 0x28);  // GetMessageMap是虚表第11项
            }
        }
    }

    return mfcFunc;
}

/*
    遍历进程的模块信息，判断是否是动态链接的mfc库
    如果是则返回dll模块中的 FromHandlePermanent 函数的地址
    mfc140ud.dll    Unicode Debug版
    mfc140d.dll     Ansi    Debug版
    mfc140u.dll     Unicode Release版
    mfc140.dll      Ansi    Release版

*/
BOOL IsMFCDll(DWORD* dwPfnFromHandleVA)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

    if (hSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    MODULEENTRY32 me = { 0 };
    int nItem = 0;

    me.dwSize = sizeof(MODULEENTRY32);

    Module32First(hSnap, &me);

    do
    {
        if (memcmp(me.szModule, "mfc140", 6) == 0)
        {
            if (strcmp(me.szModule, "mfc140ud.dll") == 0)
            {
                *dwPfnFromHandleVA = (DWORD)GetProcAddress(me.hModule, (LPCSTR)0x16D8); //mfc140ud.dll 中FromHandlePermanent函数的导出序号
            }
            else if (strcmp(me.szModule, "mfc140d.dll") == 0)
            {
                *dwPfnFromHandleVA = (DWORD)GetProcAddress(me.hModule, (LPCSTR)0x16C6);
            }
            else if (strcmp(me.szModule, "mfc140u.dll") == 0)
            {
                *dwPfnFromHandleVA = (DWORD)GetProcAddress(me.hModule, (LPCSTR)0x131A);
            }
            else if (strcmp(me.szModule, "mfc140.dll") == 0)
            {
                *dwPfnFromHandleVA = (DWORD)GetProcAddress(me.hModule, (LPCSTR)0x130A);
            }
            return TRUE;
        }
    } while (Module32Next(hSnap, &me));

    return FALSE;
}


void SetMFCSpyData(MY_MFCFUNC mfcFunc)
{
    char szBuff[0x100];
    wsprintf(szBuff, "窗口过程函数地址：0x%08X\r\n\r\n", g_pfnAfxWndProc);
    DWORD len = strlen(szBuff);
    DWORD dwOffset = 0;
    memcpy(&g_szBuffMFCSpyData[dwOffset], szBuff, len);
    dwOffset += len;
    g_szBuffMFCSpyData[dwOffset] = '\0';

    //从CRuntimeClass中获取类名
    DWORD* pCRuntimeClass = mfcFunc.pfn_GetRuntimeClass();
    LPCSTR m_lpszClassName = (LPCSTR)*pCRuntimeClass;
    if (m_lpszClassName != NULL)
    {
        wsprintf(szBuff, "窗口类：%s\r\n\r\n", m_lpszClassName);
        DWORD dwLen = strlen(szBuff);
        memcpy(&g_szBuffMFCSpyData[dwOffset], szBuff, dwLen);
        dwOffset += dwLen;
        g_szBuffMFCSpyData[dwOffset] = '\0';
    }

    //从消息映射表中获取到消息处理函数
    AFX_MSGMAP* pMsgMap = mfcFunc.pfn_GetMessageMap();
    if (pMsgMap != NULL && pMsgMap->lpEntries != NULL)
    {
        strcat(&g_szBuffMFCSpyData[dwOffset], "消息\r\n");
        dwOffset += strlen("消息\r\n");

        //遍历MessageMapEntry并显示
        const AFX_MSGMAP_ENTRY* pCurMsgMapEntry = pMsgMap->lpEntries;
        while (pCurMsgMapEntry->nMessage != 0 &&
            pCurMsgMapEntry->pfn != 0)
        {
            char szBuff[0x100];
            const char* szMsgName = GetMsgName(pCurMsgMapEntry->nMessage);
            wsprintf(szBuff, "%-20s    nCode=%d  nID=%d    VA=0x%p\r\n",
                szMsgName == NULL ? "unknown" : szMsgName,
                pCurMsgMapEntry->nCode,
                pCurMsgMapEntry->nID, pCurMsgMapEntry->pfn);

            DWORD dwLen = strlen(szBuff);
            memcpy(&g_szBuffMFCSpyData[dwOffset], szBuff, dwLen);
            dwOffset += dwLen;
            g_szBuffMFCSpyData[dwOffset] = '\0';

            pCurMsgMapEntry++;
        }
    }
}


struct MSG_KEYVALUE
{
    WORD nID;
    const char* szMSG;
};

//从WinUser.h头文件中提取的消息ID
#define MY_MSG_VALUE(WM_MSG) {WM_MSG, #WM_MSG},

MSG_KEYVALUE g_MsgValue[] = {
    MY_MSG_VALUE(WM_NULL)
    MY_MSG_VALUE(WM_CREATE)
    MY_MSG_VALUE(WM_DESTROY)
    MY_MSG_VALUE(WM_MOVE)
    MY_MSG_VALUE(WM_SIZE)
    MY_MSG_VALUE(WM_ACTIVATE)
    MY_MSG_VALUE(WM_SETFOCUS)
    MY_MSG_VALUE(WM_KILLFOCUS)
    MY_MSG_VALUE(WM_ENABLE)
    MY_MSG_VALUE(WM_SETREDRAW)
    MY_MSG_VALUE(WM_SETTEXT)
    MY_MSG_VALUE(WM_GETTEXT)
    MY_MSG_VALUE(WM_GETTEXTLENGTH)
    MY_MSG_VALUE(WM_PAINT)
    MY_MSG_VALUE(WM_CLOSE)
    MY_MSG_VALUE(WM_QUERYENDSESSION)
    MY_MSG_VALUE(WM_QUERYOPEN)
    MY_MSG_VALUE(WM_ENDSESSION)
    MY_MSG_VALUE(WM_QUIT)
    MY_MSG_VALUE(WM_ERASEBKGND)
    MY_MSG_VALUE(WM_SYSCOLORCHANGE)
    MY_MSG_VALUE(WM_SHOWWINDOW)
    MY_MSG_VALUE(WM_WININICHANGE)
    MY_MSG_VALUE(WM_SETTINGCHANGE)
    MY_MSG_VALUE(WM_DEVMODECHANGE)
    MY_MSG_VALUE(WM_ACTIVATEAPP)
    MY_MSG_VALUE(WM_FONTCHANGE)
    MY_MSG_VALUE(WM_TIMECHANGE)
    MY_MSG_VALUE(WM_CANCELMODE)
    MY_MSG_VALUE(WM_SETCURSOR)
    MY_MSG_VALUE(WM_MOUSEACTIVATE)
    MY_MSG_VALUE(WM_CHILDACTIVATE)
    MY_MSG_VALUE(WM_QUEUESYNC)
    MY_MSG_VALUE(WM_GETMINMAXINFO)
    MY_MSG_VALUE(WM_PAINTICON)
    MY_MSG_VALUE(WM_ICONERASEBKGND)
    MY_MSG_VALUE(WM_NEXTDLGCTL)
    MY_MSG_VALUE(WM_SPOOLERSTATUS)
    MY_MSG_VALUE(WM_DRAWITEM)
    MY_MSG_VALUE(WM_MEASUREITEM)
    MY_MSG_VALUE(WM_DELETEITEM)
    MY_MSG_VALUE(WM_VKEYTOITEM)
    MY_MSG_VALUE(WM_CHARTOITEM)
    MY_MSG_VALUE(WM_SETFONT)
    MY_MSG_VALUE(WM_GETFONT)
    MY_MSG_VALUE(WM_SETHOTKEY)
    MY_MSG_VALUE(WM_GETHOTKEY)
    MY_MSG_VALUE(WM_QUERYDRAGICON)
    MY_MSG_VALUE(WM_COMPAREITEM)
    MY_MSG_VALUE(WM_GETOBJECT)
    MY_MSG_VALUE(WM_COMPACTING)
    MY_MSG_VALUE(WM_COMMNOTIFY)
    MY_MSG_VALUE(WM_WINDOWPOSCHANGING)
    MY_MSG_VALUE(WM_WINDOWPOSCHANGED)
    MY_MSG_VALUE(WM_POWER)
    MY_MSG_VALUE(WM_COPYDATA)
    MY_MSG_VALUE(WM_CANCELJOURNAL)
    MY_MSG_VALUE(WM_NOTIFY)
    MY_MSG_VALUE(WM_INPUTLANGCHANGEREQUEST)
    MY_MSG_VALUE(WM_INPUTLANGCHANGE)
    MY_MSG_VALUE(WM_TCARD)
    MY_MSG_VALUE(WM_HELP)
    MY_MSG_VALUE(WM_USERCHANGED)
    MY_MSG_VALUE(WM_NOTIFYFORMAT)
    MY_MSG_VALUE(WM_CONTEXTMENU)
    MY_MSG_VALUE(WM_STYLECHANGING)
    MY_MSG_VALUE(WM_STYLECHANGED)
    MY_MSG_VALUE(WM_DISPLAYCHANGE)
    MY_MSG_VALUE(WM_GETICON)
    MY_MSG_VALUE(WM_SETICON)
    MY_MSG_VALUE(WM_NCCREATE)
    MY_MSG_VALUE(WM_NCDESTROY)
    MY_MSG_VALUE(WM_NCCALCSIZE)
    MY_MSG_VALUE(WM_NCHITTEST)
    MY_MSG_VALUE(WM_NCPAINT)
    MY_MSG_VALUE(WM_NCACTIVATE)
    MY_MSG_VALUE(WM_GETDLGCODE)
    MY_MSG_VALUE(WM_SYNCPAINT)
    MY_MSG_VALUE(WM_NCMOUSEMOVE)
    MY_MSG_VALUE(WM_NCLBUTTONDOWN)
    MY_MSG_VALUE(WM_NCLBUTTONUP)
    MY_MSG_VALUE(WM_NCLBUTTONDBLCLK)
    MY_MSG_VALUE(WM_NCRBUTTONDOWN)
    MY_MSG_VALUE(WM_NCRBUTTONUP)
    MY_MSG_VALUE(WM_NCRBUTTONDBLCLK)
    MY_MSG_VALUE(WM_NCMBUTTONDOWN)
    MY_MSG_VALUE(WM_NCMBUTTONUP)
    MY_MSG_VALUE(WM_NCMBUTTONDBLCLK)
    MY_MSG_VALUE(WM_NCXBUTTONDOWN)
    MY_MSG_VALUE(WM_NCXBUTTONUP)
    MY_MSG_VALUE(WM_NCXBUTTONDBLCLK)
    MY_MSG_VALUE(WM_INPUT_DEVICE_CHANGE)
    MY_MSG_VALUE(WM_INPUT)
    MY_MSG_VALUE(WM_KEYFIRST)
    MY_MSG_VALUE(WM_KEYDOWN)
    MY_MSG_VALUE(WM_KEYUP)
    MY_MSG_VALUE(WM_CHAR)
    MY_MSG_VALUE(WM_DEADCHAR)
    MY_MSG_VALUE(WM_SYSKEYDOWN)
    MY_MSG_VALUE(WM_SYSKEYUP)
    MY_MSG_VALUE(WM_SYSCHAR)
    MY_MSG_VALUE(WM_SYSDEADCHAR)
    MY_MSG_VALUE(WM_UNICHAR)
    MY_MSG_VALUE(WM_KEYLAST)
    MY_MSG_VALUE(WM_KEYLAST)
    MY_MSG_VALUE(WM_IME_STARTCOMPOSITION)
    MY_MSG_VALUE(WM_IME_ENDCOMPOSITION)
    MY_MSG_VALUE(WM_IME_COMPOSITION)
    MY_MSG_VALUE(WM_IME_KEYLAST)
    MY_MSG_VALUE(WM_INITDIALOG)
    MY_MSG_VALUE(WM_COMMAND)
    MY_MSG_VALUE(WM_SYSCOMMAND)
    MY_MSG_VALUE(WM_TIMER)
    MY_MSG_VALUE(WM_HSCROLL)
    MY_MSG_VALUE(WM_VSCROLL)
    MY_MSG_VALUE(WM_INITMENU)
    MY_MSG_VALUE(WM_INITMENUPOPUP)
    MY_MSG_VALUE(WM_GESTURE)
    MY_MSG_VALUE(WM_GESTURENOTIFY)
    MY_MSG_VALUE(WM_MENUSELECT)
    MY_MSG_VALUE(WM_MENUCHAR)
    MY_MSG_VALUE(WM_ENTERIDLE)
    MY_MSG_VALUE(WM_MENURBUTTONUP)
    MY_MSG_VALUE(WM_MENUDRAG)
    MY_MSG_VALUE(WM_MENUGETOBJECT)
    MY_MSG_VALUE(WM_UNINITMENUPOPUP)
    MY_MSG_VALUE(WM_MENUCOMMAND)
    MY_MSG_VALUE(WM_CHANGEUISTATE)
    MY_MSG_VALUE(WM_UPDATEUISTATE)
    MY_MSG_VALUE(WM_QUERYUISTATE)
    MY_MSG_VALUE(WM_CTLCOLORMSGBOX)
    MY_MSG_VALUE(WM_CTLCOLOREDIT)
    MY_MSG_VALUE(WM_CTLCOLORLISTBOX)
    MY_MSG_VALUE(WM_CTLCOLORBTN)
    MY_MSG_VALUE(WM_CTLCOLORDLG)
    MY_MSG_VALUE(WM_CTLCOLORSCROLLBAR)
    MY_MSG_VALUE(WM_CTLCOLORSTATIC)
    MY_MSG_VALUE(WM_MOUSEFIRST)
    MY_MSG_VALUE(WM_MOUSEMOVE)
    MY_MSG_VALUE(WM_LBUTTONDOWN)
    MY_MSG_VALUE(WM_LBUTTONUP)
    MY_MSG_VALUE(WM_LBUTTONDBLCLK)
    MY_MSG_VALUE(WM_RBUTTONDOWN)
    MY_MSG_VALUE(WM_RBUTTONUP)
    MY_MSG_VALUE(WM_RBUTTONDBLCLK)
    MY_MSG_VALUE(WM_MBUTTONDOWN)
    MY_MSG_VALUE(WM_MBUTTONUP)
    MY_MSG_VALUE(WM_MBUTTONDBLCLK)
    MY_MSG_VALUE(WM_MOUSEWHEEL)
    MY_MSG_VALUE(WM_XBUTTONDOWN)
    MY_MSG_VALUE(WM_XBUTTONUP)
    MY_MSG_VALUE(WM_XBUTTONDBLCLK)
    MY_MSG_VALUE(WM_MOUSEHWHEEL)
    MY_MSG_VALUE(WM_MOUSELAST)
    MY_MSG_VALUE(WM_MOUSELAST)
    MY_MSG_VALUE(WM_MOUSELAST)
    MY_MSG_VALUE(WM_MOUSELAST)
    MY_MSG_VALUE(WM_PARENTNOTIFY)
    MY_MSG_VALUE(WM_ENTERMENULOOP)
    MY_MSG_VALUE(WM_EXITMENULOOP)
    MY_MSG_VALUE(WM_NEXTMENU)
    MY_MSG_VALUE(WM_SIZING)
    MY_MSG_VALUE(WM_CAPTURECHANGED)
    MY_MSG_VALUE(WM_MOVING)
    MY_MSG_VALUE(WM_POWERBROADCAST)
    MY_MSG_VALUE(WM_DEVICECHANGE)
    MY_MSG_VALUE(WM_MDICREATE)
    MY_MSG_VALUE(WM_MDIDESTROY)
    MY_MSG_VALUE(WM_MDIACTIVATE)
    MY_MSG_VALUE(WM_MDIRESTORE)
    MY_MSG_VALUE(WM_MDINEXT)
    MY_MSG_VALUE(WM_MDIMAXIMIZE)
    MY_MSG_VALUE(WM_MDITILE)
    MY_MSG_VALUE(WM_MDICASCADE)
    MY_MSG_VALUE(WM_MDIICONARRANGE)
    MY_MSG_VALUE(WM_MDIGETACTIVE)
    MY_MSG_VALUE(WM_MDISETMENU)
    MY_MSG_VALUE(WM_ENTERSIZEMOVE)
    MY_MSG_VALUE(WM_EXITSIZEMOVE)
    MY_MSG_VALUE(WM_DROPFILES)
    MY_MSG_VALUE(WM_MDIREFRESHMENU)
    MY_MSG_VALUE(WM_POINTERDEVICECHANGE)
    MY_MSG_VALUE(WM_POINTERDEVICEINRANGE)
    MY_MSG_VALUE(WM_POINTERDEVICEOUTOFRANGE)
    MY_MSG_VALUE(WM_TOUCH)
    MY_MSG_VALUE(WM_NCPOINTERUPDATE)
    MY_MSG_VALUE(WM_NCPOINTERDOWN)
    MY_MSG_VALUE(WM_NCPOINTERUP)
    MY_MSG_VALUE(WM_POINTERUPDATE)
    MY_MSG_VALUE(WM_POINTERDOWN)
    MY_MSG_VALUE(WM_POINTERUP)
    MY_MSG_VALUE(WM_POINTERENTER)
    MY_MSG_VALUE(WM_POINTERLEAVE)
    MY_MSG_VALUE(WM_POINTERACTIVATE)
    MY_MSG_VALUE(WM_POINTERCAPTURECHANGED)
    MY_MSG_VALUE(WM_TOUCHHITTESTING)
    MY_MSG_VALUE(WM_POINTERWHEEL)
    MY_MSG_VALUE(WM_POINTERHWHEEL)
    MY_MSG_VALUE(WM_POINTERROUTEDTO)
    MY_MSG_VALUE(WM_POINTERROUTEDAWAY)
    MY_MSG_VALUE(WM_POINTERROUTEDRELEASED)
    MY_MSG_VALUE(WM_IME_SETCONTEXT)
    MY_MSG_VALUE(WM_IME_NOTIFY)
    MY_MSG_VALUE(WM_IME_CONTROL)
    MY_MSG_VALUE(WM_IME_COMPOSITIONFULL)
    MY_MSG_VALUE(WM_IME_SELECT)
    MY_MSG_VALUE(WM_IME_CHAR)
    MY_MSG_VALUE(WM_IME_REQUEST)
    MY_MSG_VALUE(WM_IME_KEYDOWN)
    MY_MSG_VALUE(WM_IME_KEYUP)
    MY_MSG_VALUE(WM_MOUSEHOVER)
    MY_MSG_VALUE(WM_MOUSELEAVE)
    MY_MSG_VALUE(WM_NCMOUSEHOVER)
    MY_MSG_VALUE(WM_NCMOUSELEAVE)
    MY_MSG_VALUE(WM_WTSSESSION_CHANGE)
    MY_MSG_VALUE(WM_TABLET_FIRST)
    MY_MSG_VALUE(WM_TABLET_LAST)
    MY_MSG_VALUE(WM_DPICHANGED)
    MY_MSG_VALUE(WM_DPICHANGED_BEFOREPARENT)
    MY_MSG_VALUE(WM_DPICHANGED_AFTERPARENT)
    MY_MSG_VALUE(WM_GETDPISCALEDSIZE)
    MY_MSG_VALUE(WM_CUT)
    MY_MSG_VALUE(WM_COPY)
    MY_MSG_VALUE(WM_PASTE)
    MY_MSG_VALUE(WM_CLEAR)
    MY_MSG_VALUE(WM_UNDO)
    MY_MSG_VALUE(WM_RENDERFORMAT)
    MY_MSG_VALUE(WM_RENDERALLFORMATS)
    MY_MSG_VALUE(WM_DESTROYCLIPBOARD)
    MY_MSG_VALUE(WM_DRAWCLIPBOARD)
    MY_MSG_VALUE(WM_PAINTCLIPBOARD)
    MY_MSG_VALUE(WM_VSCROLLCLIPBOARD)
    MY_MSG_VALUE(WM_SIZECLIPBOARD)
    MY_MSG_VALUE(WM_ASKCBFORMATNAME)
    MY_MSG_VALUE(WM_CHANGECBCHAIN)
    MY_MSG_VALUE(WM_HSCROLLCLIPBOARD)
    MY_MSG_VALUE(WM_QUERYNEWPALETTE)
    MY_MSG_VALUE(WM_PALETTEISCHANGING)
    MY_MSG_VALUE(WM_PALETTECHANGED)
    MY_MSG_VALUE(WM_HOTKEY)
    MY_MSG_VALUE(WM_PRINT)
    MY_MSG_VALUE(WM_PRINTCLIENT)
    MY_MSG_VALUE(WM_APPCOMMAND)
    MY_MSG_VALUE(WM_THEMECHANGED)
    MY_MSG_VALUE(WM_CLIPBOARDUPDATE)
    MY_MSG_VALUE(WM_DWMCOMPOSITIONCHANGED)
    MY_MSG_VALUE(WM_DWMNCRENDERINGCHANGED)
    MY_MSG_VALUE(WM_DWMCOLORIZATIONCOLORCHANGED)
    MY_MSG_VALUE(WM_DWMWINDOWMAXIMIZEDCHANGE)
    MY_MSG_VALUE(WM_DWMSENDICONICTHUMBNAIL)
    MY_MSG_VALUE(WM_DWMSENDICONICLIVEPREVIEWBITMAP)
    MY_MSG_VALUE(WM_GETTITLEBARINFOEX)
    MY_MSG_VALUE(WM_HANDHELDFIRST)
    MY_MSG_VALUE(WM_HANDHELDLAST)
    MY_MSG_VALUE(WM_AFXFIRST)
    MY_MSG_VALUE(WM_AFXLAST)
    MY_MSG_VALUE(WM_PENWINFIRST)
    MY_MSG_VALUE(WM_PENWINLAST)
    MY_MSG_VALUE(WM_APP)
    MY_MSG_VALUE(WM_USER)
};

const char* GetMsgName(WORD wID)
{
    int nArySize = sizeof(g_MsgValue) / sizeof(MSG_KEYVALUE);
    for (int i = 0; i < nArySize; i++)
    {
        if (g_MsgValue[i].nID == wID)
        {
            return g_MsgValue[i].szMSG;
        }
    }
    return NULL;
}