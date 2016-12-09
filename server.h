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
#include<stdarg.h>


#define SERVER_PORT 8000

struct KeyValue
{
    char *name;
    char *value;
    struct KeyValue *next;
};

int init();
void accept_request(void *arg);
int get_line(int client,char *buf,int size);
struct KeyValue* get_headers(int client);
void not_found(int client);
void site_index(int client);
struct KeyValue* get_request_arg(char *url,int index);
void static_file(int client,char *path,char *filetype);
char *get_filetype(char *path);
void free_memory(struct KeyValue *request_arg);
struct KeyValue* get_headers(int client);
char * get_value(struct KeyValue *p,char *key);
struct KeyValue* get_post_arg(int client,int length);
void response_headers(int client,int type,struct KeyValue *header);
//type 1-html 2-jpeg 3-js 4-css
void response_file(int client,char *filepath,int type,struct KeyValue *header);
void login(int client,struct KeyValue *post_arg);

char *set_cookie();

void print_key_value(struct KeyValue *p);
#endif
