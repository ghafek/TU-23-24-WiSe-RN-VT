#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include "uthash.h" //Open Source Hash Tables Library. Used to store the Dynamic PUT Requests
#include <pthread.h> //For threading different clients

#define MAX_SIZE 8192  //Maximum amount of bits that we can save in the Buffer, and the content from Dynamic PUT
#define MAX_RESOURCES 100 //Maximum number of resources that we can have in the server
#define MAX_RESPONSE 16384 //Maximum bits for the responses that we send

const int server_running = 1; //When 1, the server is always running and listening


//The struct that is used to save the dynamic payloads
struct packet_s {
    char path[MAX_SIZE]; //used to store the path from the URI
    char content[MAX_SIZE]; //used to store the content in the path
    UT_hash_handle hh; //used for the hashtables
};

//Initializing the structs
struct packet_s *packets = NULL; // The head of the Hash Table
struct packet_s packet[MAX_RESOURCES]; //the actual structs where data is stored



/*
* Function to store the static content in our Struct, used for GET
* HASH_ADD_STR is from the library uthash
*/
void initialize_static_responses() {
    struct packet_s *s; //intitialize a help struct first

    // Add static responses
    s = &packet[0];
    strcpy(s->path, "/static/foo"); //first copy path in help struct
    strcpy(s->content, "Foo"); //Copy content in help struct
    HASH_ADD_STR(packets, path, s); //copy everything back to the main struct

    s = &packet[1];
    strcpy(s->path, "/static/baz");
    strcpy(s->content, "Baz");
    HASH_ADD_STR(packets, path, s);

    s = &packet[2];
    strcpy(s->path, "/static/bar");
    strcpy(s->content, "Bar");
    HASH_ADD_STR(packets, path, s);
}

/*
 * For Requests that are not PUT/GET/DELETE a 501 reply will be sent
*/
void handle_other_request(int new_fd, char *path) {
    int send_value = send(new_fd, "HTTP/1.1 501\r\n\r\n", strlen("HTTP/1.1 501\r\n\r\n"), 0);
    if (send_value == -1) {
        printf("Error sending 501 response\n");
    } else {
        printf("501 Response sent");
    }
}

/*
* If the first line is not there, if the method is not there, if the URI is not there or if the HTTP version is not there, then 400 BAD Request will be sent
* If The method POST/PATCH/PUT doesn't have the Content-Length in the header, a 400 Bad Request will also be sent
*/
void handle_bad_request(int new_fd) {
    int send_value = send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0);
    if (send_value == -1) {
        printf("Error sending response\n");
    } else {
        printf("400 Bad Request sent");
    }
}

