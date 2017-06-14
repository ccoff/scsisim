/*
 *  utils.c
 *  Utility functions for the scsisim library.
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
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

#include "scsisim.h"
#include "utils.h"

#define ROW_SIZE	16

#define MAX_STRERROR	128

bool scsisim_verbose = false;

static const char BCD_basic_digits[]="0123456789abcdef";
static const char BCD_telecom_digits[]="0123456789*#,--f";

static char error_buf[MAX_STRERROR];

static const char *error_list[] = {
	"Operation succeeded",				/* 0 - SCSISIM_SUCCESS */
	"Device open failed",				/* 1 - SCSISIM_DEVICE_OPEN_FAILED */
	"Device close failed",				/* 2 - SCSISIM_DEVICE_CLOSE_FAILED */
	"Device not supported",				/* 3 - SCSISIM_DEVICE_NOT_SUPPORTED */
	"Invalid file descriptor",			/* 4 - SCSISIM_INVALID_FILE_DESCRIPTOR */
	"sysfs directory traversal failed",		/* 5 - SCSISIM_SYSFS_CHDIR_FAILED */
	"USB vendor file open failed",			/* 6 - SCSISIM_USB_VENDOR_OPEN_FAILED */
	"USB product file open failed",			/* 7 - SCSISIM_USB_PRODUCT_OPEN_FAILED */
	"ioctl() for SCSI send failed",			/* 8 - SCSISIM_SCSI_SEND_ERROR */
	"No SCSI sense data",				/* 9 - SCSISIM_SCSI_NO_SENSE_DATA */
	"Unknown SCSI sense data",			/* 10 - SCSISIM_SCSI_UNKNOWN_SENSE_DATA */
	"Invalid PIN",					/* 11 - SCSISIM_INVALID_PIN */
	"Memory allocation error",			/* 12 - SCSISIM_MEMORY_ALLOCATION_ERROR */
	"Invalid parameter",				/* 13 - SCSISIM_INVALID_PARAM */
	"Invalid GSM response",				/* 14 - SCSISIM_INVALID_GSM_RESPONSE */
	"Invalid device name",				/* 15 - SCSISIM_INVALID_DEVICE_NAME */
	"Invalid SMS status",				/* 16 - SCSISIM_SMS_INVALID_STATUS */
	"Invalid SMS Center number",			/* 17 - SCSISIM_SMS_INVALID_SMSC */
	"Invalid SMS address",				/* 18 - SCSISIM_SMS_INVALID_ADDRESS */
	NULL,						/* 19 - Reserved for future use */
	"GSM: Incorrect parameter P3",			/* 20 - SCSISIM_GSM_ERROR_PARAM_3 */
	"GSM: Incorrect parameter P1 or P2",		/* 21 - SCSISIM_GSM_ERROR_PARAM_1_OR_2 */
	"GSM: Unknown instruction code in command",	/* 22 - SCSISIM_GSM_UNKNOWN_INSTRUCTION */
	"GSM: Wrong instruction class in command",	/* 23 - SCSISIM_GSM_WRONG_INSTRUCTION_CLASS */
	"GSM: Technical problem with no diagnostic given", /* 24 - SCSISIM_GSM_TECHNICAL_PROBLEM */
	"GSM: Memory problem",				/* 25 - SCSISIM_GSM_MEMORY_ERROR */
	"GSM: SIM Application Toolkit busy",		/* 26 - SCSISIM_GSM_BUSY */
	"GSM: No EF selected",				/* 27 - SCSISIM_GSM_NO_EF_SELECTED */
	"GSM: Out of range (invalid address)",		/* 28 - SCSISIM_GSM_INVALID_ADDRESS */
	"GSM: File ID or pattern not found",		/* 29 - SCSISIM_GSM_FILE_NOT_FOUND */
	"GSM: File inconsistent with command",		/* 30 - SCSISIM_GSM_FILE_INCONSISTENT_WITH_COMMAND */
	"GSM: Unknown status word SW1",			/* 31 - SCSISIM_GSM_UNKNOWN_SW1 */
	"GSM: Unknown status word SW2",			/* 32 - SCSISIM_GSM_UNKNOWN_SW2 */
	"GSM: No CHV initialized",			/* 33 - SCSISIM_GSM_NO_CHV_INITIALIZED */
	"GSM: CHV verification failed",			/* 34 - SCSISIM_GSM_CHV_VERIFICATION_FAILED */
	"GSM: CHV status contradiction",		/* 35 - SCSISIM_GSM_CHV_STATUS_CONTRADICTION */
	"GSM: Invalidation status contradiction",	/* 36 - SCSISIM_GSM_INVALIDATION_STATUS_CONTRADICTION */
	"GSM: CHV blocked",				/* 37 - SCSISIM_GSM_CHV_BLOCKED */
	"GSM: Increase cannot be performed (max value reached)", /* 38 - SCSISIM_GSM_INCREASE_FAILED */
	"GSM: Security error",				/* 39 - SCSISIM_GSM_SECURITY_ERROR */
	"GSM: Invalid ADN record",			/* 40 - SCSISIM_GSM_INVALID_ADN_RECORD */
};

