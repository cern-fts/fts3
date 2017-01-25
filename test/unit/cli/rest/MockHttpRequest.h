/*
 * Copyright (c) CERN 2015
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


#ifndef MOCKHTTPREQUEST_H
#define MOCKHTTPREQUEST_H

#include "cli/rest/HttpRequest.h"


class MockHttpRequest: public fts3::cli::HttpRequest
{
protected:
    void readRequest(void)
    {
        stream.seekg(0, stream.end);
        size_t nbytes = stream.tellg();
        std::vector<char> buffer(nbytes + 1);
        stream.seekg(0, stream.beg);
        stream.read(buffer.data(), nbytes);
        buffer[nbytes] = '\0';
        reqBody << buffer.data();
    }

public:
    MockHttpRequest(std::string const & url, std::string const & capath, std::string const & proxy,
        std::iostream& stream, std::string const &topname = std::string()):
        HttpRequest(url, capath, proxy, true, stream, topname)
    {
    }

    virtual void get()
    {
        method = "GET";
        readRequest();
    }

    virtual void del()
    {
        method = "DELETE";
        readRequest();
    }

    virtual void put()
    {
        method = "PUT";
        readRequest();
    }

    virtual void post()
    {
        method = "POST";
        readRequest();
    }

    std::string method;
    std::stringstream reqBody;
};


#endif // MOCKHTTPREQUEST_H
