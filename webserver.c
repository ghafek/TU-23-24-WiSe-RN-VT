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

       if (send(new_fd, "Reply", 5, 0) == -1){
           printf("Sending message error \n");
           close(new_fd);
           exit(1);
       }
       else printf("Message sent \n");

       //Message sent, close new socket
       close(new_fd);

    }

    //Close Socket
    close(sockfd);

}
