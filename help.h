#ifndef OO_NSPIRECTL_HELP
#define OO_NSPIRECTL_HELP

#include <stdio.h>

#define requiresAnArgument(option) fprintf(stderr, "%s: option requires an argument -- '%c'\nTry '%s --help' for more information.\n", argv[0], option, argv[0])
#define missingOperand() fprintf(stderr, "%s %s: missing operand\nTry '%s %s --help' for more information.\n", argv[0], argv[cmdArg_i], argv[0], argv[cmdArg_i])
#define missingNamedOperand(operand) fprintf(stderr, "%s %s: missing %s operand\nTry '%s %s --help' for more information.\n", argv[0], argv[cmdArg_i], operand, argv[0], argv[cmdArg_i])
#define unrecognizedGlobalOption(option) fprintf(stderr, "%s: unrecognized option '%s'\nTry '%s --help' for more information.\n", argv[0], option, argv[0])
#define unrecognizedSubcommandOption(option) fprintf(stderr, "%s %s: unrecognized option '%s'\nTry '%s %s --help' for more information.\n", argv[0], argv[cmdArg_i], option, argv[0], argv[cmdArg_i])
#define invalidGlobalOption(option) fprintf(stderr, "%s: invalid option -- '%c'\nTry '%s --help' for more information.\n", argv[0], option, argv[0])
#define invalidSubcommandOption(option) fprintf(stderr, "%s %s: invalid option -- '%c'\nTry '%s %s --help' for more information.\n", argv[0], argv[cmdArg_i], option, argv[0], argv[cmdArg_i])

#define printMainHelp() printf(main_help_fmt, argv[0], argv[0], argv[0])
#define printSendHelp() printf(send_help_fmt, argv[0], argv[0])
#define printReadHelp() printf(read_help_fmt, argv[0], argv[0])
#define printListHelp() printf(list_help_fmt, argv[0], argv[0])
#define printMoveCopyHelp() printf(moveCopy_help_fmt, argv[0], argv[0], argv[0], argv[0], argv[0], argv[0])
#define printInfoHelp() printf(info_help_fmt, argv[0], argv[0])
#define printScreenshotHelp() printf(screenshot_help_fmt, argv[0], argv[0])
#define printUpdateHelp() printf(update_help_fmt, argv[0], argv[0])
#define printHelpHelp() printf(help_help_fmt, argv[0], argv[0])

char const * const main_help_fmt =
	"%s - Manage files on TI-nspire devices\n"
	"\n"
	"Usage:\n"
	"  %s [option]... <command> [<args>]\n"
	"\n"
	"Description:\n"
	"  A simple wrapper for the libnspire functions as a command line tool. I made\n"
	"  this mostly because I just cant get tilp to work, but the raw functions do.\n"
	"  If there's any mistakes or you have any suggestions, even tiny little picky\n"
	"  ones, please dont hesitate to contact me or submit a pull request.\n"
	"\n"
	"Commands:\n"
	"  send, upload      Send a file or directory to the device\n"
	"  read, download    Read a file from the device\n"
	"  move, mv          Move or rename a file or directory\n"
	"  copy, cp          Duplicate a file or directory\n"
	"  list, ls          List files in a directory\n"
	// "  del, rm         Delete a file or directory\n" // im too scared im gonna screw it up and delete the wrong things
	"  info              Show OS and device information\n"
	"  screenshot        Take a screenshot of the device\n"
	"  update            Update the OS on the calculator\n"
	"  help              Show this help or the help for a command\n"
	"\n"
	"Options:\n"
	"  -v, --verbose     log what is being done\n"
	"  -d, --debug       extra logging (implies -v)\n"
	"      --help        show this help\n"
	"\n"
	"Run '%s help <command>' for more information.\n"
;

char const * const send_help_fmt =
	"%s send, upload - send files to the connected calculator"
	"\n"
	"Usage:\n"
	"  %s send [option]... <file>... <destination>\n"
	"\n"
	"Description:\n"
	"  Copies one or more files or directories to the specified location on the\n"
	"  calculator.\n"
	"  \n"
	"  The path is relative to the \"My Documents\" folder in calculator. That is the\n"
	"  folder that every file is in when you press \"Browse\" in the home screen.\n"
	"  Paths beginning with a slash to denote root (for example `/folder/file.tns`)\n"
	"  are also placed relative to the \"My Documents\" folder. This means\n"
	"  `/folder/file.tns` and `folder/file.tns` refer to the same file.\n"
   "\n"
   "Options:\n"
   // "  -f, --force        Overwrite existing file\n"
   "      --help         show this help\n"
