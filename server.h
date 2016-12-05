#ifndef _SERVER_H

#define _SERVER_H

#include<stdio.h>
#include<sys/types.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<string.h>

#define SERVER_PORT 8000

struct RequestArg
{
    char *name;
    char *value;
    struct RequestArg *next;
};

int init();
void accept_request(void *arg);
int get_line(int client_sock,char *buf,int size);
void not_found(int client_sock);
void site_index(int client_sock);
struct RequestArg *get_request_arg(char *url,int index);
void static_file(int client_sock,char *path,char *filetype);
char *get_filetype(char *path);
void free_memory(struct RequestArg *request_arg);


#endif
