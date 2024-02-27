#include "csapp.h"

int main(int argc, char **argv) {
    /***** VARIABLE DECLARATION *****/
    int client_fd;
    char *host;
    char *port;
    char buffer[MAXLINE];
    rio_t rio;

    /***** CHECKING VALID # OF ARGS *****/
    // exits/terminates if input number of argument is not 3
    // 1 = src executable ; 2 = host ; 3 = port
    if (argc != 3) {
	    fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
	    exit(0);
    }
    host = argv[1];
    port = argv[2];

    /***** CONNECT -> RETURN FILE DESCRIPTOR *****/
    client_fd = Open_clientfd(host, port);
    Rio_readinitb(&rio, client_fd);

    /***** LOOP UNTIL EOF OCCURS *****/
    while(Fgets(buffer, MAXLINE, stdin) != NULL) {
	    Rio_writen(client_fd, buffer, strlen(buffer)); // writes into the server with the buffer content
	    Rio_readlineb(&rio, buffer, MAXLINE);          // reads from server thru socket (provided by buffer)
	    Fputs(buffer, stdout);                         // prints the content of buffer to std ouputu
    }
    Close(client_fd);
    exit(0);
}