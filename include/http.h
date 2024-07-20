#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <unistd.h>



#define MAX_HEADER_NAME_SIZE 40
#define MAX_HEADER_SIZE 4096 // HTTP RFC doesn't give a limit
#define MAX_HEADER_COUNT 64
#define MAX_URL_SIZE 2048
#define MAX_RESP_SIZE 1024

#define UNUSED(x) (void)(x)

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_TRACE
} http_method;

typedef struct {
    char name[MAX_HEADER_SIZE];
    char value[MAX_HEADER_SIZE];
} http_header;

typedef struct {
    http_method method;
    char path[MAX_URL_SIZE];
    size_t header_count;
    http_header headers[MAX_HEADER_COUNT];
} http_request;

typedef struct {
    unsigned int status_code;
    size_t headers_count;
    http_header headers[MAX_HEADER_COUNT];
    size_t response_size;
    char *response;
} http_response;

typedef http_response (*handle_t)(http_request *);

int parse_before_header(http_request *req, char* line);
int parse_header(http_request *req, char* line);

char* get_header(http_request *req, char *name);
int set_header(http_response *resp, char *name, char *value);
http_response generate_error(http_request *req, unsigned int status);
char* get_status_text(unsigned int status);

int parse_http_headers(int fd, http_request *req);
void send_response(int fd, http_response *resp);

char* pretty_method(http_method method);

#endif