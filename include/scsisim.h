/*
 *  scsisim.h
 *  API interface file for the scsisim library.
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

#ifndef __SCSISIM_H__
#define __SCSISIM_H__

#include <stdbool.h>
#include <stdint.h>

/* Global verbose output flag */
extern bool scsisim_verbose;

/* API return values -- general */
#define SCSISIM_SUCCESS				 0
#define SCSISIM_DEVICE_OPEN_FAILED		-1
#define SCSISIM_DEVICE_CLOSE_FAILED		-2
#define SCSISIM_DEVICE_NOT_SUPPORTED		-3
#define SCSISIM_INVALID_FILE_DESCRIPTOR		-4
#define SCSISIM_SYSFS_CHDIR_FAILED		-5
#define SCSISIM_USB_VENDOR_OPEN_FAILED		-6
#define SCSISIM_USB_PRODUCT_OPEN_FAILED		-7
#define SCSISIM_SCSI_SEND_ERROR			-8
#define SCSISIM_SCSI_NO_SENSE_DATA		-9
#define SCSISIM_SCSI_UNKNOWN_SENSE_DATA		-10
#define SCSISIM_INVALID_PIN			-11
#define SCSISIM_MEMORY_ALLOCATION_ERROR		-12
#define SCSISIM_INVALID_PARAM			-13
#define SCSISIM_INVALID_GSM_RESPONSE		-14
#define SCSISIM_INVALID_DEVICE_NAME		-15
#define SCSISIM_SMS_INVALID_STATUS		-16
#define SCSISIM_SMS_INVALID_SMSC		-17
#define SCSISIM_SMS_INVALID_ADDRESS		-18

/* API return values -- GSM error codes */
#define SCSISIM_GSM_ERROR_PARAM_3		-20
#define SCSISIM_GSM_ERROR_PARAM_1_OR_2		-21
#define SCSISIM_GSM_UNKNOWN_INSTRUCTION		-22
#define SCSISIM_GSM_WRONG_INSTRUCTION_CLASS	-23
#define SCSISIM_GSM_TECHNICAL_PROBLEM		-24
#define SCSISIM_GSM_MEMORY_ERROR		-25
#define SCSISIM_GSM_BUSY			-26
#define SCSISIM_GSM_NO_EF_SELECTED		-27
#define SCSISIM_GSM_INVALID_ADDRESS		-28
#define SCSISIM_GSM_FILE_NOT_FOUND		-29
#define SCSISIM_GSM_FILE_INCONSISTENT_WITH_COMMAND	-30
#define SCSISIM_GSM_UNKNOWN_SW1			-31
#define SCSISIM_GSM_UNKNOWN_SW2			-32
#define SCSISIM_GSM_NO_CHV_INITIALIZED		-33
#define SCSISIM_GSM_CHV_VERIFICATION_FAILED	-34
#define SCSISIM_GSM_CHV_STATUS_CONTRADICTION	-35
#define SCSISIM_GSM_INVALIDATION_STATUS_CONTRADICTION	-36
#define SCSISIM_GSM_CHV_BLOCKED			-37
#define SCSISIM_GSM_INCREASE_FAILED		-38
#define SCSISIM_GSM_SECURITY_ERROR		-39
#define SCSISIM_GSM_INVALID_ADN_RECORD		-40

/* Master file and 'root' file IDs: use these
 * in scsisim_select_file() calls */
#define GSM_FILE_MF			0x3f00
#define GSM_FILE_EF_ELP			0x2f05
#define GSM_FILE_EF_ICCID		0x2fe2

/* Telecom file IDs: use these in scsisim_select_file() calls */
#define GSM_FILE_DF_TELECOM		0x7f10
#define GSM_FILE_EF_ADN			0x6f3a
#define GSM_FILE_EF_FDN			0x6f3b
#define GSM_FILE_EF_SMS			0x6f3c
#define GSM_FILE_EF_CCP			0x6f3d
#define GSM_FILE_EF_MSISDN		0x6f40
#define GSM_FILE_EF_SMSP		0x6f42
#define GSM_FILE_EF_SMSS		0x6f43
#define GSM_FILE_EF_LND			0x6f44
#define GSM_FILE_EF_SMSR		0x6f47
#define GSM_FILE_EF_SDN			0x6f49
#define GSM_FILE_EF_EXT1		0x6f4a
#define GSM_FILE_EF_EXT2		0x6f4b
#define GSM_FILE_EF_EXT3		0x6f4c
#define GSM_FILE_EF_BDN			0x6f4d
#define GSM_FILE_EF_EXT4		0x6f4e

