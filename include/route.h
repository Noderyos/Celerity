#ifndef ROUTE_H
#define ROUTE_H

#include "http.h"

void register_route(http_method method, char *path, handle_t func);

void register_get(char *path, handle_t func);
void register_post(char *path, handle_t func);
void register_put(char *path, handle_t func);
void register_delete(char *path, handle_t func);
void register_patch(char *path, handle_t func);
void register_head(char *path, handle_t func);
void register_options(char *path, handle_t func);
void register_trace(char *path, handle_t func);

handle_t get_handle(http_request *req);

void handle_client(int fd, struct sockaddr_in client_addr);

#endif