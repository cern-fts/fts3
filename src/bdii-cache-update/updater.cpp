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
 */

#include "common/error.h"
#include "common/logger.h"

#include "config/serverconfig.h"

#include "infosys/SiteNameCacheRetriever.h"

#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <exception>

#include <pugixml.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::infosys;
using namespace boost::property_tree;
using namespace pugi;

/* -------------------------------------------------------------------------- */
int main(int argc, char** argv)
{

    // exit status
    int ret = EXIT_SUCCESS;

    try
        {
            theServerConfig().read(argc, argv);
        }
    catch (Err& e)
        {
            std::string msg = "Fatal error, exiting...";
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
            return EXIT_FAILURE;
        }
    catch (...)
        {
            std::string msg = "Fatal error (unknown origin), exiting...";
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
            return EXIT_FAILURE;
        }

    // path to local BDII cache
    const string bdii_path = "/var/lib/fts3/bdii_cache.xml";

    // instance of the site name cache retriever
    SiteNameCacheRetriever& bdii_cache = SiteNameCacheRetriever::getInstance();

    try
        {

            xml_document doc;
            doc.load_file(bdii_path.c_str());

            map<string, string> cache;
            bdii_cache.get(cache);

            map<string, string>::iterator it;
            for (it = cache.begin(); it != cache.end(); ++it)
                {
                    string xpath = "/entry[hostname='" + it->first + "']";
                    xpath_node node = doc.select_single_node(xpath.c_str());
                    if (node)
                        {
                            node.node().child("sitename").last_child().set_value(it->second.c_str());
                        }
                    else
                        {
                            // add new entry
                            xml_node entry = doc.append_child();
                            entry.set_name("entry");

                            xml_node host = entry.append_child();
                            host.set_name("hostname");
                            host.append_child(pugi::node_pcdata).set_value(it->first.c_str());

                            xml_node site = entry.append_child();
                            site.set_name("sitename");
                            site.append_child(pugi::node_pcdata).set_value(it->second.c_str());
                        }
                }

            if (!doc.save_file(bdii_path.c_str()))
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BDII cache: it has not been possible to save the file." << commit;

        }
    catch(std::exception& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BDII cache: " << ex.what() << commit;
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
