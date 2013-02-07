#include "site_name.h"
#include "parse_url.h"
#include "logger.h"
#include "error.h"

#include "infosys/SiteNameRetriever.h"

using namespace fts3::common;
using namespace fts3::infosys;

SiteName::SiteName() {

}

SiteName::~SiteName(){
}


std::string SiteName::getSiteName(std::string& hostname){

	std::string se;
	char *base_scheme = NULL;
	char *base_host = NULL;
	char *base_path = NULL;
	int base_port = 0;
	parse_url(hostname.c_str(), &base_scheme, &base_host, &base_port, &base_path);
	if(base_host) se = std::string(base_host);
	
	if(base_scheme) free(base_scheme);
	if(base_host)   free(base_host);
	if(base_path)	free(base_path);

	return SiteNameRetriever::getInstance().getSiteName(se);
}


