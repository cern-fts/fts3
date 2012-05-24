#include "parse_url.h"

void parse_url(const char *url,
                      char **scheme, char **host, int *port, char **path)
{
    char *p=NULL, *q=NULL;

    *scheme = *host = *path = 0;
    *port = -1;

    for(p = (char *)url; *p; p++)
        if(*p == ':' || *p == '/')
            break;

    if(p > url && *p == ':')
    {
        *scheme = (char*) malloc(p - url + 1);
        strncpy(*scheme, url, p - url);
        (*scheme)[p - url] = '\0';
        url = p+1;
    }    

    if(url[0] == '/' && url[1] == '/')
    {
        url += 2;

        for(p = (char *)url; *p; p++)
            if(*p == '/')
                break;

        /* Does it have a port number? */

        for(q = p-1; q >= url; q--)
            if(!isdigit(*q))
                break;

        if(q < p-1 && *q == ':')
            *port = atoi(q+1);
        else
            q = p;

        *host = (char*) malloc(q - url + 1);
        strncpy(*host, url, q - url);
        (*host)[q - url] = '\0';
        url = p;
    }

    if(*url)
        *path = strdup(url);
    else
        *path = strdup("/");

    for(p=*path; *p; p++)
        if(*p == '\\')
        {
            *p = '/';
        }
}


