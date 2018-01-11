/*
 *  scsi.c
 *  SCSI-related functions for the scsisim library.
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

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <scsi/sg.h>

#include "scsisim.h"
#include "scsi.h"
#include "utils.h"

static inline void scsi_init_io_hdr(struct sg_io_hdr *io_hdr);


/**
 * Function: scsi_send_cdb
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * my_cmd:	Pointer to scsi_cmd struct.
 *
 * Description: 
 * Given a pointer to an sg_dev struct, set up the SCSI generic
 * sg_io_hdr struct with the settings passed in the scsi_cmd struct.
 * Then do the actual ioctl() on the device to send the CDB (command
 * data block).
 *
 * Return values: 
 * SCSISIM_SCSI_SEND_ERROR
 * SCSISIM_SUCCESS
 */
int scsi_send_cdb(const struct sg_dev *device, struct scsi_cmd *my_cmd)
{
	int ret = SCSISIM_SCSI_SEND_ERROR;
	struct sg_io_hdr io_hdr;

	/* Initialize the sg_io_hdr struct: */
	scsi_init_io_hdr(&io_hdr);

	/* Set the transfer direction: */
	io_hdr.dxfer_direction =
		(my_cmd->direction == SIM_WRITE) ? SG_DXFER_TO_DEV : SG_DXFER_FROM_DEV;

	/* Set the SCSI command buffer: */
	io_hdr.cmdp = my_cmd->cdb;
	io_hdr.cmd_len = my_cmd->cdb_len;

	/* Set the SCSI data buffer: */
	io_hdr.dxferp = my_cmd->data;
	io_hdr.dxfer_len = my_cmd->data_len;

	/* Set the SCSI sense buffer: */
	io_hdr.sbp = my_cmd->sense;
	io_hdr.mx_sb_len = my_cmd->sense_len;

	/* Print some debug info if requested: */
	if (scsisim_verbose())
	{
		scsisim_pinfo("%s: >>> SENDING COMMAND >>>", __func__);
		print_binary_buffer(io_hdr.cmdp, io_hdr.cmd_len);

		if (io_hdr.dxfer_direction == SG_DXFER_TO_DEV)
		{
			scsisim_pinfo("%s: >>> SENDING DATA >>>",
				      __func__, io_hdr.dxfer_len);
			print_binary_buffer(io_hdr.dxferp, io_hdr.dxfer_len);
		}
	}

	/* We're ready -- send the command to the SCSI generic kernel driver: */
	if (ioctl(device->fd, SG_IO, &io_hdr) == 0)
	{
		my_cmd->data_xfered = io_hdr.dxfer_len - io_hdr.resid;
		my_cmd->sense_xfered = io_hdr.sb_len_wr;
		ret = SCSISIM_SUCCESS;
	}

	/* Print a whole bunch more debug info if requested: */
	if (scsisim_verbose())
	{
		scsisim_pinfo("%s: io_hdr.status = %d",
			      __func__, io_hdr.status);
		scsisim_pinfo("%s: %d data bytes transferred",
			      __func__, my_cmd->data_xfered);

		if (io_hdr.dxfer_len > 0 && io_hdr.resid > 0)
		{
			scsisim_pinfo("%s: data transfer underrun by %d bytes",
				      __func__, io_hdr.resid);
		}

		if (io_hdr.dxfer_direction == SG_DXFER_FROM_DEV && my_cmd->data_xfered)
		{
			scsisim_pinfo("%s: <<< RECEIVED DATA <<<", __func__);
			print_binary_buffer(my_cmd->data, my_cmd->data_xfered);
		}

		if (my_cmd->sense_xfered)
		{
			scsisim_pinfo("%s: received %d bytes of sense data",
				      __func__, my_cmd->sense_xfered);
			print_binary_buffer(my_cmd->sense, my_cmd->sense_xfered);
		}

		scsisim_pinfo("%s: returning %d (%s)",
			      __func__, ret, scsisim_strerror(ret));
	}

	return ret;
}

/**
 * Function: scsi_init_io_hdr
 *
 * Parameters:
 * io_hdr:	Pointer to SCSI generic sg_io_hdr struct.
 *
 * Description: 
 * Initialize the specified sg_io_hdr struct with settings
 * common to all SCSI commands sent to the device.
 *
 * Return value: 
 * None
 */
static inline void scsi_init_io_hdr(struct sg_io_hdr *io_hdr)
{
	memset(io_hdr, 0, sizeof(struct sg_io_hdr));

	io_hdr->interface_id = 'S';	/* Per scsi/sg.h, this must always be set to 'S' */
	io_hdr->timeout = 1000;		/* In milliseconds */
}


/* EOF */

