﻿#include "stub_driver.h"
#include "stub_dbg.h"
#include "stub_dev.h"
#include "stub_reg.h"

#define INITGUID
#include "usbip_stub_api.h"

#define NAMEBUF_LEN	128
typedef WCHAR	namebuf_t[NAMEBUF_LEN];

static void
build_devname(namebuf_t namebuf, int idx)
{
	RtlStringCchPrintfW(namebuf, sizeof(namebuf_t) / sizeof(WCHAR), L"\\Device\\usbip_stub_%04d", idx);
}

static void
build_linkname(namebuf_t namebuf, int idx)
{
	RtlStringCchPrintfW(namebuf, sizeof(namebuf_t) / sizeof(WCHAR), L"\\DosDevices\\usbip_stub_%04d", idx);
}

void
init_dev_removal_lock(usbip_stub_dev_t *devstub)
{
	KeInitializeEvent(&devstub->remove_lock.event, NotificationEvent, FALSE);
	devstub->remove_lock.count = 1;
	devstub->remove_lock.remove_pending = FALSE;
}

NTSTATUS
lock_dev_removal(usbip_stub_dev_t *devstub)
{
	InterlockedIncrement(&devstub->remove_lock.count);

	if (devstub->remove_lock.remove_pending) {
		if (InterlockedDecrement(&devstub->remove_lock.count) == 0) {
			KeSetEvent(&devstub->remove_lock.event, 0, FALSE);
		}
		return STATUS_DELETE_PENDING;
	}
	return STATUS_SUCCESS;
}

void
unlock_dev_removal(usbip_stub_dev_t *devstub)
{
	if (InterlockedDecrement(&devstub->remove_lock.count) == 0) {
		KeSetEvent(&devstub->remove_lock.event, 0, FALSE);
	}
}

void
unlock_wait_dev_removal(usbip_stub_dev_t *devstub)
{
	devstub->remove_lock.remove_pending = TRUE;
	unlock_dev_removal(devstub);
	unlock_dev_removal(devstub);
	KeWaitForSingleObject(&devstub->remove_lock.event, Executive, KernelMode, FALSE, NULL);
}

void
remove_devlink(usbip_stub_dev_t *devstub)
{
	namebuf_t	linkname;
	UNICODE_STRING	linkname_uni;

	build_linkname(linkname, devstub->id);
	RtlInitUnicodeString(&linkname_uni, linkname);
	IoDeleteSymbolicLink(&linkname_uni);
}

static PDEVICE_OBJECT
create_devobj_idx(PDRIVER_OBJECT drvobj, ULONG devtype, int idx)
{
	PDEVICE_OBJECT	devobj;
	UNICODE_STRING	devname_uni, linkname_uni;
	namebuf_t	devname, linkname;
	usbip_stub_dev_t	*devstub;
	NTSTATUS	status;

	build_devname(devname, idx);
	RtlInitUnicodeString(&devname_uni, devname);

	// 创建设备。
	// 参数一：该参数用于在驱动程序和新设备对象之间建立连接，这样I/O管理器就可以向设备发送指定的IRP。
	// 参数二：设备扩展结构的大小。I/O管理器自动分配这个内存，并把设备对象中的DeviceExtension指针指向这块内存。
	// 参数三：命名该设备对象的UNICODE_STRING串的地址。
	// 参数四：设备类型枚举。
	// 参数五：设备特性枚举。
	// 参数六：设备是否是排斥的。通常，对于排斥设备，I/O管理器仅允许打开该设备的一个句柄。这个值同样也能被注册表中硬件键和类键中的值超越，如果两个超越值都存在，硬件键中的超越值具有更高的优先权。
	// 参数七：接收被创建出来的设备对象指针。属于输出值。
	/* create the object */
	status = IoCreateDevice(drvobj, sizeof(usbip_stub_dev_t), &devname_uni, devtype, 0, FALSE, &devobj);
	if (NT_ERROR(status)) {
		if (status != STATUS_OBJECT_NAME_COLLISION)
			DBGE(DBG_GENERAL, "create_devobj_idx: IoCreateDevice failed: %S: err: %s\n", devname, dbg_ntstatus(status));
		return NULL;
	}

	build_linkname(linkname, idx);
	RtlInitUnicodeString(&linkname_uni, linkname);
	// 创建符号链接，即别名。
	status = IoCreateSymbolicLink(&linkname_uni, &devname_uni);
	if (NT_ERROR(status)) {
		DBGE(DBG_GENERAL, "create_devobj_idx: IoCreateSymbolicLink failed: %S: err: %s\n", devname, dbg_ntstatus(status));
		IoDeleteDevice(devobj);
		return NULL;
	}

	// 对 DeviceExtension 中的数据进行初始化清零。
	devstub = (usbip_stub_dev_t *)devobj->DeviceExtension;
	RtlZeroMemory(devstub, sizeof(usbip_stub_dev_t));
	devstub->id = idx;
	return devobj;
}

