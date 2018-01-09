/*
 *  demo.c
 *  Demonstrate how to use the scsisim library to access a SIM card's 
 *  contents.
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
#include <unistd.h>
#include <ctype.h>

/* Include scsisim.h for API access */
#include "scsisim.h"

/* Command-line options */
static char *opt_pin;
static char *opt_device;

/* Internal functions */
static void parse_cmd_opts (int argc, char *argv[]);
static void print_usage_and_exit(void);


/**
 * Function: main
 *
 * Description: Demonstrate how to use the scsisim library. This program does 
 * the following:
 *
 * 1. Open the device with scsisim_open_device() using the SCSI generic device 
 *    name provided as a command-line argument (for example, "sg3").
 *
 * 2. Initialize the device with scsisim_init_device().
 *
 * 3. Select various files and directories on the SIM card, and then run 
 *    appropriate commands to read records, etc. This includes contacts and 
 *    SMS messages.
 *
 * 4. Close the device with scsisim_close_device().
 *
 * See the print_usage_and_exit() function below for additional information
 * about available command-line options.
 *
 */
int main(int argc, char *argv[])
{
	int ret, i, num_records;
	uint8_t *tmp_buf, bin_buf[128] = { 0 };
	char *tmp_str;
	struct sg_dev device;		/* defined in scsisim.h */
	struct GSM_response resp;	/* defined in scsisim.h */

	/* Process command-line arguments. */
	parse_cmd_opts(argc, argv);

	/* Open the device with the specified SCSI generic name. This function
	 * fills in the 'device' struct with data needed by other scsisim API 
	 * functions that interact directly with the SIM card. */
	if ((ret = scsisim_open_device(opt_device, &device)) != SCSISIM_SUCCESS)
	{
		scsisim_perror(__func__, ret);
		goto exit;
	}

	/* Initialize the device. This function sends the 'magic' sequence of
	 * device-specific initialization commands defined in device.h. */
	if ((ret = scsisim_init_device(&device)) != SCSISIM_SUCCESS)
	{
		scsisim_perror(__func__, ret);
		goto close_device;
	}

	/* Device is open and initialized. Try to select the Master File (root)
	 * directory on the SIM card, and get the response data. */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_MF,
							bin_buf,
							sizeof(bin_buf),
							SELECT_MF_DF,
							&resp)) != SCSISIM_SUCCESS)
	{
		scsisim_perror("Select MF failed", ret);
		goto close_device;
	}

	/* We successfully selected the Master File. Is there a PIN (CHV) enabled 
	 * on the card? If so, try to use the PIN provided on the command line. */
	if (resp.type.mf_df.CHV1_enabled == true)
	{
		if (opt_pin == NULL)
		{
			scsisim_pinfo("PIN enabled on card, but no PIN specified");
			goto close_device;
		}

		/* Is the SIM card's PIN already blocked? */
		if (resp.type.mf_df.CHV1_attempts_remaining == 0)
		{
			scsisim_pinfo("PIN blocked; %d PIN unblock attempts remaining",
				      resp.type.mf_df.CHV1_unblock_attempts_remaining);
			goto close_device;
		}

		scsisim_printf("PIN enabled on card; %d attempts remaining\n",
			       resp.type.mf_df.CHV1_attempts_remaining);

		scsisim_printf("Do you want to send PIN %s to the card? [y/n] ",
			       opt_pin);

		i = getchar();

		if (i == 'y' || i == 'Y')
		{
			/* Send the PIN to the card */
			if ((ret = scsisim_verify_chv(&device, 1, opt_pin)) == SCSISIM_SUCCESS)
			{
				scsisim_pinfo("PIN verification successful.");
			}
			else
			{
				scsisim_perror(__func__, ret);
				goto close_device;
			}
		}
		else
			scsisim_pinfo("Accessing SIM card without PIN authentication -- some files will be unreadable.");
	}

	/* Select the ICCID (ICC Identification) file */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_EF_ICCID,
							bin_buf,
							sizeof(bin_buf),
							SELECT_EF,
							&resp)) == SCSISIM_SUCCESS)
	{
		/* Read the raw binary data in the ICCID file */
		if ((ret = scsisim_read_binary(&device,
					       bin_buf,
					       0,
					       resp.type.ef.file_size)) == SCSISIM_SUCCESS)
		{
			/* Unpack the raw BCD data to an ASCII string */
			tmp_str = scsisim_packed_bcd_to_ascii(bin_buf,
							      resp.type.ef.file_size,
							      true,
							      true,
							      false);

			scsisim_printf("ICCID = %s\n", tmp_str);

			/* Caller's responsibility to free memory from a 
			 * scsisim_packed_bcd_to_ascii() return value */
			free(tmp_str);
		}
		else
			scsisim_perror("Read EF-ICCID failed", ret);
	}
	else
		scsisim_perror("Select EF-ICCID failed", ret);

	/* Select the GSM directory */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_DF_GSM,
							bin_buf,
							sizeof(bin_buf),
							SELECT_MF_DF,
							&resp)) != SCSISIM_SUCCESS)
	{
		scsisim_perror("Select DF-GSM failed", ret);
		goto close_device;
	}

	/* Select the SPN (Service Provider Name) file */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_EF_SPN,
							bin_buf,
							sizeof(bin_buf),
							SELECT_EF,
							&resp)) == SCSISIM_SUCCESS)
	{
		/* Read the raw binary data in the SPN file */
		if ((ret = scsisim_read_binary(&device,
					       bin_buf,
					       0,
					       resp.type.ef.file_size)) == SCSISIM_SUCCESS)
		{
			/* Map the character codes to their printable GSM 
			 * alphabet counterparts */
			tmp_str = scsisim_map_gsm_chars(bin_buf+1, resp.type.ef.file_size-1);

			scsisim_printf("SPN = %s\n", tmp_str);

			/* Caller's responsibility to free memory from a 
			 * scsisim_map_gsm_chars() return value */
			free(tmp_str);
		}
		else
			scsisim_perror("Read EF-SPN failed", ret);
	}
	else
		scsisim_perror("Select EF-SPN failed", ret);

	/* Select the Master File (root) directory */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_MF,
							bin_buf,
							sizeof(bin_buf),
							SELECT_MF_DF,
							&resp)) != SCSISIM_SUCCESS)
	{
		scsisim_perror("Select MF failed", ret);
		goto close_device;
	}

	/* Select the TELECOM directory */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_DF_TELECOM,
							bin_buf,
							sizeof(bin_buf),
							SELECT_MF_DF,
							&resp)) != SCSISIM_SUCCESS)
	{
		scsisim_perror("Select DF-TELECOM failed", ret);
		goto close_device;
	}

	/* Select the ADN (Abbreviated Dialing Numbers - AKA the "contacts") file */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_EF_ADN,
							bin_buf,
							sizeof(bin_buf),
							SELECT_EF,
							&resp)) == SCSISIM_SUCCESS)
	{
		/* Allocate space for an ADN record based on its size on the
		 * card */
		if ((tmp_buf = malloc((size_t)resp.type.ef.record_len)) == NULL)
			goto close_device;

		/* Calculate how many records the ADN file contains, which
		 * varies by SIM card manufacturer */
		num_records = resp.type.ef.file_size / resp.type.ef.record_len;

		/* Go through all of the records in the ADN file */
		for (i = 1; i <= num_records; i++)
		{
			scsisim_printf("====================\nADN record #%i\n", i);

			/* Read the current record */
			if (scsisim_read_record(&device,
						i,
						tmp_buf,
						resp.type.ef.record_len) == SCSISIM_SUCCESS)
			{
				/* Parse the current ADN record */
				if ((ret = scsisim_parse_adn(tmp_buf,
							     resp.type.ef.record_len)) != SCSISIM_SUCCESS)
					scsisim_perror("ADN record parse failed", ret);
			}
		}

		free(tmp_buf);
	}
	else
		scsisim_perror("Select EF-ADN failed", ret);

	/* Select the SMS (Short Message Service) file */
	if ((ret = scsisim_select_file_and_get_response(&device,
							GSM_FILE_EF_SMS,
							bin_buf,
							sizeof(bin_buf),
							SELECT_EF,
							&resp)) == SCSISIM_SUCCESS)
	{
		/* Allocate space for an SMS record based on its size on the
		 * card */
		if ((tmp_buf = malloc((size_t)resp.type.ef.record_len)) == NULL)
			goto close_device;

		/* Calculate how many records the SMS file contains -- this
		 * varies by SIM card manufacturer */
		num_records = resp.type.ef.file_size / resp.type.ef.record_len;

		/* Go through all of the records in the SMS file */
		for (i = 1; i <= num_records; i++)
		{
			scsisim_printf("====================\nSMS record #%i\n", i);

			/* Read the current record */
			if (scsisim_read_record(&device,
						i,
						tmp_buf,
						resp.type.ef.record_len) == SCSISIM_SUCCESS)
			{
				/* Parse the current SMS record */
				if ((ret = scsisim_parse_sms(tmp_buf,
							     resp.type.ef.record_len)) != SCSISIM_SUCCESS)
					scsisim_perror("SMS record parse failed", ret);
			}
		}
		free(tmp_buf);
	}
	else
		scsisim_perror("Select EF-SMS failed", ret);

