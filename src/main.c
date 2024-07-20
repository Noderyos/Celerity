#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "http.h"
#include "route.h"

http_response handle_request(http_request *req);

int main(int argc, char *argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    int o = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                  &o, sizeof(o)) < 0)
    {
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = 0x0;

    if(bind(sockfd,
            (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        return -1;
    }

    if(listen(sockfd, 10))
    {
        perror("listen");
        return -1;
    }

    register_route(HTTP_GET, "/uwu", handle_request);
    register_post("/uwu", handle_request);

    for (;;) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (clientfd < 0) {
            perror("accept");
            return -1;
        }

        handle_client(clientfd, client_addr);
    }

    close(sockfd);

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