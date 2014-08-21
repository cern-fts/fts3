/*
 * HttpGet.cpp
 *
 *  Created on: Feb 18, 2014
 *      Author: simonm
 */

#include "HttpRequest.h"

#include "exception/cli_exception.h"

#include <sys/stat.h>

using namespace fts3::cli;

const std::string HttpRequest::PORT = "8446";

HttpRequest::HttpRequest(std::string const & url, std::string const & capath, std::string const & proxy, iostream& stream) :
    stream(stream),
    curl(curl_easy_init())
{
    if (!curl) throw cli_exception("failed to initialise curl context (curl_easy_init)");

    // the url we are going to contact
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // our proxy certificate (the '-E' option)
    curl_easy_setopt(curl, CURLOPT_SSLCERT, proxy.c_str());
    // path to certificates (the '--capath' option)
    curl_easy_setopt(curl, CURLOPT_CAPATH, capath.c_str());
    // our proxy again (the '-cacert' option)
    curl_easy_setopt(curl, CURLOPT_CAINFO, proxy.c_str());
    // the write callback function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    // the stream the data will be written to
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &stream);
    // the read callback function
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_data);
    // the stream the data will be read from
    curl_easy_setopt(curl, CURLOPT_READDATA, &stream);
}

HttpRequest::~HttpRequest()
{
    // RAII fashion clean up
    if(curl) curl_easy_cleanup(curl);
}

size_t HttpRequest::write_data(void *ptr, size_t size, size_t nmemb, ostream* ostr)
{
    // clear the stream if it reached EOF beforehand
    if (!*ostr) ostr->clear();
    // calculate the size in bytes
    size_t realsize = size * nmemb;
    // write it to the stream
    ostr->write(static_cast<char*>(ptr), realsize);

    return realsize;
}

size_t HttpRequest::read_data(void *ptr, size_t size, size_t nmemb, std::istream* istr)
{
    // calculate the size in bytes
    size_t realsize = size * nmemb;
    // read from the stream
    istr->read(static_cast<char*>(ptr), realsize);

    return istr->gcount();
}

void HttpRequest::request()
{
    /* Perform the request, res will get the return code */
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) throw cli_exception(curl_easy_strerror(res));
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
