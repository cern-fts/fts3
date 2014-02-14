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
 * RestTransferStatus.cpp
 *
 *  Created on: Feb 6, 2014
 *      Author: Michal Simon
 */

#include "RestTransferStatus.h"

using namespace fts3::cli;

const string RestTransferStatus::PORT = "8446";
const string RestTransferStatus::CERTIFICATE = "/tmp/x509up_u500";
const string RestTransferStatus::CAPATH = "/etc/grid-security/certificates";

RestTransferStatus::RestTransferStatus(string endpoint, string jobId, MsgPrinter& printer) : curl(curl_easy_init()), printer(printer)
{
    // use the right web service
    string url = endpoint + "/jobs/" + jobId;

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

RestTransferStatus::~RestTransferStatus()
{
    // RAII fashion clean up
    if(curl) curl_easy_cleanup(curl);
}

size_t RestTransferStatus::write_data(void *ptr, size_t size, size_t nmemb, stringstream* ss)
{
    // calculate the size in bytes
    size_t realsize = size * nmemb;
    // create a string from the received bytes
    string str(static_cast<char*>(ptr), realsize);
    // write it to the stream
    *ss << str;

    return realsize;
}

void RestTransferStatus::setPort(string& endpoint)
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