#define MAXERR	(sizeof(error_list) / sizeof(error_list[0]))

/**
 * Function: print_binary_buffer
 *
 * Parameters:
 * buf:	Pointer to binary buffer.
 * len:	Length of binary buffer.
 *
 * Description: 
 * Print out a nicely formatted hex dump of a binary buffer, similar
 * to what `hexdump -C` does.
 *
 * Return values: 
 * None
 */
void print_binary_buffer(const unsigned char *buf, const unsigned int len)
{
	unsigned int i;
	char ascii[ROW_SIZE + 1] = "";

	if (buf == NULL || len <= 0 )
		return;

	for (i = 0; i < len; i++)
	{
		/* Print ASCII string when at end of row */
		if (i % ROW_SIZE == 0 && i != 0)
		{
			fprintf(stderr, "\t%s\n", ascii);
		}

		/* Print hex value of current byte */
		fprintf(stderr, "%02x ", buf[i]);

		/* Build up ASCII string for current row */
		ascii[i % ROW_SIZE] = isprint(buf[i]) ? buf[i] : '.';
		ascii[i % ROW_SIZE + 1] = '\0';
	}

	/* Pad any leftover bytes from the last row */
	while (i % ROW_SIZE != 0)
	{
		fprintf(stderr, "   ");
		i++;
	}

	fprintf(stderr, "\t%s\n", ascii);
}

/**
 * For information about this function, see scsisim.h
 */
char *scsisim_packed_bcd_to_ascii(const unsigned char *bcd,
				  const unsigned int len,
				  bool little_endian,
				  bool strip_sign_flag,
				  bool use_telecom_digits)
{
	unsigned int i;
	unsigned char lo_nibble, hi_nibble;
	char lo_char, hi_char, *ascii = NULL, *tmp = NULL;

	if (bcd == NULL ||
	    len <= 0 ||
	    (ascii = malloc((size_t)len * 2 + 1)) == NULL)
		return NULL;

	tmp = ascii;

	for (i = 0; i < len; i++)
	{
		lo_nibble = bcd[i] & 0xf;
		hi_nibble = bcd[i] >> 4;

		lo_char = use_telecom_digits ? BCD_telecom_digits[lo_nibble] : BCD_basic_digits[lo_nibble];
		hi_char = use_telecom_digits ? BCD_telecom_digits[hi_nibble] : BCD_basic_digits[hi_nibble];

		if (little_endian)
		{
			*tmp++ = lo_char;
			*tmp++ = hi_char;
		}
		else
		{
			*tmp++ = hi_char;
			*tmp++ = lo_char;
		}
	}

	/* Strip off any trailing 'f' characters from the string that are 
	 * functioning as the sign flag */
	if ( strip_sign_flag && *(tmp - 1) == 'f' )
		*(tmp - 1) = '\0';
	else
		*tmp = '\0';

	return ascii;
}

