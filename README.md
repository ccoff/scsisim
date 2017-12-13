# scsisim

The **scsisim** (SCSI SIM) library provides an API for accessing USB SIM card readers that use the SCSI protocol. It is a user-space "driver" that calls into the Linux [SCSI generic](http://sg.danny.cz/sg/) kernel driver.

## Motivation

There are many serial-based and PC/SC-based USB SIM card readers available, and Linux support for them generally isn't an issue. However, I purchased an inexpensive "APM UR-200" USB SIM card reader in France awhile back. It includes a basic "SimCardRead.exe" Windows application that allows you to view and edit SIM card contacts and SMS messages, but nothing else. Linux and Mac users are completely out of luck, with no end-user application available.

I did some USB packet sniffing on Windows to see what was going back and forth between the application and the USB device. It turns out that the device uses [standard GSM commands](http://www.etsi.org/deliver/etsi_gts/11/1111/05.03.00_60/gsmts_1111v050300p.pdf) embedded in SCSI commands to read from and write to the SIM card. I reverse-engineered the device's initialization packets and SIM commands, and the result is the **scsisim** library. Now I can read and edit SIM cards using the device on Linux, as well as view and edit additional "hidden" SIM files and data that the Windows application does not.

I have yet to see another SIM card reader that uses SCSI, so the **scsisim** library is necessarily based on a sample size of one device (and over a dozen different SIM cards). If you come across another SCSI-based SIM device, you should be able to add the device-specific code to **device.h** and get things working relatively quickly. Pull requests welcome.

For the record, here is what `lsusb` reports for the "APM UR-200" USB SIM card reader:

`ID 0420:1307 Chips and Technologies Celly SIM Card Reader`

## How to build

Assuming `gcc` and `make` are installed on your system, building everything is as easy as running `make` from the project root directory. The makefile builds the following in the **build** subdirectory:

* **libscsisim.so** (shared library)
* **libscsisim.a** (static library)
* **demo** (demo application linked to the static library)

To run the **demo** application, type `./demo [DEVICE]` at the command line, where [DEVICE] is the SCSI generic name (for example, sg3). You can determine the SCSI generic name for a device by running `dmesg | grep "scsi generic sg"`. Or if you have the `lsscsi` program installed, just run `lsscsi -g`.

Additionally, you can use the following command-line options with the **demo** application:

`-p [PIN]`: If the SIM card has a PIN (CHV) enabled, you must specify a PIN to access some of the card's contents.

`-v`: Display verbose information, including raw SCSI commands and responses, diagnostic information, etc. All verbose information goes to stderr, so you can redirect output as needed.

**NOTE:** On some Linux distros (Debian and Ubuntu; maybe others), you must add the current user to the **disk** group. This ensures that the user has sufficient privileges to access the device directly using SCSI. Otherwise, you will have to run the demo app as the root user.

For example, on Debian type the following command to add a user to the **disk** group (replace [USERNAME] with the current username):

    $ usermod -a -G disk [USERNAME]

## API usage

To use the **scsisim** library in your own applications, all you need to do is include **scsisim.h** in your source code (and link the static or shared library, of course). For detailed information about the API functions, see the documentation in **scsisim.h**. See also **demo.c** for examples.

Briefly, here is how an application uses the **scsisim** library to access a SIM card:

1. Call the *scsisim_open_device()* function to open the device.
2. Call the *scsisim_init_device()* function to send any required initialization commands to the device. Device-specific initialization data is defined in **device.h**.
3. Call functions that interact directly with the SIM card:

    * *scsisim_select_file()*
    * *scsisim_get_response()*
    * *scsisim_select_file_and_get_response()*
    * *scsisim_read_record()*
    * *scsisim_read_binary()*
    * *scsisim_update_record()*
    * *scsisim_update_binary()*
    * *scsisim_verify_chv()*
    * *scsisim_send_raw_command()*

    There are also functions that parse and process data returned from the SIM card. Although these functions are technically not necessary for the "driver", they do make it easier to work with SIM card data: 

    * *scsisim_packed_bcd_to_ascii()*
    * *scsisim_unpack_septets()*
    * *scsisim_parse_sms()*
    * *scsisim_parse_adn()*
    * *scsisim_map_gsm_chars()*
    * *scsisim_get_gsm_text()*

4. When done, call the *scsisim_close_device()* function to close the device.

