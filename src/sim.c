/*
 *  sim.c
 *  SIM-related functions for the scsisim library.
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
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <linux/limits.h>

#include "scsisim.h"
#include "gsm.h"
#include "scsi.h"
#include "device.h"
#include "usb.h"
#include "utils.h"

static int sim_process_scsi_sense(const struct scsisim_dev *device,
				  const uint8_t *sense,
				  unsigned int len);

static inline void sim_free_device_name(struct scsisim_dev *device);


/**
 * For information about this function, see scsisim.h
 */
int scsisim_open_device(const char *dev_name, struct scsisim_dev *device)
{
	int ret;
	char full_path[PATH_MAX];

	if (dev_name == NULL ||
	    strlen(dev_name) < 3 ||	/* Name must be at least 3 bytes long (e.g., 'sg1') */
	    strncmp(dev_name, "sg", 2) != 0)	/* Name must start with 'sg' */
		return SCSISIM_INVALID_DEVICE_NAME;

	if ((device->name = malloc(strlen(dev_name) + 1)) == NULL)
		return SCSISIM_MEMORY_ALLOCATION_ERROR;

	strcpy(device->name, dev_name);

	snprintf(full_path, PATH_MAX, "/dev/%s", device->name);

	if (scsisim_verbose())
		scsisim_pinfo("%s: ready to open %s", __func__, full_path);

	/* Try to open device */
	device->fd = open(full_path, O_RDWR);

	if (device->fd > 0)
	{
		/* We have a valid file descriptor */
		if (scsisim_verbose())
			scsisim_pinfo("%s: device opened, fd = %d, name = %s",
				      __func__, device->fd, device->name);

		ret = SCSISIM_SUCCESS;
	}
	else
	{
		/* Device open failed, so clean up */
		sim_free_device_name(device);
		ret = SCSISIM_DEVICE_OPEN_FAILED;
	}

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_close_device(struct scsisim_dev *device)
{
	int ret;

	if (device == NULL)
		return SCSISIM_INVALID_PARAM;

	sim_free_device_name(device);

	if (device->fd > 0)
	{
		if (close(device->fd))
			ret = SCSISIM_DEVICE_CLOSE_FAILED;
		else
		{
			if (scsisim_verbose())
				scsisim_pinfo("%s: device closed", __func__);

			device->fd = 0;
			device->index = 0;
			ret = SCSISIM_SUCCESS;
		}
	}
	else
		ret = SCSISIM_INVALID_FILE_DESCRIPTOR;

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_init_device(struct scsisim_dev *device)
{
	int ret, i;
	unsigned int idVendor, idProduct;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t sense[32] = { 0 };

	if (device == NULL)
		return SCSISIM_INVALID_PARAM;

	/* Obtain the USB vendor and product ID based on the device name */
	if ((ret = usb_get_vendor_product(device, &idVendor, &idProduct)) != SCSISIM_SUCCESS)
		return ret;

	/* Make sure the attached device is a SIM card reader we support.
	   We do NOT want to write data to some random, hapless device! */
	if (usb_is_device_supported(device, idVendor, idProduct, supported_devices) == false)
		return SCSISIM_DEVICE_NOT_SUPPORTED;

	/* If we get this far, we have a supported SIM card reader. Now we can
	   send 'magic' sequence of SCSI commands to get the device working */
	for (i = 0; sim_devices[device->index].init_cmd[i].direction != SIM_NO_XFER; i++)
	{
		/* Set up the command block */
		my_cmd.direction = sim_devices[device->index].init_cmd[i].direction;
		my_cmd.cdb = (uint8_t*)sim_devices[device->index].init_cmd[i].cdb;
		my_cmd.cdb_len = sim_devices[device->index].cdb_len;
		my_cmd.data = sim_devices[device->index].init_cmd[i].data;
		my_cmd.data_len = sim_devices[device->index].init_cmd[i].data_len;
		my_cmd.sense = sense;
		my_cmd.sense_len = (uint8_t)sizeof(sense);

		/* Send the commmand */
		if ((ret = scsi_send_cdb(device, &my_cmd)) != SCSISIM_SUCCESS)
		{
			break;
		}
	}

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_select_file(const struct scsisim_dev *device, uint16_t file)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t data[GSM_CMD_SELECT_DATA_LEN] = { 0 };
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL)
		return SCSISIM_INVALID_PARAM;

	memset(sense, 0, sizeof(sense));

	/* Get the base SELECT command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_select_file, sizeof(cdb));

	/* Set up the data block with the requested file ID */
	data[0] = file >> 8;
	data[1] = file & 0xff;

	/* Set up the command block */
	my_cmd.direction = SIM_WRITE;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = (uint8_t)sizeof(data);
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	if (ret == SCSISIM_SUCCESS)
	{
		/* There should ALWAYS be sense data after selecting a file, 
		 * so we can find out how many bytes to request for GET RESPONSE */
		if (my_cmd.sense_xfered)
			ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);
		else
			ret = SCSISIM_SCSI_NO_SENSE_DATA;
	}

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_get_response(const struct scsisim_dev *device,
			 uint8_t *data,
			 uint8_t len,
			 int command,
			 struct GSM_response *resp)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL || data == NULL || len <= 0 || resp == NULL)
		return SCSISIM_INVALID_PARAM;

	memset(sense, 0, sizeof(sense));

	/* Get the base GET RESPONSE command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_get_response, sizeof(cdb));

	/* Set the length */
	cdb[sim_devices[device->index].get_response_len_offset] = len;

	/* Set up the command block */
	my_cmd.direction = SIM_READ;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = len;
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	resp->command = command;

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	if (scsisim_verbose())
	{
		if (my_cmd.data_xfered != len)
			scsisim_pinfo("%s: bytes transferred (%d) is less than data buffer length (%d)",
				      __func__, my_cmd.data_xfered, len);
	}

	if (ret == SCSISIM_SUCCESS)
	{
		/* Parse out the GSM response data and fill in the resp struct */
		if ((ret = gsm_parse_response(data, len, resp)) != SCSISIM_SUCCESS)
		{
			scsisim_perror("scsisim_get_response()", ret);
		}

		/* If there is sense data, process it and use it as the return code instead: */
		if (my_cmd.sense_xfered)
			ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);
	}

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_select_file_and_get_response(const struct scsisim_dev *device,
					 uint16_t file,
					 uint8_t *data,
					 uint8_t len,
					 int command,
					 struct GSM_response *resp)
{
	int ret;

