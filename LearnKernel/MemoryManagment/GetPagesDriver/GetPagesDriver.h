extern "C" 
{
    
#include <ntifs.h>
#include <ntddk.h>

NTSTATUS DispatchCreate(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DispatchControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS DriverEntry(__in struct _DRIVER_OBJECT  *DriverObject, __in PUNICODE_STRING  RegistryPath );
VOID DriverUnload(__in struct _DRIVER_OBJECT  *DriverObject);

#pragma alloc_text("PAGE", DispatchCreate)
#pragma alloc_text("PAGE", DispatchClose)
#pragma alloc_text("PAGE", DispatchControl)
#pragma alloc_text("PAGE", DriverUnload)
#pragma alloc_text("INIT", DriverEntry)


ULONG64 GetQuadByPA(ULONG64 llPA);
NTSTATUS GetProcessPages(PVOID pInBuff, PVOID pOutBuff, ULONG nOutLength, ULONG* nBytes);

#define IOCTL_GET_PAGES_PAE CTL_CODE( \
    FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

}