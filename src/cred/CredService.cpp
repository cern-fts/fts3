/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 ***********************************************/

#include "CredService.h"
#include "cred-utility.h"
#include "TempFile.h"
#include "Handle.h"
#include "common/logger.h"
#include "common/error.h"

using namespace FTS3_COMMON_NAMESPACE;

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus
// Fix Warning
#ifdef IOV_MAX 
#undef IOV_MAX
#endif //IOV_MAX
#include "globus_gsi_credential.h"
#ifdef __cplusplus
}
#endif //__cplusplus
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <boost/scoped_ptr.hpp>

using boost::scoped_ptr;

namespace {
const char * const TMP_DIRECTORY = "/tmp";
}


/*
 * CredService
 *
 * Constructor
 */
CredService::CredService(){
}
   
/**
 * get
 * Get The proxy Certificate for the requested user
 */
void CredService::get(
    const std::string&  userDn, 
    const std::string&  id, 
    std::string&        filename) {

    // Check Preconditions
    if(userDn.empty()){
        //m_log_error("Invalid User DN specified");
        //throw InvalidArgumentException("Invalid User DN specified");
	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid User DN specified" << commit;
    }
    if(id.empty()){
        //m_log_error("Invalid credential id specified");
        //throw InvalidArgumentException("Invalid credential id specified");
	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid credential id specified" << commit;
    }
    
    // Get the filename to be for the given DN
    std::string fname = getFileName(userDn,id);
    //m_log_debug("Get the filename to be for the given DN: " << fname);
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Get the filename for the given DN: " << fname << commit;
    // Post-Condition Check: filename length should be max (FILENAME_MAX - 7)
    if(fname.length() > (FILENAME_MAX - 7)){
        //m_log_error("Invalid credential file name generated: length exceeded");
        //throw LogicError("Invalid credential file name generated");
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Invalid credential file name generated" << commit;
    }
    
    // Check if the Proxy Certificate is already there and it's valid
    if(true == isValidProxy(fname)){
        filename = fname;
        //m_log_debug("Proxy Certificate is already on file " << filename);
	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Proxy Certificate is already on file " << filename << commit;
        return;
    }
    
    // Create a Temporary File
    Handle h;
    std::string tmp_proxy_fname;
    try{
        tmp_proxy_fname = TempFile::generate("cred",TMP_DIRECTORY,h.get());
    } catch(const std::exception& exc){
        //m_log_error("Cannot create temporary file: " << exc.what());
       // throw CredServiceException("Cannot create file for proxy certificate");
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot create file for proxy certificate" << exc.what() << commit;
    }
    // RAII
    TempFile tmp_proxy(tmp_proxy_fname);

    // Get certificate 
    try{
        getNewCertificate(userDn,id,tmp_proxy.name());
    } catch(...){
        throw;
    }

    // Rename the Temporary File    
    try{
        tmp_proxy.rename(fname);
    } catch(const std::exception& exc){
        //m_log_error("Cannot rename temporary file <" << tmp_proxy.name() << ">: " << exc.what());
        //throw CredServiceException("Cannot create file for proxy certificate");
	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot rename temporary file <" << tmp_proxy.name() << ">: " << exc.what() << commit;	
    }
    // Return the proxy certificate File name
    filename = fname;
}

/*
 * isValidProxy
 *
 * Check if the Proxy Already Exists and is Valid. 
 */
bool CredService::isValidProxy(const std::string& filename){

    // Check if it's valid
    time_t lifetime = get_proxy_lifetime(filename);
    if(lifetime < 0){
        //m_log_debug("Proxy Certificate expired");
	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy Certificate expired" << commit;
        return false;
    }
    /*m_log_debug("Proxy filename :" << filename);
    m_log_debug("Lifetime       : " << lifetime);
    m_log_debug("Min Valid  time: " << this->minValidityTime());*/
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy filename :" << filename << commit;    	
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Lifetime       : " << lifetime << commit;    	
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Min Valid  time: " << this->minValidityTime() << commit;    	

    // casting to unsigned long is safe, condition lifetime < 0 already checked
    if(this->minValidityTime() >= (unsigned long)lifetime){ 
        //m_log_debug("Proxy Certificate should be renewed");
	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy Certificate should be renewed" << commit;    	
        return false;
    }
    //m_log_debug("Proxy Certificate is still valid");
	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy Certificate is still valid" << commit;    
    
    return true;
}
