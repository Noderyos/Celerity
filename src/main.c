#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "http.h"

int parse_http_headers(int fd, http_request *req);

int main(int argc, char *argv[]) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0)
    {
        perror("socket");
        return -1;
    }

    int o = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o)) < 0)
    {
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in sock_info = {0};
    sock_info.sin_family = AF_INET;
    sock_info.sin_port = htons(8080);
    sock_info.sin_addr.s_addr = 0x0;

    if(bind(sockfd, (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0)
    {
        perror("bind");
        return -1;
    }

    if(listen(sockfd, 10))
    {
        perror("listen");
        return -1;
    }


    int clientfd = accept(sockfd, NULL, NULL);
    if(clientfd < 0)
    {
        perror("accept");
        return -1;
    }


    http_request req = {0};
    char response[MAX_RESP_SIZE+1] = {0};
    req.response = response;
    if(parse_http_headers(clientfd, &req) == 0)
    {
        handle_request(&req, MAX_RESP_SIZE);
    }
    send(clientfd, response, strlen(response), 0);

    close(clientfd);
    close(sockfd);

    return 0;
}

int parse_http_headers(int fd, http_request *req)
{
    int header_idx = -1;

    char line[MAX_HEADER_SIZE+1];
    char buf[1];
    int line_idx = 0;
    while (0 != recv(fd, buf, 1, 0))
    {
        if(buf[0] == '\r')
        {
            recv(fd, buf, 1, 0);  // Skip \n after \r
            line[line_idx] = 0;

            if(line_idx == 0) break;

            if(header_idx > MAX_HEADER_COUNT)
            {
                prepare_error(req, 413);
                return -1;
            }

            if (header_idx++ < 0)
            {
                if(parse_before_header(req, line) < 0)
                {
                    prepare_error(req, 400);
                    return -1;
                }
            }
            else
            {
                if(parse_header(req, line) < 0)
                {
                    prepare_error(req, 400);
                    return -1;
                }
            }
            header_idx++;
            line_idx = 0;
        }
        else
        {
            if(line_idx > MAX_HEADER_SIZE)
            {
                if(header_idx < 0) prepare_error(req, 414);
                else               prepare_error(req, 431);
                return -1;
            }
            line[line_idx++] = buf[0];
        }
    }
    return 0;
}