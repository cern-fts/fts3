#include "common/error.h"
#include "common/logger.h"

#include "config/serverconfig.h"

#include "infosys/SiteNameCacheRetriever.h"

#include <string>
#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

using namespace std;
using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::infosys;
using namespace boost::property_tree;

/* -------------------------------------------------------------------------- */
int main(int argc, char** argv) {

	// exit status
	int ret = EXIT_SUCCESS;

	theServerConfig().read(argc, argv);

	// path to local BDII cache
	const string bdii_path = "/var/lib/fts3/bdii_cache.xml";

	// instance of the site name cache retriever
	SiteNameCacheRetriever& cache = SiteNameCacheRetriever::getInstance();

	try {
		// get the cache from BDII
		ptree root;
		cache.get(root);
		// write the cache to the disc in XML format
		fstream out (bdii_path.c_str());
		write_xml(out, root);
		out.flush();
		out.close();
	} catch(...) {
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BDII cache: An unknown exception has been catched!" << commit;
		return EXIT_FAILURE;
	}

    return ret;
}
