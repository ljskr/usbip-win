#pragma once

#include <ntddk.h>
#include <usbdi.h>

#define NEXT_DESC(dsc)			(PUSB_COMMON_DESCRIPTOR)((PUCHAR)(dsc) + ((PUSB_COMMON_DESCRIPTOR)dsc)->bLength)
#define NEXT_DESC_INTF(dsc)		(PUSB_INTERFACE_DESCRIPTOR)NEXT_DESC(dsc)
#define NEXT_DESC_EP(dsc)		(PUSB_ENDPOINT_DESCRIPTOR)NEXT_DESC(dsc)

/*
* 通过usb配置描述符获取usb接口描述符信息。只拿第一个。
*/
PUSB_INTERFACE_DESCRIPTOR
dsc_find_first_intf(PUSB_CONFIGURATION_DESCRIPTOR dsc_conf);

/*
* 通过usb配置描述符获取usb接口描述符信息。通过索引来获取。
*/
PUSB_INTERFACE_DESCRIPTOR
dsc_find_intf(PUSB_CONFIGURATION_DESCRIPTOR dsc_conf, UCHAR intf_num, USHORT alt_setting);

/*
* 通过usb配置描述符获取usb接口描述符信息。通过usb端点描述符信息来获取。
*/
PUSB_INTERFACE_DESCRIPTOR
dsc_find_intf_by_ep(PUSB_CONFIGURATION_DESCRIPTOR dsc_conf, PUSB_ENDPOINT_DESCRIPTOR dsc_ep);

/*
* 通过usb配置描述符和usb接口描述符获取usb端点描述符信息。
*/
PUSB_ENDPOINT_DESCRIPTOR
dsc_find_intf_ep(PUSB_CONFIGURATION_DESCRIPTOR dsc_conf, PUSB_INTERFACE_DESCRIPTOR dsc_intf, UCHAR epaddr);

/*
* 通过usb配置描述符获取usb端点描述符信息。
*/
PUSB_ENDPOINT_DESCRIPTOR
dsc_next_ep(PUSB_CONFIGURATION_DESCRIPTOR dsc_conf, PVOID start);

/*
* 通过usb配置描述符获取usb接口数量。
*/
ULONG
dsc_conf_get_n_intfs(PUSB_CONFIGURATION_DESCRIPTOR dsc_conf);
