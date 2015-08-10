#include "shared.h"
 
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//buffer and string length n
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
