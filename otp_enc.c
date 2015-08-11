#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "shared.h"

#ifndef BSIZE
#define BSIZE 256
#endif

int main(int argc, char *argv[])
{
    int sockfd, portno, n, plainfd, keyfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int numKey, numPlain;
    FILE *fpKey;
    char buffer[BSIZE];
    char *rmLf;

    //Checking that all parameters have been passed in
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    //Checking that number of characters in Key file is 
    //greater than plaintext file
    numPlain = checkFile(argv[1]);
    numKey = checkFile(argv[2]);
    if (numKey < numPlain)
        error("Key is not large enough");
	
    //Open the plaintext and key files for writing to socket
    plainfd = open(argv[1], O_RDONLY);
    if (plainfd < 0)
    {
        fprintf(stderr, "error opening file");
        exit(1);
    }
    keyfd = open(argv[2], O_RDONLY);
    if (fpKey < 0)
    {
        fprintf(stderr, "error opening key");
        exit(1);
    }

    //convert the port number from string to socket
    portno = atoi(argv[3]);
    //Create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    //Get server details
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    //Setting server details in struct serv_addr
    memset((char *) &serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr,
            (char *)server->h_addr,
            server->h_length);
    serv_addr.sin_port = htons(portno);

    //Connect to the socket
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    //Initialize the buffer to nulls
    memset(buffer, '\0', BSIZE);
    //write the plaintext file to the socket
    while((n = read(plainfd, buffer, BSIZE - 1)) > 0)
    {
        rmLf = strchr(buffer, '\n');
        if(rmLf != NULL)
        {
            *rmLf = '\0';
            n--;
        }
        write(sockfd, buffer, n);
        memset(buffer, '\0', BSIZE);
    }
    //write the delimiting character to the socket
    write(sockfd, "&", 1);
    //write the key file to the socket 
    while((n = read(keyfd, buffer, BSIZE - 1)) > 0)
    {
        rmLf = strchr(buffer, '\n');
        if(rmLf != NULL)
        {
            *rmLf = '\0';
            n--;
        }
        write(sockfd, buffer, n);
        memset(buffer, '\0', BSIZE);
    }

    //Shutdown the client write operation to send EOF to server
    //to break read loop
    if (shutdown(sockfd, SHUT_WR) < 0)
        error("shutdown write client failed");

    //PROCESS THE SERVER'S RESPONSE
    memset(buffer, '\0', BSIZE);
    while((n = read(sockfd,buffer,BSIZE - 1)) > 0)
    {
        printf("%s",buffer);
        memset(buffer, '\0', BSIZE);
    }
    close(sockfd);
    printf("\n");
    return 0;
}
