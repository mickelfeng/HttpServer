#include "server.h"


int init()
{
    struct sockaddr_in name;
    struct sockaddr_in remote_client;
    unsigned int client_len;

    int server_socket=socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket==-1)
    {
        printf("Error\n");
        exit(-1);
    }

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

int get_line(int client_sock,char *buf,int size)
{
    char c='0';
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
            buf[read_size]=c;
            read_size++;
        }
        else
        {
            c='\n';
        }
    }
    buf[read_size] = '\0';
    return read_size;
}

struct RequestArg *get_request_arg(char *url,int index)
{
    struct RequestArg * request_arg=(struct RequestArg*)malloc(sizeof(struct RequestArg));
    struct RequestArg *tail=request_arg;
    tail->next=NULL;
    tail->name=NULL;

    while(url[index]!='\0')
    {
        index++;
        char name[255];
        char value[255];
        int i=0;
        while(url[index]!='='&&url[index]!='\0')
        {
            name[i]=url[index];
            i++;
            index++;
        }
        name[i]='\0';

        i=0;
        index++;
        while(url[index]!='\0'&&url[index]!='&')
        {
            value[i]=url[index];
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

        tail->next=(struct RequestArg*)malloc(sizeof(struct RequestArg));
        tail=tail->next;
        tail->next=NULL;
        tail->name=NULL;
    }
    if(request_arg->name==NULL)
    {
        free(request_arg);
        return NULL;
    }
    return request_arg;
}

void accept_request(void *arg)
{
    int client=(intptr_t)arg;
    char buf[1024];
    char method[255];
    char path[255];
    char url[255];
    struct RequestArg * request_arg=NULL;
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
        request_arg=get_request_arg(url,index);
    }

    //GET请求
    if(!strcmp(method,"GET"))
    {
        if(!strcmp(path,"/")||!strcmp(path,"/index.html"))
        {
            site_index(client);
        }
        else
        {
            static_file(client,path);
        }
    }

    //POST请求
    if(!strcmp(method,"POST"))
    {

    }
    close(client);
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

void static_file(int client,char *path)
{
    char filetype[20];
    char buf[1024];
    int index=strlen(path)-1;
    while(index>0&&path[index]!='.')
    {
        index--;
    }
    if(index==0)
    {
        strcpy(filetype,"text");
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
    if(!strcmp(filetype,"png")||!strcmp(filetype,"jpg")||!strcmp(filetype,"jpeg"))
    {
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
        sprintf(buf,"Accept-Ranges:bytes\r\n");
        send(client, buf, strlen(buf), 0);
        sprintf(buf, "Content-Type: image/jpeg\r\n");
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
        while(fgets(buf,1024,fp)!=NULL)
        {
            send(client,buf,strlen(buf),0);
        }
        fclose(fp);
    }
}
