#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int num, i, tmpch;

    if(argc < 2)
    {
        printf("not enough arguments\n");
        exit(1);
    }
    
    srand(time(NULL));
    num = atoi(argv[1]);

    for(i = 0; i < num; i++)
    {
        tmpch = rand() % 27;
        switch(tmpch)
        {
            case 0:
                printf(" ");
                break;
            default:
                printf("%c", (char)(tmpch + 64));
        }
    }

    printf("\n");

    return 0;
}
