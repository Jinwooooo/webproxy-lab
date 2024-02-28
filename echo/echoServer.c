#include "csapp.h"

/***** FUNCTION DECLARATION *****/
void echo(int connect_fd);
void *thread(void *vargp); // [concurrency]

int main(int argc, char **argv) {
    int listen_fd;
    // int connect_fd;
    int *connect_fd_ptr; // [concurrency]
    char client_hostname[MAXLINE];
    char client_port[MAXLINE];
    struct sockaddr_storage client_address;
    socklen_t client_buffer_size;

    // [concurrency]
    pthread_t thread_id;

    // 1 = src executable ; 2 = port
    if (argc != 2) {
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
	    exit(0);
    }

    listen_fd = Open_listenfd(argv[1]);

    while(1) {
        client_buffer_size = sizeof(client_address);
        connect_fd_ptr = Malloc(sizeof(int));
        *connect_fd_ptr = Accept(listen_fd, (SA *)&client_address, &client_buffer_size);

        Getnameinfo((SA *)&client_address, client_buffer_size, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", client_hostname, client_port);

        Pthread_create(&thread_id, NULL, thread, connect_fd_ptr); 

        // [legacy] sequential
	    // client_buffer_size = sizeof(struct sockaddr_storage); 
	    // connect_fd = Accept(listen_fd, (SA *)&client_address, &client_buffer_size);
        // Getnameinfo((SA *) &client_address, client_buffer_size, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        // printf("Connected to (%s, %s)\n", client_hostname, client_port);
	    // echo(connect_fd);
	    // Close(connect_fd);
    }
    exit(0);
}

void echo(int connect_fd) {
    size_t no_bytes_read;
    char buffer[MAXLINE]; 
    rio_t rio;

    Rio_readinitb(&rio, connect_fd);

    while((no_bytes_read = Rio_readlineb(&rio, buffer, MAXLINE)) != 0) {
        printf("server received %d bytes\n", (int)no_bytes_read);
        Rio_writen(connect_fd, buffer, no_bytes_read);
    }
}

// [concurrency]
void *thread(void *vargp){
    int connect_fd = *((int *)vargp);

    Pthread_detach(pthread_self());
    Free(vargp);
    echo(connect_fd);
    Close(connect_fd);

    return NULL;
}