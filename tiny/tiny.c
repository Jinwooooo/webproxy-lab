#include "csapp.h"

/***** FUNCTION DECLARATION *****/
void doit(int fd);
void read_request_hdr(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgi_args);
void get_file_type(char *filename, char *file_type);
// [11.11] support http head method
// void serve_static(int fd, char *filename, int file_size);
void serve_static(int fd, char *filename, int file_size, char *method);
// void serve_dynamic(int fd, char *filename, char *cgi_args);
void serve_dynamic(int fd, char *filename, char *cgi_args, char *method);
void client_error(int fd, char *cause, char *err_no, char *short_msg, char *long_msg);
void *thread(void *vargp); // [concurrency]

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
    pthread_t thread_id;

    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
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

        Pthread_create(&thread_id, NULL, thread, connect_fd_ptr);   

        // [legacy] sequential
        // client_buffer_size = sizeof(client_address);
        // connect_fd = Accept(listen_fd, (SA *)&client_address, &client_buffer_size);
        // Getnameinfo((SA *)&client_address, client_buffer_size, hostname, MAXLINE, port, MAXLINE, 0);
        // printf("Accepted connection from (%s, %s)\n", hostname, port);
        // doit(connect_fd);
        // Close(connect_fd);
    }
}

void doit(int fd) {
    int is_static;
    struct stat static_buffer;
    char buffer[MAXLINE];
    char method[MAXLINE];
    char uri[MAXLINE];
    char version[MAXLINE];
    char filename[MAXLINE];
    char cgi_args[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    if(!Rio_readlineb(&rio, buffer, MAXLINE))
        return;
    printf("%s", buffer);
    sscanf(buffer, "%s %s %s", method, uri, version);
    // Tiny will only allow GET and HEAD methods
    // [11.11] support http head method
    if(!(strcasecmp(method, "GET") == 0 || strcasecmp(method, "HEAD") == 0)) {
        client_error(fd, method, "501", "Not Implemented", "Tiny does not implement this method");
        return;
    }
    read_request_hdr(&rio);

    is_static = parse_uri(uri, filename, cgi_args);
    if(stat(filename, &static_buffer) < 0) {
        client_error(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if(is_static) {      
        if(!(S_ISREG(static_buffer.st_mode)) || !(S_IRUSR & static_buffer.st_mode)) {
            client_error(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, static_buffer.st_size, method);
    } else {
        if(!(S_ISREG(static_buffer.st_mode)) || !(S_IXUSR & static_buffer.st_mode)) {
          client_error(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
          return;
        }
        serve_dynamic(fd, filename, cgi_args, method);
    }
}

void read_request_hdr(rio_t *rio_ptr) {
    char buffer[MAXLINE];

    Rio_readlineb(rio_ptr, buffer, MAXLINE);
    printf("%s", buffer);
    while(strcmp(buffer, "\r\n")) {
        Rio_readlineb(rio_ptr, buffer, MAXLINE);
        printf("%s", buffer);
    }
    return;
}

int parse_uri(char *uri, char *filename, char *cgi_args) {
    char *ptr;

    if(!strstr(uri, "cgi-bin")) {
        strcpy(cgi_args, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        if(uri[strlen(uri) - 1] == '/')
            strcat(filename, "home.html");
        
        return 1;
    } else {
        ptr = index(uri, '?');
        if (ptr) {
            strcpy(cgi_args, ptr + 1);
            *ptr = '\0';
        } else 
            strcpy(cgi_args, "");
        strcpy(filename, ".");
        strcat(filename, uri);
        
        return 0;
    }
}

void get_file_type(char *filename, char *file_type) {
    if(strstr(filename, ".html"))
        strcpy(file_type, "text/html");
    else if(strstr(filename, ".gif"))
        strcpy(file_type, "image/gif");
    else if(strstr(filename, ".png"))
        strcpy(file_type, "image/png");
    else if(strstr(filename, ".jpg"))
        strcpy(file_type, "image/jpeg");
    // [11.7 ; MOD] extend to include MPG video file
    else if(strstr(filename, ".mp4"))
        strcpy(file_type, "video/mp4");
    else
        strcpy(file_type, "text/plain");
} 

void serve_static(int fd, char *filename, int file_size, char *method) {
    int src_fd;
    char *src_ptr;
    char file_type[MAXLINE];
    char buffer[MAXBUF];

    get_file_type(filename, file_type);
    sprintf(buffer, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "Content-length: %d\r\n", file_size);
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "Content-type: %s\r\n\r\n", file_type);
    Rio_writen(fd, buffer, strlen(buffer));

    if(strcasecmp(method, "HEAD") == 0)
        return;

    src_fd = Open(filename, O_RDONLY, 0);
    // [11.9] copy requested file to fd with malloc instead of mmap
    // src_ptr = Mmap(0, file_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    src_ptr = (char*)Malloc(file_size);
    Rio_readn(src_fd, src_ptr, file_size);
    Close(src_fd);
    Rio_writen(fd, src_ptr, file_size);
    // Munmap(src_ptr, file_size);
    free(src_ptr);
}

void serve_dynamic(int fd, char *filename, char *cgi_args, char *method) {
    char buffer[MAXLINE], *empty_list[] = { NULL };

    sprintf(buffer, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buffer, strlen(buffer));
  
    if(Fork() == 0) {                           // generate child and run if generated
        setenv("QUERY_STRING", cgi_args, 1);
        setenv("REQUEST_METHOD", method, 1);
        Dup2(fd, STDOUT_FILENO);                // get CGI output -> server socket -> client 
        Execve(filename, empty_list, environ);  // run CGI
    }
    Wait(NULL);                                 // wait until reap ded child
}

void client_error(int fd, char *cause, char *err_no, char *short_msg, char *long_msg) {
    char buffer[MAXLINE];

    sprintf(buffer, "HTTP/1.0 %s %s\r\n", err_no, short_msg);
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buffer, strlen(buffer));
 
    sprintf(buffer, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "%s: %s\r\n", err_no, short_msg);
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "<p>%s: %s\r\n", long_msg, cause);
    Rio_writen(fd, buffer, strlen(buffer));
    sprintf(buffer, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buffer, strlen(buffer));
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

