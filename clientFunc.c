#include "clientFunc.h"

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
