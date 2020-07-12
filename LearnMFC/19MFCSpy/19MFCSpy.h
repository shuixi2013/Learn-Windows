
// 19MFCSpy.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"		// 主符号


// CMy19MFCSpyApp:
// 有关此类的实现，请参阅 19MFCSpy.cpp
//

class CMy19MFCSpyApp : public CWinApp
{
public:
	CMy19MFCSpyApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CMy19MFCSpyApp theApp;
