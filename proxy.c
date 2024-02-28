#include "csapp.h"

/*** MACRO DECLARATION ***/
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/*** FUNCTION DECLARATION ***/
void doit(int connect_fd);
void generate_http_hdr(char *http_hdr, char *hostname, char *path, int port, rio_t *client_rio);
void parse_uri(char *uri, char *hostname, char *path, int *port);
int connect_end_server(char *hostname, char *http_header, int port);

// [concurrency]
void *thread(void *vargp);

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

// argv[0] = executable ; argv[1] = port_no
int main(int argc, char **argv) {
    int listen_fd;
    // int connect_fd;
    int *connect_fd_ptr; // [concurrency]
    char hostname[MAXLINE];
    char port[MAXLINE];
    struct sockaddr_storage client_address;
    socklen_t client_buffer_size;
    
    // [concurrency]
    pthread_t tid;
    
    if(argc != 2){
        fprintf(stderr,"usage :%s <port> \n",argv[0]);
        exit(1);
    }

    listen_fd = Open_listenfd(argv[1]);
    // everytime client calls -> generate new socket
    while(1) {
        // [concurrency]
        client_buffer_size = sizeof(client_address);
        connect_fd_ptr = Malloc(sizeof(int));
        *connect_fd_ptr = Accept(listen_fd, (SA *)&client_address, &client_buffer_size);

        Getnameinfo((SA *)&client_address, client_buffer_size, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

        Pthread_create(&tid, NULL, thread, connect_fd_ptr); 

        // [legacy] sequential
        // client_buffer_size = sizeof(client_address);
        // connect_fd = Accept(listen_fd,(SA *)&client_address, &client_buffer_size);
        // Getnameinfo((SA*)&client_address, client_buffer_size, hostname, MAXLINE, port, MAXLINE, 0);
        // printf("Accepted connection from (%s %s).\n", hostname, port);
        // doit(connect_fd);
        // Close(connect_fd);
    }

    return 0;
}

void doit(int connect_fd) {
    int port;
    int end_server_fd;
    char buffer[MAXLINE];
    char method[MAXLINE];
    char uri[MAXLINE];
    char version[MAXLINE];
    char end_server_http_hdr[MAXLINE];
    char hostname[MAXLINE];
    char path[MAXLINE];
    rio_t client_rio;
    rio_t server_rio;

    Rio_readinitb(&client_rio, connect_fd);
    Rio_readlineb(&client_rio, buffer, MAXLINE);
    sscanf(buffer, "%s %s %s", method, uri, version);

    if(strcasecmp(method, "GET")){
        printf("Proxy does not implement the method");
        return;
    }

    parse_uri(uri, hostname, path, &port);
    generate_http_hdr(end_server_http_hdr, hostname, path, port, &client_rio);

    end_server_fd = connect_end_server(hostname, end_server_http_hdr, port);
    if(end_server_fd < 0){
        printf("connection failed\n");
        return;
    }

    Rio_readinitb(&server_rio, end_server_fd);
    Rio_writen(end_server_fd, end_server_http_hdr, strlen(end_server_http_hdr));

    size_t n;
    while((n = Rio_readlineb(&server_rio,buffer,MAXLINE)) != 0) {
        printf("proxy received %d bytes\n", n);
        Rio_writen(connect_fd, buffer, n);
    }
    Close(end_server_fd);
}

void generate_http_hdr(char *http_header, char *hostname, char *path, int port, rio_t *client_rio) {
    char buffer[MAXLINE];
    char request_hdr[MAXLINE];
    char host_hdr[MAXLINE];
    char other_hdr[MAXLINE];

    sprintf(request_hdr, "GET %s HTTP/1.0\r\n", path);

    while(Rio_readlineb(client_rio, buffer, MAXLINE) > 0) {
        if(strcmp(buffer, "\r\n")==0) 
            break;

        if(!strncasecmp(buffer, "Host", 4)) {
            strcpy(host_hdr, buffer);
            continue;
        }

        if(!strncasecmp(buffer, "Connection", 10)
         &&!strncasecmp(buffer, "Proxy-Connection", 16)
         &&!strncasecmp(buffer, "Host", 4))
            strcat(other_hdr, buffer);
    }

    if(strlen(host_hdr) == 0)
        sprintf(host_hdr, "Host: %s\r\n", hostname);

    sprintf(http_header, "%s%s%s%s%s%s%s",
            request_hdr, host_hdr, "Connection: close\r\n", 
            "Proxy_connection: close\r\n", user_agent_hdr, other_hdr, "\r\n");

    return ;
}

void parse_uri(char *uri, char *hostname, char *path, int *port) {
    *port = 7942;                           // default port value
    char *ip_ptr = strstr(uri, "//");       // start after '//' (https:// [start here])

    if(ip_ptr != NULL)
      ip_ptr += 2;                          // since '//' is 2 characters
    else
      ip_ptr = uri;

    char *port_ptr = strstr(ip_ptr, ":");   // [ip]:[port; start here]

    if(port_ptr != NULL) {
        *port_ptr = '\0';
        sscanf(ip_ptr, "%s", hostname);
        sscanf(port_ptr + 1, "%d%s", port, path);
    } else {
        port_ptr = strstr(ip_ptr, "/");

        if(port_ptr != NULL) {
            *port_ptr = '\0';
            sscanf(ip_ptr, "%s", hostname);
            *port_ptr = '/';
            sscanf(port_ptr, "%s", path);
        } else
            sscanf(ip_ptr, "%s", hostname);
    }
    return;
}

int connect_end_server(char *hostname, char *http_header, int port) {
    char port_value[100];

    sprintf(port_value, "%d", port);
    
    return Open_clientfd(hostname, port_value);
}

// [concurrency]
void *thread(void *vargp){
    int connect_fd = *((int *)vargp);

    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connect_fd);
    Close(connect_fd);

    return NULL;
}
