#pragma once
#include <ntddk.h>

#define DEVICE_NAME L"\\Device\\YourDriva"
#define SYMBOLIC_LINK_NAME L"\\DosDevices\\YourDriva"

PDEVICE_OBJECT gDeviceObject = NULL;

typedef unsigned short WORD;

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID UnloadDriver(PDRIVER_OBJECT DriverObject);
NTSTATUS DeviceIoControlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS CreateCloseHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp);

void LogBufferContents(PVOID buffer, ULONG length) {
    if (length > 0) {
        DbgPrintEx(0, 0, "Buffer contents:\n");
        for (ULONG i = 0; i < length; i++) {
            DbgPrintEx(0, 0, "%02X ", ((PUCHAR)buffer)[i]);
            if ((i + 1) % 16 == 0) {
                DbgPrintEx(0, 0, "\n");
            }
        }
        DbgPrintEx(0, 0, "\n");
    }
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLinkName;

    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &gDeviceObject);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0, "Failed to create device: %X\n", status);
        return status;
    }

    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);
    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);

    if (!NT_SUCCESS(status)) {
        DbgPrintEx(0, 0, "Failed to create symbolic link: %X\n", status);
        IoDeleteDevice(gDeviceObject);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlHandler;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseHandler;
    DriverObject->DriverUnload = UnloadDriver;

    DbgPrintEx(0, 0, "Driver Loaded Successfully\n");
    return STATUS_SUCCESS;
}

VOID UnloadDriver(PDRIVER_OBJECT DriverObject) {
    UNICODE_STRING symbolicLinkName;
    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);

    IoDeleteSymbolicLink(&symbolicLinkName);
    IoDeleteDevice(DriverObject->DeviceObject);

    DbgPrintEx(0, 0, "Driver Unloaded\n");
}

NTSTATUS CreateCloseHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS DeviceIoControlHandler(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    UNREFERENCED_PARAMETER(DeviceObject);
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    ULONG inputBufferLength = stack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputBufferLength = stack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID inputBuffer = Irp->AssociatedIrp.SystemBuffer;
    PVOID outputBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG bytesReturned = 0;
    NTSTATUS status = STATUS_SUCCESS;

    DbgPrintEx(0, 0, "Received IOCTL: 0x%X\n", ioControlCode);
    DbgPrintEx(0, 0, "Input Buffer Length: %lu\n", inputBufferLength);
    DbgPrintEx(0, 0, "Output Buffer Length: %lu\n", outputBufferLength);

    if (inputBufferLength > 0 && Irp->AssociatedIrp.SystemBuffer != NULL) {
        DbgPrintEx(0, 0, "Logging Input Buffer...\n");
        LogBufferContents(Irp->AssociatedIrp.SystemBuffer, inputBufferLength);
    }

    if (outputBufferLength > 0 && Irp->UserBuffer != NULL) {
        DbgPrintEx(0, 0, "Logging Output Buffer...\n");
        LogBufferContents(Irp->UserBuffer, outputBufferLength);
    }

    ULONG_PTR information = 0;


    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = information;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
