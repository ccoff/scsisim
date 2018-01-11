/*
 *  gsm.c
 *  GSM-related functions for the scsisim library.
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
#include <string.h>
#include <stdbool.h>

#include "scsisim.h"
#include "gsm.h"
#include "utils.h"

static void dump_gsm_response(const struct GSM_response *resp);

const char *GSM_basic_charset[] = {
	/* 0x00 to 0x07: */
	"@", "\u00a3", "$", "\u00a5", "\u00e8", "\u00e9", "\u00f9", "\u00ec",
	/* 0x08 to 0x0f: */
	"\u00f2", "\u00c7", "\n", "\u00d8", "\u00f8", "\r", "\u00c5", "\u00e5",
	/* 0x10 to 0x17: */
	"\u0394", "_", "\u03a6", "\u0393", "\u039b", "\u03a9", "\u03a0", "\u03a8",
	/* 0x18 to 0x1f: */
	"\u03a3", "\u0398", "\u039e", "\uffff", "\u00c6", "\u00e6", "\u00df", "\u00c9",
	/* 0x20 to 0x27: */
	" ", "!", "\"", "#", "\u00a4", "%", "&", "'",
	/* 0x28 to 0x2f: */
	"(", ")", "*", "+", ",", "-", ".", "/",
	/* 0x30 to 0x37: */
	"0", "1", "2", "3", "4", "5", "6", "7",
	/* 0x38 to 0x3f: */
	"8", "9", ":", ";", "<", "=", ">", "?",
	/* 0x40 to 0x47: */
	"\u00a1", "A", "B", "C", "D", "E", "F", "G",
	/* 0x48 to 0x4f: */
	"H", "I", "J", "K", "L", "M", "N", "O",
	/* 0x50 to 0x57: */
	"P", "Q", "R", "S", "T", "U", "V", "W",
	/* 0x58 to 0x5f: */
	"X", "Y", "Z", "\u00c4", "\u00d6", "\u00d1", "\u00dc", "\u00a7",
	/* 0x60 to 0x67: */
	"\u00bf", "a", "b", "c", "d", "e", "f", "g",
	/* 0x68 to 0x6f: */
	"h", "i", "j", "k", "l", "m", "n", "o",
	/* 0x70 to 0x77: */
	"p", "q", "r", "s", "t", "u", "v", "w",
	/* 0x78 to 0x7f */
	"x", "y", "z", "\u00e4", "\u00f6", "\u00f1", "\u00fc", "\u00e0"
};

const char *GSM_basic_charset_extension[] = {
	/* 0x00 to 0x07: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x08 to 0x0f: */
	" ", " ", "\f", " ", " ", " ", " ", " ",
	/* 0x10 to 0x17: */
	" ", " ", " ", " ", "^", " ", " ", " ",
	/* 0x18 to 0x1f: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x20 to 0x27: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x28 to 0x2f: */
	"{", "}", " ", " ", " ", " ", " ", "\\",
	/* 0x30 to 0x37: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x38 to 0x3f: */
	" ", " ", " ", " ", "[", "~", "]", " ",
	/* 0x40 to 0x47: */
	"|", " ", " ", " ", " ", " ", " ", " ",
	/* 0x48 to 0x4f: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x50 to 0x57: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x58 to 0x5f: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x60 to 0x67: */
	" ", " ", " ", " ", " ", "\u20ac", " ", " ",
	/* 0x68 to 0x6f: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x70 to 0x77: */
	" ", " ", " ", " ", " ", " ", " ", " ",
	/* 0x78 to 0x7f */
	" ", " ", " ", " ", " ", " ", " ", " "
};

const char *GSM_sms_status[] = {
	"Unused space",
	"Message received and read",
	"[Undefined]",
	"Message received but unread",
	"[Undefined]",
	"Message sent",
	"[Undefined]",
	"Message not sent"
};

const char *GSM_file_type[] = {
	"Reserved",
	"MF",
	"DF",
	"[Undefined]",
	"EF"
};

const char *GSM_ef_structure[] = {
	"Transparent",
	"Linear fixed",
	"[Undefined]",
	"Cyclic"
};


/**
 * For information about this function, see scsisim.h
 */
