//
// Created by Nyloner on 2016/12/12.
//

#include "views.h"


void accept_request(struct HttpRequest *request)
{
    //GET请求
    if (!strcmp(request->method, "GET"))
    {
        char *filetype = get_filetype(request->path);
        if (!strcmp(request->path, "/"))
        {
            site_index(request->client);
        } else if (!strcmp(filetype, ""))
        {
            not_found(request->client);
        } else
        {
            static_file(request->client, request->path, filetype);
        }
    }

    //POST请求
    if (!strcmp(request->method, "POST"))
    {
        if (!strcmp(request->path, "/login"))
        {
            login(request->client, request->post_arg);
        } else
        {
            not_found(request->client);
        }
    }
    free_memory(request->headers);
    free_memory(request->request_arg);
    free(request);
}

void response_headers(int client, int type, struct KeyValue *header)
{
    char buf[255];
    char *loc_time = local_time();

    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Server: NyServer/0.1.0\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Date: %s\r\n", loc_time);
    free(loc_time);
    send(client, buf, strlen(buf), 0);
    if (header != NULL)
    {
        struct KeyValue *p = header;
        while (p != NULL)
        {
            sprintf(buf, "%s: %s\r\n", p->name, p->value);
            send(client, buf, strlen(buf), 0);
            p = p->next;
        }
    }
    switch (type)
    {
        case 1:
            sprintf(buf, "Content-Type: text/html;charset=utf-8\r\n");
            send(client, buf, strlen(buf), 0);
            break;
        case 2:
            sprintf(buf, "Content-Type: image/jpeg\r\n");
            send(client, buf, strlen(buf), 0);
            break;
        case 3:
            sprintf(buf, "Content-Type: application/javascript\r\n");
            send(client, buf, strlen(buf), 0);
            break;
        case 4:
            sprintf(buf, "Content-Type: text/css\r\n");
            send(client, buf, strlen(buf), 0);
            break;
        default:
            break;
    }
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

void response_file(int client, char *filepath, int type, struct KeyValue *header)
{
    FILE *fp;
    char filename[255];
    sprintf(filename, "www%s", filepath);
    if ((fp = fopen(filename, "r")) == NULL)
    {
        not_found(client);
        return;
    }
    response_headers(client, type, header);
    size_t read_num;
    char buf[1024];
    while ((read_num = fread(buf, 1, 1024, fp)) > 0)
    {
        send(client, buf, read_num, 0);
    }
    fclose(fp);
}

void site_index(int client)
{
    char buf[1024];
    response_headers(client, 1, NULL);
    FILE *fp;
    if ((fp = fopen("www/index.html", "r")) == NULL)
    {
        sprintf(buf, "<HTML><TITLE>Welcome</TITLE>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>Welcome to my site!</P>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
        return;
    }
    while (fgets(buf, 1024, fp) != NULL)
    {
        send(client, buf, strlen(buf), 0);
    }
    fclose(fp);
}

void not_found(int client)
{
    char buf[1024];
    response_headers(client, 1, NULL);
    FILE *fp;
    if ((fp = fopen("www/404.html", "r")) == NULL)
    {
        sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>404 Not Found</P>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
        return;
    }
    while (fgets(buf, 1024, fp) != NULL)
    {
        send(client, buf, strlen(buf), 0);
    }
    fclose(fp);
}

char *get_filetype(char *path)
{
    char *filetype = (char *) malloc(sizeof(char) * 20);
    size_t index = strlen(path) - 1;
    while (index > 0 && path[index] != '.')
    {
        index--;
    }

    if (index == 0)
    {
        strcpy(filetype, "");
    } else
    {
        index++;
        int i = 0;
        while (path[index] != '\0')
        {
            filetype[i] = path[index];
            index++;
            i++;
        }
        filetype[i] = '\0';
    }
    return filetype;
}

void static_file(int client, char *path, char *filetype)
{
    if (!strcmp(filetype, "png") || !strcmp(filetype, "jpg") || !strcmp(filetype, "jpeg") || !strcmp(filetype, "ico"))
    {
        response_file(client, path, 2, NULL);
    } else if (!strcmp(filetype, "js"))
    {
        response_file(client, path, 3, NULL);
    } else if (!strcmp(filetype, "css"))
    {
        response_file(client, path, 4, NULL);
    } else
    {
        response_file(client, path, 1, NULL);
    }
}

void login(int client, struct KeyValue *post_arg)
{
    char *username = get_value(post_arg, "username");
    char *password = get_value(post_arg, "password");

    if (username == NULL || password == NULL)
    {
        site_index(client);
        return;
    }
    if (!strcmp(username, "nyserver") && !strcmp(password, "nyserver"))
    {
        struct KeyValue *p = (struct KeyValue *) malloc(sizeof(struct KeyValue));
        p->name = (char *) malloc(sizeof(char) * strlen("Set-Cookie") + 1);
        strcpy(p->name, "Set-Cookie");
        char *cookie = set_cookie();
        p->value = cookie;
        p->next = NULL;

        response_file(client, "/user.html", 1, p);
        free_memory(p);
    } else
    {
        response_file(client, "/login.html", 1, NULL);
    }
}

char *set_cookie()
{
    char *cookie = (char *) malloc(sizeof(char) * 255);
    char *loc_time = local_time();

    sprintf(cookie, "username=nyserver; sessionid=123456; date=%s", loc_time);
    return cookie;
}

void print_key_value(struct KeyValue *head)
{
    struct KeyValue *p = head;
    while (p != NULL)
    {
        if (p->name != NULL)
        {
            printf("%s  %s\n", p->name, p->value);
        }
        p = p->next;
    }
}