	if ((ret = scsisim_select_file(device, file)) > 0)
	{
		ret = scsisim_get_response(device, data, MIN(ret, len), command, resp);
	}

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_read_record(const struct scsisim_dev *device,
			uint8_t recno,
			uint8_t *data,
			uint8_t len)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL || recno == 0 || data == NULL || len <= 0)
		return SCSISIM_INVALID_PARAM;

	memset(sense, 0, sizeof(sense));

	/* Get the base READ RECORD command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_read_record, sizeof(cdb));

	/* Set the record number and length */
	cdb[sim_devices[device->index].read_record_rec_offset] = recno;
	cdb[sim_devices[device->index].read_record_len_offset] = len;

	/* Set up the command block */
	my_cmd.direction = SIM_READ;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = len;
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	if (scsisim_verbose())
	{
		if (my_cmd.data_xfered != len)
			scsisim_pinfo("%s: bytes transferred (%d) is less than data buffer length (%d)",
				      __func__, my_cmd.data_xfered, len);
	}

	/* If there is sense data, process it and use it as the return code instead: */
	if (my_cmd.sense_xfered)
		ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_read_binary(const struct scsisim_dev *device,
			uint8_t *data,
			uint16_t offset,
			uint8_t len)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL || data == NULL || len == 0)
		return SCSISIM_INVALID_PARAM;

	memset(sense, 0, sizeof(sense));

	/* Get the base READ BINARY command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_read_binary, sizeof(cdb));

	/* Set the offsets and length */
	cdb[sim_devices[device->index].read_binary_hi_offset] = offset >> 8;   /* offset high */
	cdb[sim_devices[device->index].read_binary_lo_offset] = offset & 0xff; /* offset low */
	cdb[sim_devices[device->index].read_binary_len_offset] = len;

	/* Set up the command block */
	my_cmd.direction = SIM_READ;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = len;
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	if (scsisim_verbose())
	{
		if (my_cmd.data_xfered != len)
			scsisim_pinfo("%s: bytes transferred (%d) is less than data buffer length (%d)",
				      __func__, my_cmd.data_xfered, len);
	}

	/* If there is sense data, process it and use it as the return code instead: */
	if (my_cmd.sense_xfered)
		ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_update_record(const struct scsisim_dev *device,
			  uint8_t recno,
			  uint8_t *data,
			  uint8_t len)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL || recno == 0 || data == NULL || len <= 0)
		return SCSISIM_INVALID_PARAM;

	memset(sense, 0, sizeof(sense));

	/* Get the base UPDATE RECORD command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_update_record, sizeof(cdb));

	/* Set the record number and length */
	cdb[sim_devices[device->index].update_record_rec_offset] = recno;
	cdb[sim_devices[device->index].update_record_len_offset] = len;

	/* Set up the command block */
	my_cmd.direction = SIM_WRITE;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = len;
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	if (scsisim_verbose())
	{
		if (my_cmd.data_xfered != len)
			scsisim_pinfo("%s: bytes transferred (%d) is less than data buffer length (%d)",
				      __func__, my_cmd.data_xfered, len);
	}

	/* If there is sense data, process it and use it as the return code instead: */
	if (my_cmd.sense_xfered)
		ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_update_binary(const struct scsisim_dev *device,
			  uint8_t *data,
			  uint16_t offset,
			  uint8_t len)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL || data == NULL || len == 0)
		return SCSISIM_INVALID_PARAM;

	memset(sense, 0, sizeof(sense));

	/* Get the base UPDATE BINARY command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_update_binary, sizeof(cdb));

	/* Set the offsets and length */
	cdb[sim_devices[device->index].update_binary_hi_offset] = offset >> 8;   /* offset high */
	cdb[sim_devices[device->index].update_binary_lo_offset] = offset & 0xff; /* offset low */
	cdb[sim_devices[device->index].update_binary_len_offset] = len;

	/* Set up the command block */
	my_cmd.direction = SIM_WRITE;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = len;
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	if (scsisim_verbose())
	{
		if (my_cmd.data_xfered != len)
			scsisim_pinfo("%s: bytes transferred (%d) is less than data buffer length (%d)",
				      __func__, my_cmd.data_xfered, len);
	}

	/* If there is sense data, process it and use it as the return code instead: */
	if (my_cmd.sense_xfered)
		ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_verify_chv(const struct scsisim_dev *device,
		       uint8_t chv,
		       const char *pin)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t *data = NULL;
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL || pin == NULL)
		return SCSISIM_INVALID_PARAM;

	if (is_digit_string(pin) == false)
		return SCSISIM_INVALID_PIN;

	if (strlen(pin) > GSM_CMD_VERIFY_CHV_DATA_LEN)
		return SCSISIM_GSM_ERROR_PARAM_3;

	memset(sense, 0, sizeof(sense));

	/* Get the base VERIFY CHV command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_verify_chv, sizeof(cdb));

	/* Set the CHV number */
	cdb[sim_devices[device->index].verify_chv_chvnum_offset] = chv;

	if ((data = malloc(GSM_CMD_VERIFY_CHV_DATA_LEN)) == NULL)
		return SCSISIM_MEMORY_ALLOCATION_ERROR;

	/* Set up the data block with the specified PIN */
	memset(data, 0xff, GSM_CMD_VERIFY_CHV_DATA_LEN);
	memcpy(data, pin, strlen(pin));

	/* Set up the command block */
	my_cmd.direction = SIM_WRITE;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = GSM_CMD_VERIFY_CHV_DATA_LEN;
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	free(data);

	/* No sense data if successful; error condition = sense data */
	if (my_cmd.sense_xfered)
		ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);

	return ret;
}

