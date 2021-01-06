/*
 * Gadget Function Driver for MTP
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __LINUX_USB_F_MTP_H
#define __LINUX_USB_F_MTP_H

#include <uapi/linux/usb/f_mtp.h>
#include <linux/ioctl.h>

#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif

#ifdef __KERNEL__
#ifdef CONFIG_COMPAT
struct __compat_mtp_event {
	compat_size_t	length;
	compat_caddr_t	data;
};

#endif
#endif

#define COMPAT_MTP_SEND_EVENT	_IOW('M', 3, struct __compat_mtp_event)

#ifdef VENDOR_EDIT
//yan.chen@Swdp.shanghai, 2015/12/3, add mtp callback for hypnus
extern int mtp_register_notifier(struct notifier_block *nb);
extern int mtp_unregister_notifier(struct notifier_block *nb);
#endif /*VENDOR_EDIT*/
#endif /* __LINUX_USB_F_MTP_H */