/**
 * For information about this function, see scsisim.h
 */
void scsisim_unpack_septets(const unsigned int num_septets,
			    const unsigned char *packed,
			    const unsigned int packed_len,
			    unsigned char **unpacked,
			    unsigned int *unpacked_len)
{
	unsigned int i, cur_pos;
	unsigned char tmp, *ptr = NULL;

	if (num_septets <= 0 || packed == NULL || packed_len <= 0)
		return;

	/* Calculate the length of the unpacked buffer */
	*unpacked_len = packed_len * 8 / 7;

	if ((*unpacked = malloc((size_t)*unpacked_len)) == NULL)
		return;

	ptr = *unpacked;

	if (scsisim_verbose)
		scsisim_pinfo("%s: unpacked_len = %d septets",
			      __func__, *unpacked_len);

	for (i = 0; i < packed_len; i++)
	{
		cur_pos = i % 7;
		tmp = packed[i];

		/* The first septet in a 7-byte group doesn't require any bit 
		 * shifting, but subsequent septets do: */
		if (cur_pos > 0)
		{
			/* Shift the high bits into position: */
			tmp <<= cur_pos;

			/* Get the low bits from the previous byte: */
			tmp |= packed[i-1] >> (8 - cur_pos);
		}

		tmp &= 0x7f;

		*ptr++ = tmp;

		/* If we're at the end of a 7-byte group, we need to grab the
		 * last (eighth) septet: */
		if (cur_pos == 6)
		{
			tmp = packed[i] >> 1;
			*ptr++ = tmp;
		}

	}

	/* By rounding up packed_len earlier to ensure a whole byte, there
	 * may now be an extra 'unpacked' character -- if so, remove it: */
	if (*unpacked_len > num_septets)
	{
		if (scsisim_verbose)
			scsisim_pinfo("%s: fixing mismatch between unpacked_len (%d) and num_septets (%d)",
				      __func__, *unpacked_len, num_septets);

		*unpacked_len = num_septets;
	}
}

/**
 * Function: is_digit_string
 *
 * Parameters:
 * str:	String to examine.
 *
 * Description: 
 * Determine if the specified string contains only digits (0-9).
 *
 * Return values: 
 * true - string contains only digits
 * false - string contains at least one non-digit
 */
bool is_digit_string(const char *str)
{
	int i;

	if (str == NULL)
		return false;

	for (i = 0; str[i] != '\0'; i++)
	{
		if (!isdigit(str[i]))
			return false;
	}

	return true;
}

/**
 * For information about this function, see scsisim.h
 */
char *scsisim_strerror(int err)
{
	/* Remove negative value */
	err = abs(err);

	if ((unsigned)err >= MAXERR || error_list[err] == NULL)
	{
		snprintf(error_buf, MAX_STRERROR, "Unknown error %d", err);
	}
	else
	{
		strncpy(error_buf, error_list[err], MAX_STRERROR - 1);
		error_buf[MAX_STRERROR - 1] = '\0';
	}

	return error_buf;
}

/**
 * For information about this function, see scsisim.h
 */
void scsisim_perror(const char *str, int err)
{
	if (str == NULL || str[0] == '\0')
		fprintf(stderr, "[ERROR: %s]\n", scsisim_strerror(err));
	else
		fprintf(stderr, "[ERROR: %s: %s]\n", str, scsisim_strerror(err));
}

/**
 * For information about this function, see scsisim.h
 */
void scsisim_pinfo(const char* format, ...)
{
	va_list args;

	fprintf(stderr, "[INFO: ");
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "]\n");
}

/**
 * For information about this function, see scsisim.h
 */
void scsisim_printf(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}

/* EOF */

