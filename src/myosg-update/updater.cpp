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

#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>

#include "config/serverconfig.h"

#include <string>
#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "common/Exceptions.h"
#include "common/Logger.h"

using namespace std;
using namespace fts3::common;
using namespace fts3::config;
using namespace boost::property_tree;

/**
 * Function that writes the file down
 *
 * It has to be complaint with the libcurl requirements.
 */
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return fwrite(ptr, size, nmemb, stream);
}


/* -------------------------------------------------------------------------- */
int main(int argc, char** argv)
{

    // exit status
    int ret = EXIT_SUCCESS;

    try
        {
            theServerConfig().read(argc, argv);

            // path to local the MyOSG XML file and a temporary file
            const string myosg_path_part = "/var/lib/fts3/myosg.xml.part";
            const string myosg_path = "/var/lib/fts3/myosg.xml";
            // false string
            const string false_str = "false";
            // URL of the MyOSG
            const string url = theServerConfig().get<string>("MyOSG");

            // if the MyOSG file is not specified return
            if (url == false_str) return ret;

            // curl context
            CURL *curl;
            // return code
            CURLcode res;
            // output file descriptor
            FILE *fp = fopen(myosg_path_part.c_str(), "wb");

            if (!fp)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "failed to create/open file (" << myosg_path_part << ")" << commit;
                    return EXIT_FAILURE;
                }

            curl = curl_easy_init();
            if(curl)
                {
                    /* Set options */
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

                    /* Perform the request, res will get the return code */
                    res = curl_easy_perform(curl);

                    /* Check for errors */
                    if(res != CURLE_OK)
                        {
                            ret = EXIT_FAILURE;
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "curl_easy_perform() failed: " << curl_easy_strerror(res) << commit;
                        }
                    /* always cleanup */
                    curl_easy_cleanup(curl);

                }
            else
                {
                    ret = EXIT_FAILURE;
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "failed to initialize curl context (curl_easy_init)" << commit;
                }

            // close the part file
            fclose (fp);

            // check if the file has a valid XML syntax
            try
                {
                    fstream in (myosg_path_part.c_str());
                    ptree pt;
                    // if the format is invalid an exception will be thrown
                    read_xml(in, pt);
                }
            catch (xml_parser_error& ex)
                {
                    ret = EXIT_FAILURE;
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "XML syntax error: " << ex.what() << commit;
                }

            if (ret == EXIT_SUCCESS)
                {
                    // if every thing went well rename the file
                    if (rename (myosg_path_part.c_str(), myosg_path.c_str()) < 0)
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not rename the part file " << myosg_path_part << ": " << errno << commit;
                }
            else if (ret == EXIT_FAILURE)
                {
                    // if there was an error remove the part file
                    if (remove (myosg_path_part.c_str()) < 0)
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not remove the part file " << myosg_path_part << ": " << errno << commit;
                }

        }
    catch (BaseException& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
            return EXIT_FAILURE;
        }
    catch (...)
        {
            std::string msg = "Fatal error (unknown origin), exiting...";
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
            return EXIT_FAILURE;
        }

    return ret;
}
