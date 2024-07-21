#ifndef CELERITY_H
#define CELERITY_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>


#define MAX_HEADER_NAME_SIZE 40
#define MAX_HEADER_COUNT 64
#define MAX_URL_SIZE 2048

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_TRACE
} celerity_method;

#define MAX_HEADER_SIZE 4096
typedef struct {
    char name[MAX_HEADER_SIZE];
    char value[MAX_HEADER_SIZE];
} celerity_header;

typedef struct {
    celerity_method method;
    char path[MAX_URL_SIZE];
    size_t header_count;
    celerity_header headers[MAX_HEADER_COUNT];
} celerity_request;

typedef struct {
    unsigned int status_code;
    size_t headers_count;
    celerity_header headers[MAX_HEADER_COUNT];
    size_t response_size;
    char *response;
} celerity_response;

typedef celerity_response (*handle_t)(celerity_request *);

// ========== Celerity functions ==========

void celerity_init(unsigned int port);
void celerity_listen(int max_clients);
void celerity_loop();

void celerity_route(celerity_method method, char *path, handle_t func);
#define celerity_get(path, func)     celerity_route(HTTP_GET, path, func)
#define celerity_post(path, func)    celerity_route(HTTP_POST, path, func)
#define celerity_put(path, func)     celerity_route(HTTP_PUT, path, func)
#define celerity_delete(path, func)  celerity_route(HTTP_DELETE, path, func)
#define celerity_patch(path, func)   celerity_route(HTTP_PATCH, path, func)
#define celerity_head(path, func)    celerity_route(HTTP_HEAD, path, func)
#define celerity_options(path, func) celerity_route(HTTP_OPTIONS, path, func)
#define celerity_trace(path, func)   celerity_route(HTTP_TRACE, path, func)

char* celerity_get_header(celerity_request *req, char *name);
int celerity_set_header(celerity_response *resp, char *name, char *value);

// ========== Internal functions ==========
// ===== HTTP parsing =====
int celerity_parse_before_header(celerity_request *req, char* line);
int celerity_parse_header(celerity_request *req, char* line);
int celerity_parse_http_headers(int fd, celerity_request *req);

// ===== HTTP response =====
celerity_response celerity_generate_error(unsigned int status);
char* celerity_get_status_text(unsigned int status);
void celerity_send_response(int fd, celerity_response *resp);

char* celerity_pretty_method(celerity_method method);

// ===== Client handler =====
handle_t celerity_get_handle(celerity_request *req);
void celerity_handle_client(int fd, struct sockaddr_in client_addr);
#endif


#ifdef CELERITY_IMPLEMENTATION
#undef CELERITY_IMPLEMENTATION

struct {
    celerity_method method;
    char *path;
    handle_t func;
} routes[256];
size_t route_count = 0;

#define STATUS_COUNT 6

