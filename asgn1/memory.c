#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <linux/limits.h>
#define BUFFER_SIZE 9000
char buf[BUFFER_SIZE];
void freeab(char *a, char *b) {
    free(a);
    free(b);
}

int main() {
    char *comcop = NULL;
    int readbytes = 0;
    int totalbytes = 0;
    char *command = (char *) calloc(BUFFER_SIZE, sizeof(char));
    do {
        readbytes = read(STDIN_FILENO, buf, 1);
        if (readbytes == -1) {
            fprintf(stderr, "Operation Failed\n");
            freeab(comcop, command);
            exit(1);
        }
        memcpy(command + totalbytes, buf, readbytes);
        totalbytes += readbytes;
    } while (readbytes > 0 && totalbytes < 4);
    if (strncmp(command, "get", 3) == 0) {
        while ((readbytes = read(STDIN_FILENO, buf, BUFFER_SIZE)) > 0) {
            memcpy(command + totalbytes, buf, readbytes);
            totalbytes += readbytes;
        }
        if (readbytes == -1) {
            fprintf(stderr, "Operation Failed\n");
            freeab(comcop, command);
            exit(1);
        }
        command[totalbytes] = '\0';
        char *comcop = strdup(command);
        strtok(command, "\n");
        char *location;
        location = strtok(NULL, "\n");
        if (strtok(NULL, "\n") != NULL || location == NULL) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }
        if (comcop[totalbytes - 1] != '\n' || comcop[3] != '\n') {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }
        if (comcop[totalbytes - 1] != '\n' || comcop[totalbytes - 2] == '\n' || comcop[3] != '\n'
            || comcop[4] == '\n') {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }
        if (strtok(NULL, "\n") != NULL) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }
        int fd = open(location, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Invalid Command\n");
            close(fd);
            exit(1);
        }
        for (;;) {
            readbytes = read(fd, buf, BUFFER_SIZE);
            if (readbytes <= 0) {
                if (readbytes == 0) {
                    break;
                }
                fprintf(stderr, "Operation Failed\n");
                close(fd);
                exit(1);
            }

            ssize_t bytes_written_total = 0;
            while (bytes_written_total < readbytes) {
                ssize_t bytes_written = write(
                    STDOUT_FILENO, buf + bytes_written_total, readbytes - bytes_written_total);
                if (bytes_written < 0) {
                    fprintf(stderr, "Operation Failed\n");
                    close(fd);
                    exit(1);
                } else if (bytes_written == 0) {
                    fprintf(stderr, "Operation Failed\n");
                    close(fd);
                    exit(1);
                }
                bytes_written_total += bytes_written;
            }
        }
        close(fd);
        free(comcop);
        free(command);
        return 0;
    } else if (strncmp(command, "set\n", 4) == 0) {
        int newline = 0;
        do {
            readbytes = read(STDIN_FILENO, buf, 1);
            if (readbytes == -1) {
                break;
            }
            command = realloc(command, totalbytes + readbytes + 1);
            memcpy(command + totalbytes, buf, readbytes);
            totalbytes += readbytes;
            if (command[totalbytes - 1] == '\n') {
                newline++;
            }
            if (newline == 2) {
                break;
            }
        } while (readbytes > 0);

        command = realloc(command, totalbytes + 1);
        command[totalbytes] = '\0';
        char *comcop = strdup(command);
        char *location;
        char *content_length;
        strtok(command, "\n");
        location = strtok(NULL, "\n");
        content_length = strtok(NULL, "\n");
        if (!content_length || !location) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        if (comcop[3] != '\n') {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        int fd = open(location, O_CREAT | O_RDWR | O_TRUNC, 0666);
        if (fd == -1) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        int bytes_to_write = atoi(content_length);
        if (strcmp(content_length, "0") != 0 && bytes_to_write == 0) {
            fprintf(stderr, "Invalid Command\n");
            close(fd);
            exit(1);
        }

        if (bytes_to_write < 0) {
            fprintf(stderr, "Invalid Command\n");
            close(fd);
            exit(1);
        }

        int readbytes, bytes_written_total = 0, write_ret;
        while ((readbytes = read(STDIN_FILENO, buf, BUFFER_SIZE)) > 0) {
            if (bytes_written_total < bytes_to_write) {
                int to_write = bytes_to_write - bytes_written_total;
                if (to_write > readbytes) {
                    to_write = readbytes;
                }
                write_ret = write(fd, buf, to_write);
                if (write_ret < 0) {

                    close(fd);
                    exit(1);
                }
                bytes_written_total += write_ret;
            }
            if (bytes_written_total >= bytes_to_write) {
                while ((readbytes = read(STDIN_FILENO, buf, BUFFER_SIZE)) > 0) {
                    continue;
                }
                break;
            }
        }
        if (readbytes == -1) {
            close(fd);
            exit(1);
        }
        fprintf(stdout, "OK\n");
        close(fd);
        free(command);
        free(comcop);
        return 0;
    } else {

        fprintf(stderr, "Invalid Command\n");
        free(command);
        exit(1);
    }
}
