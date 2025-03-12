#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFFER_SIZE        512
#define OUTPUT_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Not enough arguments\n");
        exit(2);
    }

    if (strlen(argv[1]) > 1) {
        fprintf(stderr, "Cannot handle multi-character splits: %s\n", argv[1]);
        exit(3);
    }

    char delimiter = argv[1][0];
    for (int i = 2; i < argc; i++) {
        const char *filename = argv[i];

        if (strcmp(filename, "..") == 0) {
            fprintf(stderr, "Cannot process file '..'\n");
            exit(21);
        }

        int file_descriptor;
        if (strcmp(filename, "-") == 0) {
            file_descriptor = STDIN_FILENO;
        } else {
            file_descriptor = open(filename, O_RDONLY);
            if (file_descriptor == -1) {
                fprintf(stderr, "split: %s: Unable to open file\n", filename);
                continue;
            }
        }

        char buffer[BUFFER_SIZE];
        char output_buffer[OUTPUT_BUFFER_SIZE];
        size_t output_index = 0;
        ssize_t bytes_read;
        while ((bytes_read = read(file_descriptor, buffer, BUFFER_SIZE)) > 0) {
            for (ssize_t j = 0; j < bytes_read; j++) {
                if (buffer[j] == delimiter) {
                    output_buffer[output_index++] = '\n';
                    if (output_index >= OUTPUT_BUFFER_SIZE - 1) {
                        write(STDOUT_FILENO, output_buffer, output_index);
                        output_index = 0;
                    }
                } else {
                    output_buffer[output_index++] = buffer[j];
                    if (output_index >= OUTPUT_BUFFER_SIZE - 1) {
                        write(STDOUT_FILENO, output_buffer, output_index);
                        output_index = 0;
                    }
                }
            }
            if (buffer[bytes_read - 1] == '\n') {
                write(STDOUT_FILENO, output_buffer, output_index);
                output_index = 0;
            }
        }

        if (output_index > 0) {
            write(STDOUT_FILENO, output_buffer, output_index);
        }

        close(file_descriptor);
    }

    return 0;
}
