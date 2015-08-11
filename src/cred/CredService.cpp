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
#include "SingleDbInstance.h"

using namespace FTS3_COMMON_NAMESPACE;

#ifdef __cplusplus
extern "C" {
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
#include <boost/lexical_cast.hpp>

using namespace db;

static boost::mutex qm_cred_service;


namespace
{
const char * const TMP_DIRECTORY = "/tmp";
}


/*
 * CredService
 *
 * Constructor
 */
CredService::CredService()
{
}

/**
 * get
 * Get The proxy Certificate for the requested user
 */
void CredService::get(
    const std::string&  userDn,
    const std::string&  id,
    std::string&        filename)
{
    try
        {

            // Check Preconditions
            if(userDn.empty())
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid User DN specified" << commit;
                }
            if(id.empty())
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid credential id specified" << commit;
                }

            // Get the filename to be for the given DN
            std::string fname = getFileName(userDn,id);

            // Post-Condition Check: filename length should be max (FILENAME_MAX - 7)
            if(fname.length() > (FILENAME_MAX - 7))
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid credential file name generated" << commit;
                }

            // Check if the Proxy Certificate is already there and it's valid
            std::string message;
            if(true == isValidProxy(fname, message))
                {
                    filename = fname;
                    return;
                }

            // Check if the database contains a valid proxy for this dlg id and DN
            if(false == DBSingleton::instance().getDBObjectInstance()->isCredentialExpired(id, userDn) )
                {
                    filename = fname;
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Proxy for dlg id "<< id << " and DN " << userDn << " has expired in the DB, needs renewal!" << commit;
                    return;
                }

            // Create a Temporary File
            Handle h;
            std::string tmp_proxy_fname;
            try
                {
                    tmp_proxy_fname = TempFile::generate("cred",TMP_DIRECTORY,h.get());
                    if(tmp_proxy_fname.length()==0)
                        {
                            filename = fname;
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot create file for proxy certificate" << commit;
                            return;
                        }
                }
            catch(const std::exception& exc)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot create file for proxy certificate" << exc.what() << commit;
                }
            catch(...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot create file for proxy certificate" << commit;
                }

            // RAII
            TempFile tmp_proxy(tmp_proxy_fname);

            // Get certificate
            try
                {
                    getNewCertificate(userDn,id,tmp_proxy.name());
                }
            catch(...)
                {
                    throw;
                }

            // Rename the Temporary File
            try
                {
                    tmp_proxy.rename(fname);
                }
            catch(const std::exception& exc)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot rename temporary file <" << tmp_proxy.name() << ">: " << exc.what() << commit;
                }
            catch(...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot rename temporary file <" << tmp_proxy.name() << commit;
                }
            // Return the proxy certificate File name
            filename = fname;
        }
    catch(const std::exception& exc)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't get The proxy Certificate for the requested user" << exc.what() << commit;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't get The proxy Certificate for the requested user" << commit;
        }

}

/*
 * isValidProxy
 *
 * Check if the Proxy Already Exists and is Valid.
 */
bool CredService::isValidProxy(const std::string& filename, std::string& message)
{
    //prevent ssl_library_init from getting called by multiple threads
    boost::mutex::scoped_lock lock(qm_cred_service);

    // Check if it's valid
    time_t lifetime, voms_lifetime;
    get_proxy_lifetime(filename, &lifetime, &voms_lifetime);

    std::string time1 = boost::lexical_cast<std::string>(lifetime);
    std::string time2 = boost::lexical_cast<std::string>(this->minValidityTime());

    if(lifetime < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Proxy Certificate expired" << commit;
            message = " Proxy Certificate ";
            message += filename;
            message += " expired, lifetime is ";
            message += time1;
            message +=  " secs, while min validity time is ";
            message += time2;
            message += " secs";

            return false;
        }
    else if (voms_lifetime < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Proxy Certificate VO extensions expired" << commit;
            message = " Proxy Certificate ";
            message += filename;
            message += " lifetime is ";
            message += time1;
            message +=  " secs, VO extensions expired ";
            message += boost::lexical_cast<std::string>(abs((int)voms_lifetime));
            message += " secs ago";

            return false;
        }

    // casting to unsigned long is safe, condition lifetime < 0 already checked
    if(this->minValidityTime() >= (unsigned long)lifetime)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Proxy Certificate should be renewed" << commit;
            message = " Proxy Certificate ";
            message += filename;
            message += " should be renewed, lifetime is ";
            message += time1;
            message +=  " secs, while min validity time is ";
            message += time2;
            message += " secs";
            return false;
        }

    return true;
}
