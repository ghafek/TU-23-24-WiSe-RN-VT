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

    ////Only Implemented for IPv4 Till now, Can use getaddrinfo to simult. use for IPv6 and IPv4
    ////Preferably use errno, perror when throwing errors, instead of simple printf.


    //Check the command line Arguments count (webserver 0.0.0.0 1234).
    if (argc != 3)
    {
        printf("Please enter the correct number of Arguments \n");
        exit(1);
    }

    //Struct for Socket Address (IPv4):
    struct sockaddr_in sa;

    //Save the IP Address and Port from the Command Line Arguments
    //Check if Port is passed as a number format
    const char* IP = argv[1];
    const int PORT = atoi(argv[2]);

    //Fill the sa structure
    memset(&sa, 0, sizeof(sa)); //clear up the struct first
    sa.sin_family = AF_INET; //IPv4 Address Family
    sa.sin_port = htons(PORT); //Port in Network Byte Order
    inet_pton(AF_INET, IP, &(sa.sin_addr)); //IP from text to binary form

    //Check if a correct IP Address format was entered (0.0.0.0)
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



    //Socket Creation
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

    // Bind the Socket to the IP and Port
    if (bind(sockfd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        printf("Bind error");
        close(sockfd);
        exit(1);
    } else {
        printf("Bind successful \n");
    }

    // Listen for incoming connections
    // Listen to any number of connections
    if (listen(sockfd, 10) == -1) {  //10 is the number of connections that can Queue
        printf("Listen error");
        close(sockfd);
        exit(1);
    } else {
        printf("Listen successful \n");
    }

    //Save the incoming connections address in the struct
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    while (1) {
        addr_size = sizeof their_addr;
        //accept new connections
        int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

        if (new_fd == -1){
            printf("New socket creation error \n");
        }

        //variable to store received messages
        char temp[1024] = ""; //store messages from client
        char buf[1024] = ""; //copy the content of temp
        char* paket_end = "\r\n\r\n"; //identifier for packet end

        //receive until disconnected
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
            while (strstr(buf, paket_end) != NULL) {

                // Full packet received
                if (send(new_fd, "Reply\r\n\r\n", strlen("Reply\r\n\r\n"), 0) == -1) {
                    printf("Sending message error\n");
                    close(new_fd);
                    break;
                } else printf("Message sent\n");

                //find the end position of the processed packet
                char* pos = strstr(buf, paket_end) + strlen(paket_end);

                //remove the processed packet
                strcpy(buf, pos);
            }
        }
    }

    //Close Socket
    close(sockfd);

}