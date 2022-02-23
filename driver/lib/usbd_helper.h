#pragma once

#include <ntddk.h>
#include <usb.h>

/*
* usbip 状态转 usbd 状态
*/
USBD_STATUS to_usbd_status(int usbip_status);
/*
* usbd 状态转 usbip 状态
*/
int to_usbip_status(USBD_STATUS usbd_status);

/*
* usbip 标志位转 usbd 标志位
*/
ULONG to_usbd_flags(int flags);

/*
* usbip iso 结构转 usbd iso 结构
*/
void to_usbd_iso_descs(ULONG n_pkts, USBD_ISO_PACKET_DESCRIPTOR *usbd_iso_descs,
		       const struct usbip_iso_packet_descriptor *iso_descs, BOOLEAN as_result);

/*
* usbd iso 结构转 usbip iso 结构
*/
void to_iso_descs(ULONG n_pkts, struct usbip_iso_packet_descriptor *iso_descs, const USBD_ISO_PACKET_DESCRIPTOR *usbd_iso_descs, BOOLEAN as_result);

/*
* 获取 usbip iso 长度
*/
ULONG get_iso_descs_len(ULONG n_pkts, const struct usbip_iso_packet_descriptor *iso_descs, BOOLEAN is_actual);
/*
* 获取 usbd iso 长度
*/
ULONG get_usbd_iso_descs_len(ULONG n_pkts, const USBD_ISO_PACKET_DESCRIPTOR *usbd_iso_descs);

enum { USB_REQUEST_RESET_PIPE = 0xfe };
