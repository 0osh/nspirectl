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

int sortDirInfo(const void *arg1, const void *arg2) {
	return strcmp(((struct nspire_dir_item *) arg1)->name, ((struct nspire_dir_item *) arg2)->name);
}


int main(int argc, char *argv[]) {
	// idek bro
	if (argc == 0) { puts("tf there are no arguments i think your shell is broken"); return 1; }

	// help if theres no input
	if (argc == 1) { printMainHelp(); return 0; }


	int cmdArg_i = 1;

	for (int arg_i = 1; arg_i < argc; arg_i++) {
		if (argv[arg_i][0] == '-') {
			if (argv[arg_i][1] == '-') {
				if (strcmp(argv[arg_i] + 2, "verbose") == 0) { verbose = 1; DEBUG("Verbose logging enabled"); }
				if (strcmp(argv[arg_i] + 2, "help") == 0) { printMainHelp(); return 0; }
				else if (strcmp(argv[arg_i] + 2, "debug") == 0) { verbose = 1; debug = 1; DEBUG("Debug logging enabled"); }
			} else if (argv[arg_i][1] == '\0') {
				// we need an option name twin
			} else {
				for (int shortFlag_i = 1; argv[arg_i][shortFlag_i] != '\0'; shortFlag_i++) {
					switch (argv[arg_i][shortFlag_i]) { // its a short flag, iterate through each item in it
						case 'v': verbose = 1; break;
						case 'd': debug = 1; verbose = 1; break;
						default:  invalidGlobalOption(argv[arg_i][shortFlag_i]); return 2;
					}
				}
			}
		} else {
			cmdArg_i = arg_i;
			goto parseSubcommand; // i could put the code for every subcommand here but this is cleaner
		}
	}

	printMainHelp();
	return 2;

	parseSubcommand:
	if (strcmp(argv[cmdArg_i], "send") == 0) {
		for (int i = cmdArg_i+1; i < argc; i++) { if (strcmp(argv[i], "--help") == 0) { printSendHelp(); }}
		if (argc-1 < cmdArg_i + 1) { missingNamedOperand("send", "file"); return 2; }
		if (argc-1 < cmdArg_i + 2) { missingNamedOperand("send", "destination"); return 2; }

		// init libnspire
		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }


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
			if (!file_fd) { perror("[FATAL] failed to open file"); return errno; }

			ret = fseek(file_fd, 0, SEEK_END);
			if (ret < 0) { perror("[FATAL] failed to get file length"); return errno; }

			long file_len = ftell(file_fd);
			if (file_len < 0) { perror("[FATAL] failed to get file length"); return errno; }
			else if (file_len == 0) {
				// i could use a do while but its harder to read
				sendEmptyFileConfirmation:
				fputs("[WARNING] file length is 0. Continue? (y/n)  ", stderr);
				char c = getchar();
				if (c == 'n' || c == 'N') { return 0; }
				else if (c != 'y' && c != 'Y') { goto sendEmptyFileConfirmation; }
			}

			fseek(file_fd, 0, SEEK_SET);
			if (ret < 0) { perror("[FATAL] failed to get file length"); return errno; }

			char *file_data = malloc(file_len + 1);
			fread(file_data, file_len, 1, file_fd);
			fclose(file_fd);
			file_data[file_len] = '\0';

			// send it to the calculator
			LOG("Uploading %s\n", argv[i]);
			ret = nspire_file_write(handle, argv[i], file_data, file_len);
			if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to send file %s: %s\n", argv[i], nspire_strerror(ret)); return ret; }

			if (strchr(dest+1, '/') != NULL) { // no other slash; the file as at root
				printf("dest: %s\n", dest);
				// ret = nspire_file_move(handle, "/test.tns", "/testfolder/test.tns"); // stupid function broken in the calculator
				DEBUG("Copying to %s\n", dest);
				ret = nspire_file_copy(handle, argv[i], dest);
				if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to copy file %s to %s: %s\n", argv[i], dest, nspire_strerror(ret)); return ret; }
				DEBUG("Removing original\n");
				ret = nspire_file_delete(handle, argv[i]);
				if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to remove file %s: %s\n", argv[i], nspire_strerror(ret)); return ret; }
			}
		}

		DEBUG("Freeing resources...\n");
		free(dest);
		nspire_free(handle);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "read") == 0) {
		int output_fd = 1; // stdout by default
		char *srcPath = NULL;
		char *destPath = "stdout";
		for (int arg_i = cmdArg_i+1; arg_i < argc; arg_i++) {
			if (argv[arg_i][0] == '-') { // is the argument a flag?
				if (argv[arg_i][1] == '-') { // its a long flag
					if (strcmp(argv[arg_i] + 2, "help") == 0) { printReadHelp(); }
					else { unrecognizedSubcommandOption("read", argv[arg_i]); return 2; }
				} else { // its a short flag
					for (int shortFlag_i = 1; argv[arg_i][shortFlag_i] != '\0'; shortFlag_i++) {
						switch (argv[arg_i][shortFlag_i]) { // its a short flag, iterate through each item in it
							case 'o': // set output file
								// accept `-ofile.txt` or `-o file.txt`
								if (argv[arg_i][2] == '\0') {
									if (arg_i + 1 < argc)  { destPath = argv[++arg_i]; }
									else { requiresAnArgument('o'); }
								} else { destPath = argv[arg_i] + shortFlag_i + 1; }

								if (output_fd != 1 && close(output_fd)) { perror("Error: failed to close original file after another output file was specified, leaving open"); }
								output_fd = open(destPath, O_WRONLY | O_CREAT, 0755);
								if (output_fd < 0) { perror("[FATAL] failed to open output file"); return errno; }
								DEBUG("Set output file to %s\n", destPath);
								goto shortFlag_exit;
							default: // invalid option
								invalidSubcommandOption("read", argv[arg_i][shortFlag_i]);
								return 2;
						}
					}
					shortFlag_exit: // C23 extention!? ive done things like this in assembly it cant be that hard
				}
			} else { srcPath = argv[arg_i]; DEBUG("Set source file to %s\n", srcPath); } // setting file to read doesnt need a flag
		}

		if (!srcPath) { missingOperand("read"); return 2; }

		// init libnspire
		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }

		LOG("Getting file size...\n");
		struct nspire_dir_item attr = {0};
		ret = nspire_attr(handle, srcPath, &attr);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to get file info: %s\n", nspire_strerror(ret)); return ret; }
		DEBUG("Name: %s, size: %ld bytes, date: %ld, type: %d\n", attr.name, attr.size, attr.date, attr.type);
		if (attr.size == 0) { attr.size = 1280000; DEBUG("File size overridden to %ld\n", attr.size); }

		size_t len = 0;
		uint8_t *buffer = malloc(attr.size);
		if (!buffer) { perror("[FATAL] failed to allocate memory for read buffer"); return errno; }

		LOG("Reading from file...\n");
		ret = nspire_file_read(handle, srcPath, buffer, attr.size, &len);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to read file: %s\n", nspire_strerror(ret)); return ret; }
		DEBUG("Read %ld bytes\n", len);
		DEBUG("Writing to %s\n", destPath);
		if (write(output_fd, buffer, attr.size) < 0) { perror("[FATAL] failed to write data to file\n"); return errno; }

		DEBUG("Freeing recources...\n");
		nspire_free(handle);
		free(buffer);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "move") == 0 || strcmp(argv[cmdArg_i], "copy") == 0) {
		char *command = argv[cmdArg_i];
		if (argc - 1 >= cmdArg_i+1) {
			if (strcmp(argv[cmdArg_i+1], "--help") == 0) { printMoveCopyHelp(); }
			else if (argv[cmdArg_i+1][0] == '-') {
				if (argv[cmdArg_i+1][1] == '-') { unrecognizedSubcommandOption(command, argv[cmdArg_i+1]); return 2; }
				else { invalidSubcommandOption(command, argv[cmdArg_i+1][1]); return 2; }
			}
		}
		if (argc-1 < cmdArg_i + 1) { missingNamedOperand(command, "file"); return 2; }
		if (argc-1 < cmdArg_i + 2) { missingNamedOperand(command, "destination"); return 2; }

		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }

		char *dest = argv[argc-1];
		size_t dirnameLen = 0;

		LOG("Getting destination type...\n");
		struct nspire_dir_item attr = {0};
		ret = nspire_attr(handle, dest, &attr);
		if (ret == -NSPIRE_ERR_NONEXIST) {
			if (dest[strlen(dest)-1] == '/' || argc-1 > cmdArg_i+2) { fprintf(stderr, "[FATAL] path %s doesnt exist\n", dest); return -1; }
			goto move_loop; // arg is a file name and it doesnt exist (good)
		} else if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to get file info: %s\n", nspire_strerror(ret)); return ret; }

		switch (attr.type) {
			case NSPIRE_DIR:
				dirnameLen = strlen(argv[argc-1]);
				dest = malloc(dirnameLen);
				strcpy(dest, argv[argc-1]);
				if (dest[dirnameLen-1] != '/') { dest[dirnameLen++] = '/'; }
				break;
			case NSPIRE_FILE: fprintf(stderr, "[FATAL] file %s already exists\n", dest); return -NSPIRE_ERR_EXISTS;
			default: fprintf(stderr, "[FATAL] invalid file type (code %d)\n", attr.type); return -1;
		}

		move_loop:
		for (int i = cmdArg_i + 1; i < argc - 1; i++) {
			if (attr.type == NSPIRE_DIR) {
				char *basename = strrchr(argv[i], '/');
				if (!basename) { basename = argv[i]; }
				dest = realloc(dest, dirnameLen + strlen(basename));
				strcpy(dest + dirnameLen, basename);
			}

			// printf("gonna move %s to %s\n", argv[i], dest);

			// ret = nspire_file_move(handle, "/test.tns", "/testfolder/test.tns"); // stupid function broken in the calculator
			DEBUG("Copying %s to %s\n", argv[i], dest);
			ret = nspire_file_copy(handle, argv[i], dest);
			if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to copy file %s to %s: %s\n", argv[i], dest, nspire_strerror(ret)); return ret; }
			if (command[0] == 'm') { // m for move
				DEBUG("Removing original (%s)\n", argv[i]);
				ret = nspire_file_delete(handle, argv[i]);
				if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to remove file %s: %s\n", argv[i], nspire_strerror(ret)); return ret; }
			}
		}

		DEBUG("Freeing recources...\n");
		nspire_free(handle);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "list") == 0) {

		uint8_t format = 1;
		char *dirPath = "/";
		for (int arg_i = cmdArg_i+1; arg_i < argc; arg_i++) {
			if (argv[arg_i][0] == '-') { // is the argument a flag?
				if (argv[arg_i][1] == '-') { // its a long flag
					if (strcmp(argv[arg_i] + 2, "no-format") == 0) { format = 0; DEBUG("Disabled output formatting\n"); }
					else if (strcmp(argv[arg_i] + 2, "help") == 0) { printListHelp(); }
					else { unrecognizedSubcommandOption("list", argv[arg_i]); return 2; }
				} else { // its a short flag
					invalidSubcommandOption("list", argv[arg_i][1]);
					return 2;
				}

			} else { dirPath = argv[arg_i]; DEBUG("Set read directory file to %s\n", dirPath); } // setting directory to list doesnt need a flag
		}

		// init libnspire
		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }

		struct nspire_dir_info *dirInfo;

		LOG("Getting list...\n");
		ret = nspire_dirlist(handle, dirPath, &dirInfo);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to get file list: %s\n", nspire_strerror(ret)); return ret; }

		// your cpu is gonna be so happy it doesnt have to do extra work for nothing
		if (dirInfo->num == 1) { puts(dirInfo->items[0].name); goto list_cleanUp; }
		else if (dirInfo->num == 0) { goto list_cleanUp; }

		LOG("Formatting...\n");
		qsort(dirInfo->items, dirInfo->num, sizeof(struct nspire_dir_item), sortDirInfo); // sort alphabetically

		FILE *outputStream = format ? popen("column", "w") : stdout;
		if (!outputStream) {
			perror("Error: Failed to open pipe to column command to format list, printing without formatting instead");
			format = 0;
			outputStream = stdout;
		}

		for (long unsigned int i = 0; i < dirInfo->num; i++) {
			fputs(dirInfo->items[i].name, outputStream);
			fputc('\n', outputStream);
		}

		if (format) {
			ret = pclose(outputStream);
			if (ret != 0) { fprintf(stderr, "Error: failed to close pipe to column command to format list with code %d, leaving open", ret); }
		}


		list_cleanUp:
		DEBUG("Freeing resources...\n");
		nspire_dirlist_free(dirInfo);
		nspire_free(handle);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "info") == 0) {
		if (argc - 1 >= cmdArg_i+1) {
			if (strcmp(argv[cmdArg_i+1], "--help") == 0) { printInfoHelp(); }
			else if (argv[cmdArg_i+1][0] == '-') {
				if (argv[cmdArg_i+1][1] == '-') { unrecognizedSubcommandOption("info", argv[cmdArg_i+1]); return 2; }
				else { invalidSubcommandOption("info", argv[cmdArg_i+1][1]); return 2; }
			}
		}

		// init libnspire
		LOG("Initializing usb connection...\n");
		nspire_handle_t *handle;
		ret = nspire_init(&handle);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to init libnspire: %s\n", nspire_strerror(ret)); return ret; }

		struct nspire_devinfo info = {0};
		LOG("Getting device information..\n");
		ret = nspire_device_info(handle, &info);
		if (ret != NSPIRE_ERR_SUCCESS) { fprintf(stderr, "[FATAL] failed to get info: %s\n", nspire_strerror(ret)); return ret; }
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

		DEBUG("Freeing resources...\n");
		nspire_free(handle);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "help") == 0) {
		for (int arg_i = cmdArg_i+1; arg_i < argc; arg_i++) {
			if (argv[arg_i][0] == '-') { // is the argument a flag?
				if (argv[arg_i][1] == '-') { // its a long flag
					if (strcmp(argv[arg_i] + 2, "help") == 0) { printHelpHelp(); return 0; }
					else { unrecognizedSubcommandOption("help", argv[arg_i]); return 2; }
				} else { // its a short flag
					invalidSubcommandOption("help", argv[arg_i][1]);
					return 2;
				}
			} else {
				fprintf(stderr, "%s: unknown command '%s'\nTry '%s help --help' for more information.\n", argv[0], argv[arg_i], argv[0]);
				return 2;
			}
		}

		if (strcmp(argv[cmdArg_i+1], "send") == 0) { printSendHelp(); return 0; }
		if (strcmp(argv[cmdArg_i+1], "read") == 0) { printReadHelp(); return 0; }
		if (strcmp(argv[cmdArg_i+1], "list") == 0) { printListHelp(); return 0; }
		if (strcmp(argv[cmdArg_i+1], "move") == 0 || strcmp(argv[cmdArg_i+1], "copy") == 0) { printMoveCopyHelp(); return 0; }
		if (strcmp(argv[cmdArg_i+1], "info") == 0) { printInfoHelp(); return 0; }
		if (strcmp(argv[cmdArg_i+1], "help") == 0) { printHelpHelp(); return 0; }
	}

	// invalid command
	fprintf(stderr, "%s: unknown command '%s'\nTry '%s --help' for more information.\n", argv[0], argv[cmdArg_i], argv[0]);
	return 1;
}
