//
// Created by Nyloner on 2016/12/12.
//

#ifndef HTTPSERVER_SERVER_H
#define HTTPSERVER_SERVER_H

#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>

#define SERVER_PORT 8000

struct KeyValue
{
    char *name;
    char *value;
    struct KeyValue *next;
};

struct Request
{
    int client;
    char method[5];
    char path[80];
    struct KeyValue *post_arg;
    struct KeyValue *request_arg;
    struct KeyValue *headers;
};

struct SocketArg
{
    intptr_t client_sock;
    char * ip_address;
};

FILE *log_f;//log

void init_server();
void parser_request(void *arg);
int get_line(int client,char *buf,int size);
struct KeyValue * get_headers(int client);
struct KeyValue * get_request_arg(char *url,int index);
struct KeyValue * get_headers(int client);
char * get_value(struct KeyValue *p,char *key);
struct KeyValue * get_post_arg(int client,int length);
void free_memory(struct KeyValue *p);
void server_log(char *string);
void stop_server();
char * local_time();

void accept_request(struct Request *request);

#endif //HTTPSERVER_SERVER_H