/* GSM file IDs: use these in scsisim_select_file() calls */
#define GSM_FILE_DF_GSM			0x7f20
#define GSM_FILE_EF_LP			0x6f05
#define GSM_FILE_EF_IMSI		0x6f07
#define GSM_FILE_EF_KC			0x6f20
#define GSM_FILE_EF_DCK			0x6f2c
#define GSM_FILE_EF_PLMNSEL		0x6f30
#define GSM_FILE_EF_HPLMN		0x6f31
#define GSM_FILE_EF_CNL			0x6f32
#define GSM_FILE_EF_ACMMAX		0x6f37
#define GSM_FILE_EF_SST			0x6f38
#define GSM_FILE_EF_ACM			0x6f39
#define GSM_FILE_EF_GID1		0x6f3e
#define GSM_FILE_EF_GID2		0x6f3f
#define GSM_FILE_EF_PUCT		0x6f41
#define GSM_FILE_EF_CBMI		0x6f45
#define GSM_FILE_EF_SPN			0x6f46
#define GSM_FILE_EF_CBMID		0x6f48
#define GSM_FILE_EF_CBMIR		0x6f50
#define GSM_FILE_EF_NIA			0x6f51
#define GSM_FILE_EF_KCGPRS		0x6f52
#define GSM_FILE_EF_LOCIGPRS		0x6f53
#define GSM_FILE_EF_BCCH		0x6f74
#define GSM_FILE_EF_ACC			0x6f78
#define GSM_FILE_EF_FPLMN		0x6f7b
#define GSM_FILE_EF_LOCI		0x6f7e
#define GSM_FILE_EF_AD			0x6fad
#define GSM_FILE_EF_PHASE		0x6fae
#define GSM_FILE_EF_VGCS		0x6fb1
#define GSM_FILE_EF_VGCSS		0x6fb2
#define GSM_FILE_EF_VBS			0x6fb3
#define GSM_FILE_EF_VBSS		0x6fb4
#define GSM_FILE_EF_EMLPP		0x6fb5
#define GSM_FILE_EF_AAEM		0x6fb6
#define GSM_FILE_EF_ECC			0x6fb7


/* Struct to hold SCSI generic device */
struct sg_dev {
	int fd;			/* File descriptor */
	unsigned int index;	/* Index into sim_devices[] in device.h */
	char *name;		/* Name, such as "sg3" */
};

/* Struct to hold fields for master file and directory files:
 * See GSM spec, 9.2.1 SELECT command*/
struct GSM_MF_DF {
	uint16_t file_memory;
	uint16_t file_id;
	uint8_t file_type;
	uint8_t characteristics;
	bool CHV1_enabled;
	uint8_t df_children;
	uint8_t ef_children;
	uint8_t num_chvs;
	bool CHV1_initialized;
	uint8_t CHV1_attempts_remaining;
	uint8_t CHV1_unblock_attempts_remaining;
	bool CHV2_initialized;
	uint8_t CHV2_attempts_remaining;
	uint8_t CHV2_unblock_attempts_remaining;
};

/* Struct to hold fields for elementary files: 
 * See GSM spec, 9.2.1 SELECT command */
struct GSM_EF {
	uint16_t file_size;
	uint16_t file_id;
	uint8_t file_type;
	/*int access_conditions;*/	/* $TODO */
	uint8_t status;
	uint8_t structure;
	uint8_t record_len;
};

/* Struct to hold everything from a GSM GET RESPONSE
 * command */
struct GSM_response {
	int command;
	union {
		struct GSM_MF_DF mf_df;
		struct GSM_EF ef;
	} type;
};


/* GET RESPONSE command constants */
enum {
	SELECT_EF = 1,
	SELECT_MF_DF,
	RUN_GSM_ALGORITHM,	/* $TODO */
	SEEK,			/* $TODO */
	INCREASE,		/* $TODO */
	ENVELOPE		/* $TODO */
};

/* SCSI direction constants */
enum {
	SIM_NO_XFER = 0,
	SIM_WRITE,
	SIM_READ
};