/**
 * This function is called if the method is a GET Request
 * It checks the struct for the path and if its there, it sends back the content
 * If it is not there, it will check if the path was dynamic path and will send a 404 with the path name
 * In other cases if the path is not found, it will send a 404 Not found
 * HASH_FIND_STR is from the uthash library that will find the path.
*/
void handle_get_request(int new_fd, char *path) {
    struct packet_s *s; //help struct
    char response[MAX_RESPONSE] = {0}; //The response that will be sent back to the client
    HASH_FIND_STR(packets, path, s);
    //when if (s) is true, that means we have found the path, and will send the content in response
    if (s) {
        snprintf(response, MAX_RESPONSE, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n%s", strlen(s->content), s->content);
        int send_value = send(new_fd, response, strlen(response), 0);
        if (send_value == -1) {
            printf("Error sending 200 OK response\n");
        } else {
            printf("200 OK Response sent");
        }

    } else {
        if (strncmp(path, "/dynamic/", strlen("/dynamic/")) == 0) {
            char *p_content = path + strlen("/dynamic/");
            snprintf(response, MAX_RESPONSE, "HTTP/1.1 404 Not Found\r\nContent-Length: %ld\r\n\r\n%s", strlen(p_content), p_content);
            int send_value = send(new_fd, response, strlen(response), 0);
            if (send_value == -1) {
                printf("Error sending 404 response\n");
            } else {
                printf("404 Response sent");
            }
        }
        else {
            int send_value = send(new_fd, "HTTP/1.1 404 Not Found\r\n\r\n", strlen("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
            if (send_value == -1) {
                printf("Error sending 404 response\n");
            } else {
                printf("404 Response sent");
            }
        }
    }
}

/*
 * This function is called when the method is PUT.
 * request needs and has Content-Length
 * Content-Length must be a positive natural number
 * content_length_position = position of the first character from "Content-Length"
 * content_length_string = size of Content-Length as string
 * length = size of Content-Length as integer
 * atoi(): converts string to int
 * The function checks if the content length is there, if not sends a 400. If the content-length is there, it will -
 *  - search for the content using the functions from uthash library, if the path is found, it will overwrite it, if not -
 * - found it will create a new path with the content, again with the help of uthash library
 * - It also sends a forbidden 403 request if the path isn't dynamic
*/
void handle_put_request(int new_fd, char *path, char *buf) {

    char temp2[MAX_SIZE]; //help variable used to strstr for payload end
    memset(temp2, 0, sizeof(temp2));
    strcpy(temp2, buf);


    char *content_length = strstr(buf, "Content-Length: "); //strstr searches for Content-Length: in header
    if (content_length == NULL) {
        handle_bad_request(new_fd); //if not there send 400
        return;
    }
    else {
        content_length += strlen("Content-Length: ");
        content_length = strtok(content_length, "\r\n");
        if (content_length != NULL) {
            int length = atoi(content_length);
            if (length <= 0) {
                handle_bad_request(new_fd);
                return;
            }
            else {
                //the path must start with dynamic
                if (strncmp(path, "/dynamic/", strlen("/dynamic/")) == 0) {

                    //used to store the path after /dynamic
                    char *p_content = path + strlen("/dynamic/");

                    //used to store the payload after the header
                    char *payload_start = strstr(temp2, "\r\n\r\n");
                    if (payload_start == NULL) {
                        handle_bad_request(new_fd);
                        return;
                    }

                    payload_start += strlen("\r\n\r\n");

                    char *payload = payload_start;

                    struct packet_s *s; //help struct
                    HASH_FIND_STR(packets, path, s);
                    if (s) {
                        // If the path already exists, update the content
                        strcpy(s->content, payload);
                        char response[MAX_RESPONSE] = {0};
                        snprintf(response, MAX_RESPONSE, "HTTP/1.1 204 No Content\r\nContent-Length: %ld\r\n\r\n%s", strlen(s->content), s->content);
                        int send_value = send(new_fd, response, strlen(response), 0);
                        if (send_value == -1) {
                            printf("Error sending 204 response\n");
                        } else {
                            printf("204 Response sent");
                        }
                    } else {
                        // If the path doesn't exist, add a new entry, create a new struct and the copy everything to the main struct in the end.
                        struct packet_s *new_packet = malloc(sizeof(struct packet_s));
                        strncpy(new_packet->path, path, strlen(path));
                        strncpy(new_packet->content, payload, strlen(payload));
                        HASH_ADD_STR(packets, path, new_packet);
                        char response[MAX_RESPONSE] = {0};
                        snprintf(response, MAX_RESPONSE, "HTTP/1.1 201 Created\r\nContent-Length: %ld\r\n\r\n%s", strlen(new_packet->content), new_packet->content);
                        int send_value = send(new_fd, response, strlen(response), 0);
                        if (send_value == -1) {
                            printf("Error sending 201 response\n");
                        } else {
                            printf("201 Response sent");
                        }
                    }


                }
                else {
                    //send forbidden if the path isn't dynamic
                    int send_value = send(new_fd, "HTTP/1.1 403 Forbidden\r\n\r\n", strlen("HTTP/1.1 403 Forbidden\r\n\r\n"), 0);
                    if (send_value == -1) {
                        printf("Error sending 403 response\n");
                    } else {
                        printf("403 Response sent");
                    }
                }

            }

        }

    }
}

/*
 * This function is called when the method is POST or PATCH
 * It checks if it has a content Length, if not, it will send a 400 BAD Request
 * If it has a Content-Length, it will send a 501 Request.
*/
void handle_post_patch_request(int new_fd, char *path, char *buf) {
    char *content_length = strstr(buf, "Content-Length: "); //strstr searches for Content-Length: in header
    if (content_length == NULL) {
        handle_bad_request(new_fd); //if not there send 400
        return;
    }
    else {
        content_length += strlen("Content-Length: ");
        content_length = strtok(content_length, "\r\n");
        if (content_length != NULL) {
            int length = atoi(content_length);
            if (length <= 0) {
                handle_bad_request(new_fd);
                return;
            }
            else {
                handle_other_request(new_fd, path);
            }
        }
    }
}

/*
 * This Function is called when a DELETE request is received
 * It will find the path using uthash, and then delete the content and send a response if the deletion was successful
 * If the path isn't in the struct, it will return 404 not found
*/
void handle_delete_request(int new_fd, char *path) {
    struct packet_s *s; //help struct
    HASH_FIND_STR(packets, path, s);
    if (s) {
        HASH_DEL(packets, s);
        free(s);
        char response[MAX_RESPONSE] = {0};
        snprintf(response, MAX_RESPONSE, "HTTP/1.1 204 No Content\r\n\r\n%s", s->content);
        int send_value = send(new_fd, response, strlen(response), 0);
        if (send_value == -1) {
            printf("Error sending 204 response\n");
        } else {
            printf("204 Response sent");
        }
    } else {
        int send_value = send(new_fd, "HTTP/1.1 404 Not Found\r\n\r\n", strlen("HTTP/1.1 404 Not Found\r\n\r\n"), 0);
        if (send_value == -1) {
            printf("Error sending 404 response\n");
        } else {
            printf("404 Response sent");
        }
    }
}

/*
 * This Function is called when we receive a complete packet, it will determine if the header is correct, if not, it will send a 400 Bad Request
 * It will then check what the method is and call the responsible function
*/
void handle_request(int new_fd, char *buf) {
    /**
     * Help variable to store the content of buf, and used to tokenize the path, method, and version from the Header
    */
    char *temp = strdup(buf);
    //memset(temp2, 0, sizeof(temp2));

    char *method = strtok(temp, " ");
    char *path = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");

    //If method, path, or version is not there, send 400 BAD Request
    if (method == NULL || path == NULL || version == NULL) {
        handle_bad_request(new_fd);
        return;
    }

    if (strcmp(method, "GET") == 0) {
        handle_get_request(new_fd, path);
    }
    else if (strcmp(method, "PUT") == 0) {
        handle_put_request(new_fd, path, buf);
    }
    else if (strcmp(method, "DELETE") == 0) {
        handle_delete_request(new_fd, path);
    }
    else if(strcmp(method, "POST") == 0 || strcmp(method, "PATCH") == 0) {
        handle_post_patch_request(new_fd, path, buf);
    }
    else {
        handle_other_request(new_fd, path);
    }
}

/*
* This method will be called when a client has been accepted
* It will keep receiving until we have a complete packet that ends with /r/n/r/n
* When we receive the whole packet it will call the function handle requests
*/
void *handle_client(void *arg) {
    char buf[MAX_SIZE] = {0}; //The buffer that stores the requests from client
    int new_fd = *(int *)arg; //the client socket ID
    int receive_value; //Number of bits that we receive
    char *temp_ptr; //The ptr that will be used to find the end of the packet "/r/n/r/n"

    //the server will keep receiving until the client disconnects, or isn't sending requests anymore
    while(server_running) {
        receive_value = recv(new_fd, buf + strlen(buf), MAX_SIZE - strlen(buf) - 1, 0);

        if (receive_value <= 0) {
            break;
        }

        buf[strlen(buf) + receive_value] = '\0'; //null termination of buffer

        //The loop that will keep receiving until the end of packet is there
        while ((strstr(buf, "\r\n\r\n")) != NULL) {
            char *temp_ptr = strstr(buf, "\r\n\r\n") + 4;

            handle_request(new_fd, buf);

            memset(buf, 0, sizeof(buf)); //empty the buffer at the end with the request is processed
        }
    }

    if (receive_value == 0) {
        printf("Client disconnected\n");
    } else if (receive_value == -1) {
        printf("Receive error");
    }

    close(new_fd); //close the socket for the client when everything has been processed
    free(arg); //free the malloced space for the client socket

    return NULL;
}


/**
     * Create the address structures with the IP and Port
     * ai_family = AF_UNSPEC for IPv4 and IPv6
     * ai_socktype = SOCK_STREAM for TCP
     * ai_flags = AI_PASSIVE for bind()
 */

int create_server_socket(const char *IP, const char *PORT) {
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Use AF_INET for IPv4, AF_INET6 for IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(IP, PORT, &hints, &res) != 0) {
        printf("getaddrinfo error\n");
        return -1;
    }

    /*
    * Iterate through the address structures and bind to the first one we can
    * Socket = Create a socket with the address structure
    * Setsockopt = Make the socket reusable
    * Bind = Bind the socket to the IP and Port
    * sockfd = Socket File Descriptor (Host Socket)
    * yes = Make the socket reusable
    * p = Address structure to bind to (First one we can)
    */

    int sockfd;
    int yes = 1;

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) {
            printf("Socket creation failure \n");
            continue;
        }
        else
        {
            printf("Socket creation success \n");
        }

        // Make the socket reusable
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            printf("Socket reusability failure \n");
            close(sockfd);
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            printf("Bind Error");
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    return sockfd;
}

int main (int argc, char** argv) {

    //Check the number of arguments(webserver <host> <port>)
    if (argc != 3) {
        printf("Please enter the correct number of Arguments \n");
        exit(1);
    }

    /**
     * For-loop to initialize each packet in the 'packet' array.
     * The loop sets the first character of the 'path' and 'content' fields
     * of each packet to the null character '\0', effectively making them empty strings.
     * packets = strut array of chars
     * MAX_RESOURCES        = int 100
     * packet[].path[]      = chars parsed path
     * packet[].content[]   = chars payload of the request
     *
     **/
    for (int i = 0; i < MAX_RESOURCES; i++) {
        packet[i].path[0] = '\0';
        packet[i].content[0] = '\0';
    }
    /**
     * Extracting the IP address and port number from the command line arguments.
     * IP   = char IP address from the command line argument.
     * PORT = char Port number from the command line argument.
     **/
    const char *IP = argv[1];
    const char *PORT = argv[2];

    // Creating a server socket
    int sockfd = create_server_socket(IP, PORT);

    // Initialize static responses, in case path is static
    initialize_static_responses();

    // Server main loop
    while (server_running) {

        // Listen for incoming connections, with a maximum of 10 pending connections
        if (listen(sockfd, 10) == -1) {
            printf("Listen error");
            close(sockfd);
            exit(1);
        } else {
            printf("Listen successful \n");
        }

        // Client address structure
        struct sockaddr_storage client_addr;

        // Size of the client address structure
        socklen_t addr_size = sizeof client_addr;

        // Dynamically allocate memory for new file descriptor
        int *new_fd = malloc(sizeof(int));

        // Accept incoming connection, store client's address, and create a new socket file descriptor
        *new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);

        if (*new_fd == -1){
            printf("New socket creation error \n");
        }
        // Thread variable
        pthread_t thread;

        // Create a new thread to handle the client request
        pthread_create(&thread, NULL, handle_client, new_fd);

        // Detach the thread so that it cleans up after itself
        pthread_detach(thread);
    }
    //Return 0 to indicate successful execution
    return 0;
}