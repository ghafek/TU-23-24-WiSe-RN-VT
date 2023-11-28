#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
//#include <ws2tcpip.h>


int main (int argc, char* argv[]) {

    /**
     * only Implemented for IPv4
     * preferably use errno, perror when throwing errors, instead of simple printf
     */


    //Check the command line Arguments count (webserver 0.0.0.0 1234)
    if (argc != 3)
    {
        printf("Please enter the correct number of Arguments \n");
        exit(1);
    }

    //Struct for Socket Address (IPv4):
    struct sockaddr_in sa;

    /** IP = Save the IP Address and Port from the Command Line Arguments
        PORT = Check if Port is passed as a number format **/

    const char* IP = argv[1];
    const int PORT = atoi(argv[2]);

    /**
     * fill the sa structure
     * sin_family = IPv4/6 Address Family
     * sin_port = Port in Network Byte Order
     * inet_pton(): converts IP from text to binary form
     */

    memset(&sa, 0, sizeof(sa)); //clear up the struct first
    sa.sin_family = AF_INET; //IPv4 Address Family
    sa.sin_port = htons(PORT); //Port in Network Byte Order
    inet_pton(AF_INET, IP, &(sa.sin_addr)); //IP from text to binary form

    //Check if correct IP Address format was entered (0.0.0.0)
    if (inet_pton(AF_INET, IP, &(sa.sin_addr)) != 1)
    {
        printf("IP Address Format wrong \n");
        exit(1);
    }

    //Check if Port is correctly passed as an Argument
    if (PORT < 1024 || PORT > 65535)
    {
        printf("Please enter a valid Port number \n");
        exit(1);
    }



    /**
     * socket creation
     *
     * sockfd = host socket
     * bind(): binds the socket to the IP and Port
     */

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("Socket creation failure \n");
        exit(1);
    }
    else
    {
        printf("Socket created successfully \n");
    }

    //option to reuse address
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        printf("setting SO_REUSEADDR failed \n");
        close(sockfd);
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        printf("Bind error \n");
        close(sockfd);
        exit(1);
    } else {
        printf("Bind successful \n");
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

    /**
     * save the incoming connections address in the struct
     * their_addr = client address
     * new_fd = client socket
     * accept(): host accepts new connection from client
     */
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    while (1) {
        addr_size = sizeof their_addr;

        //accept new connections
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

        if (new_fd == -1){
            printf("New socket creation error \n");
        }

        /**
         * variable to store received messages
         * temp = messages from client
         * buf = combined content from client (requests to be processed)
         * paket_end = identifier for the end of request
         */

        char temp[1024] = "";
        char buf[1024] = "";
        char packet[1024] = "";
        char* paket_end = "\r\n\r\n";


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

            memset(temp, 0, 1024);
            int receive_value = recv(new_fd, temp, 1024, 0);

            if (receive_value == -1) {
                printf("Receive error\n");
                close(new_fd);
                break;

            } else if (receive_value == 0) {
                printf("Client disconnected\n");
                close(new_fd);
                break;
            }

            strcat(buf, temp); //add the content of temp to buffer
            strcpy(packet, buf);

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
             *      check if method that needs Content-Length has one with a positive natural number, if no: 400 Bad Request
             *      check if method is a GET method, if yes:
             *          check if URI is /static/bar, if yes: 200, Bar
             *          check if URI is /static/foo, if yes: 200, Foo
             *          check if URI is /static/baz, if yes: 200, Baz
             *          else normal GET method, 404 GET Request
             *      else method is either DELETE, HEAD, or POST, PUT, PATCH with a positive Content-Length size, 501 Other Request
             *
             */

            while (strstr(buf, paket_end) != NULL) {

                char *pos = strstr(buf, paket_end) + strlen(paket_end);
                char *start_line = strtok(buf, "\r\n");
                char *method, *URI, *HTTP_version;

                //check if the request is valid
                if (start_line == NULL) {
                    if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                        printf("Sending message error\n");
                        close(new_fd);
                        break;
                    } else
                        printf("400 Bad Request sent\n");
                } else {

                    //store the method, URI, and HTTP_Version of the request
                    method = strtok(start_line, " ");
                    URI = strtok(NULL, " ");
                    HTTP_version = strtok(NULL, " ");

                    if (method == NULL || URI == NULL || HTTP_version == NULL) {

                        //request has either no method, URI, or HTTP_Version
                        if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                            printf("Sending message error\n");
                            close(new_fd);
                            break;
                        } else
                            printf("400 Bad Request sent\n");

                    } else if (strcmp(method, "GET") != 0 && strcmp(method, "POST") != 0 && strcmp(method, "PUT") != 0 && strcmp(method, "DELETE") != 0 && strcmp(method, "PATCH") != 0 && strcmp(method, "HEAD") != 0) {

                        //request method is wrong/undefined
                        if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                            printf("Sending message error\n");
                            close(new_fd);
                            break;
                        } else
                            printf("400 Bad Request sent\n");

                    } else if (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "PATCH") == 0) {

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
                                    } else
                                        printf("400 Bad Request sent\n");
                                } else {
                                    if (send(new_fd, "HTTP/1.1 501 Other Request\r\n\r\n", strlen("HTTP/1.1 501 Other Request\r\n\r\n"), 0) == -1) {
                                        printf("Sending message error\n");
                                        close(new_fd);
                                        break;
                                    } else
                                        printf("501 Other Request sent\n");
                                }
                            }
                            else {
                                //content_length_size invalid
                                if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n",
                                         strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                                    printf("Sending message error\n");
                                    close(new_fd);
                                    break;
                                } else
                                    printf("400 Bad Request sent\n");
                            }

                        }

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
                            else
                            {
                                //normal GET request
                                if (send(new_fd, "HTTP/1.1 404 GET Request\r\n\r\n", strlen("HTTP/1.1 404 GET Request\r\n\r\n"), 0) == -1) {
                                    printf("Sending message error\n");
                                    close(new_fd);
                                    break;
                                } else
                                    printf("404 GET Request sent\n");
                            }
                        } else {
                            //request is either a DELETE or HEAD request, or a POST, PUT, PATCH request with a positive Content-Length size
                            if (send(new_fd, "HTTP/1.1 501 Other Request\r\n\r\n", strlen("HTTP/1.1 501 Other Request\r\n\r\n"), 0) == -1) {
                                printf("Sending message error\n");
                                close(new_fd);
                                break;
                            } else
                                printf("501 Other Request sent\n");
                        }
                    }
                }
                // Remove the processed packet
                strcpy(buf, pos);
            }
        }
    }

    //Close Socket
    close(sockfd);
    return 0;

}