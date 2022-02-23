#pragma once

#include "stub_dev.h"

/*
* 获取设备硬件ID
*/
char *
reg_get_id_hw(PDEVICE_OBJECT pdo);
/*
* 获取设备兼容性ID
*/
char *
reg_get_id_compat(PDEVICE_OBJECT pdo);
/*
* 获取设备属性
*/
BOOLEAN
reg_get_properties(usbip_stub_dev_t *devstub);