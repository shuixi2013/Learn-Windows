#pragma once
#include <windows.h>
#include <winsvc.h>
#include <string>

/*
	Pages PAE
*/
namespace PAEPaging 
{
	struct PDPTE
	{
		UINT64 Present : 1;
		UINT64 Reserved1 : 2;
		UINT64 PWT : 1;
		UINT64 PCD : 1;
		UINT64 Reserved2 : 4;
		UINT64 Ignored : 3;
		UINT64 PageDirctoryBaseAddr : 24;
		UINT64 Reserved3 : 28;
	};
	struct PDE
	{
		union 
		{
			UINT64 uint64;
			struct PDE_2M 
			{
				UINT64 Present : 1;
				UINT64 ReadWrite : 1;
				UINT64 Privilege : 1;
				UINT64 PWT : 1;
				UINT64 PCD : 1;
				UINT64 Accessed : 1;
				UINT64 Dirty : 1;
				UINT64 PageSize : 1;
				UINT64 Global : 1;
				UINT64 Ignored : 3;
				UINT64 PAT : 1;
				UINT64 Reserved1 : 8;
				UINT64 PageTableBaseAddr : 15;
				UINT64 Reserved2 : 27;
				UINT64 XD4 : 1;
			}PDE_2M;
			struct PDE_4K
			{
				UINT64 Present : 1;
				UINT64 ReadWrite : 1;
				UINT64 Privilege : 1;
				UINT64 PWT : 1;
				UINT64 PCD : 1;
				UINT64 Accessed : 1;
				UINT64 Ignored1 : 1;
				UINT64 PageSize : 1;
				UINT64 Ignored2 : 4;
				UINT64 PageTableBaseAddr : 24;
				UINT64 Reserved : 27;
				UINT64 XD4 : 1;
			}PDE_4K;
		};
	};
	struct PTE
	{
		UINT64 Present : 1;
		UINT64 ReadWrite : 1;		// 1 RWE   0 RE
		UINT64 Privilege : 1;		// 1 User3环  0System0环
		UINT64 PWT : 1;				// Write-through
		UINT64 PCD : 1;				// Cache disabled
		UINT64 Accessed : 1;
		UINT64 Dirty : 1;
		UINT64 PAT : 1;				// Page Attribute Table Index
		UINT64 GlobalPage : 1;
		UINT64 Ignored : 3;			// Availiable for system programmer's use
		UINT64 PageBaseAddr : 24;
		UINT64 Reserved : 27;
		UINT64 XD4 : 1;
	};
	struct PDE_T
	{
		PDE val;
		PTE PTEs[512];
		BOOL LoadedFlag;
	};

	struct PDPTE_T
	{
		PDPTE val;
		PDE_T PDEs[512];
		BOOL LoadedFlag;
	};

	struct PDPTE_TT
	{
		ULONG cr3Val;
		PDPTE_T PDPTEs[4];
	};
}


#define IOCTL_GET_PAGES_PAE CTL_CODE( \
    FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)


class COperateKernel
{
public:
	COperateKernel();
	~COperateKernel();

private:
	SC_HANDLE m_hSCM = NULL;
	SC_HANDLE m_hService = NULL;

	LPCSTR m_szDeviceName = "\\\\?\\MyTestDevice";
	LPCSTR m_szDriverName = "GetPagesDriver.sys";
	LPCSTR m_szServiceName = "MyTestDeviceServcie";
	std::string m_strDriverFullName;

	// 是否成功安装了驱动，析构时卸载
	BOOL m_bDriverInstalled = FALSE;
	// 安装驱动
	BOOL InstallDriver();
	// 卸载驱动
	BOOL UnInstallDriver();

public:
	// 初始化，装驱动
	BOOL InitOperator();

	// PAE 分页模式：获取进程的所有分页
	BOOL LoadPages(__in DWORD dwPID, __in DWORD dwPDPTEIdx, __in DWORD dwPDEIdx, __out PAEPaging::PDPTE_TT* tt);
};