int scsisim_parse_sms(const uint8_t *record, uint8_t record_len)
{
	/* $FIXUP: This mega-function is just a quick and dirty way to parse 
	 * SMS records. It should be chopped down to size and split into 
	 * multiple functions. */

	const uint8_t *ptr = record;
	unsigned int smsc_len, address_len, msg_len, num_septets, bytes_used, bytes_remaining;
	uint8_t sms_status, charset, *tmp = NULL;
	uint8_t year, month, day, hours, minutes, seconds;
	char *tmp_str1 = NULL, *tmp_str2 = NULL, *tmp_str3 = NULL, *buf = NULL;
	bool is_alphanum = false;

	if (ptr == NULL || record_len != GSM_SMS_RECORD_LEN)
	{
		if (scsisim_verbose())
			scsisim_pinfo("%s: Invalid SMS record or length (%d bytes)",
				      __func__, record_len);
		return SCSISIM_INVALID_PARAM;
	}

	/* Now iterate through the entire SMS record, plucking out whatever is
	 * of interest to us, and skipping the rest. See the GSM spec for more
	 * information about the various fields. */

	/* Get SMS status */
	if (*ptr > sizeof(GSM_sms_status) / sizeof(GSM_sms_status[0]) - 1)
	{
		if (scsisim_verbose())
			scsisim_pinfo("%s: Invalid SMS status %d",
				      __func__, *ptr);
		return SCSISIM_SMS_INVALID_STATUS;
	}

	scsisim_printf("Status:\t%s\n", GSM_sms_status[*ptr++]);

	/* Get SMS Center (SMSC) length: subtract 1 byte for TON/NPI, which we
	 * ignore */
	smsc_len = *ptr++ - 1;

	if (scsisim_verbose())
		scsisim_pinfo("%s: SMS Center length is %d bytes",
			      __func__, smsc_len);

	if (smsc_len <= 0 || smsc_len > GSM_MAX_SMSC_LEN)
	{
		/* The entire record is probably free space or invalid, but
		   we'll press on a bit more to make sure */
		if (scsisim_verbose())
			scsisim_pinfo("%s: Invalid SMS Center length - forcing to %d bytes",
				      __func__, GSM_MAX_SMSC_LEN);

		smsc_len = GSM_MAX_SMSC_LEN;
	}

	/* Skip TON (Type of number) */
	ptr++;

	/* Determine if SMSC number contains valid data */
	if ( *ptr == 0xff )
	{
		if (scsisim_verbose())
			scsisim_pinfo("%s: Invalid SMS Center number - aborting parsing for this record",
				      __func__);

		return SCSISIM_SMS_INVALID_SMSC;
	}

	if ((tmp = malloc((size_t)smsc_len)) == NULL)
		return SCSISIM_MEMORY_ALLOCATION_ERROR;

	memcpy(tmp, ptr, (size_t)smsc_len);

	/* Unpack the SMSC number */
	buf = scsisim_packed_bcd_to_ascii(tmp, smsc_len, true, true, false);

	scsisim_printf("SMSC:\t%s\n", buf);
	free(tmp);
	free(buf);

	/* Advance past SMSC number */
	ptr += smsc_len;

	/* TPDU type: get SMS status */
	sms_status = *ptr++ & 0x03;
	switch (sms_status)
	{
		case 0:		/* SMS-DELIVER, SMS-DELIVER-REPORT */
		case 1:		/* SMS-SUBMIT, SMS-SUBMIT-REPORT */

			/* Skip TP-MR (Message Reference) for SUBMIT records */
			if (sms_status == 1)
				ptr++;

			/* Length of TP-OA (Originating Address) or
			 * TP-DA (Destination Address) in nibbles */
			address_len = (*ptr++ + (2 - 1)) / 2;

			if (address_len < GSM_MIN_ADDRESS_LEN ||
			    address_len > GSM_MAX_ADDRESS_LEN)
			{
				if (scsisim_verbose())
					scsisim_pinfo("%s: Invalid address length (%d bytes)",
						      __func__, address_len);
				return SCSISIM_SMS_INVALID_ADDRESS;
			}

			if (scsisim_verbose())
				scsisim_pinfo("%s: Valid address length (%d bytes)",
					      __func__, address_len);

			/* TON / NPI: all we care about is if message is in
			 * GSM 7-bit alphanumeric instead of BCD digits, 
			 * so examine bits 4-6 */
			if ( (*ptr++ & 0x70) == 0x50 )
				is_alphanum = true;

			/* Get address name or number */
			if ((tmp = malloc((size_t)address_len)) == NULL)
			    return SCSISIM_MEMORY_ALLOCATION_ERROR;

			memcpy(tmp, ptr, (size_t)address_len);

			if ( is_alphanum )
			{
				/* Convert to GSM 7-bit alphanumeric chars */
				num_septets = address_len * 8 / 7;
				buf = scsisim_get_gsm_text(tmp, address_len, num_septets);
			}
			else
			{
				/* Unpack BCD buffer */
				buf = scsisim_packed_bcd_to_ascii(tmp,
								  address_len,
								  true,
								  true,
								  false);
			}

			scsisim_printf("%s:\t%s\n",
				       (sms_status == 1) ? "Recipient": "Sender", buf);
			free(buf);
			free(tmp);

			/* Advance past address number */
			ptr += address_len;

			/* Skip TP-PID (Protocol Identifier) */
			ptr++;

			/* TP-DCS (Data Coding Scheme): bits 2-3 */
			charset = (*ptr++ & 0x0c) >> 2;

			if (sms_status == 1)
			{
				/* Skip TP-VP (Validity Period) for SUBMIT records */
				ptr++;
			}
			else
			{
				/* Get date */
				year = *ptr++;
				month = *ptr++;
				day = *ptr++;
				tmp_str1 = scsisim_packed_bcd_to_ascii(&month,
								       1,
								       true,
								       false,
								       false);
				tmp_str2 = scsisim_packed_bcd_to_ascii(&day,
								       1,
								       true,
								       false,
								       false);
				tmp_str3 = scsisim_packed_bcd_to_ascii(&year,
								       1,
								       true,
								       false,
								       false);

				scsisim_printf("Date:\t%s/%s/20%s\n",
					       tmp_str1, tmp_str2, tmp_str3);

				free(tmp_str1);
				free(tmp_str2);
				free(tmp_str3);

				/* Get time */
				hours = *ptr++;
				minutes = *ptr++;
				seconds = *ptr++;
				tmp_str1 = scsisim_packed_bcd_to_ascii(&hours,
								       1,
								       true,
								       false,
								       false);
				tmp_str2 = scsisim_packed_bcd_to_ascii(&minutes,
								       1,
								       true,
								       false,
								       false);
				tmp_str3 = scsisim_packed_bcd_to_ascii(&seconds,
								       1,
								       true,
								       false,
								       false);

				scsisim_printf("Time:\t%s:%s:%s\n",
					       tmp_str1, tmp_str2, tmp_str3);

				free(tmp_str1);
				free(tmp_str2);
				free(tmp_str3);

				/* Get timezone */
				scsisim_printf("Timezone: %02d\n", *ptr++);
			}

			/* Get text length and data */
			num_septets = *ptr++;
			msg_len = (num_septets * 7 + (8 - 1)) / 8;

			/* Make sure we don't walk off the end of the record buffer */
			bytes_used = ptr - record;
			bytes_remaining = GSM_SMS_RECORD_LEN - bytes_used;

			if (scsisim_verbose())
				scsisim_pinfo("%s: Currently at offset %d in record. %d bytes remaining.",
					      __func__,bytes_used, bytes_remaining);

			if (msg_len <= 0)
			{
				scsisim_printf("Message is empty\n");
				return SCSISIM_SUCCESS;
			}
			else if (msg_len > bytes_remaining)
			{
				/* This should only happen in rare cases, e.g., a corrupted SIM card */
				scsisim_pinfo("%s: Parsed message length (%d bytes) exceeds bytes remaining in record by %d bytes, truncating message text to %d bytes",
					      __func__,
					      msg_len,
					      msg_len - bytes_remaining,
					      bytes_remaining);

				/* Truncate message text to however many bytes remain in record */
				msg_len = bytes_remaining;
			}
			else
			{
				if (scsisim_verbose())
					scsisim_pinfo("%s: TP-UD has %d septets packed into %d bytes",
						      __func__, num_septets, msg_len);
			}

			/* $TODO: Add multi-part SMS support. Currently we only
			 * process single, discrete SMS messages. */

			if ((tmp = malloc((size_t)msg_len)) == NULL)
				return SCSISIM_MEMORY_ALLOCATION_ERROR;

			memcpy(tmp, ptr, (size_t)msg_len);

			/* See 3GPP TS 23.038, section 4, "SMS Data Coding Scheme" */
			switch (charset)
			{
				case 0:		/* 7-bit GSM alphabet */
					if (scsisim_verbose())
						scsisim_pinfo("%s: Using 7-bit GSM character set",
							      __func__);

					buf = scsisim_get_gsm_text(tmp, msg_len, num_septets);

					scsisim_printf("Message: %s\n", buf);

					free(buf);
					break;
				case 1:		/* 8-bit data */
				case 2:		/* $TODO: Add support for UTF-16/UCS-2 */
				case 3:		/* Reserved for Future Use */
				default:
					scsisim_printf("Message: [Unsupported character set]\n");

					if (scsisim_verbose())
						scsisim_pinfo("%s: Character set code %d unsupported",
							      __func__, charset);
					break;
			}

			free(tmp);

			break;
		case 2:		/* SMS-COMMAND, SMS-STATUS-REPORT */
			/* $TODO: Add support for these SMS statuses */
			break;
		default:
			/* Reserved for Future Use */
			break;

	}

	return SCSISIM_SUCCESS;
}


