#ifndef OO_NSPIRECTL_HELP
#define OO_NSPIRECTL_HELP

#include <string.h>

extern const char *main_help_fmt;
extern const char *send_help_fmt;

extern const char *badOption_fmt;

static inline int isHelpCommand(char *string) {
    return strcmp(string, "help") == 0 || strcmp(string, "--help") == 0;
}

#endif
