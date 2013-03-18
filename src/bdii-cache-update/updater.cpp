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
int main(int argc, char** argv) {

	// exit status
	int ret = EXIT_SUCCESS;

	try{
		theServerConfig().read(argc, argv);
	} catch (Err& e) {
        	std::string msg = "Fatal error, exiting...";
        	FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        	return EXIT_FAILURE;
    	} catch (...) {
        	std::string msg = "Fatal error (unknown origin), exiting...";
        	FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        	return EXIT_FAILURE;
    	}		

	// path to local BDII cache
	const string bdii_path = "/var/lib/fts3/bdii_cache.xml";

	// instance of the site name cache retriever
	SiteNameCacheRetriever& bdii_cache = SiteNameCacheRetriever::getInstance();

	try {

		xml_document doc;
		doc.load_file(bdii_path.c_str());

		map<string, string> cache;
		bdii_cache.get(cache);

		map<string, string>::iterator it;
		for (it = cache.begin(); it != cache.end(); ++it) {
			string xpath = "/entry[hostname='" + it->first + "']";
		    xpath_node node = doc.select_single_node(xpath.c_str());
		    if (node) {
		    	node.node().child("sitename").last_child().set_value(it->second.c_str());
		    } else {
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

	} catch(std::exception& ex) {
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BDII cache: " << ex.what() << commit;
		return EXIT_FAILURE;
	}

    return ret;
}
