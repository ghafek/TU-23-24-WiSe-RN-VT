#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char **argv) {

    //Check the number of arguments(webserver <host> <port>)
    if (argc != 3) {
        printf("Please enter the correct number of Arguments \n");
        exit(1);
    }

    /** IP = Save the IP Address and Port from the Command Line Arguments
    PORT = Check if Port is passed as a number format **/
    const char *IP = argv[1];
    const char *PORT = argv[2];

    // //Check if Port is correctly passed as an Argument
    // if (PORT < 1024 || PORT > 65535)
    // {
    //     printf("Please enter a valid Port number \n");
    //     exit(1);
    // }

    /**
     * Create the address structures with the IP and Port
     * ai_family = AF_UNSPEC for IPv4 and IPv6
     * ai_socktype = SOCK_STREAM for TCP
     * ai_flags = AI_PASSIVE for bind()
    */
    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // Use AF_INET for IPv4, AF_INET6 for IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(IP, PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(1);
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
        else
        {
            printf("Socket reusability success \n");
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }

    // Free the address info (not needed anymore)
    freeaddrinfo(res);

    // Check if we binded
    if (p == NULL) {
        printf("Failed to bind\n");
        exit(1);
    }
    else
    {
        printf("Bind successful\n");
    }


    /**
     * listen(): listen for incoming connections
     * listen(host socket, 10): 10 set for max. number of connections in queue
     */
    if (listen(sockfd, 10) == -1) {
        printf("Listen error");
        close(sockfd);
        exit(1);
    } else {
        printf("Listen successful \n");
    }

    // Accept incoming connections
    while (1) {

        /**
         * their_addr = connector's address information (client)
         * addr_size = size of struct sockaddr_storage
        */
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof(their_addr);

        /**
         * accept(): accept an incoming connection
        */
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

        if (new_fd == -1){
            printf("New socket creation error \n");
        }

        /**
         * variable to store received messages
         * temp = messages from client
         * buf = combined content from client (requests to be processed)
         * paket_end = identifier for the end of request
         * temp2 = copy of buf for content length
         */
        char buf[1024];
        char temp[1024];
        char packet[1024];
        char* content[100];
        memset(content, 0, sizeof(content));
        char* paket_end = "\r\n\r\n";
        memset(buf, 0, sizeof(buf));

        /**
         * receive until disconnected
         *
         * recv(): receive messages from client and store the content in temp
         *         receive_value = output of recv = size of message from client
         *         gives -1 back when there is an error
         *         gives 0 back when client disconnects from the host socket
         *
         * recv(client socket, message target, message length, 0): always set flags (last parameter) to 0
         *
         * strcat(): use to concatenate the messages from client to the buf variable so it can be processed
         */

        while (1) {

            memset(temp, 0, sizeof(temp));

            int receive_value = recv(new_fd, temp, sizeof(temp), 0);

            if (receive_value == -1) {
                printf("Receive error\n");
                close(new_fd);
                break;

            } else if (receive_value == 0) {
                printf("Client disconnected\n");
                close(new_fd);
                break;
            }

            //Print Temp
            printf("temp: %s\n", temp);

            strcat(buf, temp); //Add the content of temp to buf
            strcpy(packet, buf); //Copy buf to temp2

            //Print Buffer for testing
            printf("buffer: %s\n", buf);

            /**
             * reiterate the loop until all packets are processed
             * send a message for each processed packet
             *
             * strstr(): use to search for a particular substring from a string
             *           gives the pointer of the first character of the substring if found
             *           gives NULL pointer back otherwise
             *
             *           if a substring is found, a full packet is received and can be processed
             *
             * send(): sends a message to the client socket
             *         gives -1 back if there is and error
             *         send "400 Bad Request" for every received packet
             *
             * send(client socket, message, message length, 0): always set flags (last parameter) to 0
             *
             * pos = pointer to end position of the processed packet
             * strcpy(): use to remove the processed packet from the buf variable,
             *           store only the messages that is not processed.
             *
             * start_line = first line of an HTTP request, contains method, URI, and HTTP_version
             * method = GET, POST, PUT, DELETE, PATCH, HEAD
             *
             *
             * Structure of the packet processing:
             *
             * while: client tries to send requests
             *      check if start_line exists, if no: 400 Bad Request
             *      check if method, URI, HTTP_Version exists, if no: 400 Bad Request
             *      check if method is valid, if no: 400 Bad Request
             *      check if method that needs Content-Length has one, if no: 400 Bad Request
             *          else check if Content-Length is filled, if no: 400 Bad Request
             *              check if Content-Length has a positive natural number, if no: 400 Bad Request
             *              501 Other Request
             *      check if method is a GET method, if yes:
             *          check if URI is /static/bar, if yes: 200, Bar
             *          check if URI is /static/foo, if yes: 200, Foo
             *          check if URI is /static/baz, if yes: 200, Baz
             *          else normal GET method, 404 GET Request
             *      else method is either DELETE, HEAD, 501 Other Request
             *
             */

            while (strstr(buf, paket_end) != NULL) {

                char *pos = strstr(buf, paket_end) + strlen(paket_end);

                char *start_line = strtok(buf, "\r\n");
                char *method, *URI, *HTTP_version;

                //Check if the request is valid
                if (start_line == NULL) {
                    if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                        printf("Sending message error\n");
                        close(new_fd);
                        break;
                    } else
                        printf("400 Bad Request sent\n");
                }

                if (start_line != NULL) {
                    method = strtok(start_line, " ");
                    URI = strtok(NULL, " ");
                    HTTP_version = strtok(NULL, " ");

                    if (method == NULL || URI == NULL || HTTP_version == NULL) {

                        //Request has either no method, URI, or HTTP version
                        if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                            printf("Sending message error\n");
                            close(new_fd);
                            break;
                        } else
                            printf("400 Bad Request sent\n");

                    } else {
                        if (strcmp(method, "GET") == 0) {
                            //method is GET
                            if (strcmp(URI, "/static/bar") == 0)
                            {
                                //send the content in /static/bar
                                if (send(new_fd, "HTTP/1.1 200 Bar Request\r\nContent-Type: text/plain\r\n\r\nBar", strlen("HTTP/1.1 200 Bar Request\r\nContent-Type: text/plain\r\n\r\nBar"), 0) == -1) {
                                    printf("Sending message error\n");
                                    close(new_fd);
                                    break;
                                } else
                                    printf("HTTP/1.1 200 Bar Request sent\n");
                                close(new_fd);
                                break;
                            }
                            else if (strcmp(URI, "/static/foo") == 0)
                            {
                                //send the content in /static/foo
                                if (send(new_fd, "HTTP/1.1 200 Foo Request\r\nContent-Type: text/plain\r\n\r\nFoo", strlen("HTTP/1.1 200 Foo Request\r\nContent-Type: text/plain\r\n\r\nFoo"), 0) == -1) {
                                    printf("Sending message error\n");
                                    close(new_fd);
                                    break;
                                } else
                                    printf("HTTP/1.1 200 Foo Request sent\n");
                                close(new_fd);
                                break;
                            }
                            else if (strcmp(URI, "/static/baz") == 0)
                            {
                                //send the content in /static/baz
                                if (send(new_fd, "HTTP/1.1 200 Baz Request\r\nContent-Type: text/plain\r\n\r\nBaz", strlen("HTTP/1.1 200 Baz Request\r\nContent-Type: text/plain\r\n\r\nBaz"), 0) == -1) {
                                    printf("Sending message error\n");
                                    close(new_fd);
                                    break;
                                } else
                                    printf("HTTP/1.1 200 Baz Request sent\n");
                                close(new_fd);
                                break;
                            }

                            else {
                                //normal GET request
                                if (strncmp(URI, "/dynamic/", strlen("/dynamic/")) == 0) {
                                    char *element = strtok(URI, "/");
                                    char *next_element;
                                    while ((next_element = strtok(NULL, "/")) != NULL) {
                                        element = next_element;
                                    }
                                    int content_found = 0;
                                    for (int i = 0; i < 100; i++) {
                                        if (content[i] != NULL && strcmp(content[i], element) == 0) {
                                            content_found = 1;
                                            char response[128];
                                            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n%s", content[i]);
                                            if (send(new_fd, response, strlen(response), 0) == -1) {
                                                printf("Sending message error\n");
                                                close(new_fd);
                                                break;
                                            } else {
                                                printf("200 OK Request sent\n");
                                                close(new_fd);
                                                break;
                                            }
                                        }
                                    }
                                    if (!content_found) {
                                        char response[128];
                                        sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n%s", element);
                                        if (send(new_fd, response, strlen(response), 0) == -1) {
                                            printf("Sending message error\n");
                                            close(new_fd);
                                            break;
                                        } else {
                                            printf("404 Not Found Request sent\n");
                                            close(new_fd);
                                            break;
                                        }
                                    }
                                }
                                else {
                                    //normal GET request
                                    if (send(new_fd, "HTTP/1.1 404 Not Found\r\n\r\n", strlen("HTTP/1.1 404 Not Found\r\n\r\n"), 0) == -1) {
                                        printf("Sending message error\n");
                                        close(new_fd);
                                        break;
                                    } else
                                        printf("404 Not Found sent\n");
                                }
                            }
                        }

                        else if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "PATCH") == 0) {

                            if (strstr(packet, "Content-Length: ") == NULL) {
                                //request needs Content-Length but doesn't have one
                                if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                                    printf("Sending message error\n");
                                    close(new_fd);
                                    break;
                                } else
                                    printf("400 Bad Request sent\n");
                            }
                            else {
                                /**
                                * request needs and has Content-Length
                                *
                                * Content-Length must be a positive natural number
                                * content_length_position = position of the first character from "Content-Length"
                                * content_length_string = size of Content-Length as string
                                * content_length_size = size of Content-Length as integer
                                *
                                * atoi(): converts string to int
                                */

                                char *content_length_position = strstr(packet, "Content-Length: ");
                                char *content_length_string = strtok(content_length_position + strlen("Content-Length: "), "\r\n");
                                if (content_length_string != NULL) {
                                    int content_length_size = atoi(content_length_string);
                                    if (content_length_size <= 0) {
                                        //content_length_size invalid
                                        if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n",
                                                 strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                                            printf("Sending message error\n");
                                            close(new_fd);
                                            break;
                                        }  else
                                            printf("400 Bad Request sent\n");
                                    } else {
                                        if (strcmp(method, "PUT") == 0)
                                        {
                                            if (strncmp(URI, "/dynamic/", strlen("/dynamic/")) == 0)
                                            {
                                                char* element = strtok(URI, "/");
                                                char* next_element;
                                                while ((next_element = strtok(NULL, "/")) != NULL) {
                                                    element = next_element;
                                                }
                                                int content_found = 0;
                                                for (int i = 0; i < 100; i++) {
                                                    if (content[i] != NULL && strcmp(content[i], element) == 0) {
                                                        content_found = 1;
                                                        int content_length = strlen(content[i]);
                                                        char response[128];
                                                        sprintf(response, "HTTP/1.1 204 No Content\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s\r\n", content_length, content[i]);
                                                        if (send(new_fd, response, strlen(response), 0) == -1) {
                                                            printf("Sending message error\n");
                                                            close(new_fd);
                                                            break;
                                                        } else {
                                                            printf("HTTP/1.1 204 No Content sent\n");
                                                            //close(new_fd);
                                                            //break;
                                                        }
                                                    }
                                                }

                                                if (content_found == 0) {
                                                    for (int i = 0; i < 100; i++) {
                                                        if (content[i] == NULL) {
                                                            content[i] = strdup(element);
                                                            int content_length = strlen(content[i]);
                                                            char response[128];
                                                            sprintf(response, "HTTP/1.1 201 Created\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s\r\n", content_length, content[i]);
                                                            if (send(new_fd, response, strlen(response), 0) == -1) {
                                                                printf("Sending message error\n");
                                                                close(new_fd);
                                                                break;
                                                            } else {
                                                                printf("HTTP/1.1 201 Created sent\n");
                                                                //close(new_fd);
                                                                break;
                                                            }
                                                        }
                                                    }
                                                }

                                            }
                                            else {
                                                if (send(new_fd, "HTTP/1.1 403 Forbidden\r\n\r\n", strlen("HTTP/1.1 403 Forbidden\r\n\r\n"), 0) == -1) {
                                                    printf("Sending message error\n");
                                                    close(new_fd);
                                                    break;
                                                } else
                                                    printf("403 Forbidden Request sent\n");
                                            }

                                        }
                                        else {
                                            if (send(new_fd, "HTTP/1.1 501 Other Request\r\n\r\n", strlen("HTTP/1.1 501 Other Request\r\n\r\n"), 0) == -1) {
                                                printf("Sending message error\n");
                                                close(new_fd);
                                                break;
                                            } else
                                                printf("501 Other Request sent\n");
                                        }

                                    }
                                }
                                else {
                                    //content_length_size invalid
                                    if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                                        printf("Sending message error\n");
                                        close(new_fd);
                                        break;
                                    } else
                                        printf("400 Bad Request sent\n");
                                }

                            }

                        }

                        else if(strcmp(method, "DELETE") == 0) {
                            if (strncmp(URI, "/dynamic/", strlen("/dynamic/")) == 0){
                                char* element = strtok(URI, "/");
                                char* next_element;
                                while ((next_element = strtok(NULL, "/")) != NULL) {
                                    element = next_element;
                                }
                                int content_found = 0;
                                for (int i = 0; i < 100; i++){
                                    if (content[i] != NULL && strcmp(content[i], element) == 0){
                                        content_found = 1;
                                        char response[128];
                                        sprintf(response, "HTTP/1.1 204 No Content\r\nContent-Type: text/plain\r\n\r\n%s\r\n", content[i]);
                                        content[i] = NULL;
                                        if (send(new_fd, response, strlen(response), 0) == -1) {
                                            printf("Sending message error\n");
                                            close(new_fd);
                                            break;
                                        } else {
                                            printf("HTTP/1.1 204 No Content sent\n");
                                            close(new_fd);
                                            break;
                                        }
                                    }
                                }
                                if (content_found == 0){
                                    char response[128];
                                    sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n%s\r\n", element);
                                    if (send(new_fd, response, strlen(response), 0) == -1) {
                                        printf("Sending message error\n");
                                        close(new_fd);
                                        break;
                                    } else {
                                        printf("HTTP/1.1 404 Not Found sent\n");
                                        //close(new_fd);
                                        //break;
                                    }
                                }
                            }
                            else {
                                if (send(new_fd, "HTTP/1.1 403 Forbidden\r\n\r\n", strlen("HTTP/1.1 403 Forbidden\r\n\r\n"), 0) == -1) {
                                    printf("Sending message error\n");
                                    close(new_fd);
                                    break;
                                } else
                                    printf("403 Forbidden Request sent\n");
                            }
                        }

                        else {
                            //request is either a HEAD request
                            if (send(new_fd, "HTTP/1.1 501 Other Request\r\n\r\n", strlen("HTTP/1.1 501 Other Request\r\n\r\n"), 0) == -1) {
                                printf("Sending message error\n");
                                close(new_fd);
                                break;
                            } else
                                printf("501 Other Request sent\n");
                        }
                    }
                }
                //remove the processed packet
                strcpy(buf, pos);
            }
        }

        close(new_fd);
    }

    close(sockfd);
    return 0;
}