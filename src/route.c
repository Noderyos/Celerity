#include "route.h"

struct {
    http_method method;
    char *path;
    handle_t func;
} routes[256];
size_t route_count = 0;

void register_route(http_method method, char *path, handle_t func)
{
    routes[route_count].method = method;
    routes[route_count].path = path;
    routes[route_count++].func = func;
}

void register_get(char *path, handle_t func) {
    register_route(HTTP_GET, path, func);
}
void register_post(char *path, handle_t func)
{
    register_route(HTTP_POST, path, func);
}
void register_put(char *path, handle_t func)
{
    register_route(HTTP_PUT, path, func);
}
void register_delete(char *path, handle_t func)
{
    register_route(HTTP_DELETE, path, func);
}
void register_patch(char *path, handle_t func)
{
    register_route(HTTP_PATCH, path, func);
}
void register_head(char *path, handle_t func)
{
    register_route(HTTP_HEAD, path, func);
}
void register_options(char *path, handle_t func)
{
    register_route(HTTP_OPTIONS, path, func);
}

void register_trace(char *path, handle_t func)
{
    register_route(HTTP_TRACE, path, func);
}

handle_t get_handle(http_request *req){
    for (size_t i = 0; i < route_count; ++i) {
        if(routes[i].method == req->method && strcmp(routes[i].path, req->path) == 0)
            return routes[i].func;
    }
    return NULL;
}

void handle_client(int fd, struct sockaddr_in client_addr)
{
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    http_request req = {0};
    http_response resp;

    int error_code = parse_http_headers(fd, &req);

    if(error_code == 0){
        handle_t handle = get_handle(&req);
        if(!handle) resp = generate_error(&req, 404);
        else resp = handle(&req);
    }
    else {
        resp = generate_error(&req, error_code);
    }

    printf("%s - %s %s - %d\n",
           client_ip, pretty_method(req.method), req.path, resp.status_code);

    send_response(fd, &resp);

    close(fd);
}