;

char const * const read_help_fmt =
	"%s read, download - reads a file\n"
	"\n"
	"Usage:\n"
	"  %s read [option]... <file>...\n"
	"Description:\n"
	"  Reads the contents of a file or directory. If `read` is used, it will print\n"
	"  the contents to stdout. If `download` is used, it will save the contents to a\n"
	"  file with the same name and directory structure.\n"
	"\n"
	"Options:\n"
	"  -o <file>         write output to the specified file instead of stdout. The\n"
	"                    file is created if it doesn't exist, or overwritten if it\n"
    "                    does.\n"
	"      --help        show this help\n"
;

char const * const list_help_fmt =
	"%s list, ls - list directory contents\n"
	"\n"
	"Usage:\n"
	"  %s list [option]... <directory>...\n"
	"\n"
	"Description:\n"
	"  Lists the files in <directory> like the `ls` command\n"
	"\n"
	"Options:\n"
	"      --no-format   dont format the list\n"
	"      --help        show this help\n"
;

char const * const moveCopy_help_fmt =
	"%s move, mv - move/rename files and directories within the device\n"
	"%s copy, cp - copy files and directories within the device\n"
	"\n"
	"Usage:\n"
	"  %s move [option]... <file>... <directory>\n"
	"  %s move [option]... <file> <destination>\n"
	"  \n"
	"  %s copy [option]... <file>... <directory>\n"
	"  %s copy [option]... <file> <destination>\n"
	"\n"
	"Description:\n"
	"  Renames/duplicates <file> to <destination>, or moves/duplicates <file>(s) to\n"
	"  <directory>.\n"
	"  \n"
	"  A directory is denoted by a '/' at the end, multiple files being specified\n"
	"  or already existing in the file system as a folder. A slash ('/') is added\n"
	"  between <file> and <directory> if one is required to make a valid path.\n"
	"\n"
	"Options:\n"
	"      --help        show this help\n"
;

char const * const info_help_fmt =
	"%s info - get information related to the connected device\n"
	"\n"
	"Usage:\n"
	"  %s info [option]...\n"
	"\n"
	"Description:\n"
	"  Prints out all the fields filled in by `nspire_device_info` and formats it.\n"
	"  The libnspire library is missing 0x1c from the type code enum, so I assumed\n"
	"  it's whether or not the calculator is a CAS. If thats wrong, there's any\n"
	"  other mistakes, or you have an improvement please contant me or submit a pull\n"
	"  request.\n"
	"\n"
	"Options:\n"
	"      --help        show this help\n"
;

char const * const screenshot_help_fmt =
	"%s screenshot - take a screenshot of the device\n"
	"\n"
	"Usage:\n"
	"  %s screenshot [option]... [file]\n"
	"\n"
	"Description:\n"
	"  Take a screenshot of the device and save the image as `screenshot.bmp`, or\n"
	"  [file] if provided. Assumes that the device bpp is 16 because I don't have any\n"
	"  way to test 8bpp or 24bpp with real data. 24 bpp should work though. When\n"
	"  using 8bpp in a bitmap file for some reason it requires that you use a palate\n"
	"  and declare all the colours you use before the image data, and reference each\n"
	"  colour by index. If anyone has 8bpp or 24bpp image data it would be great if\n"
	"  you could send it to me or make a pull request.\n"
	"\n"
	"Options:\n"
	"      --help        show this help\n"
;

char const * const update_help_fmt =
	"%s update - update the device OS\n"
	"\n"
	"Usage:\n"
	"  %s update [option]... <file>\n"
	"\n"
	"Description:\n"
	"  Send an OS from <file> to the device. This is used to update the OS.\n"
	"\n"
	"Options:\n"
	"      --help        show this help\n"
;

char const * const help_help_fmt =
	"%s help - display help information about nspirectl\n"
	"\n"
	"Usage:\n"
	"  %s help [option]... [command]\n"
	"\n"
	"Description:\n"
    "  With no command name, general help for nspirectl is given. With a command\n"
    "  name, help for that command is given.\n"
    "\n"
    "Options:\n"
    "      --help        show this help\n"
;

#endif
