#ifndef MUELLJEF_SERVERFUNC_H
#define MUELLJEF_SERVERFUNC_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "shared.h"

int splitRead(char *buffer, int n, FILE *org, FILE *key);
void encode(char *org, char *key, int n);
void cipherResponse(int newsockfd, FILE *org, FILE *key);
void handleResponse(int newsockfd);

#endif