struct http_status{
    unsigned int code;
    char* desc;
} http_status[] = {
        {100, "Continue"},
        {101, "Switching Protocols"},
        {102, "Processing"},
        {103, "Early Hints"},

        {200, "OK"},
        {201, "Created"},
        {202, "Accepted"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {206, "Partial Content"},
        {205, "Reset Content"},
        {207, "Multi-Status"},
        {208, "Already Reported"},
        {214, "Transformation Applied"},
        {226, "IM Used"},

        {300, "Multiple Choices"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {305, "Use Proxy"},
        {307, "Temporary Redirect"},
        {308, "Permanent Redirect"},

        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Timeout"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Payload Too Large"},
        {414, "Request-URI Too Long"},
        {415, "Unsupported Media Type"},
        {416, "Request Range Not Satisfiable"},
        {417, "Expectation Failed"},
        {418, "I’m a teapot"},
        {420, "Enhance Your Calm"},
        {421, "Misdirected Request"},
        {422, "Unprocessable Entity"},
        {423, "Locked"},
        {424, "Failed Dependency"},
        {425, "Too Early"},
        {426, "Upgrade Required"},
        {428, "Precondition Required"},
        {429, "Too Many Requests"},
        {431, "Request Header Fields Too Large"},
        {444, "No Response"},
        {450, "Blocked by Windows Parental Controls"},
        {451, "Unavailable For Legal Reasons"},
        {497, "HTTP Request Sent to HTTPS Port"},
        {498, "Token expired/invalid"},
        {499, "Client Closed Request"},

        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Timeout"},
        {506, "Variant Also Negotiates"},
        {507, "Insufficient Storage"},
        {508, "Loop Detected"},
        {509, "Bandwidth Limit Exceeded"},
        {510, "Not Extended"},
        {511, "Network Authentication Required"},
        {521, "Web Server Is Down"},
        {522, "Connection Timed Out"},
        {523, "Origin Is Unreachable"},
        {525, "SSL Handshake Failed"},
        {530, "Site Frozen"},
        {599, "Network Connect Timeout Error"},
};

static int sockfd;

// ========== Celerity functions ==========

void celerity_init(unsigned int port)
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0)
    {
        perror("socket");
        exit(1);
    }

    int o = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                  &o, sizeof(o)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = 0x0;

    if(bind(sockfd,
            (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }
}

void celerity_listen(int max_clients){
    if(listen(sockfd, max_clients))
    {
        perror("listen");
        exit(1);
    }
}

void celerity_loop()
{
    for (;;)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (clientfd < 0)
        {
            perror("accept");
            return;
        }

        celerity_handle_client(clientfd, client_addr);
    }
}

void celerity_route(celerity_method method, char *path, handle_t func)
{
    routes[route_count].method = method;
    routes[route_count].path = path;
    routes[route_count++].func = func;
}

char* celerity_get_header(celerity_request *req, char* name)
{
    for (size_t i = 0; i < req->header_count; ++i)
    {
        if(strcmp(req->headers[i].name, name) == 0) return req->headers[i].value;
    }
    return NULL;
}

int celerity_set_header(celerity_response *resp, char *name, char *value){
    for (size_t i = 0; i < resp->headers_count; ++i)
    {
        if(strcmp(resp->headers[i].name, name) == 0){
            strncpy(resp->headers[i].value, value, MAX_HEADER_SIZE);
            return 0;
        }
    }
    if(resp->headers_count == MAX_HEADER_COUNT) return -1;
    strncpy(resp->headers[resp->headers_count].value, value, MAX_HEADER_SIZE);
    strncpy(resp->headers[resp->headers_count++].name, name, MAX_HEADER_SIZE);
    return 0;
}


// ========== Internal functions ==========

int celerity_parse_before_header(celerity_request *req, char* line)
{
    char *delimiter = " ";
    char *token = strtok(line, delimiter);

    int i = 0;

    while (token != NULL)
    {
        if(i == 0)
        {
            if(strcmp(token, "GET") == 0) req->method = HTTP_GET;
            else if(strcmp(token, "POST") == 0) req->method = HTTP_POST;
            else if(strcmp(token, "PUT") == 0) req->method = HTTP_PUT;
            else if(strcmp(token, "DELETE") == 0) req->method = HTTP_DELETE;
            else if(strcmp(token, "PATCH") == 0) req->method = HTTP_PATCH;
            else if(strcmp(token, "HEAD") == 0) req->method = HTTP_HEAD;
            else if(strcmp(token, "OPTIONS") == 0) req->method = HTTP_OPTIONS;
            else if(strcmp(token, "TRACE") == 0) req->method = HTTP_TRACE;
            else return -1;
        }
        else if(i == 1)
        {
            strncpy(req->path, token, MAX_URL_SIZE-1);
        }
        else if(i == 2)
        {
            // Nothing, it is HTTP/1.1, can be used after
        }
        else
        {
            return -1;
        }
        token = strtok(NULL, delimiter);
        i++;
    }
    return 0;
}

int celerity_parse_header(celerity_request *req, char* line)
{
    long name_size = strstr(line, ": ") - line;
    if(name_size < 0) return -1;

    if(name_size > MAX_HEADER_NAME_SIZE) return -1;

    strncpy(req->headers[req->header_count].name, line, name_size);
    strncpy(req->headers[req->header_count].value, line+name_size+2, MAX_HEADER_SIZE);

    req->header_count++;
    return 0;
}

