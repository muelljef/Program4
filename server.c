/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define BSIZE 256 

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//entry: buffer is split bye &, int is number of characters in the buffer
//      org is the plaintext file, and key is the key file
//exit: text before & will be written to plaintext, and text after will
//      be written to key file
void splitRead(char *buffer, int n, FILE *org, FILE *key)
{
    char *pch;
    pch = strtok(buffer, "&");
    fprintf(org, "%s", pch);
    pch = strtok(NULL, "&");
    fprintf(key, "%s", pch);
    bzero(buffer, BSIZE);
}

// 0 is space +32, 1-26 is [A-Z] +64
void encode(char *org, char *key, int n)
{
    int i, tmpch;
    for(i = 0; i < n; i++)
    {
        tmpch = (int)org[i] + (int)key[i];
        tmpch = tmpch % 27;
        switch(tmpch)
        {
            case 0:
                org[i] = ' ';
                break;
            default:
                org[i] = (char)(tmpch + 64);
        }
    }
}


int main(int argc, char *argv[])
{
    FILE *org, *key;
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[BSIZE], keyBuffer[BSIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
    if (newsockfd < 0) 
         error("ERROR on accept");

    //open a file to write content from stream
    org = fopen("temp", "w+");
    key = fopen("temp2", "w+");

    bzero(buffer,BSIZE);
    char *findBreak;
    int i, keyFlag, splitLine;
    keyFlag = 0;
    while((n = read(newsockfd,buffer,BSIZE - 1)) > 0)
    {
        splitLine = 0;
        for(i = 0; i < n; i++)
        {
            if(buffer[i] == '&')
            {
                keyFlag = 1;
                splitLine = 1;
            }
        }
        if(splitLine)
        {
            //splitline found this time through the loop
            splitRead(buffer, n, org, key);
            //bzero called in function
        }
        //write to key if flag set otherwise
        //plaintext
        if(keyFlag)
        {
            fprintf(key, "%s", buffer);
            bzero(buffer,BSIZE);
        }
        else
        {
            fprintf(org, "%s", buffer);
            bzero(buffer,BSIZE);
        }
    }
    rewind(org);
    rewind(key);
    
    //SERVER RESPONDING WITH CIPHER TEXT
    while((fgets(buffer, BSIZE, org)) != NULL)
    {
        n = strnlen(buffer, BSIZE);
        fgets(keyBuffer, BSIZE, key);
        for(i = 0; i < n ; i++)
        {
            if(buffer[i] == '\n')
            {
                //TODO: this needs to be more robust?
                n--;
            }
        }
        encode(buffer, keyBuffer, n);
        write(newsockfd, buffer, n);
        bzero(buffer, BSIZE);
        bzero(keyBuffer, BSIZE);
    }

    fclose(org);
    fclose(key);

    //close the socket
    close(newsockfd);
    close(sockfd);
    return 0; 
}
