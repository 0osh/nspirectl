/* debugger:
printf("here\n"); fflush(stdout);
*/

// standard libs
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h> // check file type in send command
#include <dirent.h> // get files in dir in send command

// just macros
#include <fcntl.h>

// other
#include <libnspire/nspire.h>
#include "help.h"

// error printf and nsprire error printf
#define perrorf(fmt, ...) fprintf(stderr, fmt": %s\n", ##__VA_ARGS__, strerror(ret))
#define nperrorf(fmt, ...) fprintf(stderr, fmt": %s\n", ##__VA_ARGS__, nspire_strerror(ret))
#define LOG(message, ...) if (verbose) { fprintf(stderr, "[LOG] " message, ##__VA_ARGS__); }
#define DEBUG(message, ...) if (debug) { fprintf(stderr, "[DEBUG] " message, ##__VA_ARGS__); }


int ret = 0; // dont have to worry about making new ret variables everywhere. just dont use without setting before hand
int verbose = 0; // -v flag
int debug = 0; // -d flag

// for qsort
int sortDirInfo(const void *arg1, const void *arg2) {
	return strcmp(((struct nspire_dir_item *) arg1)->name, ((struct nspire_dir_item *) arg2)->name);
}

// info for a malloc'd path variable to pass around
struct dynPath {
	char *path;
	size_t len;
	size_t allocLen;
};

// so one of the function calls arent messy
// cant believe i need all these vars...
struct cmd_ctx {
	nspire_handle_t *handle;
	struct nspire_dir_item attr;

	struct dynPath path;

	int cmdArg_i;
	int argc;
	char **argv; // first thing i dont like about c: cant assign *char[] to *char[] because arrays arent assignable (but *char[] to **char works and is the same functionality)
};


// compare 2 paths (without including slashes at the start or end)
int pathcmp(const char *s1, const char *s2) {
	int i = s1[0] == '/', j = s2[0] == '/';

	for (;;) {
		if (s1[i] != s2[j]) {
			if ((s1[i] == '/' && s2[j] == '\0') || (s1[i] == '\0' && s2[j] == '/')) { return 0; }
			return s1[i] - s2[j];
		} else if (s1[i] == '\0') { return 0; }

		i++; j++;
	}
}

// init a dynPath struct with a file (and its length for efficiency)
int initPathWithFile(struct dynPath *ctx, const char *path, size_t pathLen) {
	ctx->len = pathLen;
	ctx->allocLen = ctx->len + 2; // len + potential '/' + '\0'
	ctx->path = malloc(ctx->allocLen);
	if (!ctx->path) { perrorf("[FATAL] Failed to malloc %zu bytes for path variable", ctx->allocLen); return errno; }
	memcpy(ctx->path, path, ctx->len);
	ctx->path[ctx->len] = '\0';
	return 0;
}

// init a dynPath struct with a directory (and its length for efficiency)
int initPathWithDir(struct dynPath *ctx, const char *path, size_t pathLen) {
	ret = initPathWithFile(ctx, path, pathLen);
	if (ret != 0) { return ret; }
	if (ctx->len > 0 && ctx->path[ctx->len-1] != '/') { ctx->path[ctx->len++] = '/'; ctx->path[ctx->len] = '\0'; }
	return 0;
}

