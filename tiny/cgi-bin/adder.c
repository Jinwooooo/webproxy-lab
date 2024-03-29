// adder.c - CGI practice
#include "csapp.h"

int main(int argc, char *argv[]) {
    char *buffer;             // in order to retrieve QUERY STRING from client
    char *ptr;                // a naive tokenizer
    char arg_1[MAXLINE];
    char arg_2[MAXLINE];
    char content[MAXLINE];
    long value_1 = 0;
    long value_2 = 0;

    // retrieving argument to compute addition
    if((buffer = getenv("QUERY_STRING")) != NULL) {
        ptr = strchr(buffer, '&');
        *ptr = '\0';

        strcpy(arg_1, buffer);
        strcpy(arg_2, ptr + 1);

        value_1 = strtol(arg_1, NULL, 10);
        value_2 = strtol(arg_2, NULL, 10);
    }

    // RESPONSE : BODY
    sprintf(content, "%sTiny ADDER CGI\r\n<p>", content);
    sprintf(content, "%sInput Values : %ld and %ld\r\n<p>", content, value_1, value_2);
    sprintf(content, "%sAdded Value : %ld\r\n<p>", content, value_1 + value_2);

    // RESPONSE : HTTP
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
