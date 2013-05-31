/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

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
            *scheme = (char*) malloc((size_t) (p - url + 1));
            strncpy(*scheme, url, (size_t) (p - url));
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

            *host = (char*) malloc((size_t) (q - url + 1));
            strncpy(*host, url, (size_t) (q - url));
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


