//
// Created by Nyloner on 2016/12/12.
//

#include "server.h"

void init_server()
{
    struct sockaddr_in name;
    struct sockaddr_in remote_client;
    unsigned int client_len;
    int opt = 1;

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        printf("Error\n");
        exit(-1);
    }
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //绑定IP和端口
    name.sin_family = AF_INET;
    name.sin_port = htons(SERVER_PORT);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr *) &name, sizeof(name)) == -1)
    {
        printf("Bind Error !\n");
        exit(-1);
    }

    pool = init_thread_pool(20);
    pthread_create(&keepalive_pid, NULL, (void *) thread_pool_keepalive, (void *) pool);
    //开始监听
    if (listen(server_socket, 5) == -1)
    {
        printf("Listen Error !\n");
        exit(-1);
    }

    log_f = fopen("log.output", "a");
    signal(SIGINT, stop_server);

    printf("Starting server at http://127.0.0.1:%d/ \nQuit the server with CONTROL-C.\n", SERVER_PORT);

    fd_Setnonblocki
    while (1)
    {
        int client_sock = accept(server_socket, (struct sockaddr *) &remote_client, &client_len);
        if (client_sock != -1)
        {
            struct SocketArg *arg = (struct SocketArg *) malloc(sizeof(struct SocketArg));
            arg->client_sock = client_sock;
            arg->ip_address = inet_ntoa(remote_client.sin_addr);
            push_thread_worker(pool, parser_request, (void *) arg);
        }
    }
}

void stop_server()
{
    char *loc_time = local_time();
    printf("\n%s Stop!\n", loc_time);
    destroy_thread_pool(pool);
    pthread_cancel(keepalive_pid);
    free(loc_time);
    fclose(log_f);
    exit(0);
}

void *parser_request(void *arg)
{
    struct SocketArg *socket_arg = (struct SocketArg *) arg;
    int client = socket_arg->client_sock;
    char buf[1024];
    char url[255];
    int read_size;
    struct HttpRequest *request = (struct HttpRequest *) malloc(sizeof(struct HttpRequest));
    request->request_arg = NULL;
    request->post_arg = NULL;
    request->client = client;

    read_size = get_line(client, buf, sizeof(buf));

    char *loc_time = local_time();
    char log_string[255];
    sprintf(log_string, "%s\t%s\t%s\n", loc_time, socket_arg->ip_address, buf);
    printf("%s", log_string);
    server_log(log_string);
    free(socket_arg);

    //Get method
    int index = 0;
    while (buf[index] != ' ' && index < read_size)
    {
        request->method[index] = buf[index];
        index++;
    }
    request->method[index] = '\0';

    //Get url
    index++;
    int url_i = 0;
    while (buf[index] != ' ' && index < read_size && url_i < sizeof(url))
    {
        url[url_i] = buf[index];
        url_i++;
        index++;
    }
    url[url_i] = '\0';

    //Get Path
    index = 0;
    while (url[index] != '\0' && url[index] != '?')
    {
        request->path[index] = url[index];
        index++;
    }
    request->path[index] = '\0';

    if (url[index] == '?')
    {
        request->request_arg = get_request_arg(url, index + 1);
    }

    request->headers = get_headers(client);

    if (!strcmp(request->method, "POST"))
    {
        char *content_length = get_value(request->headers, "Content-Length");
        int length = atoi(content_length);
        char *content_type = get_value(request->headers, "Content-Type");
        if (!strcmp(content_type, "application/x-www-form-urlencoded"))
        {
            request->post_arg = get_post_arg(request->client, length);
        }
    }
    accept_request(request);
    close(request->client);
    return NULL;
}

struct KeyValue *get_post_arg(int client, int length)
{
    char *post_string = (char *) malloc(sizeof(char) * (length + 2));
    struct KeyValue *post_arg = NULL;
    char c;
    int index;
    for (index = 0; index < length; ++index)
    {
        ssize_t num = recv(client, &c, 1, 0);
        if (num > 0)
        {
            post_string[index] = c;
        } else
        {
            break;
        }
    }
    post_string[index] = '\0';
    post_arg = get_request_arg(post_string, 0);
    free(post_string);
    return post_arg;
}

