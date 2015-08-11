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

#define BSIZE 256 

void error(const char *msg);
void removeNewline(char *buffer, int *n);
int splitRead(char *buffer, int n, FILE *org, FILE *key);
void encode(char *org, char *key, int n);
void cipherResponse(int newsockfd, FILE *org, FILE *key);
void handleResponse(int newsockfd);

//From TLPI
void clearChildren(int sig)
{
    //save the errno before calling waitpid
    int savedErrno;
    savedErrno = errno;
    //clear the waiting children
    while(waitpid(-1, NULL, WNOHANG) > 0)
        continue;
    //restore the errno
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

    //SIG_CHLD handler to wait on children
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
    memset((char *) &serv_addr, '\0', sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    //Binding the socket
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
         error("ERROR on binding");

    //Listening on the socket
    listen(sockfd,5);
 
    //SERVER LOOP
    //infinite loop waiting for clients 
    while(1)
    {
        //getting new clients
        newsockfd = accept(sockfd, NULL, NULL);
        if (newsockfd < 0) 
        {
            perror("accept");
            continue;
        }

        //Fork the client their own process, and return to listen
        pid_t spawnpid;
        spawnpid = fork(); 
        if(spawnpid == 0)
        {
            close(sockfd);         //not needed copy of listening socket
            //handle the server response
            handleResponse(newsockfd);
            exit(0);
        }
        else if (spawnpid > 0)
        {
            //close the extra copy of data socket
            close(newsockfd);
        }
        else
        {
            fprintf(stderr, "fork failed, closing client\n");
            //close the extra copy of data socket
            close(newsockfd);
        }
    }
    return 0; 
}

//entry: message to display with perror
//exit: will display perror message then exit with status 1
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//entry: string and address of string length n
//exit: It will remove the first newline found and decrement n
void removeNewline(char *buffer, int *n)
{
    int i;
    for(i = 0; i < *n ; i++)
    {
        if(buffer[i] == '\n')
        {
            buffer[i] = '\0';
            *n--;
        } 
    }
}

//entry: buffer is split bye &, int is number of characters in the buffer
//      org is the plaintext file, and key is the key file
//exit: text before & will be written to plaintext, and text after will
//      be written to key file
int splitRead(char *buffer, int n, FILE *org, FILE *key)
{
    int i;
    char *pch;
    //iterate through buffer looking for &
    for(i = 0; i < n; i++)
    {
        if(buffer[i] == '&')
        {
            //when fount take first token and write to org file
            pch = strtok(buffer, "&");
            fprintf(org, "%s", pch);
            //take second token and write to key file
            pch = strtok(NULL, "&");
            fprintf(key, "%s", pch);
            //clear the buffer and return 1 to indicate found
            memset(buffer, '\0', BSIZE);
            return 1;
        }
    }
    //'&' not found, write buffer to org file
    //return 0 to indicate & was not found
    fprintf(org, "%s", buffer);
    memset(buffer, '\0', BSIZE);
    return 0;
}

//entry: character of space or [A-Z]
//exit: integer of 0 for space [1-26] for
//      character range
int convertToInt(char ch)
{
   int chInt;
   //if a space return 0
   //else convert to 1-26 character range
   if(ch == ' ')
   {
       return 0;
   }
   else
   {
       chInt = (int)ch - 64; 
       return chInt;
   }
}

//entry: integer from 0-26, 0==space, 1-26 is [A-Z]
//exit: return character corresponding to above codes
char convertToChar(int chInt)
{
    //if 0 return space
    if(chInt == 0)
    {
        return ' ';
    }
    else
    {
        //if not add 64
        //cast to char and return
        chInt += 64;
        return ((char) chInt);
    }
}

// entry: org string to encode and key string to use for encoding,
//      key must be as long as org. n is the length of org string
// exit: org will be encoded text based on key string
//         0 is space +32, 1-26 is [A-Z] +64
void encode(char *org, char *key, int n)
{
    int i, tmpch;
    //loop through all characters
    for(i = 0; i < n; i++)
    {
        //convert characters to integer and add org to key at i
        //integer range of 0-26
        tmpch = (convertToInt(org[i]) + convertToInt(key[i])) % 27;
        //convert the result in tmpch to either space or [A-Z]
        //based on the integer in tmpch
        org[i] = convertToChar(tmpch);
    }
}

//entry: a valid socket stream  with the client to transfer data on, and open 
//      plaintext and key files
//exit; plaintext is encrypted based on key text and sent over
//      the socket
void cipherResponse(int newsockfd, FILE *org, FILE *key)
{
    int n;
    char buffer[BSIZE], keyBuffer[BSIZE];
    //clear the buffers
    memset(buffer, '\0', BSIZE);
    memset(keyBuffer, '\0', BSIZE);

    //SERVER RESPONDING WITH CIPHER TEXT
    //get BSIZE characters from plaintext
    while((fgets(buffer, BSIZE, org)) != NULL)
    {
        //get BSIZE characters from keyBuffer
        if((fgets(keyBuffer, BSIZE, key)) == NULL)
            fprintf(stderr, "inside response: key not large enough\n");
        //get the string size of the buffer
        n = strnlen(buffer, BSIZE);
        //remove the newline if it is there
        //adjust the size n
        removeNewline(buffer, &n);
        //encode the buffer using the keyBuffer
        encode(buffer, keyBuffer, n);
        //write to the socket
        write(newsockfd, buffer, n);
        //clear the buffers
        memset(buffer, '\0', BSIZE);
        memset(keyBuffer, '\0', BSIZE);
    }
}

//entry: a valid open socket with the client
//exit: will verify connecting with correct client,
//      then recevie message, save to temp file, use
//      temp files to encode text and send it back to
//      client
//      
void handleResponse(int newsockfd)
{

    FILE *org, *key;
    char buffer[BSIZE];
    int n, i, keyFlag;
    char temp[BSIZE], temp2[BSIZE], pidStr[BSIZE];

    //ensure clear buffer
    memset(buffer, '\0', BSIZE);
    memset(pidStr, '\0', BSIZE);
    memset(temp, '\0', BSIZE);
    memset(temp2, '\0', BSIZE);

    //ensure contacting the encode function
    n = read(newsockfd, buffer, 2);
    write(newsockfd, "@@", 2);
    if (strncmp(buffer, "@@", 2) != 0)
        return;

    //Set temp file names
    sprintf(pidStr, "%d", (int)getpid());
    strncat(temp, "org_", BSIZE);
    strncat(temp, pidStr, BSIZE  - strnlen(temp, BSIZE));
    strncat(temp2, "key_", BSIZE);
    strncat(temp2, pidStr, BSIZE  - strnlen(temp, BSIZE));
    
    //open a file to write content from stream
    org = fopen(temp, "w+");
    key = fopen(temp2, "w+");

    //initialize bool for key input being sent to false
    //plaintext send first, then key input after flag
    keyFlag = 0;
    while((n = read(newsockfd,buffer,BSIZE - 1)) > 0)
    {
        if (keyFlag == 0)
        {
            //splitRead writes to files, to plaintext before '&'
            //and key to after
            keyFlag = splitRead(buffer, n, org, key);   
        }
        else
        {
            //all the rest is written to the key
            fprintf(key, "%s", buffer);
            memset(buffer, '\0', BSIZE);
        }
    }

    //rewind the files to start from beginning
    rewind(org);
    rewind(key);
    
    //call the cipherResponse to encode and send the message
    cipherResponse(newsockfd, org, key);

    //close the files
    fclose(org);
    fclose(key);
    //delete the temp files
    remove(temp);
    remove(temp2);
}
