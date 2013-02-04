#include "site_name.h"
#include "parse_url.h"
#include "logger.h"
#include "error.h"

using namespace FTS3_COMMON_NAMESPACE;

bool SiteName::connected = false;

SiteName::SiteName(): siteName(""), infosys(""), pPath(NULL), bdiiUsed(false){	
	if(!connected){
  	pPath = getenv ("LCG_GFAL_INFOSYS");
  	if (pPath!=NULL){
		infosys = std::string(pPath);
		if(infosys.compare("false")!=0){
			connected = BdiiBrowser::getInstance().connect(infosys);
			bdiiUsed = true;	
		}
	}
	}
}

SiteName::~SiteName(){
}


std::string SiteName::getSiteName(std::string& hostname){

  if(bdiiUsed){
    std::string bdiiHost("");
    char *base_scheme = NULL;
    char *base_host = NULL;
    char *base_path = NULL;
    int base_port = 0;
    parse_url(hostname.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    if(base_host)
    	bdiiHost = std::string(base_host);   
	
   if(base_scheme)
   	free(base_scheme);
   if(base_host)
   	free(base_host);
   if(base_path)
   	free(base_path);
	    		
     siteName = BdiiBrowser::getInstance().getSiteName(bdiiHost);	
 
    }
    return  siteName;    
}


