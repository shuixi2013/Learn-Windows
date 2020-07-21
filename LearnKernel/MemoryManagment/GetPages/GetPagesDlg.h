
// GetPagesDlg.h : 头文件
//

#pragma once
#include "OperateKernel.h"
#include "afxcmn.h"
#include <Tlhelp32.h>
#include <Psapi.h>

enum LOAD_PAGES
{
	PAGES_LOADED,
	PAGES_UNLOADED,
};

// CGetPagesDlg 对话框
class CGetPagesDlg : public CDialogEx
{
// 构造
public:
	CGetPagesDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GETPAGES_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

protected:
	PAEPaging::PDPTE_TT* m_pTT = NULL;
	COperateKernel m_operKer;

	DWORD m_dwPDPTEIdx = -1;	// PDPTE表下标
	DWORD m_dwPDEIdx = -1;		// PDE表下标
	DWORD m_dwCurPID = -1;
	BOOL InitProcessList();

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	CListCtrl m_lstCtrlProcess;
	CListCtrl m_lstCtrlPDPTE;
	CListCtrl m_lstCtrlPDE;
	CListCtrl m_lstCtrlPTE;
	afx_msg void OnLvnItemchangedGetPagesChoosePID(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemChangedPagesPDPTE(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemChangedPagesPDE(NMHDR *pNMHDR, LRESULT *pResult);

};
