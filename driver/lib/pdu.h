#pragma once

#include <ntddk.h>

#include "usbip_proto.h"

/*
* 对header中的数字进行大小端反转，如大端数字转为小端数字
*/
void swap_usbip_header(struct usbip_header *hdr);
/*
* 对iso descs中的数字进行大小端反转，如大端数字转为小端数字
*/
void swap_usbip_iso_descs(struct usbip_header *hdr);