/**
 * Function: scsisim_open_device
 *
 * Parameters:
 * dev_name:	Name of SCSI generic device to open, e.g., 'sg1'.
 * device:	Pointer to sg_dev struct.
 *
 * Description: 
 * Given the name of a SCSI generic device (e.g., 'sg3'), this function 
 * obtains a file descriptor to the associated device file (e.g., '/dev/sg1').
 *
 * NOTE: Depending on your Linux distribution, you may need to add the user 
 * to the 'disk' (Debian 8) or 'fuse' (Debian 7) group to allow direct access 
 * to the device.
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_DEVICE_NAME
 * SCSISIM_MEMORY_ALLOCATION_ERROR
 * SCSISIM_DEVICE_OPEN_FAILED
 */
int scsisim_open_device(const char *dev_name, struct sg_dev *device);


/**
 * Function: scsisim_close_device
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 *
 * Description: 
 * Given a pointer to an sg_dev struct, this function closes the file 
 * descriptor to the associated device file and does other cleanup.
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * SCSISIM_DEVICE_CLOSE_FAILED
 * SCSISIM_INVALID_FILE_DESCRIPTOR
 */
int scsisim_close_device(struct sg_dev *device);


/**
 * Function: scsisim_init_device
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 *
 * Description: 
 * Given a pointer to an sg_dev struct, this function makes sure the 
 * attached USB device is supported, and if so, sends SCSI 
 * initialization commands to the device.
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * SCSISIM_DEVICE_NOT_SUPPORTED
 * Return value from usb_get_vendor_product
 * Return value from scsi_send_cdb
 */
int scsisim_init_device(struct sg_dev *device);


/**
 * Function: scsisim_select_file
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * file:	ID number of file to select (2 bytes, per GSM 
 *		standard -- you can use the GSM_FILE_* constants
 *		defined above).
 *
 * Description: 
 * Run the GSM SELECT command on the given file ID.
 * See GSM TS 100 977, sections 8.1 and 9.2.1
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense: number of bytes waiting in GET RESPONSE
 * SCSISIM_SCSI_NO_SENSE_DATA
 */
int scsisim_select_file(const struct sg_dev *device, uint16_t file);


/**
 * Function: scsisim_get_response
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * data:	Buffer for response data.
 * len:		Length of data buffer.
 * command:	The GSM command to get a response for: see GET RESPONSE command constants.
 * resp:	Pointer to GSM_response struct.
 *
 * Description: 
 * Run the GSM GET RESPONSE command.
 * See GSM TS 100 977, section 9.2.18
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense
 */
int scsisim_get_response(const struct sg_dev *device,
			 uint8_t *data,
			 uint8_t len,
			 int command,
			 struct GSM_response *resp);


/**
 * Function: scsisim_select_file_and_get_response
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * file:	ID number of file to select (2 bytes, per GSM 
 *		standard -- you can use the GSM_FILE_* constants
 *		defined above).
 * data:	Buffer for response data.
 * len:		Length of data buffer.
 * command:	The GSM command to get a response for: see GET RESPONSE command constants.
 * resp:	Pointer to GSM_response struct.
 *
 * Description: 
 * Wrapper function -- combines scsisim_select_file() and 
 * scsisim_get_response() into one function.
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * Return value from scsisim_select_file
 * Return value from scsisim_get_response
 */
int scsisim_select_file_and_get_response(const struct sg_dev *device,
					 uint16_t file,
					 uint8_t *data,
					 uint8_t len,
					 int command,
					 struct GSM_response *resp);


/**
 * Function: scsisim_read_record
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * recno:	Record number to read in currently selected file (records
 *		start at 1, NOT zero!).
 * data:	Buffer for record data.
 * len:		Length of data buffer.
 *
 * Description: 
 * Run the GSM READ RECORD command.
 * See GSM TS 100 977, sections 8.5 and 9.2.5
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense
 */
int scsisim_read_record(const struct sg_dev *device,
			uint8_t recno,
			uint8_t *data,
			uint8_t len);


/**
 * Function: scsisim_read_binary
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * data:	Buffer for binary data.
 * offset:	Offset from which to begin read (zero-based).
 * len:		Length of data buffer.
 *
 * Description: 
 * Run the GSM READ BINARY command.
 * See GSM TS 100 977, sections 8.3 and 9.2.3
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense
 */
