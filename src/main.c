#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "http.h"

int parse_http_headers(int fd, http_request *req);
void send_response(int fd, http_response *resp);

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

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (clientfd < 0) {
        perror("accept");
        return -1;
    }
    char client_ip[16];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    http_request req = {0};
    http_response resp;

    int error_code = parse_http_headers(clientfd, &req);

    if(error_code == 0)
    {
        resp = handle_request(&req);
    }
    else
    {
        resp = generate_error(&req, error_code);
    }

    printf("%s - %s %s - %d\n",
           client_ip, pretty_method(req.method), req.path, resp.status_code);

    send_response(clientfd, &resp);

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
                if(parse_before_header(req, line) < 0) return 400;
            }
            else
            {
                if(parse_header(req, line) < 0) return 400;
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


void send_response(int fd, http_response *resp){

    send(fd, "HTTP/1.1 ", 9, 0);

    // Send status code
    int code = resp->status_code;
    char buf[3];
    buf[0] = (char)(48 + code/100); code -= code/100*100;
    buf[1] = (char)(48 + code/10);
    buf[2] = (char)(48 + (code%10));
    send(fd, buf, 3, 0);

    // Send status text
    send(fd, " ", 1, 0);
    char* status_text = get_status_text(resp->status_code);
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