static PDEVICE_OBJECT
create_devobj(PDRIVER_OBJECT drvobj, ULONG devtype)
{
	int	i;

	/* try to create a new device object */
	for (i = 1; i < N_DEVICES_USBIP_STUB; i++) {
		// id 从1开始到32，循环创建devobj，直到成功为止。
		PDEVICE_OBJECT	devobj = create_devobj_idx(drvobj, devtype, i);
		if (devobj != NULL)
			return devobj;
	}
	return NULL;
}

static BOOLEAN
is_usbip_stub_attached(PDEVICE_OBJECT pdo)
{
	DEVICE_OBJECT	*attached;

	attached = pdo->AttachedDevice;
	while (attached) {
		PDRIVER_OBJECT	drvobj = attached->DriverObject;

		if (drvobj != NULL) {
			UNICODE_STRING	name_uni;
			RtlInitUnicodeString(&name_uni, L"\\driver\\usbip_stub");
			if (RtlEqualUnicodeString(&drvobj->DriverName, &name_uni, TRUE))
				return TRUE;
		}
		attached = attached->AttachedDevice;
	}
	return FALSE;
}

static BOOLEAN
is_addable_pdo(DEVICE_OBJECT *pdo)
{
	char	*id_hw, *id_compat;

	id_hw = reg_get_id_hw(pdo);
	if (id_hw == NULL) {
		DBGW(DBG_DISPATCH, "unable to get HW id from registry\n");
		return FALSE;
	}

	/* only attach the (filter) driver to USB devices, skip hubs */
	if (strncmp(id_hw, "USB\\", 4) != 0 || strncmp(id_hw + 4, "VID_", 4) != 0 || strlen(id_hw + 8) < 4 ||
		strncmp(id_hw + 12, "&PID_", 5) != 0) {
		DBGI(DBG_DISPATCH, "skipping non-usb device or hub: %s\n", id_hw);
		ExFreePool(id_hw);
		return FALSE;
	}
	ExFreePool(id_hw);

	id_compat = reg_get_id_compat(pdo);
	if (id_compat == NULL) {
		DBGW(DBG_DISPATCH, "unable to get compatible id from registry\n");
		return FALSE;
	}

	// Don't attach to usb device hubs
	if (strncmp(id_compat, "USB\\Class_09", 12) == 0) {
		DBGI(DBG_DISPATCH, "skipping usb device hub: compatible id: %s\n", id_compat);
		ExFreePool(id_compat);
		return FALSE;
	}
	ExFreePool(id_compat);

	if (is_usbip_stub_attached(pdo)) {
		DBGI(DBG_DISPATCH, "skipping usbip stub\n");
		return FALSE;
	}

	return TRUE;
}

static ULONG
get_device_type(PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT	devobj;
	ULONG	devtype = FILE_DEVICE_UNKNOWN;

	devobj = IoGetAttachedDeviceReference(pdo);
	if (devobj) {
		devtype = devobj->DeviceType;
		ObDereferenceObject(devobj);
	}

	return devtype;
}