char *get_value(struct KeyValue *p, char *key)
{
    while (p->name != NULL)
    {
        if (!strcmp(p->name, key))
            return p->value;
        p = p->next;
    }
    return NULL;
}

struct KeyValue *get_headers(int client)
{
    char line[255];
    int read_size = 0;
    struct KeyValue *header = (struct KeyValue *) malloc(sizeof(struct KeyValue));
    header->name = NULL;
    header->next = NULL;
    header->value = NULL;
    struct KeyValue *tail = header;
    while ((read_size = get_line(client, line, sizeof(line))) > 0)
    {
        char name[80];
        char value[255];
        int index = 0;
        while (line[index] != ':')
        {
            name[index] = line[index];
            index++;
        }
        name[index] = '\0';
        index += 2;

        int i = 0;
        while (line[index] != '\0')
        {
            value[i] = line[index];
            i++;
            index++;
        }
        value[i] = '\0';

        tail->name = (char *) malloc(sizeof(char) * strlen(name));
        strcpy(tail->name, name);
        tail->value = (char *) malloc(sizeof(char) * strlen(value));
        strcpy(tail->value, value);

        tail->next = (struct KeyValue *) malloc(sizeof(struct KeyValue));
        tail = tail->next;
        tail->name = NULL;
        tail->value = NULL;
        tail->next = NULL;
    }
    if (header->name == NULL)
    {
        free_memory(header);
        return NULL;
    }
    return header;
}

int get_line(int client, char *buf, int size)
{
    char c = '\0';
    int read_size = 0;
    while (read_size < size - 1 && c != '\n')
    {
        ssize_t num = recv(client, &c, 1, 0);
        if (num > 0)
        {
            if (c == '\r')
            {
                num = recv(client, &c, 1, MSG_PEEK);
                if (num > 0 && c == '\n')
                    recv(client, &c, 1, 0);
                else
                    c = '\n';
            }
            if (c != '\n')
            {
                buf[read_size] = c;
                read_size++;
            }
        } else
        {
            c = '\n';
        }
    }
    buf[read_size] = '\0';
    return read_size;
}

struct KeyValue *get_request_arg(char *string, int index)
{
    struct KeyValue *request_arg = (struct KeyValue *) malloc(sizeof(struct KeyValue));
    struct KeyValue *tail = request_arg;
    tail->next = NULL;
    tail->name = NULL;
    tail->value = NULL;
    size_t length = strlen(string);

    while (index < length)
    {
        char name[255];
        char value[255];
        int i = 0;
        while (index < length && string[index] != '=')
        {
            name[i] = string[index];
            i++;
            index++;
        }
        name[i] = '\0';
        if (index >= length)
            break;
        i = 0;
        index++;
        while (index < length && string[index] != '&')
        {
            value[i] = string[index];
            index++;
            i++;
        }
        value[i] = '\0';

        if (strcmp(value, "") == 0)
            continue;
        tail->name = (char *) malloc(sizeof(char) * strlen(name));
        strcpy(tail->name, name);

        tail->value = (char *) malloc(sizeof(char) * strlen(value));
        strcpy(tail->value, value);

        tail->next = (struct KeyValue *) malloc(sizeof(struct KeyValue));
        tail = tail->next;
        tail->value = NULL;
        tail->next = NULL;
        tail->name = NULL;
        index++;
    }
    if (request_arg->name == NULL)
    {
        free_memory(request_arg);
        return NULL;
    }
    return request_arg;
}

void free_memory(struct KeyValue *p)
{
    if (p == NULL)
        return;
    free_memory(p->next);
    if (p->name != NULL)
        free(p->name);
    if (p->value != NULL)
        free(p->value);
    free(p);
}

void server_log(char *string)
{
    fprintf(log_f, "%s", string);
}

char *local_time()
{
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);

    char *loc_time = (char *) malloc(sizeof(char) * 20);
    sprintf(loc_time, "%04d-%02d-%02d %02d:%02d:%02d", 1900 + p->tm_year, p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min,
            p->tm_sec);
    return loc_time;
}
