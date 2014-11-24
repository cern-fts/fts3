/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <algorithm>

//TODO: to be removed
void parse_url(const char *url,
               char **scheme, char **host, int *port, char **path);

struct Uri
{
public:
    std::string QueryString, Path, Protocol, Host, Port;

    std::string getSeName(void)
    {
        return Protocol + "://" + Host;
    }

    static Uri Parse(const std::string &uri)
    {
        Uri result;

        typedef std::string::const_iterator iterator_t;

        if (uri.length() == 0)
            return result;

        iterator_t uriEnd = uri.end();

        // get query start
        iterator_t queryStart = std::find(uri.begin(), uriEnd, '?');

        // protocol
        iterator_t protocolStart = uri.begin();
        iterator_t protocolEnd = std::find(protocolStart, uriEnd, ':');            //"://");

        if (protocolEnd != uriEnd)
            {
                std::string prot = &*(protocolEnd);
                if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
                    {
                        result.Protocol = std::string(protocolStart, protocolEnd);
                        protocolEnd += 3;   //      ://
                    }
                else
                    protocolEnd = uri.begin();  // no protocol
            }
        else
            protocolEnd = uri.begin();  // no protocol

        // host
        iterator_t hostStart = protocolEnd;
        iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart

        iterator_t hostEnd = std::find(protocolEnd,
                                       (pathStart != uriEnd) ? pathStart : queryStart, ':');  // check for port

        result.Host = std::string(hostStart, hostEnd);

        // port
        if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == ':'))  // we have a port
            {
                hostEnd++;
                iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
                result.Port = std::string(hostEnd, portEnd);
            }

        // path
        if (pathStart != uriEnd)
            result.Path = std::string(pathStart, queryStart);

        // query
        if (queryStart != uriEnd)
            result.QueryString = std::string(queryStart, uri.end());

        return result;

    }   // Parse
};  // uri