/**
 * For information about this function, see scsisim.h
 */
char *scsisim_get_gsm_text(const uint8_t *packed,
			   unsigned int packed_len,
			   unsigned int num_septets)
{
	uint8_t *unpacked = NULL;
	unsigned int unpacked_len;
	char *smsText = NULL;

	if (packed == NULL || packed_len <= 0 || num_septets <= 0)
		return NULL;

	/* Unpack the block of septets into bytes */
	scsisim_unpack_septets(num_septets,
			       packed,
			       packed_len,
			       &unpacked,
			       &unpacked_len);

	/* Map the GSM character codes to printable characters */
	smsText = scsisim_map_gsm_chars(unpacked, unpacked_len);

	free(unpacked);

	return(smsText);
}


/**
 * For information about this function, see scsisim.h
 */
char *scsisim_map_gsm_chars(const uint8_t *src, unsigned int src_len)
{
	unsigned int i;
	char *result = NULL, *dest;
	const char *ptr;
	bool escapeChar = false;

	/* Allocate maximum of 4 bytes per Unicode character, 
	 * plus a null-terminating character: */
	if (src == NULL || src_len <= 0 ||
	    (result = malloc((size_t)src_len * 4 + 1)) == NULL)
		return NULL;

	dest = result;

	for (i = 0; i < src_len; i++)
	{
		/* Check for invalid character index */
		if (src[i] > 0x7f)
		{
			/* 0xff marks unused bytes, and isn't really "invalid", so: */
			if (scsisim_verbose() && src[i] != 0xff)
				scsisim_pinfo("%s: Invalid GSM character code (%d), %d unmapped characters remaining",
					      __func__, src[i], src_len - i);
			break;
		}

		if (src[i] == GSM_ESCAPE_CHAR)
		{
			escapeChar = true;
			continue;
		}

		if (escapeChar == true)
		{
			/* Look up the extended character code */
			ptr = GSM_basic_charset_extension[src[i]];
			escapeChar = false;
		}
		else
		{
			/* Look up the basic character code */
			ptr = GSM_basic_charset[src[i]];
		}

		/* Avoid repetitive strcat'ing (AKA Shlemiel's algorithm) */
		while ((*dest++ = *ptr++) != '\0')
			;

		/* Back up to null character for next concatenation */
		dest--;
	}

	return result;
}


