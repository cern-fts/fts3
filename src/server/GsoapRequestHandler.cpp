/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GsoapAcceptor.h"
#include "GsoapRequest_handler.h"

using namespace fts3::common;
using namespace fts3::server;


GSoapRequestHandler::GSoapRequestHandler(GSoapAcceptor& acceptor): ctx(acceptor.getSoapContext()),acceptor(acceptor)
{

}

GSoapRequestHandler::~GSoapRequestHandler()
{
    if(ctx)
        acceptor.recycleSoapContext(ctx);
}

const char* peek_http_method(SOAP_SOCKET socket, char* method, size_t msize)
{
    // To get the HTTP verb, 10 chars should be enough.
    // Something longer will likely not be HTTP
    char buffer[10]= {0};

    ssize_t received = recv(socket, buffer, sizeof(buffer), MSG_PEEK);
    if (received < 1)
        return NULL;

    char *space = (char*)memchr(buffer, ' ', sizeof(buffer));
    if (!space)
        return NULL;

    *space = '\0';

    // Make sure it is a known verb
    static const char *verbs[] = {"HEAD", "GET", "POST", "PUT",
                                  "DELETE", "TRACE", "OPTIONS",
                                  "CONNECT", NULL
                                 };

    for (const char** v = verbs; *v != NULL; ++v)
        {
            if (strcasecmp(*v, buffer) == 0)
                {
                    strncpy(method, buffer, msize);
                    return method;
                }
        }

    return NULL;
}

void GSoapRequestHandler::run(boost::any&)
{
    if(ctx)
        {
            char method[16] = {0};
            if (peek_http_method(ctx->socket, method, sizeof(method)))
                {
                    FTS3_COMMON_LOGGER_NEWLOG (WARNING) << "Someone sent a plain HTTP request ("
                                                        << method
                                                        << ")"
                                                        << commit; 

                    // Ugly, but soap doesn't seem to be able to respond
                    // (Probably because cgsi expects a ssl negotiation)
                    static const char msg[] =
                        "HTTP/1.1 400 Bad Request\r\n"
                        "Connection: close\r\n"
                        "Content-Type: text/xml\r\n"
                        "Content-Length: 266\r\n\r\n"
                        "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\">"
                        "<SOAP-ENV:Body>"
                        "<SOAP-ENV:Fault><faultcode>SOAP-ENV:Client</faultcode>"
                        "<faultstring>Use the HTTPS scheme to access this URL</faultstring>"
                        "</SOAP-ENV:Fault>"
                        "</SOAP-ENV:Body>"
                        "</SOAP-ENV:Envelope>";
                    // -1 = skip \0
                    if (send(ctx->socket, msg, sizeof(msg) - 1, MSG_NOSIGNAL) < 0)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Could not set the 400 error code: " << errno << commit;
                        }
                }
            else
                {
                    if (fts3_serve(ctx) != SOAP_OK)
                        {
                            char buf[2048] = {0};
                            size_t len=2048;

                            soap_sprint_fault(ctx, buf, len);
                            FTS3_COMMON_LOGGER_NEWLOG (ERR) << buf << commit;
                            soap_send_fault(ctx);
                        }
                }
        }
}

