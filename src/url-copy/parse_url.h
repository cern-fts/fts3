#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void parse_url(const char *url,
                      char **scheme, char **host, int *port, char **path);
