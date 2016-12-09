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
#include<unistd.h>
#include<time.h>

#define SERVER_PORT 8000

struct KeyValue
{
    char *name;
    char *value;
    struct KeyValue *next;
};

int init();
void accept_request(void *arg);
int get_line(int client_sock,char *buf,int size);
struct KeyValue* get_headers(int client_sock);
void not_found(int client_sock);
void site_index(int client_sock);
struct KeyValue* get_request_arg(char *url,int index);
void static_file(int client_sock,char *path,char *filetype);
char *get_filetype(char *path);
void free_memory(struct KeyValue *request_arg);
struct KeyValue* get_headers(int client_sock);
char * get_value(struct KeyValue *p,char *key);
struct KeyValue* get_post_arg(int client,int length);
void response_headers(int client,int type);
//type 1-html 2-jpeg 3-js 4-css
void response_file(int client,char *filepath,int type);

void print_key_value(struct KeyValue *p);
#endif
