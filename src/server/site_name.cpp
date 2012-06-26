#include "site_name.h"
#include "parse_url.h"

SiteName::SiteName(){
}

SiteName::~SiteName(){
}


std::string SiteName::getSiteName(std::string& hostname){
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
	
  if( ((int) sdStore.size()) > 5000)
    	sdStore.clear();
	
    std::map <std::string, std::string> ::const_iterator iter = sdStore.find(bdiiHost);
    if (iter != sdStore.end()) {
        siteName = iter->second;
    } else {
        siteName = getFromBDII(bdiiHost);
	if(siteName.length() > 0)
		sdStore.insert(std::make_pair(bdiiHost, siteName));   
    }
    
    return  siteName;    
}


std::string  SiteName::getFromBDII(std::string& hostname){
	std::string site("");
        char* sitename = NULL;
	sitename = SD_getServiceSite(hostname.c_str(), &exception);
	if (exception.status != SDStatus_SUCCESS)
            SD_freeException(&exception);
	if(sitename){
		site =  std::string(sitename);	    
		free(sitename);
		return site;	
	}

	return std::string("");		
}


