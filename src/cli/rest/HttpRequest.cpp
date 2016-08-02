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

#include "HttpRequest.h"
#include "ResponseParser.h"

#include "exception/cli_exception.h"
#include "exception/rest_failure.h"
#include "exception/rest_invalid.h"
#include "exception/wrong_protocol.h"

#include <sys/stat.h>

using namespace fts3::cli;

const std::string HttpRequest::PORT = "8446";

HttpRequest::HttpRequest(std::string const & url, std::string const & capath, std::string const & proxy,
    std::iostream& stream, std::string const &topname /* = std::string() */) : stream(stream), curl(curl_easy_init()), topname(topname)
{
    if (!curl) throw cli_exception("failed to initialise curl context (curl_easy_init)");

    // the url we are going to contact
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // path to certificates (the '--capath' option)
    curl_easy_setopt(curl, CURLOPT_CAPATH, capath.c_str());

    if (!proxy.empty() && access(proxy.c_str(), F_OK) == 0) {
        curl_easy_setopt(curl, CURLOPT_SSLCERT, proxy.c_str());
        curl_easy_setopt(curl, CURLOPT_CAINFO, proxy.c_str());
    }
    else if (getenv("X509_USER_CERT") != NULL) {
        curl_easy_setopt(curl, CURLOPT_SSLKEY, getenv("X509_USER_KEY"));
        curl_easy_setopt(curl, CURLOPT_SSLCERT, getenv("X509_USER_CERT"));
    }
    else {
        curl_easy_setopt(curl, CURLOPT_SSLKEY, "/etc/grid-security/hostkey.pem");
        curl_easy_setopt(curl, CURLOPT_SSLCERT, "/etc/grid-security/hostcert.pem");
    }

    // the write callback function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    // the stream the data will be written to
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    // the read callback function
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_data);
    // the stream the data will be read from
    curl_easy_setopt(curl, CURLOPT_READDATA, this);
    // capture the contents of the header
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, keep_header);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerrbuf);

    headerslist = 0;
    if (url.find('?') != std::string::npos) {
        headerslist = curl_slist_append(headerslist, "Content-Type: application/x-www-form-urlencoded");
    } else {
        headerslist = curl_slist_append(headerslist, "Content-Type: application/json");
    }
    headerslist = curl_slist_append(headerslist, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerslist);
}

HttpRequest::~HttpRequest()
{
    // RAII fashion clean up
    if(curl) curl_easy_cleanup(curl);
    curl_slist_free_all(headerslist);
}

size_t HttpRequest::write_data(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    HttpRequest *req = static_cast<HttpRequest*>(userdata);
    // clear the stream if it reached EOF beforehand
    if (!req->stream) {
        req->stream.clear();
    }
    size_t realsize = size * nmemb;
    if (realsize == 0) return realsize;
    char *cdata = static_cast<char*>(ptr);
    if (req->firstWrite) {
        req->firstWrite = false;
        if (*cdata == '[') {
            if (req->topname.empty()) {
                throw rest_invalid("Reply unexpectedly contains multiple results");
            }
            std::string s = "{\"" + req->topname + "\":";
            req->stream.write(s.c_str(),s.length());
            req->addedTopLevel = true;
        }
    }
    // calculate the size in bytes
    // write it to the stream
    req->stream.write(cdata, realsize);

    return realsize;
}

size_t HttpRequest::read_data(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    HttpRequest *req = static_cast<HttpRequest*>(userdata);
    // calculate the size in bytes
    size_t realsize = size * nmemb;
    // read from the stream
    req->stream.read(static_cast<char*>(ptr), realsize);

    return req->stream.gcount();
}

size_t HttpRequest::keep_header(char *buffer, size_t size, size_t nitems, void *userdata)
{
    HttpRequest *req = static_cast<HttpRequest*>(userdata);
    size_t nb = size*nitems;
    std::string s(buffer, nb);
    req->headlines.push_back(s);

    return nb;
}


CURLcode HttpRequest::perform(void)
{
    return curl_easy_perform(curl);
}