/**
 * For information about this function, see scsisim.h
 */
int scsisim_parse_adn(const uint8_t *record, uint8_t record_len)
{
	const uint8_t* ptr = record;
	unsigned int name_len, number_len;
	uint8_t *tmp = NULL;
	char *buf = NULL;

	/* Check record_len: 14 bytes (required) for number buffer, 
	 * plus at least one byte for the name */
	if (ptr == NULL || record_len < GSM_ADN_NUMBER_BUFFER_LEN + 1u)
	{
		return SCSISIM_GSM_INVALID_ADN_RECORD;
	}

	/* Calculate the name length */
	name_len = record_len - GSM_ADN_NUMBER_BUFFER_LEN;

	if (*ptr == 0xff)
	{
		scsisim_printf("ADN record unused\n");
		return SCSISIM_SUCCESS;
	}

	/* Get the name */
	buf = scsisim_map_gsm_chars(ptr, name_len);
	scsisim_printf("Contact name:\t%s\n", buf);
	free(buf);

	ptr += name_len;

	/* Get the number length: subtract 1 byte for TON/NPI, which we ignore */
	number_len = *ptr++ - 1;

	if (number_len <= 0 || number_len > GSM_MAX_ADN_NUMBER_LEN )
	{
		scsisim_pinfo("%s: Invalid number_len %d, forcing to %d",
			      __func__, number_len, GSM_MAX_ADN_NUMBER_LEN);
		number_len = GSM_MAX_ADN_NUMBER_LEN;
	}

	/* Skip TON/NPI */
	ptr++;

	if ((tmp = malloc((size_t)number_len)) == NULL)
		return SCSISIM_MEMORY_ALLOCATION_ERROR;

	memcpy(tmp, ptr, (size_t)number_len);

	/* Get the number */
	buf = scsisim_packed_bcd_to_ascii(tmp, number_len, true, true, true);
	scsisim_printf("Contact number:\t%s\n", buf);
	free(buf);
	free(tmp);

	return SCSISIM_SUCCESS;
}


