#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BSIZE 256

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n, plainfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BSIZE];
    if (argc < 2) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
	
    plainfd = open("plaintext2", O_RDONLY);
    if (plainfd < 0)
    {
        fprintf(stderr, "error opening file");
    }


//    while((n = read(plainfd, buffer, BSIZE - 1)) > 0)
//    {
//        printf("%s", buffer);
//        bzero(buffer, BSIZE);
//    }

    portno = atoi(argv[1]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");


    bzero(buffer, BSIZE);
    while((n = read(plainfd, buffer, BSIZE - 1)) > 0)
    {
        printf("client [%d]", n);
        write(sockfd, buffer, n);
        bzero(buffer, BSIZE);
    }
    if (shutdown(sockfd, SHUT_WR) < 0)
        error("shutdown write client failed");
    printf("client finished reading\n");
//    printf("Please enter the message: ");
//    bzero(buffer,BSIZE);
//    fgets(buffer,BSIZE - 1,stdin);
//    n = write(sockfd,buffer,strlen(buffer));
//    if (n < 0) 
//         error("ERROR writing to socket");

    //THE SERVER'S RESPONSE
    bzero(buffer,BSIZE);
    n = read(sockfd,buffer,BSIZE - 1);
    printf("client %d\n", n);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    printf("client closing \n");
    close(sockfd);
    return 0;
}
