#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <regex.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "asgn2_helper_funcs.h"
#include "debug.h"
#include "linkedlist.h"
#include "protocol.h"
#include "queue.h"
#include "rwlock.h"

#define response_200_PUT "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n"
#define response_200     "HTTP/1.1 200 OK\r\nContent-Length: "
#define response_201     "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n"
#define response_400     "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n"
#define response_403     "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n"
#define response_404     "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n"
#define response_500                                                                               \
    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server Error\n"
#define response_501 "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n"
#define response_505                                                                               \
    "HTTP/1.1 505 Version Not Supported\r\nContent-Length: 22\r\n\r\nVersion Not Supported\n"
struct Request {
    char method[9];
    char URI[65];
    char version[9];
    char key[129];
    char value[129];
    char rid_key[129];
    int rid_value;
};
bool is_valid_content_length(const char *value) {
    while (*value) {
        if (!isdigit(*value)) {
            return false;
        }
        value++;
    }
    return true;
}
ssize_t my_read_until(int fd, char *buf, size_t n, char *str) {
    size_t total_bytes_read = 0;
    size_t bytes_read;
    char *last_pos = buf;

    if (n == 0)
        return 0;

    do {
        bytes_read = read(fd, buf + total_bytes_read, n - total_bytes_read);

        if (bytes_read == 0) {
            break;
        } else if (bytes_read < 0) {

            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        total_bytes_read += bytes_read;

        if (str) {
            buf[total_bytes_read] = '\0';
            char *found = strstr(last_pos, str);
            if (found != NULL) {
                total_bytes_read = found - buf + strlen(str);
                break;
            }
            last_pos = buf + total_bytes_read;
        }
    } while (total_bytes_read < n);

    return total_bytes_read;
}

#define BUFFER_SIZE 8000
List l;
queue_t *q;
pthread_mutex_t mutex;

void write_response(int server_socket, const char *response) {
    size_t response_length = strlen(response);
    char *writeable_response = malloc(response_length + 1);
    if (writeable_response == NULL) {
        fprintf(stderr, "Failed to allocate memory for response\n");
        return;
    }
    strcpy(writeable_response, response);

    ssize_t bytes_written = write_n_bytes(server_socket, writeable_response, response_length);
    if (bytes_written < 0) {
        fprintf(stderr, "Failed to write response: %s\n", strerror(errno));
    }

    free(writeable_response);
}

void log_error(const char *method, const char *URI, int status_code, int rid_value) {
    fprintf(stderr, "%s,/%s,%d,%d\n", method, URI, status_code, rid_value);
}

void file_handler(int server_socket, struct Request *r) {
    printf("File Handler.");

    switch (errno) {
    case ENOENT:
        write_response(server_socket, response_404);
        log_error(r->method, r->URI, 404, r->rid_value);
        break;

    case EACCES:
    case EISDIR:
        printf("Detected a directory file in file_handler.");
        write_response(server_socket, response_403);
        log_error(r->method, r->URI, 403, r->rid_value);
        break;

    default:
        write_response(server_socket, response_500);
        log_error(r->method, r->URI, 500, r->rid_value);
    }
}
int extract_request(struct Request *r, char *buffer) {
    regex_t requestLine;
    const char *requestLine_pattern
        = "^([a-zA-Z]{1,8}) /([a-zA-Z0-9.-]{1,63}) (HTTP/[0-9][.][0-9])\r\n";
    regex_t headerField;
    const char *headerField_pattern = "([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})\r\n";
    regex_t messageBody;
    const char *messageBody_pattern = "\r\n(.*)";
    int request = regcomp(&requestLine, requestLine_pattern, REG_EXTENDED);
    int header = regcomp(&headerField, headerField_pattern, REG_EXTENDED);
    int message = regcomp(&messageBody, messageBody_pattern, REG_EXTENDED);
    regmatch_t requestLine_match[4];
    regmatch_t headerField_match[3];
    regmatch_t messageBody_match[2];
    int res = regexec(&requestLine, buffer, 4, requestLine_match, 0);
    if (res != 0) {
        regfree(&requestLine);
        regfree(&headerField);
        regfree(&messageBody);
        return -1;
    }
    char *cursor = buffer + requestLine_match[0].rm_eo;
    int finalpos = requestLine_match[0].rm_eo;
    if (request != 0 || header != 0 || message != 0) {
        return -3;
    }
    strncpy(r->method, buffer + requestLine_match[1].rm_so,
        requestLine_match[1].rm_eo - requestLine_match[1].rm_so);
    strncpy(r->URI, buffer + requestLine_match[2].rm_so,
        requestLine_match[2].rm_eo - requestLine_match[2].rm_so);
    strncpy(r->version, buffer + requestLine_match[3].rm_so,
        requestLine_match[3].rm_eo - requestLine_match[3].rm_so);
    while (regexec(&headerField, cursor, 3, headerField_match, 0) == 0) {
        char multipleHeaders[129];
        if (strcmp(cursor, cursor + headerField_match[0].rm_so) != 0) {
            regfree(&headerField);
            regfree(&messageBody);
            regfree(&requestLine);
            return -1;
        }
        memset(multipleHeaders, '\0', sizeof(multipleHeaders));
        strncpy(multipleHeaders, cursor + headerField_match[1].rm_so,
            headerField_match[1].rm_eo - headerField_match[1].rm_so);
        fprintf(stdout, " Headers are %s \n", multipleHeaders);

        if (strcmp(multipleHeaders, "Content-Length") == 0) {
            size_t keyLength = headerField_match[1].rm_eo - headerField_match[1].rm_so;
            size_t valueLength = headerField_match[2].rm_eo - headerField_match[2].rm_so;
            strncpy(r->key, cursor + headerField_match[1].rm_so, keyLength);
            r->key[keyLength] = '\0';
            strncpy(r->value, cursor + headerField_match[2].rm_so, valueLength);
            r->value[valueLength] = '\0';
        } else if (strcmp(multipleHeaders, "Request-Id") == 0) {
            size_t ridKeyLength = headerField_match[1].rm_eo - headerField_match[1].rm_so;
            size_t ridValueLength = headerField_match[2].rm_eo - headerField_match[2].rm_so;
            strncpy(r->rid_key, cursor + headerField_match[1].rm_so, ridKeyLength);
            r->rid_key[ridKeyLength] = '\0';

            char requestIdValue[129];
            strncpy(requestIdValue, cursor + headerField_match[2].rm_so, ridValueLength);
            requestIdValue[ridValueLength] = '\0';
            r->rid_value = atoi(requestIdValue);
        }
        cursor += headerField_match[0].rm_eo;
        finalpos += headerField_match[0].rm_eo;
    }
    int messageBodyResult = regexec(&messageBody, cursor, 2, messageBody_match, 0);
    if (messageBodyResult != 0 || (strcmp(cursor, cursor + messageBody_match[0].rm_so) != 0)) {
        fprintf(stderr, "error extracting message-body\n");
        regfree(&requestLine);
        regfree(&headerField);
        regfree(&messageBody);
        return -1;
    }
    regfree(&requestLine);
    regfree(&headerField);
    regfree(&messageBody);
    return finalpos + 2;
}
void *task() {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    while (true) {
        int server_socket;
        struct Request r;
        queue_pop(q, (void **) &server_socket);
        int initial_message = read_until(server_socket, buffer, 2048, "\r\n\r\n");
        memset(&r, 0, sizeof(struct Request));
        int before_message = extract_request(&r, buffer);
        if (before_message > 2048) {
            char *error_response = strdup(response_400);
            if (error_response) {
                write_n_bytes(server_socket, error_response, strlen(error_response));
                free(error_response);
            }
            memset(buffer, 0, BUFFER_SIZE);
            read_n_bytes(server_socket, buffer, BUFFER_SIZE);
            fprintf(stderr, "Buffer after exceeding limit: %s\n", buffer);
            close(server_socket);
            continue;
        }
        if (before_message == -1) {
            char *error_response = strdup(response_400);
            if (error_response) {
                write_n_bytes(server_socket, error_response, strlen(error_response));
                free(error_response);
            }
            fprintf(stderr, "Request parse failure from %s on %s, Code %d, ID %d\n", r.method,
                r.URI, 400, r.rid_value);
            close(server_socket);
            continue;
        }
        rwlock_t *rwlock = NULL;
        pthread_mutex_lock(&mutex);
        moveFront(l);
        while (curr_index(l) != -1) {
            if (strcmp((char *) getfile(l), r.URI) == 0) {
                rwlock = (rwlock_t *) getlock(l);
                break;
            }
            moveNext(l);
        }
        if (rwlock == NULL) {
            rwlock = rwlock_new(2, 1);
            append(l, r.URI, rwlock);
        }
        pthread_mutex_unlock(&mutex);

        if (strcmp(r.method, "GET") == 0) {
            reader_lock(rwlock);
            if (strcmp(r.version, "HTTP/1.1") != 0) {
                write_n_bytes(server_socket, response_505, strlen(response_505));
                fprintf(stderr, "%s,/%s,%d,%d\n", r.method, r.URI, 505, r.rid_value);
                reader_unlock(rwlock);
                close(server_socket);
                continue;
            }
            int fd = open(r.URI, O_RDWR);
            if (fd == -1) {
                file_handler(server_socket, &r);
                reader_unlock(rwlock);
                close(server_socket);
                continue;
            }
            struct stat fileStat;
            fstat(fd, &fileStat);
            uint64_t filesize = fileStat.st_size;

            char fsize[129] = { "\0" };

            write_n_bytes(server_socket, response_200, strlen(response_200));
            write_n_bytes(server_socket, fsize, strlen(fsize));
            write_n_bytes(server_socket, "\r\n\r\n", 4);
            pass_n_bytes(fd, server_socket, filesize);
            fprintf(stderr, "%s,/%s,%d,%d\n", r.method, r.URI, 200, r.rid_value);
            reader_unlock(rwlock);
            close(fd);
        } else if (strcmp(r.method, "PUT") == 0) {
            writer_lock(rwlock);
            if (strcmp(r.version, "HTTP/1.1") != 0) {
                write_n_bytes(server_socket, response_505, strlen(response_505));
                fprintf(stderr, "%s,/%s,%d,%d\n", r.method, r.URI, 505, r.rid_value);
                writer_unlock(rwlock);
                close(server_socket);
                continue;
            }
            if (strcmp(r.key, "Content-Length") != 0) {
                write_n_bytes(server_socket, response_400, strlen(response_400));
                fprintf(stderr, "%s,/%s,%d,%d\n", r.method, r.URI, 400, r.rid_value);
                writer_unlock(rwlock);
                close(server_socket);
                continue;
            }
            bool new_f = false;
            int fd = open(r.URI, O_WRONLY | O_TRUNC, 0666);
            if (fd == -1) {
                fd = open(r.URI, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                if (fd == -1) {
                    file_handler(server_socket, &r);
                    writer_unlock(rwlock);
                    close(server_socket);
                    continue;
                }
                new_f = true;
            }
            int bytes_to_write = initial_message - before_message;
            int written = write_n_bytes(fd, buffer + before_message, bytes_to_write);
            int num_written = atoi(r.value);
            pass_n_bytes(server_socket, fd, num_written - written);
            if (new_f) {
                write_n_bytes(server_socket, response_201, strlen(response_201));
                fprintf(stderr, "%s,/%s,%d,%d\n", r.method, r.URI, 201, r.rid_value);
            } else {
                write_n_bytes(server_socket, response_200_PUT, strlen(response_200_PUT));
                fprintf(stderr, "%s,/%s,%d,%d\n", r.method, r.URI, 200, r.rid_value);
            }
            writer_unlock(rwlock);
            close(fd);
        } else {
            write_n_bytes(server_socket, response_501, strlen(response_501));
            fprintf(stderr, "%s,/%s,%d,%d\n", r.method, r.URI, 501, r.rid_value);
        }
        memset(buffer, 0, BUFFER_SIZE);
        memset(&r, 0, sizeof(struct Request));
        close(server_socket);
    }
}

List l;
queue_t *q;
pthread_mutex_t mutex;

int main(int argc, char **argv) {
    int opt;
    int threads = 4;
    char *endptr;

    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, &endptr, 10);
            if (*endptr != '\0' || threads <= 0) {
                fprintf(stderr, "Invalid number of threads: '%s'\n", optarg);
                return -1;
            }
            break;
        default: fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); return -1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]);
        return -1;
    }

    long port = strtol(argv[optind], &endptr, 10);
    if (*endptr != '\0' || port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port number: '%s'\n", argv[optind]);
        return -1;
    }

    l = newList();
    if (l == NULL) {
        fprintf(stderr, "Failed to initialize the list\n");
        return -1;
    }

    pthread_mutex_init(&mutex, NULL);
    q = queue_new(threads);
    if (!q) {
        fprintf(stderr, "Failed to initialize the queue\n");
        pthread_mutex_destroy(&mutex);
        return -1;
    }

    pthread_t *workers = malloc(threads * sizeof(pthread_t));
    if (!workers) {
        fprintf(stderr, "Memory allocation failed for worker threads\n");
        pthread_mutex_destroy(&mutex);
        return -1;
    }

    for (int i = 0; i < threads; ++i) {
        if (pthread_create(&workers[i], NULL, task, NULL) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            for (int j = 0; j < i; j++) {
                pthread_cancel(workers[j]);
                pthread_join(workers[j], NULL);
            }
            free(workers);
            pthread_mutex_destroy(&mutex);
            return -1;
        }
    }

    Listener_Socket listener;
    if (listener_init(&listener, port) != 0) {
        fprintf(stderr, "Failed to initialize listener on port %ld\n", port);
        pthread_mutex_destroy(&mutex);
        free(workers);
        return 1;
    }

    while (1) {
        int socket_fd = listener_accept(&listener);
        if (socket_fd == -1) {
            fprintf(stderr, "Error accepting connection\n");
            continue;
        }
        bool pushed = queue_push(q, (void *) (intptr_t) socket_fd);
        if (!pushed) {
            close(socket_fd);
        }
    }
    pthread_mutex_destroy(&mutex);
    free(workers);
    return 0;
}