CURLcode HttpRequest::get_info(CURLINFO info, long *data)
{
    return curl_easy_getinfo(curl, info, data);
}


void HttpRequest::request()
{
    headlines.clear();
    curlerrbuf[0] = '\0';
    addedTopLevel = false;
    firstWrite = true;
    /* Perform the request, res will get the return code */
    CURLcode res = perform();
    /* Check for errors */
    if(res != CURLE_OK) {
       std::string msg = "Communication problem: ";
       std::string msg1 = std::string(curl_easy_strerror(res));
       msg += msg1;
       if (*curlerrbuf) {
           std::string msg2 = curlerrbuf;
           if (msg1 != msg2) {
               msg += ": " + msg2;
           }
       }
       throw cli_exception(msg);
    }

    // add termining brace if top level object name was added
    if (addedTopLevel) {
        std::string s = "}";
        stream.write(s.c_str(),s.length());
    }

    bool isJson = false;
    std::vector<std::string>::iterator itr;
    for(itr = headlines.begin(); itr != headlines.end(); ++itr) {
        if (itr->find("Content-Type: ") == 0 && itr->find("application/json") != std::string::npos) {
            isJson = true;
        }
        if (itr->find("Server: gSOAP/") == 0) {
            throw wrong_protocol("gSOAP server detected, not REST");
        }
    }

    long http_code = 0;
    get_info(CURLINFO_RESPONSE_CODE, &http_code);

    bool responseOk = true;
    // code 207 indicates partial success, but don't treat it as error here
    if (http_code == 0 || http_code >= 400) {
        responseOk = false;
    }

    if (responseOk) {
        // done
        return;
    }

    std::streampos pos = stream.tellg();

    if (isJson) {
        // see if the body can be parsed and has a 'message' field
        // (which may contain reply data)
        std::string m,httpm;
        bool hasRepData = false;
        try {
            ResponseParser r(stream);
            m = r.get("message");
            httpm = r.get("status");
            hasRepData = true;
        } catch(...) {
            // nothing
        }
        // if there was reply data throw rest reply error
        if (hasRepData) {
            throw rest_failure(http_code, m, httpm);
        }
        // message field not available or wasn't json data;
        // throw a cli exception with the message data if available
        if (m.length()) {
            std::stringstream ss;
            ss << "HTTP code " << http_code << ": " << m;
            throw rest_invalid(ss.str());
        }
    }

    // body could not be parsed as json or had no message field: but the body
    // may contain some helpful information, return some of it

    stream.clear();
    stream.seekg(pos);
    std::string m,l;
    std::getline(stream, l);

    std::stringstream ss;
    ss << "HTTP code " << http_code;

    if (l.length()>0) {
        do {
            m += l;
            l.clear();
            if (stream.eof()) break;
            std::getline(stream, l);
        } while(m.length()<80);
        if (l.length()>0) {
            m += "...";
        }
        ss << ": " << m;
    }
    throw rest_invalid(ss.str());
}

void HttpRequest::get()
{
    // do the request
    request();
}

void HttpRequest::del()
{
    // make it a delete
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    // do the request
    request();
}

void HttpRequest::put()
{
    // get size of the data to put:
    stream.seekg (0, stream.end);
    std::streamoff size = stream.tellg();
    stream.seekg (0, stream.beg);

    /* tell it to "upload" to the URL */
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    /* and give the size of the upload (optional) */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)size);

    // do the request
    request();
}

void HttpRequest::post()
{
    // get size of the data to post:
    stream.seekg (0, stream.end);
    std::streamoff size = stream.tellg();
    stream.seekg (0, stream.beg);

    /* tell it to post */
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    /* and give the size of the upload (optional) */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)size);

    // do the request
    request();
}

/* static member function */
std::string HttpRequest::urlencode(std::string const &value)
{
    CURL *curl = curl_easy_init();
    char *output = curl_easy_escape(curl, value.c_str(), (int)value.length());
    std::string result(output);
    curl_free(output);
    curl_easy_cleanup(curl);
    return result;
}