int resolveDir(struct cmd_ctx *ctx) {
	ctx->path.path = ctx->argv[ctx->argc-1];
	ctx->path.len = strlen(ctx->path.path);

	DEBUG("Getting destination type...\n");
	ret = nspire_attr(ctx->handle, ctx->path.path, &ctx->attr);
	if (ret == -NSPIRE_ERR_NONEXIST) {
		// if path is a directory OR there is more than 1 file (means path must be a directory)
		// error out because the directory doesent exist
		if ((ctx->path.path)[ctx->path.len-1] == '/' || ctx->argc-1 > ctx->cmdArg_i+2) { fprintf(stderr, "[FATAL] path `%s` doesnt exist\n", ctx->path.path); return -NSPIRE_ERR_NONEXIST; }
		DEBUG("path %s is a non-existing file\n", ctx->path.path);
		return initPathWithFile(&ctx->path, ctx->path.path, ctx->path.len); // return 0 if it was ok too
	} else if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to get file info"); return ret; }
	else {
		switch (ctx->attr.type) {
			case NSPIRE_DIR:
				DEBUG("path %s is an existing directory\n", ctx->path.path);
				return initPathWithDir(&ctx->path, ctx->path.path, ctx->path.len); // return 0 if it was ok too
			case NSPIRE_FILE: fprintf(stderr, "[FATAL] cannot write at `%s`: File exists\n", ctx->path.path); return -NSPIRE_ERR_EXISTS;
			default: fprintf(stderr, "[FATAL] invalid file type (code %d)\n", ctx->attr.type); return -NSPIRE_ERR_INVALID;
		}
	}
}

int setBasename(struct dynPath *dest, char *src) {
	size_t srcLen = strlen(src);

	// strrchr(src, '/') but dont include an ending slash
	int i = srcLen-2; // dont include slash at the end
	while (i >= 0 && src[i] != '/') { i--; }
	char *srcBasename = src + i + 1; // +1 so we dont include / in basename

	size_t newDestLen = dest->len + srcLen + 5; // plenty of extra for extra slashes
	if (newDestLen > dest->allocLen) { // dont realloc to a shorter array, waste of time
		dest->allocLen = newDestLen * 2; // x2 for even less reallocs
		dest->path = realloc(dest->path, dest->allocLen);
		if (!dest->path) { perrorf("[FATAL] Failed to realloc %zu bytes file destination variable", dest->allocLen); return errno; }
	}
	strcpy(dest->path + dest->len, srcBasename);

	DEBUG("Set basename (path = %s)\n", dest->path);
	return 0;
}

int sendPath(struct nspire_handle *handle, char *src, char *dest) {
	size_t srcLen = strlen(src);
	size_t destLen = strlen(dest);

	// checking type
	struct stat fileStat;
	stat(src, &fileStat);
	if (S_ISDIR(fileStat.st_mode)) { // were gonna recuse through directories
		DEBUG("Making directory %s...\n", src);
		ret = nspire_dir_create(handle, src);
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed create directory %s", src); return ret; }
		ret = nspire_dir_move(handle, src, dest);
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed move directory %s to %s", src, dest); return ret; }

		// add slash if needed
		char *originalPath = src;
		if (src[srcLen-1] != '/') {
			src = malloc(srcLen+2);
			if (!src) { perrorf("[FATAL]: Failed to malloc %zu bytes for file path variable", srcLen+2); return errno; }
			strcpy(src, originalPath);
			src[srcLen++] = '/';
			src[srcLen] = '\0';
		}

		DIR *dir = opendir(src);
		if (!dir) { perrorf("[FATAL] Failed to open directory %s", src); }
		struct dirent *entry;

		struct dynPath fullDest = {0};
		initPathWithDir(&fullDest, dest, destLen);
		struct dynPath fullSrc = {0};
		initPathWithDir(&fullSrc, src, srcLen);
		for (;;) {
			errno = 0; // see the RETURN VALUE section at readdir(3)
			entry = readdir(dir);
			if (entry == NULL) { break; }

			char *name = entry->d_name; // shorten variable name
			// dont include . and ..
			if ((name[0] == '.' && name[1] == '\0') || (name[0] == '.' && name[1] == '.' && name[2] == '\0')) { continue; }

			ret = setBasename(&fullDest, name); if (ret != 0) { return ret; } // /path/to/dest -> /path/to/dest/file.tns
			ret = setBasename(&fullSrc, name); if (ret != 0) { return ret; } // /path/to/src -> /path/to/src/file.tns
			ret = sendPath(handle, fullSrc.path, fullDest.path); if (ret != 0) { return ret; } // send it
		}

		closedir(dir);
		if (src != originalPath) { free(src); }
		free(fullDest.path);
		// error handling for readdir()
		if (errno != 0) { perrorf("[FATAL] failed to get file in %s (readdir() failed)", src); return errno; }
		return 0;
	} else if (!S_ISREG(fileStat.st_mode)) { fprintf(stderr, "[ERROR] file %s is not a regular file or directory, skipping\n", src); return 1; }
	// else, src is a file

	// get file
	DEBUG("Reading file from disk...\n");
	FILE *file_fd = fopen(src, "rb");
	if (!file_fd) { perrorf("[FATAL] failed to open file %s", src); return errno; }

	ret = fseek(file_fd, 0, SEEK_END);
	if (ret < 0) { perrorf("[FATAL] failed to fseek to file %s end", src); return errno; }

	long file_len = ftell(file_fd);
	if (file_len < 0) { perrorf("[FATAL] failed to get file %s length", src); return errno; }
	else if (file_len == 0) {
		sendEmptyFileConfirmation: // i could use a do while but its harder to read and i like gotos
		fputs("[WARNING] file length is 0. Continue? (y/n)  ", stderr);
		char c = getchar();
		if (c == 'n' || c == 'N') { return 0; }
		else if (c != 'y' && c != 'Y') { goto sendEmptyFileConfirmation; }
	}

	ret = fseek(file_fd, 0, SEEK_SET);
	if (ret < 0) { perrorf("[FATAL] failed to fseek back to file %s start", src); return errno; }

	char *file_data = malloc(file_len + 1);
	fread(file_data, file_len, 1, file_fd);
	fclose(file_fd);
	file_data[file_len] = '\0';

	// send it to the calculator (with .tns if needed)
	LOG("Uploading %s to %s\n", src, dest);
	if (strcmp(dest+destLen-4, ".tns") != 0) {
		char *ogDest = dest;
		dest = malloc(destLen + 5); // len + strlen(".tns") + '\0'
		strcpy(dest, ogDest);
		strcpy(dest + destLen, ".tns");
		DEBUG("Added .tns extention (%s)\n", dest);
		ret = nspire_file_write(handle, dest, file_data, file_len);
		free(dest);
	} else { ret = nspire_file_write(handle, dest, file_data, file_len); }
	if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to send file %s", src); free(file_data); return ret; }

	free(file_data);
	return 0;
}