/**
 * Function: gsm_parse_response
 *
 * Parameters:
 * response:		Pointer to buffer containing raw GSM GET RESPONSE data.
 * response_len:	Length of buffer.
 * resp:		(Output) Pointer to GSM_response struct holding parsed data.
 *
 * Description: 
 * Given an array of raw GSM GET RESPONSE data, parse it out into its components
 * and load it into a GSM_response struct. 
 * See GSM spec, section 9.2.1 (SELECT command) for detailed information about 
 * all of the fields parsed by this function.
 *
 * Return values: 
 * SCSISIM_SUCCESS
 * SCSISIM_INVALID_GSM_RESPONSE
 */
int gsm_parse_response(const uint8_t *response,
		       unsigned int response_len,
		       struct GSM_response *resp)
{
	int ret = SCSISIM_SUCCESS;

	if (response == NULL ||
	    (resp->command == SELECT_EF && response_len < GSM_MIN_EF_RESPONSE_LEN) ||
	    (resp->command == SELECT_MF_DF && response_len < GSM_MIN_MF_DF_RESPONSE_LEN))
		return SCSISIM_INVALID_GSM_RESPONSE;

	switch (resp->command)
	{
		case SELECT_EF:
			/* Bytes 0-1: Reserved for future use */
			resp->type.ef.file_size = response[2] << 8;
			resp->type.ef.file_size |= response[3];
			resp->type.ef.file_id = response[4] << 8;
			resp->type.ef.file_id |= response[5];
			resp->type.ef.file_type = response[6];	/* 04 = EF (should always be this) */
			/* Byte 7: Reserved for future use */
			/* resp->type.ef.access_conditions = response[8-10]; */ /* $TODO */
			resp->type.ef.status = response[11];
			resp->type.ef.structure = response[13];
			resp->type.ef.record_len = response[14];
			break;

		case SELECT_MF_DF:
			/* Bytes 0-1: Reserved for future use */
			resp->type.mf_df.file_memory = response[2] << 8;
			resp->type.mf_df.file_memory |= response[3];
			resp->type.mf_df.file_id = response[4] << 8;
			resp->type.mf_df.file_id |= response[5];
			resp->type.mf_df.file_type = response[6];	/* 01 = MF, 02 = DF */
			resp->type.mf_df.characteristics = response[13];
			resp->type.mf_df.CHV1_enabled = (response[13] & 0x80) ? false : true;
			resp->type.mf_df.df_children = response[14];
			resp->type.mf_df.ef_children = response[15];
			resp->type.mf_df.num_chvs = response[16];
			resp->type.mf_df.CHV1_initialized = (response[18] & 0x80) ? true : false;
			resp->type.mf_df.CHV1_attempts_remaining = response[18] & 0x0f;
			resp->type.mf_df.CHV1_unblock_attempts_remaining = response[19] & 0x0f;
			resp->type.mf_df.CHV2_initialized = (response[20] & 0x80) ? true : false;
			resp->type.mf_df.CHV2_attempts_remaining = response[20] & 0x0f;
			resp->type.mf_df.CHV2_unblock_attempts_remaining = response[21] & 0x0f;
			break;

		default:
			scsisim_pinfo("%s: Unsupported response type", __func__);
			break;
	}

