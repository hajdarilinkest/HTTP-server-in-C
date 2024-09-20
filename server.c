#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define RESPONSE_TEMPLATE "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n%s"

// reads the index.html file from the current folder
char* read_html_file(const char *filename) 
{
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("couldn't open file");
        return NULL;
    }

    // move the file pointer to the end and get the length
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // allocate memory for the file content and read it in
    char *content = malloc(length + 1);
    if (content) {
        fread(content, 1, length, file);
        content[length] = '\0';  // null-terminate the string
    }

    fclose(file);
    return content;
}
// handles the client request
void handle_client(int client_socket) 
{
    char buffer[BUFFER_SIZE] = {0};
    char response[BUFFER_SIZE];

    // read whatever the client sent
    read(client_socket, buffer, BUFFER_SIZE);
    printf("got request:\n%s\n", buffer);

    // read the index.html file
    char *response_body = read_html_file("index.html");
    if (response_body == NULL) {
        response_body = "<html><body><h1>Error: File not found</h1></body></html>";
    }

    // build the http response with the content
    int response_length = snprintf(response, BUFFER_SIZE, RESPONSE_TEMPLATE, strlen(response_body), response_body);

    // send the response to the client
    send(client_socket, response, response_length, 0);
    printf("response sent:\n%s\n", response);

    free(response_body);  // free the memory after sending the response

    // close the connection with the client
    close(client_socket);
}

// sets up the server stuff
int setup_server(struct sockaddr_in *address) 
{
    int server_fd;
    int opt = 1;

    // make the socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // allow port reuse so we don't get stuck if we restart
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) 
    {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // set up the server address
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    // bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)address, sizeof(*address)) < 0) 
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // start listening for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("server's up and running on port %d...\n", PORT);
    return server_fd;
}

int main() 
{
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // get the server ready to go
    server_fd = setup_server(&address);

    // infinite loop to handle clients
    while (1) 
    {
        printf("waiting for someone to connect...\n");

        // accept a new connection
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) 
        {
            perror("accept failed");
            continue;  // move on if there's an error
        }

        // deal with the connected client
        handle_client(client_socket);
    }

    // close the server (not likely to get here)
    close(server_fd);

    return 0;
}