int main(int argc, char *argv[]) {
	// idek bro
	if (argc == 0) { puts("tf there are no arguments i think your shell is broken"); return 2; }

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
				fprintf(stderr, "we need an option name twin\n");
				return 2;
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
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to init libnspire"); return 1; }

		struct cmd_ctx ctx = {0};
		ctx.handle = handle;
		ctx.argv = argv;
		ctx.argc = argc;
		ctx.cmdArg_i = cmdArg_i;

		ret = resolveDir(&ctx);
		if (ret != 0) { return 1; }

		for (int i = cmdArg_i+1; i < argc - 1; i++) {
			if (ctx.attr.type == NSPIRE_DIR) {
				ret = setBasename(&ctx.path, argv[i]);
				if (ret != 0) { return 1; }
			}

			sendPath(handle, argv[i], ctx.path.path);
		}

		DEBUG("Freeing resources...\n");
		if (ctx.path.path != argv[argc-2]) { free(ctx.path.path); }
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

								if (output_fd != 1 && close(output_fd)) { perrorf("[ERROR] failed to close original file after another output file was specified, leaving open"); }
								output_fd = open(destPath, O_WRONLY | O_CREAT, 0755);
								if (output_fd < 0) { perrorf("[FATAL] failed to open output file %s", destPath); return 1; }
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
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to init libnspire"); return 1; }

		LOG("Getting file size...\n");
		struct nspire_dir_item attr = {0};
		ret = nspire_attr(handle, srcPath, &attr);
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to get file info"); return 1; }
		DEBUG("Name: %s, size: %zu bytes, date: %ld, type: %d\n", attr.name, attr.size, attr.date, attr.type);
		if (attr.size == 0) { attr.size = 1280000; DEBUG("File size overridden to %zu\n", attr.size); }

		size_t len = 0;
		uint8_t *buffer = malloc(attr.size);
		if (!buffer) { perrorf("[FATAL] failed to allocate %zu bytes for read buffer", attr.size); return 1; }

		LOG("Reading from file...\n");
		ret = nspire_file_read(handle, srcPath, buffer, attr.size, &len);
		if (ret != NSPIRE_ERR_SUCCESS) {nperrorf("[FATAL] failed to read file"); return 1; }
		DEBUG("Read %zu bytes\n", len);
		DEBUG("Writing to %s\n", destPath);
		if (write(output_fd, buffer, attr.size) < 0) { perrorf("[FATAL] failed to write %zu bytes to file %s\n", len, destPath); return errno; }

		DEBUG("Freeing recources...\n");
		nspire_free(handle);
		free(buffer);

		return 0;
	} else if (strcmp(argv[cmdArg_i],"move")==0 || strcmp(argv[cmdArg_i],"mv")==0 || strcmp(argv[cmdArg_i],"copy")==0 || strcmp(argv[cmdArg_i],"cp")==0) {
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
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to init libnspire"); return 1; }

		struct cmd_ctx ctx = {0};
		ctx.handle = handle;
		ctx.argv = argv;
		ctx.argc = argc;
		ctx.cmdArg_i = cmdArg_i;

		ret = resolveDir(&ctx);
		if (ret != 0) { return 1; }
		DEBUG("ctx.path.path = %s\n", ctx.path.path);

		for (int i = cmdArg_i + 1; i < argc - 1; i++) {
			if (ctx.attr.type == NSPIRE_DIR) {
				ret = setBasename(&ctx.path, argv[i]);
				if (ret != 0) { return 1; }
			}

			// ret = nspire_file_move(handle, "/test.tns", "/testfolder/test.tns"); // stupid function broken in the calculator
			LOG("Copying %s to %s\n", argv[i], ctx.path.path);
			ret = nspire_file_copy(handle, argv[i], ctx.path.path);
			if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to copy file %s to %s", argv[i], ctx.path.path); return 1; }
			if (command[0] == 'm') { // m for move
				DEBUG("Removing original (%s)\n", argv[i]);
				ret = nspire_file_delete(handle, argv[i]);
				if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to remove file %s", argv[i]); return 1; }
			}
		}

		DEBUG("Freeing recources...\n");
		if (ctx.path.path != argv[argc-2]) { free(ctx.path.path); }
		nspire_free(handle);

		return 0;
	} else if (strcmp(argv[cmdArg_i], "list") == 0 || strcmp(argv[cmdArg_i], "ls") == 0) {
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
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to init libnspire"); return 1; }

		struct nspire_dir_info *dirInfo;

		LOG("Getting list...\n");
		ret = nspire_dirlist(handle, dirPath, &dirInfo);
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to get file list"); return 1; }

		// your cpu is gonna be so happy it doesnt have to do extra work for nothing
		if (dirInfo->num == 1) { puts(dirInfo->items[0].name); goto list_cleanUp; }
		else if (dirInfo->num == 0) { goto list_cleanUp; }

		LOG("Formatting...\n");
		qsort(dirInfo->items, dirInfo->num, sizeof(struct nspire_dir_item), sortDirInfo); // sort alphabetically

		FILE *outputStream = format ? popen("column", "w") : stdout;
		if (!outputStream) {
			perrorf("[ERROR] Failed to open pipe to column command to format list, printing without formatting instead");
			format = 0;
			outputStream = stdout;
		}

		for (long unsigned int i = 0; i < dirInfo->num; i++) {
			fputs(dirInfo->items[i].name, outputStream);
			fputc('\n', outputStream);
		}

		if (format) {
			ret = pclose(outputStream);
			if (ret != 0) { fprintf(stderr, "[ERROR] failed to close pipe to column command to format list with code %d, leaving open", ret); }
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
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to init libnspire"); return 1; }

		struct nspire_devinfo info = {0};
		LOG("Getting device information..\n");
		ret = nspire_device_info(handle, &info);
		if (ret != NSPIRE_ERR_SUCCESS) { nperrorf("[FATAL] failed to get info"); return 1; }
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
	return 2;
}
