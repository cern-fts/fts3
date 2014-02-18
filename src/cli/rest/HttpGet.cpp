/*
 * HttpGet.cpp
 *
 *  Created on: Feb 18, 2014
 *      Author: simonm
 */

#include "HttpGet.h"

using namespace fts3::cli;

const string HttpGet::PORT = "8446";
const string HttpGet::CERTIFICATE = "/tmp/x509up_u500";
const string HttpGet::CAPATH = "/etc/grid-security/certificates";

HttpGet::HttpGet(string url, MsgPrinter& printer) : curl(curl_easy_init()), printer(printer)
{
    // return code
    CURLcode res;

    if(curl)
        {
            // the url we are going to contact
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            // our proxy certificate (the '-E' option)
            curl_easy_setopt(curl, CURLOPT_SSLCERT, CERTIFICATE.c_str());
            // path to certificates (the '--capath' option)
            curl_easy_setopt(curl, CURLOPT_CAPATH, CAPATH.c_str());
            // our proxy again (the '-cacert' option)
            curl_easy_setopt(curl, CURLOPT_CAINFO, CERTIFICATE.c_str());
            // the callback function
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            // the stream the data will be written to
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ss);

            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);

            /* Check for errors */
            if(res != CURLE_OK)
                {
                    printer.error_msg(curl_easy_strerror(res));
                }


        }
    else
        {
            printer.error_msg("failed to initialize curl context (curl_easy_init)");
        }
}

HttpGet::~HttpGet()
{
    // RAII fashion clean up
    if(curl) curl_easy_cleanup(curl);
}

size_t HttpGet::write_data(void *ptr, size_t size, size_t nmemb, stringstream* ss)
{
    // calculate the size in bytes
    size_t realsize = size * nmemb;
    // create a string from the received bytes
    string str(static_cast<char*>(ptr), realsize);
    // write it to the stream
    *ss << str;

    return realsize;
}

void HttpGet::setPort(string& endpoint)
{
    // look for the colon that separates the hostname from port
    size_t found = endpoint.find_last_of(":");

    // if the port was not specified just add it
    if (found == string::npos)
        {
            endpoint += ":" + PORT;
            return;
        }

    // otherwise replace the port
    endpoint = endpoint.substr(0, found + 1) + PORT;
}
