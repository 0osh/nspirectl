#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libnspire/nspire.h>
#include "help.h"

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
        if (argc < 3) {
            printf(send_help_fmt, argv[0]);
            return 0;
        }

        // init libnspire
        nspire_handle_t *handle;
        ret = nspire_init(&handle);
        if (ret != 0) { fprintf(stderr, "Error: failed to init libnspire: %s", nspire_strerror(ret)); }

        // get file
        FILE *file_fd = fopen(argv[2], "rb");
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
        printf("Uploading %s\n", argv[2]);
        ret = nspire_file_write(handle, argv[2], file_data, file_len);
        if (ret != 0) { fprintf(stderr, "Error: failed to send file: %s", nspire_strerror(ret)); }

        // free resources
        nspire_free(handle);

        puts("done");
        return ret;
    }

    // invalid command
    printf(badOption_fmt, argv[0], argv[1], argv[0]);
    return 1;
}
