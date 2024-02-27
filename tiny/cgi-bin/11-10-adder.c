// [11.10] using HTML form to link cgi and compute result
#include "csapp.h"

int main(int argc, char *argv[]) {
    char *buffer;             // in order to retrieve QUERY STRING from client
    char *ptr;                // a naive tokenizer
    char content[MAXLINE];
    long value_1 = 0;
    long value_2 = 0;

    // retrieving argument to compute addition
    if((buffer = getenv("QUERY_STRING")) != NULL) {
        ptr = strchr(buffer, '&');
        *ptr = '\0';

        sscanf(buffer, "value_1=%ld", &value_1);
        sscanf(ptr + 1, "value_2=%ld", &value_2);
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
