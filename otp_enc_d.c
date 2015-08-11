/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "shared.h"
#include "serverFunc.h"

#ifndef BSIZE
#define BSIZE 256 
#endif


//From TLPI
void clearChildren(int sig)
{
    int savedErrno;
    savedErrno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0)
        continue;
    errno = savedErrno;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    struct sockaddr_in serv_addr;
    int n;
    
    //Checking enough arguments given
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    //SIG_CHILD handler to wait on children
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = &clearChildren;
    if(sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        error("sigaction failure");
    }

    //Creating the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");

    //Setting the server address to accept any clients
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    //Binding the socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");

    //Listening on the socket
    listen(sockfd,5);
 
    //SERVER LOOP
    while(1)
    {
        //getting new clients
        newsockfd = accept(sockfd, NULL, NULL);
        if (newsockfd < 0) 
             error("ERROR on accept");

        //Fork the client their own process, and return to listen
        pid_t spawnpid;
        spawnpid = fork(); 
        if(spawnpid == 0)
        {
            close(sockfd);         //not needed copy of listening socket
            //check encode function is calling
            
            //handle the server response
            handleResponse(newsockfd);
            exit(0);
        }
        else if (spawnpid > 0)
        {
            close(newsockfd);
        }
        else
        {
            close(newsockfd);
        }
    }
    return 0; 
}