close_device:
	/* Close the device */
	ret = scsisim_close_device(&device);

exit:
	return ret;
}


/**
 * Function: parse_cmd_opts
 *
 * Parameters:
 * argc, argv:	Passed straight from main().
 *
 * Description: 
 * Use the getopt() function to parse command-line arguments.
 *
 * Return values: 
 * None
 */
static void parse_cmd_opts (int argc, char *argv[])
{
	int cmdopt;

	while ((cmdopt = getopt(argc, argv, "p:v")) != -1)
	{
		switch (cmdopt)
		{
			case 'p':
				opt_pin = optarg;
				break;

			case 'v':
				scsisim_verbose = true;
				break;

			case '?':
				if (optopt == 'p')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);

			default:
				print_usage_and_exit();
		}
	}

	/* Use first non-option argument as device name, ignore anything else */
	if (optind < argc)
		opt_device = argv[optind];
	else
		print_usage_and_exit();
}


/**
 * Function: print_usage_and_exit
 *
 * Parameters:
 * None
 *
 * Description: 
 * Print available command-line arguments and exit.
 *
 * Return values: 
 * None
 */
static void print_usage_and_exit(void)
{
	fprintf(stderr, "\nUsage: ./demo [DEVICE] [OPTIONS]...");
	fprintf(stderr, "\nDemonstrates access to a SIM card reader using the Linux SCSI generic driver.\n\n");
	fprintf(stderr, "Options:\n\n");
	fprintf(stderr, "  [DEVICE]\tSCSI generic device name (for example, 'sg1')\n");
	fprintf(stderr, "  -p [PIN]\tSpecify PIN number to access card\n");
	fprintf(stderr, "  -v\t\tDisplay verbose information\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Example:\n\n");
	fprintf(stderr, "  ./demo sg2 -p 1234 -v\n");
	fprintf(stderr, "  (Open SCSI generic device sg2, use PIN 1234, and display verbose information)\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

/* EOF */

