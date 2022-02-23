#pragma once

#include <ntddk.h>
#include <usbspec.h>

#include "devconf.h"

#define INFO_INTF_SIZE(info_intf)	(sizeof(USBD_INTERFACE_INFORMATION) + ((info_intf)->NumberOfPipes - 1) * sizeof(USBD_PIPE_INFORMATION))

// 设备配置信息结构体
typedef struct {
	UCHAR	bConfigurationValue;
	UCHAR	bNumInterfaces;
	USBD_CONFIGURATION_HANDLE	hConf;			// usb配置句柄
	PUSB_CONFIGURATION_DESCRIPTOR	dsc_conf;	// usb配置描述符
	PUSBD_INTERFACE_INFORMATION	infos_intf[1];	// usb接口信息
} devconf_t;

// 创建设备
devconf_t *create_devconf(PUSB_CONFIGURATION_DESCRIPTOR dsc_conf, USBD_CONFIGURATION_HANDLE hconf, PUSBD_INTERFACE_LIST_ENTRY pintf_list);
// 释放设备
void free_devconf(devconf_t *devconf);
// 更新设备
void update_devconf(devconf_t *devconf, PUSBD_INTERFACE_INFORMATION info_intf);

// 获取接口描述符大小
USHORT get_info_intf_size(devconf_t *devconf, UCHAR intf_num, USHORT alt_setting);
// 获取设备流水线信息
PUSBD_PIPE_INFORMATION get_info_pipe(devconf_t *devconf, UCHAR epaddr);

#ifdef DBG
const char *dbg_info_intf(PUSBD_INTERFACE_INFORMATION info_intf);
const char *dbg_info_pipe(PUSBD_PIPE_INFORMATION info_pipe);
#endif