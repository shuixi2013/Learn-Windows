#include "OperateKernel.h"

COperateKernel::COperateKernel()
{
	// 获取驱动程序的全路径
	char szFileName[MAX_PATH] = "";
	GetModuleFileNameA(NULL, szFileName, sizeof(szFileName));

	m_strDriverFullName = szFileName;
	int nPos = m_strDriverFullName.rfind('\\');
	if (nPos > 0)
	{
		m_strDriverFullName = m_strDriverFullName.substr(0, nPos + 1);
		m_strDriverFullName.append(m_szDriverName);
	}
}


COperateKernel::~COperateKernel()
{
	if (m_bDriverInstalled)
	{
		UnInstallDriver();
	}
}


BOOL COperateKernel::LoadPages(DWORD dwPID, __in DWORD dwPDPTEIdx, __in DWORD dwPDEIdx, PAEPaging::PDPTE_TT* tt)
{
	if (m_bDriverInstalled == FALSE)
		return FALSE;

	BOOL bRet = FALSE;

	HANDLE hFile = CreateFile(m_szDeviceName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != NULL)
	{
		BYTE inBuff[MAXBYTE] = { 0 };
		*(DWORD*)inBuff = dwPID;
		*(DWORD*)(inBuff + 4) = dwPDPTEIdx;
		*(DWORD*)(inBuff + 8) = dwPDEIdx;

		BYTE outBuff[0x1000];
		DWORD dwBytesRead = 0;
		if (DeviceIoControl(hFile, IOCTL_GET_PAGES_PAE, inBuff, sizeof(inBuff), outBuff, sizeof(outBuff),
			&dwBytesRead, NULL))
		{
			DWORD dwOutBuffOffset = 0;
			// 加载 PDPTE 表
			if (dwPDPTEIdx == -1 && dwPDEIdx == -1)
			{
				for (int i = 0; i < 4; i++)
				{
					PAEPaging::PDPTE *pPDPTE = (PAEPaging::PDPTE*)((DWORD)outBuff + dwOutBuffOffset);
					tt->PDPTEs[i].val = *pPDPTE;
					dwOutBuffOffset += sizeof(PAEPaging::PDPTE);
				}
			}
			// 加载 PDE 表
			else if (dwPDPTEIdx != -1 && dwPDEIdx == -1)
			{
				for (int i = 0; i < 512; i++)
				{
					PAEPaging::PDE *pPDE = (PAEPaging::PDE*)((DWORD)outBuff + dwOutBuffOffset);
					tt->PDPTEs[dwPDPTEIdx].PDEs[i].val.uint64 = pPDE->uint64;
					dwOutBuffOffset += sizeof(PAEPaging::PDE);
				}
			}
			// 加载 PTE 表
			else if (dwPDPTEIdx != -1 && dwPDEIdx != -1)
			{
				for (int i = 0; i < 512; i++)
				{
					PAEPaging::PTE *pPTE = (PAEPaging::PTE*)((DWORD)outBuff + dwOutBuffOffset);
					tt->PDPTEs[dwPDPTEIdx].PDEs[dwPDEIdx].PTEs[i] = *pPTE;
					dwOutBuffOffset += sizeof(PAEPaging::PTE);
				}
			}
			bRet = TRUE;
		}

		CloseHandle(hFile);
	}

	return bRet;
}


BOOL COperateKernel::InitOperator()
{
	if (InstallDriver())
	{
		m_bDriverInstalled = TRUE;
		return TRUE;
	}
	return FALSE;
}


BOOL COperateKernel::InstallDriver()
{

	//1.安装
	m_hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (m_hSCM == NULL)
		return FALSE;

	m_hService = CreateService(
		m_hSCM,							// SCManager database 
		m_szServiceName,				// name of service 
		m_szServiceName,				// service name to display 
		SERVICE_ALL_ACCESS,				// desired access 
		SERVICE_KERNEL_DRIVER,			// service type 
		SERVICE_DEMAND_START,			// start type 
		SERVICE_ERROR_NORMAL,			// error control type 
		m_strDriverFullName.c_str(),	// service's binary 
		NULL,							// no load ordering group 
		NULL,							// no tag identifier 
		NULL,							// no dependencies 
		NULL,							// LocalSystem account 
		NULL);							// no password 

	if (m_hService == NULL)
	{
		DWORD dwErrorCode = GetLastError();
		if (dwErrorCode == 0x431)	//指定的服务已存在
		{
			try {
				m_hService = OpenService(m_hSCM, m_szServiceName, SERVICE_ALL_ACCESS);
				SERVICE_STATUS status;
				ControlService(m_hService, SERVICE_CONTROL_STOP, &status);
				DeleteService(m_hService);
				CloseServiceHandle(m_hService);
			}
			catch (...)
			{

			}

			m_hService = CreateService(
				m_hSCM, m_szServiceName, m_szServiceName,
				SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
				SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
				m_strDriverFullName.c_str(), NULL, NULL, NULL, NULL, NULL);
			if (m_hService == NULL)
			{
				CloseServiceHandle(m_hSCM);
				m_hSCM = NULL;
				return FALSE;
			}
		}
	}

	//2.启动
	if (!StartService(m_hService, 0, NULL))
	{
		try {
			SERVICE_STATUS status;
			ControlService(m_hService, SERVICE_CONTROL_STOP, &status);
			DeleteService(m_hService);
			CloseServiceHandle(m_hService);
			CloseServiceHandle(m_hSCM);
		}
		catch (...)
		{

		}
		m_hService = NULL;
		m_hSCM = NULL;
		return FALSE;
	}
	m_bDriverInstalled = TRUE;
	return TRUE;
}

BOOL COperateKernel::UnInstallDriver()
{
	BOOL bRet = FALSE;
	if (m_hService != NULL)
	{
		//3.停止
		SERVICE_STATUS status;
		ControlService(m_hService, SERVICE_CONTROL_STOP, &status);

		//4.卸载
		if (DeleteService(m_hService))
		{
			bRet = TRUE;
		}

		if (m_hService != NULL)
		{
			CloseServiceHandle(m_hService);
			m_hService = NULL;
		}

		if (m_hSCM != NULL)
		{
			CloseServiceHandle(m_hSCM);
			m_hSCM = NULL;
		}
	}
	return bRet;
}



