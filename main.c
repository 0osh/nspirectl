#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>

#include <libnspire/nspire.h>

#include "help.h"

#include <stdarg.h>

#define MAX_BUFFER_SIZE 65536


int ret = 0;
int verbose = 0;
int debug = 0;


int main(int argc, char *argv[]) {
	// idek bro
	if (argc == 0) { puts("tf there are no arguments i think your shell is broken"); return 1; }

	// help command
	if (argc == 1 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
		printf(main_help_fmt, argv[0]);
		return 0;
	}


	int cmdArg_i = 1;

	for (int arg_i = 1; arg_i < argc; arg_i++) {
		if (argv[arg_i][0] == '-') {
			if (argv[arg_i][1] == '-') {
				if (strcmp(argv[arg_i] + 2, "verbose") == 0) { verbose = 1; }
				else if (strcmp(argv[arg_i] + 2, "debug") == 0) { verbose = 1; debug = 1; }
			} else if (argv[arg_i][1] == '\0') {
				// we need an option name twin
			} else {
				for (int shortFlag_i = 1; argv[arg_i][shortFlag_i] != '\0'; shortFlag_i++) {
					switch (argv[arg_i][shortFlag_i]) { // its a short flag, iterate through each item in it
						case 'd': debug = 1; verbose = 1; break;
						case 'v': verbose = 1; break;
						default: // invalid option
							invalidGlobalOption(argv[0], argv[arg_i][shortFlag_i]);
							return 2;
					}
				}
			}
		} else {
			cmdArg_i = arg_i;
			break; // i could put the code for every subcommand here but this is cleaner
		}
	}

	if (strcmp(argv[cmdArg_i], "send") == 0) {
		if (argc < 4) { printf(send_help_fmt, argv[0], argv[0]); return 0; }
		for (int i = 2; i < argc; i++) { if (strcmp(argv[i], "--help") == 0) { printf(send_help_fmt, argv[0], argv[0]); return 0; }}

		// init libnspire
		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != 0) { fprintf(stderr, "Error: failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }


		char *destArg = argv[argc-1];
		size_t dirNameLen = strlen(destArg);

		char *dest;
		dest = malloc(dirNameLen + 1);
		strcpy(dest, destArg);
		if (destArg[dirNameLen-1] != '/') { dest[dirNameLen++] = '/'; }

		for (int i = cmdArg_i+1; i < argc - 1; i++) {
			dest = realloc(dest, dirNameLen + strlen(argv[i]) + 1);
			strcpy(dest + dirNameLen, argv[i]);

			// get file
			FILE *file_fd = fopen(argv[i], "rb");
			if (!file_fd) { perror("Error: failed to open file"); return errno; }

			ret = fseek(file_fd, 0, SEEK_END);
			if (ret < 0) { perror("Error: failed to get file length"); return errno; }

			long file_len = ftell(file_fd);
			if (file_len < 0) { perror("Error: failed to get file length"); return errno; }
			else if (file_len == 0) {
				// i could use a do while but its harder to read
				sendEmptyFileConfirmation:
				fputs("Warning: file length is 0. Continue? (y/n)  ", stdout);
				char c = getchar();
				if (c == 'n' || c == 'N') { return 0; }
				else if (c != 'y' && c != 'Y') { goto sendEmptyFileConfirmation; }
			}

			fseek(file_fd, 0, SEEK_SET);
			if (ret < 0) { perror("Error: failed to get file length"); return errno; }

			char *file_data = malloc(file_len + 1);
			fread(file_data, file_len, 1, file_fd);
			fclose(file_fd);
			file_data[file_len] = '\0';

			// send it to the calculator
			printf("Uploading %s\n", argv[i]);
			ret = nspire_file_write(handle, argv[i], file_data, file_len);
			if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "Error: failed to send file %s: %s\n", argv[i], nspire_strerror(ret)); return ret; }

			if (strcmp(dest, "/") != 0) {
				// ret = nspire_file_move(handle, "/test.tns", "/testfolder/test.tns"); // stupid function broken in the calculator
				printf("Copying to %s\n", dest);
				ret = nspire_file_copy(handle, argv[i], dest);
				if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "Error: failed to copy file %s to %s: %s\n", argv[i], dest, nspire_strerror(ret)); return ret; }
				puts("Removing original");
				ret = nspire_file_delete(handle, argv[i]);
				if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "Error: failed to remove file %s: %s\n", argv[i], nspire_strerror(ret)); return ret; }
			}

			free(dest);
		}

		DEBUG("Freeing recources...\n");
		nspire_free(handle);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "read") == 0) {
		int output_fd = 1; // stdout by default
		char *srcPath = NULL;
		char *destPath = NULL;
		for (int arg_i = cmdArg_i+1; arg_i < argc; arg_i++) {
			if (argv[arg_i][0] == '-') { // is the argument a flag?
				if (argv[arg_i][1] == '-') { // its a long flag
					unrecognizedSubcommandOption(argv[0], "read", argv[arg_i]);
					return 2;
				}

				for (int shortFlag_i = 1; argv[arg_i][shortFlag_i] != '\0'; shortFlag_i++) {
					switch (argv[arg_i][shortFlag_i]) { // its a short flag, iterate through each item in it
						case 'o': // set output file
							// accept `-ofile.txt` or `-o file.txt`
							if (argv[arg_i][2] == '\0') {
								if (arg_i + 1 < argc)  { destPath = argv[++arg_i]; }
								else { requiresAnArgument(argv[0], 'o'); }
							} else { destPath = argv[arg_i] + shortFlag_i + 1; }

							if (output_fd != 1 && close(output_fd)) { perror("Error: failed to close original file after another output file was specified"); return errno; }
							output_fd = open(destPath, O_WRONLY);
							if (output_fd < 0) { perror("Error: failed to open output file"); return errno; }
							DEBUG("Set output file to %s\n", destPath);
							goto shortFlag_exit;
						default: // invalid option
							invalidSubcommandOption(argv[0], "read", argv[arg_i][shortFlag_i]);
							return 2;
					}
				}
				shortFlag_exit: // C23 extention!? ive done things like this in assembly it cant be that hard that it wasnt in og C
			} else { srcPath = argv[arg_i]; DEBUG("Set source file to %s\n", srcPath); } // setting file to read doesnt need an argument
		}
		if (!srcPath) { missingOperand(argv[0], "read"); return 2; }

		// init libnspire
		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != 0) { fprintf(stderr, "Error: failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }

		uint8_t buffer[MAX_BUFFER_SIZE];
		size_t len = 0;

		LOG("Reading from file...\n");

		readFromFile:
		ret = nspire_file_read(handle, srcPath, buffer, MAX_BUFFER_SIZE, &len);
		DEBUG("Read %ld bytes\n", len);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "Error: failed to read file: %s\n", nspire_strerror(ret)); return ret; }
		DEBUG("Writing to %s\n", destPath);
		if (write(output_fd, buffer, len) < 0) { perror("Error: failed to write data to file\n"); return errno; }

		if (len >= MAX_BUFFER_SIZE) { goto readFromFile; }
		DEBUG("Entire file read\n");

		DEBUG("Freeing recources...\n");
		nspire_free(handle);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "info") == 0) {
		if (argc >= cmdArg_i+2 && strcmp(argv[cmdArg_i+1], "--help") == 0) { printf(info_help_fmt, argv[0], argv[0]); return 0; }
/*
		// init libnspire
		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != 0) { fprintf(stderr, "Error: failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }*/

		struct nspire_devinfo info = {0};
		LOG("Getting device information..\n");
		ret = nspire_device_info(handle, &info);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "Error: failed to get info: %s\n", nspire_strerror(ret)); return ret; }
		char const * const falseTrue_str[] = { "false", "true" };

		/* Reading nspire_devinfo.hw_type
		* an 8 bit value is returned representing the hardware type (CX, CAS, CAS II, ect).
		* with bit 1 being the least significant bit and bit 8 being the most significant bit:
		* - bit 1: 0 = non CX, 1 = CX
		* - bit 3: 0 = non II, 1 = II (as in CX vs CX II)
		* - bit 5: 0 = non CAS, 1 = non CAS (probably)
		* - bit 7 could be something seeing the pattern, let me know if it ever turns to 1
		*
		* the enum that comes with libnspire doesnt account for II calculators for some reason
		*/

		char *batteryStatus;
		switch (info.batt.status) {
			case NSPIRE_BATT_POWERED: batteryStatus = "Powered"; break;
			case NSPIRE_BATT_LOW: batteryStatus = "Low"; break;
			case NSPIRE_BATT_OK: batteryStatus = "OK"; break;
			case NSPIRE_BATT_UNKNOWN: batteryStatus = "Unknown"; break;
			default: batteryStatus = "Invalid value"; break;
		}

		printf(
			"Device information\n"
			"==================\n"
			"\n"
			"Device name: %s\n"
			"Product ID: %s\n"
			"\n"
			"Device type code: 0x%x (0b%.8b)\n"
			"CX: %s\n"
			"II: %s\n" // i need a better way to notate this
			"CAS: %s\n"
			"\n"
			"Screen resolution: %dpx x %dpx\n"
			"BBP: %d\n"
			"Sample mode: %d\n"
			"\n"
			"Storage: %ld bytes free out of %ld bytes total\n"
			"RAM: %ld bytes free out of %ld bytes total\n"
			"\n"
			"Battery status: %d - %s\n"
			"Charging: %s\n"
			"\n"
			"Clock speed: %dMhz\n"
			"\n"
			"Accepted file extention: %s\n"
			"OS extention: %s\n"
			"\n"
			"Run level code: %x - %s\n"
			"\n"
			"Boot ROM (boot1) version: %d.%d.0.%d\n"
			"Boot loader (boot2) version: %d.%d.0.%d\n"
			"OS version: %d.%d.0.%d\n"
			,

			info.device_name,
			info.electronic_id,

			info.hw_type, info.hw_type,
			falseTrue_str[(info.hw_type & 0b00000001) > 0],
			falseTrue_str[(info.hw_type & 0b00000100) > 1],
			falseTrue_str[(info.hw_type & 0b00010000) > 4],

			info.lcd.width, info.lcd.height,
			info.lcd.bbp,
			info.lcd.sample_mode,

			info.storage.free, info.storage.total,
			info.ram.free, info.ram.total,

			info.batt.status, batteryStatus,
			falseTrue_str[info.batt.is_charging],

			info.clock_speed,

			info.extensions.file,
			info.extensions.os,

			info.runlevel, info.runlevel == NSPIRE_RUNLEVEL_OS ? "OS" : (info.runlevel == NSPIRE_RUNLEVEL_RECOVERY ? "Recovery" : "Invalid value"),

			info.versions[NSPIRE_VER_BOOT1].major, info.versions[NSPIRE_VER_BOOT1].minor, info.versions[NSPIRE_VER_BOOT1].build,
			info.versions[NSPIRE_VER_BOOT2].major, info.versions[NSPIRE_VER_BOOT2].minor, info.versions[NSPIRE_VER_BOOT2].build,
			info.versions[NSPIRE_VER_OS].major, info.versions[NSPIRE_VER_OS].minor, info.versions[NSPIRE_VER_OS].build
		);

		DEBUG("Freeing recources...\n");
		nspire_free(handle);

		return 0;
	}

	// invalid command
	unrecognizedGlobalOption(argv[0], argv[cmdArg_i]);
	return 1;
}