	if (scsisim_verbose())
		dump_gsm_response(resp);

	return ret;
}

/**
 * Function: dump_gsm_response
 *
 * Parameters:
 * resp:		Pointer to GSM_response struct holding response data.
 *
 * Description: 
 * Given a pointer to a GSM_response struct, print out its members in a
 * readable format.
 *
 * Return values: 
 * None
 */
static void dump_gsm_response(const struct GSM_response *resp)
{
	switch (resp->command)
	{
		case SELECT_EF:
			scsisim_pinfo("====== GSM EF Response Data ======");
			scsisim_pinfo("ID: %x",
				      resp->type.ef.file_id);
			scsisim_pinfo("Size: %d bytes",
				      resp->type.ef.file_size);
			scsisim_pinfo("Type: %s",
				      (resp->type.ef.file_type > sizeof(GSM_file_type) / sizeof(GSM_file_type[0]) - 1) ? "[Undefined]" : GSM_file_type[resp->type.ef.file_type]);
			scsisim_pinfo("Status: %d",
				      resp->type.ef.status);
			scsisim_pinfo("Structure: %s",
				      (resp->type.ef.structure > sizeof(GSM_ef_structure) / sizeof(GSM_ef_structure[0]) - 1) ? "[Undefined]" : GSM_ef_structure[resp->type.ef.structure]);
			scsisim_pinfo("Record length: %d bytes",
				      resp->type.ef.record_len);
			scsisim_pinfo("======== End Response Data =======");
			break;

		case SELECT_MF_DF:
			scsisim_pinfo("===== GSM MF/DF Response Data ====");
			scsisim_pinfo("ID: %x",
				      resp->type.mf_df.file_id);
			scsisim_pinfo("Free memory: %d bytes",
				      resp->type.mf_df.file_memory);
			scsisim_pinfo("Type: %s",
				      (resp->type.mf_df.file_type > sizeof(GSM_file_type) / sizeof(GSM_file_type[0]) - 1) ? "[Undefined]" : GSM_file_type[resp->type.mf_df.file_type]);
			scsisim_pinfo("Characteristics: %d",
				      resp->type.mf_df.characteristics);
			scsisim_pinfo("CHV1 enabled: %s",
				      resp->type.mf_df.CHV1_enabled ? "true" : "false");
			scsisim_pinfo("Child DFs: %d",
				      resp->type.mf_df.df_children);
			scsisim_pinfo("Child EFs: %d",
				      resp->type.mf_df.ef_children);
			scsisim_pinfo("Number of CHVs: %d",
				      resp->type.mf_df.num_chvs);
			scsisim_pinfo("CHV1 initialized: %s",
				      resp->type.mf_df.CHV1_initialized ? "true" : "false");
			scsisim_pinfo("CHV1 attempts remaining: %d",
				      resp->type.mf_df.CHV1_attempts_remaining);
			scsisim_pinfo("CHV2 initialized: %s",
				      resp->type.mf_df.CHV2_initialized ? "true" : "false");
			scsisim_pinfo("CHV2 attempts remaining: %d",
				      resp->type.mf_df.CHV2_attempts_remaining);
			scsisim_pinfo("======== End Response Data =======");
			break;

		default:
			scsisim_pinfo("%s: Unsupported response type", __func__);
			break;
	}

}


/* EOF */

