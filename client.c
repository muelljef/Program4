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

#define BSIZE 256

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//Entry: filename as string
//Exit:  If all characters in file are valid the function will return
//          the number of characters, if an invalid character is found
//          the function will return -1
int checkFile(char *filename)
{
    FILE *fp;
    int c, count;

    fp = fopen(filename, "r");
    if(fp == NULL) 
    {
        fprintf(stderr, "file %s did not open\n", filename);
        return -1;
    }

    count = 0;
    while((c = fgetc(fp)) != EOF)
    {
        //If c is not a space or an alphabetic character
        if(c != 32 && !isalpha(c))
        {
            //if c is a lf character
            if(c == 10)
            {
                //get the next character
                c = fgetc(fp);
                //if newline is followed by EOF ok, return count
                if (c == EOF)
                {
                    fclose(fp);
                    return count;
                }
            }
            //all other non space or alpha characters
            //or newline not followed by EOF
            //is invalid input
            fclose(fp);
            return -1;
        }
        count++;
    }
    //in case the file happens to end in just EOF without newline prior
    fclose(fp);
    return count;
}


int main(int argc, char *argv[])
{
    int sockfd, portno, n, plainfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BSIZE];
    if (argc < 4) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    n = checkFile(argv[1]);
    printf("Number of chars in text file: %d\n", n);
    //TODO:check key when supplied
	
    plainfd = open(argv[1], O_RDONLY);
    if (plainfd < 0)
    {
        fprintf(stderr, "error opening file");
    }

    portno = atoi(argv[3]);
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
        write(sockfd, buffer, n);
        bzero(buffer, BSIZE);
    }
    if (shutdown(sockfd, SHUT_WR) < 0)
        error("shutdown write client failed");

    //PROCESS THE SERVER'S RESPONSE
    bzero(buffer,BSIZE);
    n = read(sockfd,buffer,BSIZE - 1);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("%s\n",buffer);
    close(sockfd);
    return 0;
}