celerity_response celerity_generate_error(unsigned int status)
{
    celerity_response resp = {};

    resp.status_code = status;
    resp.response = celerity_get_status_text(status);
    resp.response_size = strlen(resp.response);

    return resp;
}

char* celerity_get_status_text(unsigned int status)
{
    for (int i = 0; i < STATUS_COUNT; ++i)
    {
        if(http_status[i].code == status)
        {
            return http_status[i].desc;
        }
    }
    return NULL;
}

char* celerity_pretty_method(celerity_method method)
{
    switch (method)
    {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        case HTTP_PATCH: return "PATCH";
        case HTTP_HEAD: return "HEAD";
        case HTTP_OPTIONS: return "OPTIONS";
        case HTTP_TRACE: return "TRACE";
        default: return "ERR";
    }
}

int celerity_parse_http_headers(int fd, celerity_request *req)
{
    int header_idx = -1;

    char line[MAX_HEADER_SIZE+1];
    char buf[1];
    int line_idx = 0;
    while (0 != recv(fd, buf, 1, 0))
    {
        if (buf[0] == '\r')
        {
            recv(fd, buf, 1, 0);  // Skip \n after \r
            line[line_idx] = 0;

            if (line_idx == 0) break;

            if (header_idx > MAX_HEADER_COUNT)
            {
                return 413;
            }

            if (header_idx++ < 0)
            {
                if(celerity_parse_before_header(req, line) < 0) return 400;
            }
            else
            {
                if(celerity_parse_header(req, line) < 0) return 400;
            }
            header_idx++;
            line_idx = 0;
        }
        else
        {
            if(line_idx > MAX_HEADER_SIZE)
            {
                if(header_idx < 0) return 414;
                else return 431;
            }
            line[line_idx++] = buf[0];
        }
    }
    return 0;
}


void celerity_send_response(int fd, celerity_response *resp)
{

    send(fd, "HTTP/1.1 ", 9, 0);

    // Send status code
    unsigned int code = resp->status_code;
    char buf[3];
    buf[0] = (char)(48 + code/100);
    buf[1] = (char)(48 + (code%100)/10);
    buf[2] = (char)(48 + (code%10));
    send(fd, buf, 3, 0);

    // Send status text
    send(fd, " ", 1, 0);
    char* status_text = celerity_get_status_text(resp->status_code);
    send(fd, status_text, strlen(status_text), 0);
    send(fd, "\r\n", 2, 0);

    // Send headers
    for (size_t i = 0; i < resp->headers_count; ++i)
    {
        send(fd, resp->headers[i].name, strlen(resp->headers[i].name), 0);
        send(fd, ": ", 2, 0);
        send(fd, resp->headers[i].value, strlen(resp->headers[i].value), 0);
        send(fd, "\r\n", 2, 0);
    }
    send(fd, "\r\n", 2, 0);

    // Send content
    send(fd, resp->response, resp->response_size, 0);
}


handle_t celerity_get_handle(celerity_request *req){
    for (size_t i = 0; i < route_count; ++i)
    {
        if(routes[i].method == req->method && strcmp(routes[i].path, req->path) == 0)
            return routes[i].func;
    }
    return NULL;
}

void celerity_handle_client(int fd, struct sockaddr_in client_addr)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    handle_t handle;
    celerity_request req = {0};
    celerity_response resp;

    int error_code = celerity_parse_http_headers(fd, &req);

    if(error_code == 0)
    {
        handle = celerity_get_handle(&req);
        if(!handle) resp = celerity_generate_error(404);
        else resp = handle(&req);
    }
    else
    {
        resp = celerity_generate_error(error_code);
    }

    printf("%s - %s %s - %d\n",
           client_ip, celerity_pretty_method(req.method), req.path, resp.status_code);

    celerity_send_response(fd, &resp);

    if(error_code == 0 && handle) free(resp.response);

    close(fd);
}
#endif