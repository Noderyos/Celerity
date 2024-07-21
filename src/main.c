#define CELERITY_IMPLEMENTATION
#include "celerity.h"

celerity_response handle_request(celerity_request *req);

int main(void)
{
    celerity_init(8080);

    celerity_route(HTTP_GET, "/uwu", handle_request);
    celerity_post("/uwu", handle_request);

    celerity_listen(10);
    celerity_loop();

    close(sockfd);
    return 0;
}

celerity_response handle_request(celerity_request *req)
{
    celerity_response resp = {};
    resp.response = malloc(4096);
    resp.status_code = 200;

    char* response = resp.response;

    strcpy(response, "<h1>You have requested the path ");
    strcat(response, req->path);
    strcat(response, "</h1><br>Headers list : <ul>");
    for (size_t i = 0; i < req->header_count; ++i)
    {
        celerity_header h = req->headers[i];

        strcat(response, "<li>Name = ");
        strcat(response, h.name);
        strcat(response, "<br>Value = ");
        strcat(response, h.value);
        strcat(response, "</li><br>");
    }
    resp.response_size = strlen(resp.response);

    return resp;
}