#include "serverFunc.h"

//entry: buffer is split bye &, int is number of characters in the buffer
//      org is the plaintext file, and key is the key file
//exit: text before & will be written to plaintext, and text after will
//      be written to key file
int splitRead(char *buffer, int n, FILE *org, FILE *key)
{
    int i;
    for(i = 0; i < n; i++)
    {
        if(buffer[i] == '&')
        {
            char *pch;
            pch = strtok(buffer, "&");
            fprintf(org, "%s", pch);
            pch = strtok(NULL, "&");
            fprintf(key, "%s", pch);
            memset(buffer, '\0', BSIZE);
            return 1;
        }
    }
    //'&' not found
    fprintf(org, "%s", buffer);
    memset(buffer, '\0', BSIZE);
    return 0;
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
            error("key not large enough");
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

void handleResponse(int newsockfd)
{

    FILE *org, *key;
    char buffer[BSIZE];
    int n, i, keyFlag;
    
    //ensure clear buffer
    memset(buffer, '\0', BSIZE);

    //ensure contacting the encode function
    n = read(newsockfd, buffer, 2);
    write(newsockfd, "@@", 2);
    if (strncmp(buffer, "@@", 2) != 0)
        return;
    
    //open a file to write content from stream
    org = fopen("temp", "w+");
    key = fopen("temp2", "w+");

    
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
}
