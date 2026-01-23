#include "help.h"

const char *main_help_fmt =
	"nspirectl - control and manage TI-Nspire devices\n"
	"\n"
	"Usage:\n"
	"  %s <command> [<args>]\n"
	"\n"
	"Commands:\n"
	"  send        Send a file to the device\n"
	// "  read        Read a file from the device\n"
//    "  list        List files on the device\n"
//    "  move        Move or rename a file\n"
	"  info        Show OS and device information\n"
	"  help        Show this help or help for a command\n"
	"\n"
	"Run '%s help <command>' for details.\n"
;

const char *send_help_fmt =
	"%s send - send files to the connected TI-Nspire device."
	"\n"
	"Usage:\n"
	"  %s send <file>... <destination>\n"
	"\n"
	"Description:\n"
	"  Copies one or more files to the specified directory on the connected TI-Nspire\n"
	"  device.\n"
	"  \n"
	"  The path is relative to the \"My Documents\" folder in calculator. That is the\n"
	"  folder that every file is in when you press \"Browse\" in the home screen.\n"
	"  Paths beginning with a slash to denote root (for example `/folder/file.tns`)\n"
	"  are also placed relative to the \"My Documents\" folder. This means\n"
	"  `/folder/file.tns` and `folder/file.tns` refer to the same file.\n"
//    "\n"
//    "Options:\n"
//    "  -f, --force    Overwrite existing file\n"
;

const char *badOption_fmt =
	"%s: unrecognized option '%s'\n"
	"Try '%s --help' for more information.\n"
;
