/*
 *  gsm.h
 *  GSM-related definitions for the scsisim library.
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

#ifndef __SCSISIM_GSM_H__
#define __SCSISIM_GSM_H__

#include <stdbool.h>
#include <stdint.h>

/* GSM command codes: these are only the commands the API currently supports, not
 * every single GSM command code. If you need to use a command code not listed here, 
 * use scsisim_send_raw_command() to build your own command buffer.
 */
#define GSM_CLASS			0xa0
#define GSM_CMD_SELECT			0xa4
#define GSM_CMD_GET_RESPONSE		0xc0
#define GSM_CMD_READ_BINARY		0xb0
#define GSM_CMD_READ_RECORD		0xb2
#define GSM_CMD_UPDATE_BINARY		0xd6
#define GSM_CMD_UPDATE_RECORD		0xdc
#define GSM_CMD_VERIFY_CHV		0x20

/* Other GSM-related constants */
#define GSM_CMD_SELECT_DATA_LEN		0x02	/* Two bytes of data for a
						   SELECT command */
#define GSM_CMD_VERIFY_CHV_DATA_LEN	0x08	/* Eight bytes of data for a
						   VERIFY CHV command */

#define GSM_ESCAPE_CHAR			0x1b	/* Escape character in GSM charset */

#define GSM_SMS_RECORD_LEN		176	/* Length of SMS record, in bytes */
#define GSM_MAX_SMSC_LEN		10	/* Maximum SM Service Center length (TON/NPI
						   disregarded); see GSM 04.11, section 8.2.5.1 */
#define GSM_MIN_ADDRESS_LEN		2	/* Minimum length of TP-DA, TP-OA, or TP-RA */
#define GSM_MAX_ADDRESS_LEN		12	/* Maximum length of TP-DA, TP-OA, or TP-RA */
#define GSM_ADN_NUMBER_BUFFER_LEN	14	/* Length of ADN (contact) number data buffer; 
						   see GSM spec section 10.4.1 for EF-ADN file */
#define GSM_MAX_ADN_NUMBER_LEN		10	/* Maximum length of actual ADN (contact)
						   number (TON/NPI disregarded); see GSM spec 
						   for EF-ADN file */
#define GSM_MIN_EF_RESPONSE_LEN		15	/* Minimum length of GET RESPONSE data 
						   after selecting an EF */
#define GSM_MIN_MF_DF_RESPONSE_LEN	22	/* Minimum length of GET RESPONSE data
						   after selecting an MF or DF */


extern const char *GSM_basic_charset[];
extern const char *GSM_basic_charset_extension[];
extern const char *GSM_sms_status[];
extern const char *GSM_file_type[];
extern const char *GSM_ef_structure[];

int gsm_parse_response(const uint8_t *response,
		       unsigned int response_len,
		       struct GSM_response *resp);

#endif /* __SCSISIM_GSM_H__ */

/* EOF */

