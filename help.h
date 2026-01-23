#ifndef OO_NSPIRECTL_HELP
#define OO_NSPIRECTL_HELP

#include <stdio.h>

#define LOG(message, ...) if (verbose) { fprintf(stderr, "[LOG] " message, ##__VA_ARGS__); }
#define DEBUG(message, ...) if (debug) { fprintf(stderr, "[DEBUG] " message, ##__VA_ARGS__); }

#define requiresAnArgument(argv0, option) fprintf(stderr, "%s: option requires an argument -- '%c'\nTry '%s --help' for more information.\n", argv0, option, argv0)
#define missingOperand(argv0, cmd) fprintf(stderr, "%s %s: missing operand\nTry '%s %s --help' for more information.\n", argv0, cmd, argv0, cmd)
#define unrecognizedGlobalOption(argv0, option) fprintf(stderr, "%s: unrecognized option '%s'\nTry '%s --help' for more information.\n", argv0, option, argv0)
#define unrecognizedSubcommandOption(argv0, cmd, option) fprintf(stderr, "%s %s: unrecognized option '%s'\nTry '%s %s --help' for more information.\n", argv0, cmd, option, argv0, cmd)
#define invalidGlobalOption(argv0, option) fprintf(stderr, "%s: invalid option -- '%c'\nTry '%s --help' for more information.\n", argv0, option, argv0)
#define invalidSubcommandOption(argv0, cmd, option) fprintf(stderr, "%s %s: invalid option -- '%c'\nTry '%s %s --help' for more information.\n", argv0, cmd, option, argv0, cmd)

extern char const * const main_help_fmt;
extern char const * const send_help_fmt;
extern char const * const info_help_fmt;

extern const char *unrecognizedGlobalOption_fmt;
extern const char *unrecognizedSubcommandOption_fmt;

// extern inline void requiresAnArgument(char *argv0, char c);

#endif