// 处理设备增加
NTSTATUS
stub_add_device(PDRIVER_OBJECT drvobj, PDEVICE_OBJECT pdo)
{
	DEVICE_OBJECT	*devobj;
	usbip_stub_dev_t	*devstub;
	ULONG		devtype;
	NTSTATUS	status;

	DBGI(DBG_DEV, "add_device: %s\n", dbg_devices(pdo, TRUE));

	if (!is_addable_pdo(pdo))
		return STATUS_SUCCESS;

	// 获取设备类型枚举。
	devtype = get_device_type(pdo);

	// 创建设备对象。
	devobj = create_devobj(drvobj, devtype);
	if (devobj == NULL) {
		DBGE(DBG_DEV, "failed to create usbip stub device\n");
		return STATUS_SUCCESS;
	}

	/* setup the "device object" */
	devstub = (usbip_stub_dev_t *)devobj->DeviceExtension;

	// 把原始的 devobj 和 pdo 指针保留起来。
	devstub->self = devobj;
	devstub->pdo = pdo;

	/* get device properties from the registry */
	if (!reg_get_properties(devstub)) {
		DBGE(DBG_DEV, "failed to setup device from properties\n");
		remove_devlink(devstub);
		IoDeleteDevice(devobj);
		return STATUS_SUCCESS;
	}

	devstub->sres_ongoing = NULL;
	devstub->len_sent_partial = 0;

	init_dev_removal_lock(devstub);
	InitializeListHead(&devstub->sres_head_pending);
	InitializeListHead(&devstub->sres_head_done);

	// 注册设备接口类
	// 参数一：指向设备 PDO 的指针。
	// 参数二：指向类 GUID 的指针, 标识注册的功能 (设备接口类)。
	// 参数三：函数驱动程序通常为该参数指定NULL。筛选器驱动程序必须指定NULL。
	// 参数四：接口名称。属于输出值。
	status = IoRegisterDeviceInterface(pdo, (LPGUID)&GUID_DEVINTERFACE_STUB_USBIP, NULL, &devstub->interface_name);
	if (NT_ERROR(status)) {
		DBGE(DBG_DEV, "failed to register interface\n");
	}

	// make sure the the devices can't be removed
	// before we are done adding it.
	if (!NT_SUCCESS(lock_dev_removal(devstub))) {
		DBGI(DBG_DEV, "device is pending removal\n");
		remove_devlink(devstub);
		IoDeleteDevice(devobj);
		return STATUS_SUCCESS;
	}

	// 把SourceDevice放到TargetDevice所在的链的末端。向目标Device传递IRP时，Device处理完了IRP之后，可以把这个IRP传递给链条下一层的驱动程序。
	/* attach the newly created device object to the stack */
	devstub->next_stack_dev = IoAttachDeviceToDeviceStack(devobj, pdo);
	if (devstub->next_stack_dev == NULL) {
		DBGE(DBG_DISPATCH, "failed to attach: %s\n", dbg_devstub(devstub));
		unlock_dev_removal(devstub); // always release acquired locks
		remove_devlink(devstub);
		IoDeleteDevice(devobj);
		return STATUS_NO_SUCH_DEVICE;
	}


	status = USBD_CreateHandle(pdo, devstub->next_stack_dev, USBD_CLIENT_CONTRACT_VERSION_602, USBIP_STUB_POOL_TAG, &devstub->hUSBD);
	if (NT_ERROR(status)) {
		DBGE(DBG_DISPATCH, "add_device: failed to create USBD handle: %s: %s\n", dbg_devstub(devstub), dbg_ntstatus(status));
		IoDetachDevice(devstub->next_stack_dev);
		unlock_dev_removal(devstub);
		remove_devlink(devstub);
		IoDeleteDevice(devobj);
		return STATUS_UNSUCCESSFUL;
	}

	KeInitializeSpinLock(&devstub->lock_stub_res);

	devobj->Flags |= DO_POWER_PAGABLE | DO_BUFFERED_IO;

	// use the same DeviceType as the underlying object
	devobj->DeviceType = devstub->next_stack_dev->DeviceType;

	// use the same Characteristics as the underlying object
	devobj->Characteristics = devstub->next_stack_dev->Characteristics;

	DBGI(DBG_DEV, "add_device: device added: %s\n", dbg_devstub(devstub));

	devobj->Flags &= ~DO_DEVICE_INITIALIZING;
	unlock_dev_removal(devstub);

	return STATUS_SUCCESS;
}
