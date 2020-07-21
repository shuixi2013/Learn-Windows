
// GetPagesDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "GetPages.h"
#include "GetPagesDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CGetPagesDlg 对话框



CGetPagesDlg::CGetPagesDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_GETPAGES_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGetPagesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, LST_PAGES_PDPTE, m_lstCtrlPDPTE);
	DDX_Control(pDX, LST_PAGES_PDE, m_lstCtrlPDE);
	DDX_Control(pDX, LST_PAGES_PTE, m_lstCtrlPTE);
	DDX_Control(pDX, LST_GETPAGES_PID, m_lstCtrlProcess);
}

BEGIN_MESSAGE_MAP(CGetPagesDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(LVN_ITEMCHANGED, LST_PAGES_PDPTE, &CGetPagesDlg::OnLvnItemChangedPagesPDPTE)
	ON_NOTIFY(LVN_ITEMCHANGED, LST_PAGES_PDE, &CGetPagesDlg::OnLvnItemChangedPagesPDE)
	ON_NOTIFY(LVN_ITEMCHANGED, LST_GETPAGES_PID, &CGetPagesDlg::OnLvnItemchangedGetPagesChoosePID)
END_MESSAGE_MAP()


// CGetPagesDlg 消息处理程序

BOOL CGetPagesDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	DWORD dwStyle = m_lstCtrlProcess.GetExtendedStyle();
	m_lstCtrlProcess.SetExtendedStyle(dwStyle | LVS_EX_FULLROWSELECT);
	m_lstCtrlProcess.InsertColumn(0, _T("进程名称"), LVCFMT_LEFT, 180);
	m_lstCtrlProcess.InsertColumn(1, _T("PID"), LVCFMT_LEFT, 50);

	dwStyle = m_lstCtrlPDPTE.GetExtendedStyle();
	m_lstCtrlPDPTE.SetExtendedStyle(dwStyle | LVS_EX_FULLROWSELECT);
	m_lstCtrlPDPTE.InsertColumn(0, _T("PDPTE"), LVCFMT_LEFT, 50);
	m_lstCtrlPDPTE.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 150);

	dwStyle = m_lstCtrlPDE.GetExtendedStyle();
	m_lstCtrlPDE.SetExtendedStyle(dwStyle | LVS_EX_FULLROWSELECT);
	m_lstCtrlPDE.InsertColumn(0, _T("PDE"), LVCFMT_LEFT, 40);
	m_lstCtrlPDE.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 150);
	m_lstCtrlPDE.InsertColumn(2, _T("Size"), LVCFMT_LEFT, 33);
	m_lstCtrlPDE.InsertColumn(3, _T("U/S"), LVCFMT_LEFT, 33);
	m_lstCtrlPDE.InsertColumn(4, _T("R/W"), LVCFMT_LEFT, 35);

	dwStyle = m_lstCtrlPTE.GetExtendedStyle();
	m_lstCtrlPTE.SetExtendedStyle(dwStyle | LVS_EX_FULLROWSELECT);
	m_lstCtrlPTE.InsertColumn(0, _T("PTE"), LVCFMT_LEFT, 40);
	m_lstCtrlPTE.InsertColumn(1, _T("Value"), LVCFMT_LEFT, 150);
	m_lstCtrlPTE.InsertColumn(2, _T("U/S"), LVCFMT_LEFT, 33);
	m_lstCtrlPTE.InsertColumn(3, _T("R/W"), LVCFMT_LEFT, 35);

	// 初始化进程 CListCtrl
	if (!InitProcessList())
	{
		AfxMessageBox("初始化进程列表失败");
	}

	// 安装驱动
	if (!m_operKer.InitOperator())
	{
		AfxMessageBox("安装驱动失败");
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CGetPagesDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CGetPagesDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CGetPagesDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CGetPagesDlg::InitProcessList()
{
	HANDLE         hProcessSnap = NULL;
	BOOL           bRet = FALSE;
	PROCESSENTRY32 pe32 = { 0 };

	//  Take a snapshot of all processes in the system. 
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return (FALSE);

	//  Fill in the size of the structure before using it. 
	pe32.dwSize = sizeof(PROCESSENTRY32);

	//  Walk the snapshot of the processes, and for each process, 
	//  display information. 
	if (Process32First(hProcessSnap, &pe32))
	{
		int nLstIdx = 0;
		while (Process32Next(hProcessSnap, &pe32))
		{
			m_lstCtrlProcess.InsertItem(nLstIdx, pe32.szExeFile);

			CString csPID;
			csPID.Format("%d", pe32.th32ProcessID);
			m_lstCtrlProcess.SetItemText(nLstIdx, 1, csPID);

			nLstIdx++;
		}
		bRet = TRUE;
	}
	else
		bRet = FALSE;    // could not walk the list of processes 

						 // Do not forget to clean up the snapshot object. 
	CloseHandle(hProcessSnap);
	return (bRet);
}


void CGetPagesDlg::OnLvnItemchangedGetPagesChoosePID(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	if (pNMLV->iItem != -1)
	{
		m_lstCtrlPDPTE.DeleteAllItems();
		m_lstCtrlPDE.DeleteAllItems();
		m_lstCtrlPTE.DeleteAllItems();

		int row = pNMLV->iItem;

		// 获取PID
		CString csPID = m_lstCtrlProcess.GetItemText(row, 1);
		m_dwCurPID = _ttoi(csPID);

		// 初始化数据
		if (m_pTT != NULL)
		{
			delete m_pTT;
			m_pTT = NULL;
		}

		m_pTT = new PAEPaging::PDPTE_TT;
		BOOL bRet = m_operKer.LoadPages(m_dwCurPID, -1, -1, m_pTT);

		// 显示数据
		if (bRet)
		{
			int lstIdx = 0;
			for (int i = 0; i < 4; i++)
			{
				// 校验 Present 位
				if (m_pTT->PDPTEs[i].val.Present == 0)
					continue;

				m_pTT->PDPTEs[i].LoadedFlag = FALSE;

				CString cs;
				cs.Format("%d", i);
				m_lstCtrlPDPTE.InsertItem(lstIdx, cs);

				cs.Format(_T("%p`%p"), *(UINT*)((ULONG)(&(m_pTT->PDPTEs[i])) + 4),
					*(UINT*)((ULONG)(&(m_pTT->PDPTEs[i]))));
				m_lstCtrlPDPTE.SetItemText(lstIdx, 1, cs);

				lstIdx++;
			}
		}
	}
}


void CGetPagesDlg::OnLvnItemChangedPagesPDPTE(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	if (pNMLV->iItem != -1)
	{
		m_lstCtrlPDE.DeleteAllItems();
		m_lstCtrlPTE.DeleteAllItems();

		int row = pNMLV->iItem;

		//记录PDPTE表下标
		CString csPDPTEIdx = m_lstCtrlPDPTE.GetItemText(row, 0);
		m_dwPDPTEIdx = _ttoi(csPDPTEIdx);

		// 判断是否已经加载过了页表
		if (m_pTT->PDPTEs[m_dwPDPTEIdx].LoadedFlag == FALSE)
		{
			if(m_operKer.LoadPages(m_dwCurPID, m_dwPDPTEIdx, -1, m_pTT))
				m_pTT->PDPTEs[m_dwPDPTEIdx].LoadedFlag = TRUE;
		}

		// 显示数据
		if (m_pTT->PDPTEs[m_dwPDPTEIdx].LoadedFlag == TRUE)
		{
			int lstIdx = 0;
			for (int i = 0; i < 512; i++)
			{
				// 校验 Present 位
				if (m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[i].val.PDE_4K.Present == 0)
					continue;

				m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[i].LoadedFlag = FALSE;

				// 判断是2M页或4K页
				CString cs;
				cs.Format(_T("%d"), i);
				m_lstCtrlPDE.InsertItem(lstIdx, cs);
				if (m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[i].val.PDE_2M.PageSize == 1)
				{
					m_lstCtrlPDE.SetItemText(lstIdx, 2, _T("2M"));
				}
				else
				{
					m_lstCtrlPDE.SetItemText(lstIdx, 2, _T("4K"));
				}

				cs.Format(_T("%p`%p"), *(UINT*)((ULONG)(&(m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[i])) + 4),
					*(UINT*)((ULONG)(&(m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[i]))));
				m_lstCtrlPDE.SetItemText(lstIdx, 1, cs);

				cs.Format(_T("%d"), m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[i].val.PDE_4K.Privilege);
				m_lstCtrlPDE.SetItemText(lstIdx, 3, cs);

				cs.Format(_T("%d"), m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[i].val.PDE_4K.ReadWrite);
				m_lstCtrlPDE.SetItemText(lstIdx, 4, cs);

				lstIdx++;
			}
		}
	}
}


void CGetPagesDlg::OnLvnItemChangedPagesPDE(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	if (pNMLV->iItem != -1)
	{
		m_lstCtrlPTE.DeleteAllItems();

		int row = pNMLV->iItem;

		//记录PDE表下标
		CString cs = m_lstCtrlPDE.GetItemText(row, 0);
		int m_dwPDEIdx = _ttoi(cs);

		// 判断是否已经加载过了页表
		if (m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].LoadedFlag == FALSE)
		{
			if(m_operKer.LoadPages(m_dwCurPID, m_dwPDPTEIdx, m_dwPDEIdx, m_pTT))
				m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].LoadedFlag = TRUE;
		}

		// 显示数据
		if (m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].LoadedFlag == TRUE)
		{
			// 2M or 4K page
			if (m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].val.PDE_2M.PageSize == 1)
			{
				return;
			}

			int lstIdx = 0;
			for (int i = 0; i < 512; i++)
			{
				// 校验 Present 位
				if (m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].PTEs[i].Present == 0)
					continue;

				// 判断是2M页或4K页
				CString cs;
				cs.Format(_T("%d"), i);
				m_lstCtrlPTE.InsertItem(lstIdx, cs);

				cs.Format(_T("%p`%p"), *(UINT*)((ULONG)(&(m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].PTEs[i])) + 4),
					*(UINT*)((ULONG)(&(m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].PTEs[i]))));
				m_lstCtrlPTE.SetItemText(lstIdx, 1, cs);

				cs.Format(_T("%d"), m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].PTEs[i].Privilege);
				m_lstCtrlPTE.SetItemText(lstIdx, 2, cs);

				cs.Format(_T("%d"), m_pTT->PDPTEs[m_dwPDPTEIdx].PDEs[m_dwPDEIdx].PTEs[i].ReadWrite);
				m_lstCtrlPTE.SetItemText(lstIdx, 3, cs);
				lstIdx++;
			}
		}
	}

}

