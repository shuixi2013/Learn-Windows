#include "GetPagesDriver.h"


NTSTATUS DispatchControl(IN PDEVICE_OBJECT pDeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG nOutLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;    //输出缓冲区，给三环传数据
    ULONG nInLength =  pIrpStack->Parameters.DeviceIoControl.InputBufferLength;     //三环传入的缓冲区大小
    ULONG nIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;     //控制码
    PVOID pInBuff = Irp->AssociatedIrp.SystemBuffer;		//输入缓冲区的地址
    PVOID pOutBuff = Irp->AssociatedIrp.SystemBuffer;       //输出缓冲区的地址
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG nBytes = 0;

    switch(nIoControlCode)
    {
    case IOCTL_GET_PAGES_PAE:
    {
        status = GetProcessPages(pInBuff, pOutBuff, nOutLength, &nBytes);
        break;
    }
    }
    
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = nBytes;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

ULONG64 GetQuadByPA(ULONG64 llPA)
{
    PHYSICAL_ADDRESS pa;
    pa.QuadPart = llPA;
    //映射出虚拟地址
    PVOID pVA = MmMapIoSpace(pa, 8, MmNonCached);
    if(pVA == NULL)
    {
        return 0;
    }
    //取值
    ULONG64 llRet = *(ULONG64*)pVA;
    //Unmap
    MmUnmapIoSpace(pVA, 8);
    return llRet;
}


// 根据进程ID获取进程页表
NTSTATUS GetProcessPages(PVOID pInBuff, PVOID pOutBuff, ULONG nOutLength, ULONG* nBytes)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    *nBytes = 0;
    
    if(MmIsAddressValid(pInBuff) && MmIsAddressValid(pOutBuff) && nOutLength >= 0x1000)
    {
        ULONG nPID = *(ULONG*)pInBuff;
        ULONG nPDPTEIdx = *(ULONG*)((ULONG)pInBuff + 4);
        ULONG nPDEIdx = *(ULONG*)((ULONG)pInBuff + 8);
        DbgPrint("nPID=%d  nPDPTEIdx=%d  nPDEIdx=%d\r\n", nPID, nPDPTEIdx, nPDEIdx);
        
        //根据PID拿到EPROCESS,再获取cr3
        PEPROCESS pEPROCESS;
        NTSTATUS status2 = PsLookupProcessByProcessId((HANDLE)nPID, &pEPROCESS);
        if (NT_SUCCESS(status2))
        {
            ULONG paCR3 = *(PULONG)((ULONG)pEPROCESS + 0x18);
            if(paCR3 != NULL)
            {
                ULONG nWriteOffset = 0;
                // 加载 PDPTE 表
                if (nPDPTEIdx == -1 && nPDEIdx == -1)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        ULONG64 paPDPTE = paCR3 + 8 * i;
                        ULONG64 PDPTE = GetQuadByPA(paPDPTE);
                        *(ULONG64*)((ULONG)pOutBuff + nWriteOffset) = PDPTE;
                        nWriteOffset += sizeof(ULONG64);
                    }
                }
                // 加载 PDE 表
                else if (nPDPTEIdx != -1 && nPDEIdx == -1)
                {
                    ULONG64 paPDPTE = paCR3 + 8 * nPDPTEIdx;
                    ULONG64 PDPTE = GetQuadByPA(paPDPTE) & 0xFFFFFF000;
                    for (int i = 0; i < 512; i++)
                    {
                        ULONG64 paPDE = PDPTE + 8 * i;
                        ULONG64 PDE = GetQuadByPA(paPDE);
                        *(ULONG64*)((ULONG)pOutBuff + nWriteOffset) = PDE;
                        nWriteOffset += sizeof(ULONG64);
                    }
                }
                // 加载 PTE 表
                else if (nPDPTEIdx != -1 && nPDEIdx != -1)
                {
                    ULONG64 paPDPTE = paCR3 + 8 * nPDPTEIdx;
                    ULONG64 PDPTE = GetQuadByPA(paPDPTE) & 0xFFFFFF000;
                    ULONG64 paPDE = PDPTE + 8 * nPDEIdx;
                    ULONG64 PDE = GetQuadByPA(paPDE) & 0xFFFFFF000;
                    for (int i = 0; i < 512; i++)
                    {
                        ULONG64 paPTE = PDE + 8 * i;
                        ULONG64 PTE = GetQuadByPA(paPTE);
                        *(ULONG64*)((ULONG)pOutBuff + nWriteOffset) = PTE;
                        nWriteOffset += sizeof(ULONG64);
                    }
                }
                status = STATUS_SUCCESS;
                *nBytes = nWriteOffset;
            }
        }   
    }
    return status;
}


NTSTATUS DriverEntry(
    __in struct _DRIVER_OBJECT *DriverObject,
    __in PUNICODE_STRING RegistryPath
)
{
    DbgPrint("GetPagesDriver Hello WDK! pid=%d tid=%d\r\n", PsGetCurrentProcessId(), PsGetCurrentThreadId());
    
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;

    DriverObject->DriverUnload = DriverUnload;
    
    UNICODE_STRING ustrDevName;
    RtlInitUnicodeString(&ustrDevName, L"\\Device\\MyTestDevice");
    
    PDEVICE_OBJECT pDevObject = NULL;
    NTSTATUS status = IoCreateDevice(
        DriverObject, 
        0, 
        &ustrDevName, 
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN, 
        FALSE, 
        &pDevObject);
    if(!NT_SUCCESS(status))
    {
        DbgPrint("GetPagesDriver IoCreateDevice Error:%d\r\n", status);
        return status;
    }
    DbgPrint("GetPagesDriver IoCreateDevice OK pDevObject=%p\r\n", pDevObject);
    
    //指定缓冲区通讯方式
    pDevObject->Flags |= DO_BUFFERED_IO;
    
    // 创建符号链接
    UNICODE_STRING ustrSymbolName;
    RtlInitUnicodeString(&ustrSymbolName, L"\\DosDevices\\MyTestDevice");
    status = IoCreateSymbolicLink(&ustrSymbolName, &ustrDevName);
    if(!NT_SUCCESS(status))
    {
        DbgPrint("GetPagesDriver IoCreateSymbolicLink Error:%d\r\n", status);
        IoDeleteDevice(pDevObject);
        return status;
    }
    DbgPrint("GetPagesDriver IoCreateSymbolicLink OK\r\n");
    
    return status;
}


VOID DriverUnload(__in struct _DRIVER_OBJECT *DriverObject)
{
    DbgPrint("GetPagesDriver Unload\r\n");
    
    //删除符号链接
    UNICODE_STRING ustrSymbolName;
    RtlInitUnicodeString(&ustrSymbolName, L"\\??\\MyTestDevice");
    IoDeleteSymbolicLink(&ustrSymbolName);
    
    //删除设备
    IoDeleteDevice(DriverObject->DeviceObject);
}


NTSTATUS DispatchCreate(IN PDEVICE_OBJECT pDeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    DbgPrint("GetPagesDriver DispatchCreate\r\n");
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}


NTSTATUS DispatchClose(IN PDEVICE_OBJECT pDeviceObject, IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    
    DbgPrint("GetPagesDriver DispatchClose\r\n");
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

