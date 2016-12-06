#include "server.h"


int init()
{
    struct sockaddr_in name;
    struct sockaddr_in remote_client;
    unsigned int client_len;
    int opt=1;

    int server_socket=socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket==-1)
    {
        printf("Error\n");
        exit(-1);
    }
    setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    //绑定IP和端口
    name.sin_family = AF_INET;
    name.sin_port = htons(SERVER_PORT);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(server_socket,(struct sockaddr *)&name, sizeof(name))==-1)
    {
        printf("Bind Error !\n");
        exit(-1);
    }

    //开始监听
    if(listen(server_socket, 5)==-1)
    {
        printf("Listen Error !\n");
        exit(-1);
    }

    while(1)
    {
        int client_sock=accept(server_socket,(struct sockaddr *)&remote_client,&client_len);
        if(client_sock!=-1)
        {
            pthread_t client;
            pthread_create(&client,NULL,(void *)accept_request,(void *)(intptr_t)client_sock);
        }
    }
    return 0;
}


void accept_request(void *arg)
{
    int client=(intptr_t)arg;
    char buf[1024];
    char method[255];
    char path[255];
    char url[255];
    struct KeyValue * request_arg=NULL;
    struct KeyValue * headers=NULL;
    int read_size;

    read_size=get_line(client,buf, sizeof(buf));

    //Get method
    int index=0;
    while(buf[index]!=' '&&index<read_size)
    {
        method[index]=buf[index];
        index++;
    }
    method[index]='\0';

    //Get url
    index++;
    int url_i=0;
    while(buf[index]!=' '&&index<read_size&&url_i<sizeof(url))
    {
        url[url_i]=buf[index];
        url_i++;
        index++;
    }
    url[url_i]='\0';

    //Get Path
    index=0;
    while(url[index]!='\0'&&url[index]!='?')
    {
        path[index]=url[index];
        index++;
    }
    path[index]='\0';

    if(url[index]=='?')
    {
        request_arg=get_request_arg(url,index+1);
    }
    headers=get_headers(client);
    print_key_value(headers);
    print_key_value(request_arg);
    //GET请求
    if(!strcmp(method,"GET"))
    {
        char *filetype=get_filetype(path);
        if(!strcmp(path,"/"))
        {
            site_index(client);
        }
        else if(!strcmp(filetype,""))
        {
            not_found(client);
        }
        else
        {
            static_file(client,path,filetype);
        }
    }

    //POST请求
    if(!strcmp(method,"POST"))
    {
        char *content_length=get_value(headers,"Content-Length");
        int length=atoi(content_length);
        char *content_type=get_value(headers,"Content-Type");
        if(!strcmp(content_type,"application/x-www-form-urlencoded"))
        {
            struct KeyValue *post_arg=get_post_arg(client,length);
            print_key_value(post_arg);
            free_memory(post_arg);
            site_index(client);
        }
        else
        {
            not_found(client);
        }
    }
    free_memory(headers);
    free_memory(request_arg);
    close(client);
}

void print_key_value(struct KeyValue *head)
{
    struct KeyValue *p=head;
    while(p!=NULL)
    {
        if(p->name!=NULL)
        {
            printf("%s  %s\n",p->name,p->value);
        }
        p=p->next;
    }
}
struct KeyValue* get_post_arg(int client,int length)
{
    char *post_string=(char *)malloc(sizeof(char)*(length+2));
    struct KeyValue *post_arg=NULL;
    char c;
    int index;
    for(index=0;index<length;++index)
    {
        int num=recv(client,&c,1,0);
        if(num>0)
        {
            post_string[index]=c;
        }
        else
        {
            break;
        }
    }
    post_string[index]='\0';
    post_arg=get_request_arg(post_string,0);
    free(post_string);
    return post_arg;
}

char * get_value(struct KeyValue *p,char *key)
{
    while(p->name!=NULL)
    {
        if(!strcmp(p->name,key))
            return p->value;
        p=p->next;
    }
    return NULL;
}
struct KeyValue* get_headers(int client_sock)
{
    char line[255];
    int read_size=0;
    struct KeyValue *header=(struct KeyValue*)malloc(sizeof(struct KeyValue));
    header->name=NULL;
    header->next=NULL;
    header->value=NULL;
    struct KeyValue *tail=header;
    while((read_size=get_line(client_sock,line,sizeof(line)))>0)
    {
        char name[80];
        char value[255];
        int index=0;
        while(line[index]!=':')
        {
            name[index]=line[index];
            index++;
        }
        name[index]='\0';
        index+=2;

        int i=0;
        while(line[index]!='\0')
        {
            value[i]=line[index];
            i++;
            index++;
        }
        value[i]='\0';

        tail->name=(char *)malloc(sizeof(char)*strlen(name));
        strcpy(tail->name,name);
        tail->value=(char *)malloc(sizeof(char)*strlen(value));
        strcpy(tail->value,value);

        tail->next=(struct KeyValue*)malloc(sizeof(struct KeyValue));
        tail=tail->next;
        tail->name=NULL;
        tail->value=NULL;
        tail->next=NULL;
    }
    if(header->name==NULL)
    {
        free_memory(header);
        return NULL;
    }
    return header;
}

void free_memory(struct KeyValue *p)
{
    if(p==NULL)
        return;
    free_memory(p->next);
    if(p->name!=NULL)
        free(p->name);
    if(p->value!=NULL)
        free(p->value);
    free(p);
}

