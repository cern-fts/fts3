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
 * HttpGet.h
 *
 *  Created on: Feb 18, 2014
 *      Author: Michal Simon
 */

#ifndef HTTPGET_H_
#define HTTPGET_H_

#include <stdio.h>
#include <curl/curl.h>

#include <string>
#include <sstream>
#include <iostream>
#include <vector>

#include "MsgPrinter.h"

namespace fts3
{
namespace cli
{

class HttpRequest
{

public:

    HttpRequest(std::string const & url, std::string const & capath, std::string const & proxy,
                std::iostream& stream, std::string const &topname = std::string());
    virtual ~HttpRequest();

    void get();

    void del();

    void put();

    void post();

    static std::string urlencode(std::string const &value);

private:

    void request();
    static size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t read_data(void *ptr, size_t size, size_t nmemb, void *userdata);
    static size_t keep_header(char *buffer, size_t size, size_t nitems, void *userdata);
    static const std::string PORT;

    // input/output stream for interactions with rest service
    std::iostream& stream;

    // curl context
    CURL *curl;

    // optional top level object name to add to json output
    std::string topname;

    // indicates first write during result fetching
    bool firstWrite;

    // indicates if a top level object name has been added to the request
    // result during result fetching. Required if the result is an array.
    bool addedTopLevel;
    
    // list of headers received
    std::vector<std::string> headlines;

    // curl error buffer
    char curlerrbuf[CURL_ERROR_SIZE];

    // curl list of headers to send
    struct curl_slist *headerslist;
};

} /* namespace cli  */
} /* namespace fts3 */
#endif /* HTTPGET_H_ */