int scsisim_read_binary(const struct sg_dev *device,
			uint8_t *data,
			uint16_t offset,
			uint8_t len);


/**
 * Function: scsisim_update_record
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * recno:	Record number to update in currently selected file (records
 *		start at 1, NOT zero!).
 * data:	Buffer for record data.
 * len:		Length of data buffer.
 *
 * Description: 
 * Run the GSM UPDATE RECORD command.
 * See GSM TS 100 977, sections 8.6 and 9.2.6
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense
 */
int scsisim_update_record(const struct sg_dev *device,
			  uint8_t recno,
			  uint8_t *data,
			  uint8_t len);


/**
 * Function: scsisim_update_binary
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * data:	Buffer for binary data.
 * offset:	Offset from which to begin write (zero-based).
 * len:		Length of data buffer.
 *
 * Description: 
 * Run the GSM UPDATE BINARY command.
 * See GSM TS 100 977, sections 8.4 and 9.2.4
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense
 */
int scsisim_update_binary(const struct sg_dev *device,
			  uint8_t *data,
			  uint16_t offset,
			  uint8_t len);


/**
 * Function: scsisim_verify_chv
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * chv:		Number of CHV to check (i.e., CHV1, CHV2).
 * pin:		PIN to use for verification.
 *
 * Description: 
 * Run the GSM VERIFY CHV command. If you have a SIM card with
 * a PIN enabled, you must run this command to access certain 
 * files and directories.
 * See GSM TS 100 977, sections 8.9 and 9.2.9
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * SCSISIM_INVALID_PIN
 * SCSISIM_GSM_ERROR_PARAM_3
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense
 */
int scsisim_verify_chv(const struct sg_dev *device,
		       uint8_t chv,
		       const char *pin);


/**
 * Function: scsisim_send_raw_command
 *
 * Parameters:
 * device:	Pointer to sg_dev struct.
 * direction:	SIM_READ or SIM_WRITE.
 * command:	GSM command code to run.
 * P1:		Parameter 1 for the GSM command.
 * P2:		Parameter 2 for the GSM command.
 * P3:		Parameter 3 for the GSM command.
 * data:	Data buffer for command (can be NULL).
 * len:		Length of data buffer.
 *
 * Description: 
 * Run an arbitrary GSM command not handled by any of the 
 * previous scsisim_* functions. This function assumes you know
 * what you are doing, as you can mess up a SIM card pretty
 * good by sending arbitrary commands.
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * Return value from scsi_send_cdb
 * Return value from sim_process_scsi_sense
 */
int scsisim_send_raw_command(const struct sg_dev *device,
			     uint8_t direction,
			     uint8_t command,
			     uint8_t P1,
			     uint8_t P2,
			     uint8_t P3,
			     uint8_t *data,
			     unsigned int len);


/**
 * Function: scsisim_parse_sms
 *
 * Parameters:
 * record:		Pointer to raw SMS record.
 * record_len:		Length of SMS record.
 *
 * Description: 
 * Given a pointer to an SMS record, parse out and print the 
 * following information:
 *	SMS status (unused space, message read, etc.)
 *	SMS Center number
 *	Sender / recipient number
 *	Message timestamp
 *	Message text
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_PARAM
 * SCSISIM_SMS_INVALID_STATUS
 * SCSISIM_SMS_INVALID_SMSC
 * SCSISIM_MEMORY_ALLOCATION_ERROR
 * SCSISIM_SMS_INVALID_ADDRESS
 */
int scsisim_parse_sms(const uint8_t *record, uint8_t record_len);


/**
 * Function: scsisim_parse_adn
 *
 * Parameters:
 * record:		Pointer to raw ADN record.
 * record_len:		Length of ADN record.
 *
 * Description: 
 * Given a pointer to an ADN (abbreviated dialling number) record, 
 * parse out and print the number and name. "ADN" is just a fancy term 
 * for a contact.
 *
 * Return value: 
 * SCSISIM_SUCCESS
 * SCSISIM_GSM_INVALID_ADN_RECORD
 * SCSISIM_MEMORY_ALLOCATION_ERROR
 */
int scsisim_parse_adn(const uint8_t *record, uint8_t record_len);


