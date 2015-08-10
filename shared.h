#ifndef MUELLJEF_SHARED_H
#define MUELLJEF_SHARED_H

#include <stdio.h>
#include <stdlib.h>

#define BSIZE 256

void error(const char *msg);
void removeNewline(char *buffer, int *n);

#endif
