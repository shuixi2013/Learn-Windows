## 前言
*RTTI* 和 *消息映射* 是MFC框架中两个重要的设计，了解它们的原理，再通过逆向定位到MFC程序中的消息处理函数，则信手拈来。

本文中识别消息处理函数的工具针对mfc 14.0版本，更低版本的MFC程序，需要稍加修改代码，或参考支持更多MFC版本的[工具](https://bbs.pediy.com/thread-204746.htm)

如果不知道逆向是什么，可以看这里[看雪论坛](https://bbs.pediy.com/)

这是广告：给我们可爱的老师宣传一下，笔者在[科锐](http://51asm.com/)学习逆向，老师是《C++反汇编与逆向分析技术揭秘》的作者 钱林松，如果对逆向感兴趣，这个地方是极好的！
<br>

## 环境准备
使用VS2019新建一个MFC单文档工程，命名为 TestMFC ，并且添加两个菜单，名字为"菜单1“和"菜单2"，菜单的处理函数在TestMFCView中实现，如下：

```cpp
// TestMFCView.h
class CTestMFCView : public CView
{
...
public:
	afx_msg void OnMenu1();
	afx_msg void OnMenu2();
}

// TestMFCView.cpp
BEGIN_MESSAGE_MAP(CTestMFCView, CView)
	...
	ON_COMMAND(ID_MENU1, &CTestMFCView::OnMenu1)
	ON_COMMAND(ID_MENU2, &CTestMFCView::OnMenu2)
END_MESSAGE_MAP()

void CTestMFCView::OnMenu1()
{
	AfxMessageBox(_T("OnMenu1"));
}

void CTestMFCView::OnMenu2()
{
	AfxMessageBox(_T("OnMenu2"));
}	
```
逆向MFC程序，首先是要找到用户定义的消息处理函数，之后才能通过调试工具或反编译工具分析代码。本文只涉及前半部分，通过学习MFC框架中的两个设计，实现快速定位消息处理函数。

以这个程序为例，就是要找到 *OnMenu1* 和 *OnMenu2* 函数在程序中的地址。
<br>


## RTTI
<br>

### 1. 什么是RTTI
RTTI的全称是 Run-Time Type Identification，也就是运行时类识别。
通俗的讲，就是对于框架的代码而言，它要创建一些不知道类名的类（比如用户定义的类），但是又不能在代码中写具体哪个类名，否则框架的代码岂不是要经常改。

所谓运行时类识别，就是为了解决这样的问题：让框架的代码不变，还能创建某些用户定义的类。

如果你知道类工厂模式，那就会知道，可以通过为所有的类实现一个函数 *CreateObject* ，不同的类调用自己的 *CreateObject* 函数，返回自己类的堆对象。
<br>

### 2. MFC中使用RTTI创建 Doc 和 View 类的过程
在 TestMFC 工程中，定义了 ***CTestMFCApp*** 的全局对象，在MFC程序的入口函数 *AfxWinMain* 中会调用该类重写父类的虚函数 *InitInstance* 实现程序的初始化工作。
但是 ***CTestMFCDoc*** 和 ***CTestMFCView*** 是什么时候，怎么创建的？
<br>

先看一下 *CTestMFCDoc*  和 *CTsetMFCView* 类中都定义了的宏 *DECLARE_DYNCREATE* 和  *IMPLEMENT_DYNCREATE*，如果将宏展开，会得到下面的代码
```cpp
// DECLARE_DYNCREATE(CTestMFCDoc) 宏展开
public:
	static const CRuntimeClass classCTestMFCDoc;
	virtual CRuntimeClass* GetRuntimeClass() const;
	static CObject* PASCAL CreateObject();

// IMPLEMENT_DYNCREATE(CTestMFCDoc, CDocument) 宏展开
CObject* PASCAL CTestMFCDoc::CreateObject()
	{  return new CTestMFCDoc; }

const CRuntimeClass CTestMFCDoc::classCTestMFCDoc = { 
		"CTestMFCDoc", 
		sizeof(class CTestMFCDoc), 
		0xFFFF, 
		CTestMFCDoc::CreateObject,
		((CRuntimeClass*)(&CDocument::classCDocument)), 
		NULL, 
		NULL
	};
	
CRuntimeClass* CTestMFCDoc::GetRuntimeClass() const
	{ return ((CRuntimeClass*)(&CTestMFCDoc::classCTestMFCDoc)); }

//... CTsetMFCView 宏展开的代码与 CTsetMFCDoc 类似，这里省略

// MFC框架中定义的类 CRuntimeClass
struct CRuntimeClass
{
// Attributes
	LPCSTR m_lpszClassName;
	int m_nObjectSize;
	UINT m_wSchema; // schema number of the loaded class
	CObject* (PASCAL* m_pfnCreateObject)(); // NULL => abstract class
	CRuntimeClass* m_pBaseClass;

// Operations
	CObject* CreateObject();
	BOOL IsDerivedFrom(const CRuntimeClass* pBaseClass) const;
	...
}
```
结合上面宏展开的代码，我们以*CTestMFCDoc* 对象为例，在MFC框架代码中跟踪一下它的创建过程：
```cpp
CTestMFCApp::InitInstance()
{
	...
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
			IDR_MAINFRAME,
			RUNTIME_CLASS(CTestMFCDoc),			//重要：保存CTestMFCDoc::classCTestMFCDoc全局对象地址到CDocTemplate::m_pDocClass
			RUNTIME_CLASS(CMainFrame),
			RUNTIME_CLASS(CTestMFCView));
	...
	if (!ProcessShellCommand(cmdInfo))			// 步进 该函数
	...
}

//跟踪调用堆栈，调用顺序是自上而下
>	TestMFC.exe!CWinApp::ProcessShellCommand(CCommandLineInfo & rCmdInfo) 行 35	C++
>	TestMFC.exe!CCmdTarget::OnCmdMsg(unsigned int nID, int nCode, void * pExtra, AFX_CMDHANDLERINFO * pHandlerInfo) 行 372	C++
>	TestMFC.exe!_AfxDispatchCmdMsg(CCmdTarget * pTarget, unsigned int nID, int nCode, void(CCmdTarget::*)() pfn, void * pExtra, unsigned int nSig, AFX_CMDHANDLERINFO * pHandlerInfo) 行 77	C++
>	TestMFC.exe!CWinApp::OnFileNew() 行 21	C++
>	TestMFC.exe!CDocManager::OnFileNew() 行 912	C++
>	TestMFC.exe!CSingleDocTemplate::OpenDocumentFile(const wchar_t * lpszPathName, int bMakeVisible) 行 83	C++
>	TestMFC.exe!CSingleDocTemplate::OpenDocumentFile(const wchar_t * lpszPathName, int bAddToMRU, int bMakeVisible) 行 113	C++
>	TestMFC.exe!CDocTemplate::CreateNewDocument() 行 245	C++

CDocument* CDocTemplate::CreateNewDocument()
{
	...
	CDocument* pDocument = (CDocument*)m_pDocClass->CreateObject();	
	//m_pDocClass 中保存的是 CTestMFCDoc::classCTestMFCDoc 全局对象地址
	//这里就是调用成员函数CRuntimeClass::CreateObject()
	...
}

CObject* CRuntimeClass::CreateObject()
{
	...
	TRY
	{
		pObject = (*m_pfnCreateObject)();	
		//m_pfnCreateObject 中保存的是CTestMFCDoc::CreateObject 函数指针
		//调用后返回 CTestMFCDoc 的堆对像地址
	}
	...
}
```
总结来说，就是 *CTestMFCDoc* 类中定义了一个*CRuntimeClass* 类的静态全局对象 ，该对象保存有创建*CTestMFCDoc* 类堆对象的函数指针（可以理解为 *CTestMFCDoc* 类工厂函数的函数指针），MFC框架通过调用类成员函数*CRuntimeClass::CreateObject()* ，实现动态创建。

CTestView 的创建过程，可以在 *IMPLEMENT_DYNCREATE(CTestView, CView)* 代码处下断点，通过调用堆栈就可以跟踪到，这里不做展开。
<br>

### 3. 有关逆向的点
从第2步中可知，*CRuntimeClass*  的成员 *m_lpszClassName*  保存了类的名称，所以在逆向MFC程序时，对于一个名字未知的对象（比如*CTestMFCDoc* 对象），如果我们能够获得它的*CRuntimeClass*对象，那么访问它第一个成员，就可以获得这个类的名称。
```cpp
const CRuntimeClass CTestMFCDoc::classCTestMFCDoc = { 
		"CTestMFCDoc", 
		...
	};
```
<br>

## 消息映射机制
在写MFC程序时，可以在VS的“类向导”中添加消息处理函数，也可以重写某个类的虚函数，最终VS都会帮我们在 BEGIN_MESSAGE_MAP中添加下面这样的宏：

```cpp
// TestMFCView.cpp
BEGIN_MESSAGE_MAP(CTestMFCView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_COMMAND(ID_MENU1, &CTestMFCView::OnMenu1)
	ON_COMMAND(ID_MENU2, &CTestMFCView::OnMenu2)
	ON_WM_CREATE()		// 重写 CWnd::OnCreate 函数
END_MESSAGE_MAP()

// 宏展开代码
const AFX_MSGMAP* CTestMFCView::GetMessageMap() const 
{ 
	return GetThisMessageMap(); 
}

const AFX_MSGMAP* __stdcall CTestMFCView::GetThisMessageMap() 
{ 
	typedef CTestMFCView ThisClass; 
	typedef CView TheBaseClass; 
	static const AFX_MSGMAP_ENTRY _messageEntries[] = {
		
			{ 0x0111, 0, (WORD)0xE107, (WORD)0xE107, AfxSigCmd_v, static_cast<AFX_PMSG> (&CView::OnFilePrint) },
			{ 0x0111, 0, (WORD)0xE108, (WORD)0xE108, AfxSigCmd_v, static_cast<AFX_PMSG> (&CView::OnFilePrint) },
			{ 0x0111, 0, (WORD)0xE109, (WORD)0xE109, AfxSigCmd_v, static_cast<AFX_PMSG> (&CView::OnFilePrintPreview) },
			{ 0x0111, 0, (WORD)32773, (WORD)32773, AfxSigCmd_v, static_cast<AFX_PMSG> (&CTestMFCView::OnMenu1) },
			{ 0x0111, 0, (WORD)32774, (WORD)32774, AfxSigCmd_v, static_cast<AFX_PMSG> (&CTestMFCView::OnMenu2) },
			{ 0x0001, 0, 0, 0, AfxSig_is, (AFX_PMSG) (AFX_PMSGW) (static_cast< int ( CWnd::*)(LPCREATESTRUCT) > ( &ThisClass :: OnCreate)) },
			{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 } 
	}; 

	static const AFX_MSGMAP messageMap = { &TheBaseClass::GetThisMessageMap, &_messageEntries[0] }; 
	return &messageMap; 
}
```
可以看到，*CTestMFCView::GetMessageMap()* 函数会返回 *messageMap*  静态局部对象地址。实际上是重写了祖先类的虚函数 *CWnd::GetMessageMap()*。
接下来需要在窗口过程函数的代码中，看一下如何获取这个对象以及它的用途。

```cpp
LRESULT CALLBACK
AfxWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	// special message which identifies the window as using AfxWndProc
	if (nMsg == WM_QUERYAFXWNDPROC)
		return 1;

	// all other messages route through message map
	CWnd* pWnd = CWnd::FromHandlePermanent(hWnd);	//通过传入的窗口句柄hWnd获得对应的对象：CMainFrame、CTestMFCView等
	ASSERT(pWnd != NULL);
	ASSERT(pWnd==NULL || pWnd->m_hWnd == hWnd);
	if (pWnd == NULL || pWnd->m_hWnd != hWnd)
		return ::DefWindowProc(hWnd, nMsg, wParam, lParam);
	return AfxCallWndProc(pWnd, hWnd, nMsg, wParam, lParam);	// 步进 该调用
}
```
### 1. 获取窗口对象
*CWnd::FromHandlePermanent(hWnd)* 可以获得窗口对应的对象，这个函数对逆向也很重要。实际上，MFC框架定义了一个全局的对象 *CHandleMap* 用来保存 *<HANDLE, CObject*>* 键值对，在MFC窗口创建时，会将窗口句柄以及对应的对象加入这个表中。而  *FromHandlePermanent*  正是从这个表中获取到对象。

关于保存键值对还有很多的细节，可以在下面创建窗口调用之前看到 *AfxHookWindowCreate(this)* 代码，在里面设置了 *WH_CBT* 消息钩子，在窗口创建之后，会调用钩子设置的回调函数 *_AfxCbtFilterHook*，回调函数中负责保存 *<HANDLE, CObject*>* 的键值对。

注意：保存的键值对数据是线程相关的，必须在AfxWndProc 窗口回调的线程中才可以拿到数据。

```cpp
// wincore.cpp
BOOL CWnd::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
	LPCTSTR lpszWindowName, DWORD dwStyle,
	int x, int y, int nWidth, int nHeight,
	HWND hWndParent, HMENU nIDorHMenu, LPVOID lpParam)
{
	...
	AfxHookWindowCreate(this);
	HWND hWnd = CreateWindowEx(cs.dwExStyle, cs.lpszClass,
			cs.lpszName, cs.style, cs.x, cs.y, cs.cx, cs.cy,
			cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);
	...
}
```
在逆向时需要获取窗口对象，就是通过调用 *FromHandlePermanent* 函数，所以一般逆向静态编译的MFC程序时，可以定位到 *AfxWndProc* 函数的地址，再计算出 *FromHandlePermanent* 函数地址。如果使用的是MFC动态库，则可以找到dll中此函数的序号，直接调用即可。在"MFCSpy工具"中可以查看具体代码。
<br>
### 2. 遍历消息映射表
以 *CTestMFCView::OnMenu1*  消息为例，在代码中下断点，
可以看到 *AfWndProc* 窗口函数是如何分发消息的，调用堆栈如下：

```cpp
>	TestMFC.exe!AfxWndProc(HWND__ * hWnd, unsigned int nMsg, unsigned int wParam, long lParam) 行 418	C++
>	TestMFC.exe!AfxCallWndProc(CWnd * pWnd, HWND__ * hWnd, unsigned int nMsg, unsigned int wParam, long lParam) 行 265	C++
>	TestMFC.exe!CWnd::OnWndMsg(unsigned int message, unsigned int wParam, long lParam, long * pResult) 行 2113	C++
>	TestMFC.exe!CFrameWnd::OnCommand(unsigned int wParam, long lParam) 行 384	C++
>	TestMFC.exe!CWnd::OnCommand(unsigned int wParam, long lParam) 行 2800	C++
>	TestMFC.exe!CFrameWnd::OnCmdMsg(unsigned int nID, int nCode, void * pExtra, AFX_CMDHANDLERINFO * pHandlerInfo) 行 980	C++
>	TestMFC.exe!CView::OnCmdMsg(unsigned int nID, int nCode, void * pExtra, AFX_CMDHANDLERINFO * pHandlerInfo) 行 164	C++
>	TestMFC.exe!CCmdTarget::OnCmdMsg(unsigned int nID, int nCode, void * pExtra, AFX_CMDHANDLERINFO * pHandlerInfo) 行 372	C++

BOOL CCmdTarget::OnCmdMsg(UINT nID, int nCode, void* pExtra,
	AFX_CMDHANDLERINFO* pHandlerInfo)
{
	...
	for (pMessageMap = GetMessageMap(); pMessageMap->pfnGetBaseMap != NULL;
			pMessageMap = (*pMessageMap->pfnGetBaseMap)())
	{
		// Note: catches BEGIN_MESSAGE_MAP(CMyClass, CMyClass)!
		ASSERT(pMessageMap != (*pMessageMap->pfnGetBaseMap)());
		lpEntry = AfxFindMessageEntry(pMessageMap->lpEntries, nMsg, nCode, nID);
		if (lpEntry != NULL)
		{
			// found it
			return _AfxDispatchCmdMsg(this, nID, nCode,
				lpEntry->pfn, pExtra, lpEntry->nSig, pHandlerInfo);
		}
	}
	...
}
```
上面的 *pMessageMap = GetMessageMap()* 是虚函数调用，就是第1步中在 *AfxWndProc* 中获取的对象调用其虚函数 *CTestView::GetMessageMap()* ，获取到它的类中定义的 *messageMap* 静态局部对象地址。
for循环中代码是遍历 *messageMap*，如果匹配到相同的控件编号 *messageMap._messageEntries[i].nID*，就取出消息处理的函数指针并调用。

逆向时，只要获取到MFC窗口对象的 *GetMessageMap()* 函数，就可以遍历它的消息映射表，列出所有的消息处理函数。
<br>

## MFCSpy工具

实现工具的流程大概如下：

**19MFCSpy**
1. 获取窗口句柄，CMyPictrueCtrl 中实现
2. CreateRemoteThread注入19MFCSpyDll.dll到目标窗口进程，CMy19MFCSpyDlg::OnBnClickedSpy
3. 等待注入的代码解析数据完成的事件
4. 显示结果
5. 卸载dll

**19MFCSpyDll**
1. 替换MFC窗口过程函数AfxWndProc为MyWindowProc
2. 当MyWindowProc被调用时，调用FromHandlePermanent获取对象地址。
3. 解析数据放到dll共享数据区，并通知窗口程序处理结束

使用效果如下：

![在这里插入图片描述](https://img-blog.csdnimg.cn/20200714115006146.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3dlaXhpbl8zODUyNjE4MA==,size_16,color_FFFFFF,t_70)
