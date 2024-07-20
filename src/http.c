#include "http.h"

int parse_before_header(http_request *req, char* line)
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

int parse_header(http_request *req, char* line)
{
    long name_size = strstr(line, ": ") - line;
    if(name_size < 0) return -1;

    if(name_size > MAX_HEADER_NAME_SIZE) return -1;

    strncpy(req->headers[req->header_count].name, line, name_size);
    strncpy(req->headers[req->header_count].value, line+name_size+2, MAX_HEADER_SIZE);

    req->header_count++;
    return 0;
}

char* get_header(http_request *req, char* name)
{
    for (size_t i = 0; i < req->header_count; ++i)
    {
        if(strcmp(req->headers[i].name, name) == 0) return req->headers[i].value;
    }
    return NULL;
}

int set_header(http_response *resp, char *name, char *value){
    UNUSED(value);
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

http_response handle_request(http_request *req)
{
    http_response resp = {};
    resp.response = malloc(MAX_RESP_SIZE);
    resp.status_code = 413;

    char* response = resp.response;

    strcpy(response, "<h1>You have requested the path ");
    strcat(response, req->path);
    strcat(response, "</h1><br>Headers list : <ul>");
    for (size_t i = 0; i < req->header_count; ++i)
    {
        http_header h = req->headers[i];

        strcat(response, "<li>Name = ");
        strcat(response, h.name);
        strcat(response, "<br>Value = ");
        strcat(response, h.value);
        strcat(response, "</li><br>");
    }
    resp.response_size = strlen(resp.response);

    return resp;
}

#define STATUS_COUNT 6

struct http_status{
    unsigned int code;
    char* desc;
} http_status[] = {
        {200, "OK"},
        {404, "Not Found"},
        {413, "Payload Too Large"},
        {400, "Bad Request"},
        {414, "URI Too Long"},
        {431, "Request Header Fields Too Large"},
};

http_response generate_error(http_request *req, unsigned int status)
{
    UNUSED(req);
    http_response resp = {};

    resp.status_code = status;
    resp.response = get_status_text(status);
    resp.response_size = strlen(resp.response);

    return resp;
}

char* get_status_text(unsigned int status)
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

char* pretty_method(http_method method)
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