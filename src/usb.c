/*
 *  usb.c
 *  USB-related functions for the scsisim library.
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>

#include "scsisim.h"
#include "usb.h"

#define VENDOR_FILE	"idVendor"
#define PRODUCT_FILE	"idProduct"

#define VENDOR_INDEX	0
#define PRODUCT_INDEX	1
#define DEVICE_INDEX	2


/**
 * Function: usb_get_vendor_product
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * vendor:	(Output) USB vendor number.
 * product:	(Output) USB product number.
 *
 * Description: 
 * Given a pointer to an sg_dev struct, traverse the sysfs directory
 * structure for the associated device name to obtain the USB vendor 
 * and product numbers.
 *
 * Return values: 
 * SCSISIM_SYSFS_CHDIR_FAILED
 * SCSISIM_USB_VENDOR_OPEN_FAILED
 * SCSISIM_USB_PRODUCT_OPEN_FAILED
 * SCSISIM_SUCCESS
 */
int usb_get_vendor_product(const struct sg_dev *device,
			   unsigned int *vendor,
			   unsigned int *product)
{
	int ret;
	char cwd[PATH_MAX], sysfs_sg_full_path[PATH_MAX];
	FILE *fpVendor = NULL, *fpProduct = NULL;

	*vendor = 0;
	*product = 0;

	snprintf(sysfs_sg_full_path,
		 PATH_MAX,
		 "%s/%s",
		 SYSFS_SG_BASE_PATH,
		 device->name);

	if (scsisim_verbose)
		scsisim_pinfo("%s: ready to change directory to %s",
			      __func__, sysfs_sg_full_path);

	/* Change to the physical device directory in sysfs -- equivalent to
	 * `cd -P /sys/class/scsi_generic/sg[X]` */
	ret = chdir(sysfs_sg_full_path);

	if (ret)
	{
		if (scsisim_verbose)
			scsisim_pinfo("%s: changing to %s failed",
				      __func__, sysfs_sg_full_path);

		return(SCSISIM_SYSFS_CHDIR_FAILED);
	}

	if (scsisim_verbose)
		scsisim_pinfo("%s: current directory is %s",
			      __func__, getcwd(cwd, PATH_MAX));

	/* Back out to the directory that contains the idProduct and idVendor files.
	 * This is usually something like /sys/devices/pci0000:00/0000:00:14.0/usb1/1-3 */
	ret = chdir("../../../../../..");

	if (ret)
	{
		return(SCSISIM_SYSFS_CHDIR_FAILED);
	}

	if (scsisim_verbose)
		scsisim_pinfo("%s: current directory is %s",
			      __func__, getcwd(cwd, PATH_MAX));

	/* Get the USB vendor ID */
	if ((fpVendor = fopen(VENDOR_FILE, "r")) == NULL)
	{
		return(SCSISIM_USB_VENDOR_OPEN_FAILED);
	}

	fscanf(fpVendor, "%x", vendor);
	fclose(fpVendor);

	if (scsisim_verbose)
		scsisim_pinfo("%s: device vendor is %x", __func__, *vendor);

	/* Get the USB product ID */
	if ((fpProduct = fopen(PRODUCT_FILE, "r")) == NULL)
	{
		return(SCSISIM_USB_PRODUCT_OPEN_FAILED);
	}

	fscanf(fpProduct, "%x", product);
	fclose(fpProduct);

	if (scsisim_verbose)
		scsisim_pinfo("%s: device product is %x", __func__, *product);

	return SCSISIM_SUCCESS;
}

/**
 * Function: usb_is_device_supported
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * vendor:	(Output) USB vendor number.
 * product:	(Output) USB product number.
 * supported_devices: Array of supported USB devices organized by 
 * vendor and product number. See device.h
 *
 * Description: 
 * Given a USB vendor and product number, determine if it is in the
 * list of supported USB devices. If so, set the device index in
 * sg_dev struct so it knows which definitions in device.h to reference.
 *
 * Return values: 
 * true - The specified device is supported.
 * false - The specified device is not supported.
 */
bool usb_is_device_supported(struct sg_dev *device,
			     unsigned int vendor,
			     unsigned int product,
			     const unsigned int supported_devices[][3])
{
	int i;

	for (i = 0; supported_devices[i][VENDOR_INDEX] != 0; i++)
	{
		if (supported_devices[i][VENDOR_INDEX] == vendor &&
			supported_devices[i][PRODUCT_INDEX] == product)
		{
			/* We have a match */
			device->index = supported_devices[i][DEVICE_INDEX];

			if (scsisim_verbose)
				scsisim_pinfo("%s: device vendor/product is supported", __func__);

			return true;
		}
	}

	return false;
}

/* EOF */

