#pragma once

#include <ntddk.h>
#include "stub_dev.h"

NTSTATUS stub_dispatch(PDEVICE_OBJECT devobj, IRP* irp);

NTSTATUS stub_dispatch_pnp(usbip_stub_dev_t* devstub, IRP* irp);
NTSTATUS stub_dispatch_power(usbip_stub_dev_t* devstub, IRP* irp);
NTSTATUS stub_dispatch_ioctl(usbip_stub_dev_t* devstub, IRP* irp);
NTSTATUS stub_dispatch_read(usbip_stub_dev_t* devstub, IRP* irp);
NTSTATUS stub_dispatch_write(usbip_stub_dev_t* devstub, IRP* irp);
