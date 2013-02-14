#include "common/error.h"
#include "common/logger.h"

#include "config/serverconfig.h"

#include "infosys/SiteNameCacheRetriever.h"

#include <string>
#include <fstream>
#include <exception>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::infosys;
using namespace boost::property_tree;

/* -------------------------------------------------------------------------- */
int main(int argc, char** argv) {

	// replace Infoproviders from the fts3config file so the cache is always updated!
	// options count
	int c = 3;
	// program options
	char* v[] = {"fts_bdii_cache_updater", "--InfoProviders", "glue1;glue2"};

	// exit status
	int ret = EXIT_SUCCESS;

	theServerConfig().read(c, v);

	// path to local BDII cache
	const string bdii_path = "/var/lib/fts3/bdii_cache.xml";

	// instance of the site name cache retriever
	SiteNameCacheRetriever& cache = SiteNameCacheRetriever::getInstance();

	try {
		// get the cache from BDII
		ptree root;
		cache.get(root);
		// write the cache to the disc in XML format
		ofstream out (bdii_path.c_str());
		write_xml(out, root);
		out.flush();
		out.close();
	} catch(std::exception& ex) {
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BDII cache: " << ex.what() << commit;
		return EXIT_FAILURE;
	}

    return ret;
}