int get_line(int client_sock,char *buf,int size)
{
    char c='\0';
    int read_size=0;
    while(read_size<size-1&&c!='\n')
    {
        int num=recv(client_sock,&c,1,0);
        if(num>0)
        {
            if(c=='\r')
            {
                num=recv(client_sock,&c,1,MSG_PEEK);
                if(num>0&&c=='\n')
                    recv(client_sock,&c,1,0);
                else
                    c='\n';
            }
            if(c!='\n')
            {
                buf[read_size]=c;
                read_size++;
            }
        }
        else
        {
            c='\n';
        }
    }
    buf[read_size]='\0';
    return read_size;
}

struct KeyValue *get_request_arg(char *string,int index)
{
    struct KeyValue * request_arg=(struct KeyValue*)malloc(sizeof(struct KeyValue));
    struct KeyValue *tail=request_arg;
    tail->next=NULL;
    tail->name=NULL;
    tail->value=NULL;
    int length=strlen(string);

    while(index<length)
    {
        char name[255];
        char value[255];
        int i=0;
        while(index<length&&string[index]!='=')
        {
            name[i]=string[index];
            i++;
            index++;
        }
        name[i]='\0';
        if(index>=length)
            break;
        i=0;
        index++;
        while(index<length&&string[index]!='&')
        {
            value[i]=string[index];
            index++;
            i++;
        }
        value[i]='\0';

        if(strcmp(value,"")==0)
            continue;
        tail->name=(char *)malloc(sizeof(char)*strlen(name));
        strcpy(tail->name,name);

        tail->value=(char *)malloc(sizeof(char)*strlen(value));
        strcpy(tail->value,value);

        tail->next=(struct KeyValue*)malloc(sizeof(struct KeyValue));
        tail=tail->next;
        tail->value=NULL;
        tail->next=NULL;
        tail->name=NULL;
        index++;
    }
    if(request_arg->name==NULL)
    {
        free_memory(request_arg);
        return NULL;
    }
    return request_arg;
}

void site_index(int client)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.1 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Server: NyServer/0.1.0\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html;charset=utf-8\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    FILE *fp;
    if((fp=fopen("www/index.html","r"))==NULL)
    {
        sprintf(buf, "<HTML><TITLE>Welcome</TITLE>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>Welcome to my site!</P>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
        return;
    }
    while(fgets(buf,1024,fp)!=NULL)
    {
        send(client,buf,strlen(buf),0);
    }
    fclose(fp);
}

void not_found(int client)
{
    char buf[1024];
    sprintf(buf, "HTTP/1.1 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Server: NyServer/0.1.0\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html;charset=utf-8\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    FILE *fp;
    if((fp=fopen("www/404.html","r"))==NULL)
    {
        sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "<BODY><P>404 Not Found</P>\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "</BODY></HTML>\r\n");
        send(client, buf, strlen(buf), 0);
        return;
    }
    while(fgets(buf,1024,fp)!=NULL)
    {
        send(client,buf,strlen(buf),0);
    }
    fclose(fp);
}

char * get_filetype(char *path)
{
    char *filetype=(char *)malloc(sizeof(char)*20);
    int index=strlen(path)-1;
    while(index>0&&path[index]!='.')
    {
        index--;
    }
    if(index==0)
    {
        strcpy(filetype,"");
    }
    else
    {
        index++;
        int i=0;
        while(path[index]!='\0')
        {
            filetype[i]=path[index];
            index++;
            i++;
        }
        filetype[i]='\0';
    }
    return filetype;
}

void static_file(int client,char *path,char *filetype)
{
    char buf[1024];
    if(!strcmp(filetype,"png")||!strcmp(filetype,"jpg")||!strcmp(filetype,"jpeg"))
    {
        free(filetype);
        FILE *fp;
        char filename[255];
        sprintf(filename, "www%s",path);
        if((fp=fopen(filename,"r"))==NULL)
        {
            not_found(client);
            return;
        }
        sprintf(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Server: NyServer/0.1.0\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: image/jpeg\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        int read_num;
        while((read_num=fread(buf,1,1024,fp))>0)
        {
            send(client,buf,read_num,0);
        }
        fclose(fp);
    }
    else
    {
        FILE *fp;
        char filename[255];
        sprintf(filename, "www%s",path);
        if((fp=fopen(filename,"r"))==NULL)
        {
            not_found(client);
            free(filetype);
            return;
        }
        sprintf(buf, "HTTP/1.1 200 OK\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Server: NyServer/0.1.0\r\n");
        send(client, buf, strlen(buf), 0);
        if(!strcmp(filetype,"js"))
        {
            sprintf(buf, "Content-Type: application/javascript\r\n");
            send(client, buf, strlen(buf), 0);
        }
        else if(!strcmp(filetype,"css"))
        {
            sprintf(buf, "Content-Type: text/css\r\n");
            send(client, buf, strlen(buf), 0);
        }
        else
        {
            sprintf(buf, "Content-Type: text/html\r\n");
            send(client, buf, strlen(buf), 0);
        }
        sprintf(buf, "\r\n");
        send(client, buf, strlen(buf), 0);
        while(fgets(buf,1024,fp)!=NULL)
        {
            send(client,buf,strlen(buf),0);
        }
        fclose(fp);
        free(filetype);
    }
}
