
// TestMFCView.h: CTestMFCView 类的接口
//

#pragma once


class CTestMFCView : public CView
{
protected: // 仅从序列化创建
	CTestMFCView() noexcept;
	DECLARE_DYNCREATE(CTestMFCView)

// 特性
public:
	CTestMFCDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// 实现
public:
	virtual ~CTestMFCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMenu1();
	afx_msg void OnMenu2();
};

#ifndef _DEBUG  // TestMFCView.cpp 中的调试版本
inline CTestMFCDoc* CTestMFCView::GetDocument() const
   { return reinterpret_cast<CTestMFCDoc*>(m_pDocument); }
#endif

