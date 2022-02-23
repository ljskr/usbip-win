#pragma once

#include <ntddk.h>
#include <ntstrsafe.h>
#include <usbdi.h>
#include <usbdlib.h>

#include "stub_devconf.h"

#define N_DEVICES_USBIP_STUB	32

typedef struct {
	long	count;
	int	remove_pending;
	KEVENT	event;
} usbip_stub_remove_lock_t;

struct stub_res;


// usb_stub设备结构体 ***
typedef struct {
	PDEVICE_OBJECT	self;	// 当前设备对象指针
	PDEVICE_OBJECT	pdo;	// pdo设备对象指针
	PDEVICE_OBJECT	next_stack_dev;	// 下一个设备对象指针
	usbip_stub_remove_lock_t	remove_lock;	// 删除锁
	BOOLEAN		is_started;
	int	id;		// id

	char	id_hw[256];		// 硬件名

	devconf_t	*devconf;	// 设备信息结构体

	UNICODE_STRING	interface_name;	// 接口名称

	USBD_HANDLE	hUSBD;	// usb句柄

	KSPIN_LOCK	lock_stub_res;
	PIRP		irp_stub_read;	// irp 指针头
	/* save an ongoing stub result which has been sent partially */
	struct stub_res	*sres_ongoing;
	ULONG		len_sent_partial;

	LIST_ENTRY	sres_head_done;
	LIST_ENTRY	sres_head_pending;
} usbip_stub_dev_t;

void init_dev_removal_lock(usbip_stub_dev_t *devstub);
NTSTATUS lock_dev_removal(usbip_stub_dev_t *devstub);
void unlock_dev_removal(usbip_stub_dev_t *devstub);
void unlock_wait_dev_removal(usbip_stub_dev_t *devstub);

void remove_devlink(usbip_stub_dev_t *devstub);

NTSTATUS stub_add_device(PDRIVER_OBJECT drvobj, PDEVICE_OBJECT pdo);