/**
 * Function: scsisim_map_gsm_chars
 *
 * Parameters:
 * src:			Pointer to unpacked buffer.
 * src_len:		Length of buffer.
 *
 * Description: 
 * Given an unpacked source buffer of GSM character codes, map them to 
 * their corresponding characters. Handles basic and extended 
 * characters in the GSM 7-bit alphabet.
 *
 * Return value: 
 * Pointer to null-terminated string. Caller's responsibility to free buffer when done.
 */
char *scsisim_map_gsm_chars(const uint8_t *src, unsigned int src_len);


/**
 * Function: scsisim_get_gsm_text
 *
 * Parameters:
 * packed:		Pointer to buffer of packed septets.
 * packed_len:		Length of packed buffer.
 * num_septets:		Number of septets in packed buffer.
 *
 * Description: 
 * Given a binary array of packed septets, unpack the septets into octets (bytes),
 * and then map those values to their corresponding characters in the GSM alphabet.
 *
 * Return value: 
 * Pointer to unpacked, null-terminated string. Caller's responsibility to free buffer when done.
 */
char *scsisim_get_gsm_text(const uint8_t *packed,
			   unsigned int packed_len,
			   unsigned int num_septets);


/**
 * Function: scsisim_packed_bcd_to_ascii
 *
 * Parameters:
 * bcd:			Pointer to packed BCD buffer.
 * len:			Length of packed BCD buffer.
 * little_endian:	Whether the nibbles of each byte are little endian (true) or big endian (false).
 * strip_sign_flag:	Strip terminating sign flag (usually 0xf) from unpacked ASCII string.
 * use_telecom_digits:	Use telecom digits (*#, etc) in translation to ASCII chars.
 *
 * Description: 
 * Convert a packed BCD buffer to an ASCII string.
 *
 * Return value: 
 * Pointer to ASCII buffer. Caller's responsibility to free buffer when done.
 */
char *scsisim_packed_bcd_to_ascii(const uint8_t *bcd,
				  const unsigned int len,
				  bool little_endian,
				  bool strip_sign_flag,
				  bool use_telecom_digits);


/**
 * Function: scsisim_unpack_septets
 *
 * Parameters:
 * num_septets:		Number of septets in the packed buffer.
 * packed:		Pointer to buffer of packed septets.
 * packed_len:		Length of packed buffer in bytes.
 * unpacked:		(Output) Pointer to unpacked buffer of octets. 
 *			Caller's responsibility to free buffer when done.
 * unpacked_len:	(Output) Length of unpacked buffer.
 *
 * Description: 
 * Unpack a buffer of packed septets into a buffer of octets (bytes).
 *
 * Return values: 
 * None
 */
void scsisim_unpack_septets(const unsigned int num_septets,
			    const uint8_t *packed,
			    const unsigned int packed_len,
			    uint8_t **unpacked,
			    unsigned int *unpacked_len);


/**
 * Function: scsisim_strerror
 *
 * Parameters:
 * err:			Error code number.
 *
 * Description: 
 * Given an error code, return a corresponding human-readable string.
 *
 * Based in part on sample code from 'The Linux Programming Interface'
 * by Michael Kerrisk; see:
 * http://man7.org/tlpi/code/online/dist/threads/strerror.c.html
 *
 * Return value: 
 * Pointer to string message.
 */
char *scsisim_strerror(int err);


/**
 * Function: scsisim_perror
 *
 * Parameters:
 * str:			Optional user-defined string.
 * err:			Error code number.
 *
 * Description: 
 * Given an error code and an optional user-defined string, print 
 * a formatted error message to stderr (includes newline).
 *
 * Return values: 
 * None
 */
void scsisim_perror(const char *str, int err);


/**
 * Function: scsisim_pinfo
 *
 * Parameters:
 * format:		Message format.
 * ...:			Optional arguments.
 *
 * Description: 
 * Print a formatted diagnostic message to stderr (includes newline). 
 *
 * Return values: 
 * None
 */
void scsisim_pinfo(const char* format, ...);


/**
 * Function: scsisim_printf
 *
 * Parameters:
 * format:		Message format.
 * ...:			Optional arguments.
 *
 * Description: 
 * Print a message to stdout (does not include newline).
 *
 * Return values: 
 * None
 */
void scsisim_printf(const char* format, ...);


#endif /* __SCSISIM_H__ */

/* EOF */