/**
 * For information about this function, see scsisim.h
 */
int scsisim_send_raw_command(const struct scsisim_dev *device,
			     uint8_t direction,
			     uint8_t command,
			     uint8_t P1,
			     uint8_t P2,
			     uint8_t P3,
			     uint8_t *data,
			     unsigned int len)
{
	int ret;
	struct scsi_cmd my_cmd = { 0 };
	uint8_t cdb[sim_devices[device->index].cdb_len];
	uint8_t sense[sim_devices[device->index].sense_len];

	if (device == NULL)
		return SCSISIM_INVALID_PARAM;

	memset(sense, 0, sizeof(sense));

	/* Get the base raw command for the current device */
	memcpy(cdb, sim_devices[device->index].CDB_raw_cmd, sizeof(cdb));

	/* Set the command parameters */
	cdb[sim_devices[device->index].raw_cmd_direction_offset] =
		(direction == SIM_WRITE) ? sim_devices[device->index].scsi_cmd_write : sim_devices[device->index].scsi_cmd_read;
	cdb[sim_devices[device->index].raw_cmd_gsm_cmd_offset] = command;
	cdb[sim_devices[device->index].raw_cmd_p1_offset] = P1;
	cdb[sim_devices[device->index].raw_cmd_p2_offset] = P2;
	cdb[sim_devices[device->index].raw_cmd_p3_offset] = P3;

	/* Set up the command block */
	my_cmd.direction = direction;
	my_cmd.cdb = cdb;
	my_cmd.cdb_len = (uint8_t)sizeof(cdb);
	my_cmd.data = data;
	my_cmd.data_len = len;
	my_cmd.sense = sense;
	my_cmd.sense_len = (uint8_t)sizeof(sense);

	/* Send the command */
	ret = scsi_send_cdb(device, &my_cmd);

	if (scsisim_verbose())
	{
		if (my_cmd.data_xfered != len)
			scsisim_pinfo("%s: bytes transferred (%d) is less than data buffer length (%d)",
				      __func__, my_cmd.data_xfered, len);
	}

	/* If there is sense data, process it and use it as the return code instead: */
	if (my_cmd.sense_xfered)
		ret = sim_process_scsi_sense(device, my_cmd.sense, my_cmd.sense_xfered);

	return ret;
}

