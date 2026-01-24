#ifndef OO_NSPIRECTL_HELP
#define OO_NSPIRECTL_HELP

#include <stdio.h>

#define LOG(message, ...) if (verbose) { fprintf(stderr, "[LOG] " message, ##__VA_ARGS__); }
#define DEBUG(message, ...) if (debug) { fprintf(stderr, "[DEBUG] " message, ##__VA_ARGS__); }

#define requiresAnArgument(option) fprintf(stderr, "%s: option requires an argument -- '%c'\nTry '%s --help' for more information.\n", argv[0], option, argv[0])
#define missingOperand(cmd) fprintf(stderr, "%s %s: missing operand\nTry '%s %s --help' for more information.\n", argv[0], cmd, argv[0], cmd)
#define unrecognizedGlobalOption(option) fprintf(stderr, "%s: unrecognized option '%s'\nTry '%s --help' for more information.\n", argv[0], option, argv[0])
#define unrecognizedSubcommandOption(cmd, option) fprintf(stderr, "%s %s: unrecognized option '%s'\nTry '%s %s --help' for more information.\n", argv[0], cmd, option, argv[0], cmd)
#define invalidGlobalOption(option) fprintf(stderr, "%s: invalid option -- '%c'\nTry '%s --help' for more information.\n", argv[0], option, argv[0])
#define invalidSubcommandOption(cmd, option) fprintf(stderr, "%s %s: invalid option -- '%c'\nTry '%s %s --help' for more information.\n", argv[0], cmd, option, argv[0], cmd)

extern char const * const main_help_fmt;
extern char const * const send_help_fmt;
extern char const * const read_help_fmt;
extern char const * const list_help_fmt;
extern char const * const moveCopy_help_fmt;
extern char const * const info_help_fmt;

// extern inline void requiresAnArgument(char *argv0, char c);

#endif
