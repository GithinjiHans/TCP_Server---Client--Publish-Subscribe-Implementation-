//
// csse2310a3.h
//
#ifndef _CSSE2310A3_H
#define _CSSE2310A3_H

#include <stdio.h>

// See the man pages for function documentation

char *read_line(FILE *stream);

char **split_line(char *line, char delimiter);

char **split_space_not_quote(char *s, int *numTokens);

#endif