/**
 * Function: sim_free_device_name
 *
 * Parameters:
 * device:	Pointer to scsisim_dev struct.
 *
 * Description: 
 * Given a pointer to an scsisim_dev struct, this function does all of the
 * necessary memory cleanup.
 *
 * Return values: 
 * None
 */
static inline void sim_free_device_name(struct scsisim_dev *device)
{
	free(device->name);
	device->name = NULL;
}

/**
 * Function: sim_process_scsi_sense
 *
 * Parameters:
 * device:	Pointer to scsisim_dev struct.
 * sense:	Sense buffer.
 * len:		Length of sense buffer.
 *
 * Description: 
 * Given a buffer of SCSI sense data, parse out the correct return value.
 * See GSM TS 100 977, section 9.4 for status conditions returned by SIM.
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_SCSI_NO_SENSE_DATA
 * SCSISIM_SCSI_UNKNOWN_SENSE_DATA
 * Number of bytes in response data
 * SCSISIM_GSM_* error code
 */
static int sim_process_scsi_sense(const struct scsisim_dev *device,
				  const uint8_t *sense,
				  unsigned int len)
{
	int ret;

	if (len < sim_devices[device->index].sense_ascq_offset + 1u)
		return SCSISIM_SCSI_NO_SENSE_DATA;

	/* 0x70 = Fixed format, current sense. See SCSI spec for more info */
	if (sense[sim_devices[device->index].sense_type_offset] != 0x70)
	    return SCSISIM_SCSI_UNKNOWN_SENSE_DATA;

	/* Examine the ASC (additional sense code). This corresponds to 
	 * 'SW1' (status word 1) in the GSM spec. */
	switch (sense[sim_devices[device->index].sense_asc_offset])
	{
		case 0x67:
			ret = SCSISIM_GSM_ERROR_PARAM_3;
			break;
		case 0x6b:
			ret = SCSISIM_GSM_ERROR_PARAM_1_OR_2;
			break;
		case 0x6d:
			ret = SCSISIM_GSM_UNKNOWN_INSTRUCTION;
			break;
		case 0x6e:
			ret = SCSISIM_GSM_WRONG_INSTRUCTION_CLASS;
			break;
		case 0x6f:
			ret = SCSISIM_GSM_TECHNICAL_PROBLEM;
			break;
		case 0x90:	/* "Responses to commands which are correctly
				  executed" */
			/* Examine the ASCQ (additional sense code qualifier).
			 * This corresponds to 'SW2' (status word 2) in the
			 * GSM spec. */
			switch (sense[sim_devices[device->index].sense_ascq_offset])
			{
				case 0x00:
					ret = SCSISIM_SUCCESS;
					break;
				default:
					ret = SCSISIM_GSM_UNKNOWN_SW2;
					break;
			}
			break;
		case 0x92:	/* "Memory management" */
			/* Examine the ASCQ (additional sense code qualifier).
			 * This corresponds to 'SW2' (status word 2) in the
			 * GSM spec. */
			switch (sense[sim_devices[device->index].sense_ascq_offset])
			{
				case 0x40:
					ret = SCSISIM_GSM_MEMORY_ERROR;
					break;
				default:
					/* According to the spec: "Command 
					 * successful but after using an
					 * internal update retry routine." */
					ret = SCSISIM_SUCCESS;
					break;
			}
			break;
		case 0x93:	/* "Responses to commands which are postponed" */
			ret = SCSISIM_GSM_BUSY;
			break;
		case 0x94:	/* "Referencing management" */
			/* Examine the ASCQ (additional sense code qualifier).
			 * This corresponds to 'SW2' (status word 2) in the
			 * GSM spec. */
			switch (sense[sim_devices[device->index].sense_ascq_offset])
			{
				case 0x00:
					ret = SCSISIM_GSM_NO_EF_SELECTED;
					break;
				case 0x02:
					ret = SCSISIM_GSM_INVALID_ADDRESS;
					break;
				case 0x04:
					ret = SCSISIM_GSM_FILE_NOT_FOUND;
					break;
				case 0x08:
					ret = SCSISIM_GSM_FILE_INCONSISTENT_WITH_COMMAND;
					break;
				default:
					ret = SCSISIM_GSM_UNKNOWN_SW2;	/* unknown ASCQ */
					break;
			}
			break;
		case 0x98:	/* "Security management" */
			/* See GSM spec section 9.4.5 for specific codes */
			switch (sense[sim_devices[device->index].sense_ascq_offset])
			{
				case 0x02:
					ret = SCSISIM_GSM_NO_CHV_INITIALIZED;
					break;
				case 0x04:
					ret = SCSISIM_GSM_CHV_VERIFICATION_FAILED;
					break;
				case 0x08:
					ret = SCSISIM_GSM_CHV_STATUS_CONTRADICTION;
					break;
				case 0x10:
					ret = SCSISIM_GSM_INVALIDATION_STATUS_CONTRADICTION;
					break;
				case 0x40:
					ret = SCSISIM_GSM_CHV_BLOCKED;
					break;
				case 0x50:
					ret = SCSISIM_GSM_INCREASE_FAILED;
					break;
				default:
					ret = SCSISIM_GSM_SECURITY_ERROR;
					break;
			}
			break;
		case 0x91:	/* Command for ME (Mobile Equipment) */
		case 0x9e:	/* SIM data download error */
		case 0x9f:	/* Normal response data: return the number of
				   bytes that should be read in GET RESPONSE */
			ret = sense[sim_devices[device->index].sense_ascq_offset];
			break;
		default:
			if (scsisim_verbose())
				scsisim_pinfo("%s: unknown GSM Status Word 1 (%d); Status Word 2 = %d",
				      __func__,
				      sense[sim_devices[device->index].sense_asc_offset],
				      sense[sim_devices[device->index].sense_ascq_offset]);
			ret = SCSISIM_GSM_UNKNOWN_SW1;
			break;
	}

	if (scsisim_verbose())
	{
		if (ret <= 0)
			scsisim_pinfo("%s: returning %d (%s)", __func__, ret, scsisim_strerror(ret));
		else
			scsisim_pinfo("%s: returning %d", __func__, ret);
	}

	return ret;
}

/* EOF */

