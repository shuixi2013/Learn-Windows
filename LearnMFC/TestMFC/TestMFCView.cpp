
// TestMFCView.cpp: CTestMFCView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "TestMFC.h"
#endif

#include "TestMFCDoc.h"
#include "TestMFCView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestMFCView

IMPLEMENT_DYNCREATE(CTestMFCView, CView)

BEGIN_MESSAGE_MAP(CTestMFCView, CView)
	// 标准打印命令
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
	ON_COMMAND(ID_MENU1, &CTestMFCView::OnMenu1)
	ON_COMMAND(ID_MENU2, &CTestMFCView::OnMenu2)
END_MESSAGE_MAP()

// CTestMFCView 构造/析构

CTestMFCView::CTestMFCView() noexcept
{
	// TODO: 在此处添加构造代码

}

CTestMFCView::~CTestMFCView()
{
}

BOOL CTestMFCView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CTestMFCView 绘图

void CTestMFCView::OnDraw(CDC* /*pDC*/)
{
	CTestMFCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
}


// CTestMFCView 打印

BOOL CTestMFCView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// 默认准备
	return DoPreparePrinting(pInfo);
}

void CTestMFCView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加额外的打印前进行的初始化过程
}

void CTestMFCView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: 添加打印后进行的清理过程
}


// CTestMFCView 诊断

#ifdef _DEBUG
void CTestMFCView::AssertValid() const
{
	CView::AssertValid();
}

void CTestMFCView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTestMFCDoc* CTestMFCView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTestMFCDoc)));
	return (CTestMFCDoc*)m_pDocument;
}
#endif //_DEBUG


// CTestMFCView 消息处理程序


void CTestMFCView::OnMenu1()
{
	AfxMessageBox(_T("OnMenu1"));
}


void CTestMFCView::OnMenu2()
{
	AfxMessageBox(_T("OnMenu2"));
}
