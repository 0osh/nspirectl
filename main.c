#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libnspire/nspire.h>
#include "help.h"

#include <stdarg.h>

int ret = 0;

int main(int argc, char *argv[]) {
	// idek bro
	if (argc == 0) { puts("tf there are no arguments i think your shell is broken"); return 1; }

	// help command
	if (argc == 1 || isHelpCommand(argv[1])) {
		printf(main_help_fmt, argv[0], argv[0]);
		return 0;
	}

	if (strcmp(argv[1], "send") == 0) {
		if (argc < 4) {
			printf(send_help_fmt, argv[0], argv[0]);
			return 0;
		}

		// init libnspire
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != 0) { fprintf(stderr, "Error: failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }

		char *destArg = argv[argc-1];
		size_t dirNameLen = strlen(destArg);

		char *dest;
		dest = malloc(dirNameLen + 1);
		strcpy(dest, destArg);
		if (destArg[dirNameLen-1] != '/') { dest[dirNameLen++] = '/'; }

		for (int i = 2; i < argc - 1; i++) {
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

		// free resources
		nspire_free(handle);

		puts("done");
		return ret;
	} else if (strcmp(argv[1], "read") == 0) {
		// nspire_file_read
	} else if (strcmp(argv[1], "info") == 0) {
		// init libnspire
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != 0) { fprintf(stderr, "Error: failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }

		struct nspire_devinfo info = {0};
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
			default: batteryStatus = "Unknown"; break;
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

			info.runlevel, info.runlevel == NSPIRE_RUNLEVEL_OS ? "OS" : (info.runlevel == NSPIRE_RUNLEVEL_RECOVERY ? "Recovery" : "Unknown"),

			info.versions[NSPIRE_VER_BOOT1].major, info.versions[NSPIRE_VER_BOOT1].minor, info.versions[NSPIRE_VER_BOOT1].build,
			info.versions[NSPIRE_VER_BOOT2].major, info.versions[NSPIRE_VER_BOOT2].minor, info.versions[NSPIRE_VER_BOOT2].build,
			info.versions[NSPIRE_VER_OS].major, info.versions[NSPIRE_VER_OS].minor, info.versions[NSPIRE_VER_OS].build
		);

		return 0;
	}

	// invalid command
	printf(badOption_fmt, argv[0], argv[1], argv[0]);
	return 1;
}
