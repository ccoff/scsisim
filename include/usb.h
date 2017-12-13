/*
 *  usb.h
 *  USB-related definitions for the scsisim library.
 *  This is an internal interface file for the scsisim library.
 *
 *  Copyright (c) 2017, Chris Coffey <kpuc@sdf.org>
 *
 *  Permission to use, copy, modify, and/or distribute this software
 *  for any purpose with or without fee is hereby granted, provided
 *  that the above copyright notice and this permission notice appear
 *  in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
 *  WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *  AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *  DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
 *  OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *  PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __SCSISIM_USB_H__
#define __SCSISIM_USB_H__

#include <stdbool.h>

#define SYSFS_SG_BASE_PATH	"/sys/class/scsi_generic"

int usb_get_vendor_product(const struct sg_dev *device,
			   unsigned int *vendor,
			   unsigned int *product);

bool usb_is_device_supported(struct sg_dev *device,
			     unsigned int vendor,
			     unsigned int product,
			     const unsigned int supported_devices[][3]);

#endif  /* __SCSISIM_USB_H__ */

/* EOF */

