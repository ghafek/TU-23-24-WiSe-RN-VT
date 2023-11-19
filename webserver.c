#include <stdio.h>
#include <stdlib.h>
#include <string.h>
<<<<<<< HEAD
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
//#include <errno.h>
#include <arpa/inet.h>
//#include <errno.h>
//#include <netinet/in.h>
#include <unistd.h>

int main (int argc, char* argv[]) {

    ////Only Implemented for IPv4 Till now, Can use getaddrinfo to simult. use for IPv6 and IPv4
    ////Preferably use errno, perror when throwing errors, instead of simple printf.
    

    //Check the command line Arguments count (webserver 0.0.0.0 1234).
=======
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
>>>>>>> origin/chris
    if (argc != 3)
    {
        printf("Please enter the correct number of Arguments \n");
        exit(1);
    }

    //Struct for Socket Address (IPv4):
    struct sockaddr_in sa;

<<<<<<< HEAD
    //Save the IP Address and Port from the Command Line Arguments
    //Check if Port is passed as a number format
    const char* IP = argv[1];
    const int PORT = atoi(argv[2]);
    
     //Fill the sa structure
=======
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

>>>>>>> origin/chris
    memset(&sa, 0, sizeof(sa)); //clear up the struct first
    sa.sin_family = AF_INET; //IPv4 Address Family
    sa.sin_port = htons(PORT); //Port in Network Byte Order
    inet_pton(AF_INET, IP, &(sa.sin_addr)); //IP from text to binary form
<<<<<<< HEAD
    
    //Check if a correct IP Address format was entered (0.0.0.0)
=======

    //Check if correct IP Address format was entered (0.0.0.0)
>>>>>>> origin/chris
    if (inet_pton(AF_INET, IP, &(sa.sin_addr)) != 1)
    {
        printf("IP Address Format wrong \n");
        exit(1);
    }

    //Check if Port is correctly passed as an Argument
<<<<<<< HEAD
    if (PORT < 1024 || PORT > 65535) 
=======
    if (PORT < 1024 || PORT > 65535)
>>>>>>> origin/chris
    {
        printf("Please enter a valid Port number \n");
        exit(1);
    }
<<<<<<< HEAD
    


    //Socket Creation
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) 
=======



    /**
     * socket creation
     *
     * sockfd = host socket
     * bind(): binds the socket to the IP and Port
     */

    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
>>>>>>> origin/chris
    {
        printf("Socket creation failure \n");
        exit(1);
    }
    else
    {
        printf("Socket created successfully \n");
    }

<<<<<<< HEAD
    // Bind the Socket to the IP and Port
=======
>>>>>>> origin/chris
    if (bind(sockfd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        printf("Bind error");
        close(sockfd);
        exit(1);
    } else {
        printf("Bind successful \n");
    }

<<<<<<< HEAD
    // Listen for incoming connections
    // Listen to any number of connections
    if (listen(sockfd, 10) == -1) {  //10 is the number of connections that can Queue
=======

    /**
     * listen(): listen for incoming connections
     * listen(host socket, 10): 10 set for max. number of connections in queue
     */

    if (listen(sockfd, 10) == -1) {
>>>>>>> origin/chris
        printf("Listen error");
        close(sockfd);
        exit(1);
    } else {
        printf("Listen successful \n");
    }

<<<<<<< HEAD
    //Save the incoming connections address in the struct
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    addr_size = sizeof their_addr;

    //accept new connections
    int newfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)
    

    while (1) {

=======
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
             */

            while (strstr(buf, paket_end) != NULL) {

                //Error message for invalid request
                if (send(new_fd, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0) == -1) {
                    printf("Sending message error\n");
                    close(new_fd);
                    break;
                } else printf("Error message sent\n");

//                if (send(new_fd, "Reply\r\n\r\n", strlen("Reply\r\n\r\n"), 0) == -1) {
//                    printf("Sending message error\n");
//                    close(new_fd);
//                    break;
//                } else printf("Message sent\n");

                char* pos = strstr(buf, paket_end) + strlen(paket_end);

                //remove the processed packet
                strcpy(buf, pos);
            }
        }
>>>>>>> origin/chris
    }

    //Close Socket
    close(sockfd);

<<<<<<< HEAD
}
=======
}
>>>>>>> origin/chris
