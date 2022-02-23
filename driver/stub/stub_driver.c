/* libusb-win32, Generic Windows USB Library
 * Copyright (c) 2002-2005 Stephan Meyer <ste_meyer@web.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "stub_driver.h"
#include "stub_dbg.h"
#include "stub_dispatch.h"
#include "stub_dev.h"

/*
* 驱动卸载方法
*/
static VOID
stub_unload(DRIVER_OBJECT *drvobj)
{
	UNREFERENCED_PARAMETER(drvobj);

	DBGI(DBG_DISPATCH, "unload\n");
}

/*
* 驱动加载入口
*/
NTSTATUS
DriverEntry(DRIVER_OBJECT *drvobj, UNICODE_STRING *regpath)
{
	int i;

	UNREFERENCED_PARAMETER(regpath);

	DBGI(DBG_DISPATCH, "DriverEntry: Enter\n");

	/* initialize the driver object's dispatch table */
	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
		drvobj->MajorFunction[i] = stub_dispatch;  // 改写所有的分发指针，都使用自定义的stub_dispath方法
	}

	drvobj->DriverExtension->AddDevice = stub_add_device;  // 有新设备时，调用 stub_add_device 方法
	drvobj->DriverUnload = stub_unload;   // 设备断开时，调用 stub_unload 方法

	return STATUS_SUCCESS;
}