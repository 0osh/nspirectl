#include "help.h"

const char *main_help_fmt =
    "nspirectl — control and manage TI-Nspire devices\n"
    "\n"
    "Usage:\n"
    "  %s <command> [options]\n"
    "\n"
    "Commands:\n"
    "  send        Send a file to the device\n"
    "  read        Read a file from the device\n"
    "  list        List files on the device\n"
    "  move        Move or rename a file\n"
    "  info        Show OS and device information\n"
    "  help        Show this help or help for a command\n"
    "\n"
    "Run '%s help <command>' for details.\n"
;

const char *send_help_fmt =
    "Usage:\n"
    "  %s send <file> [remote_path]\n"
    "\n"
    "Description:\n"
    "  Sends a file to the connected TI-Nspire device.\n"
    "\n"
    "Options:\n"
    "  -f, --force    Overwrite existing file\n"
;

const char *badOption_fmt =
    "%s: unrecognized option '%s'\n"
    "Try '%s --help' for more information.\n"
;
