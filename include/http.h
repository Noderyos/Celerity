#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>


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
    char _data[MAX_HEADER_SIZE];
    char* name;
    char* value;
} http_header;

typedef struct {
    http_method method;
    char path[MAX_URL_SIZE];
    int header_count;
    http_header headers[MAX_HEADER_COUNT];
} http_request;

typedef struct {
    unsigned int status_code;
    size_t headers_size;
    http_header headers[MAX_HEADER_COUNT];
    size_t response_size;
    char *response;
} http_response;

http_response handle_request(http_request *req);

int parse_before_header(http_request *req, char* line);
int parse_header(http_request *req, char* line);

char* get_header(http_request *req, char* name);
http_response generate_error(http_request *req, unsigned int status);
char* get_status_text(unsigned int status);

#endif