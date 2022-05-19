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

#include "TempFile.h"
#include "DelegCred.h"

#include "CredUtility.h"
#include "db/generic/SingleDbInstance.h"

#include <globus_gsi_credential.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <boost/lexical_cast.hpp>
#include <fstream>

#include "common/Exceptions.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"

using namespace db;
using namespace fts3::common;


namespace
{
const char * const TMP_DIRECTORY = "/tmp/";
const char * const PROXY_NAME_PREFIX = "x509up_h";
}

/*
 * encodeName
 *
 * Encode a string (not URL encoded since the hash is always prefixed)
 */
static std::string encodeDN(const std::string& dn)
{
    std::string encoded;
    encoded.reserve(dn.length());

    // for each character
    std::string::const_iterator s_it;
    for (s_it = dn.begin(); s_it != dn.end(); ++s_it) {
        if (isalnum((*s_it))) {
            encoded.push_back(static_cast<char>(tolower((*s_it))));
        }
        else {
            encoded.push_back('X');
        }
    }
    return encoded;
}


std::string DelegCred::getProxyFile(const std::string& userDn,
        const std::string& id)
{
    try {
        // Check Preconditions
        if (userDn.empty()) {
            throw SystemError("Invalid User DN specified");
        }

        if (id.empty()) {
            throw SystemError("Invalid credential id specified");
        }

        std::list<std::string> proxy_filenames;
        proxy_filenames.push_back(generateProxyName(userDn, id));

        if (fts3::config::ServerConfig::instance().get<bool>("BackwardsCompatibleProxyNames")) {
            proxy_filenames.push_back(generateProxyName(userDn, id, true));
        }

        for (auto it = proxy_filenames.begin(); it != proxy_filenames.end(); it++) {
            // Post-Condition Check: filename length should be max (FILENAME_MAX - 7)
            if ((*it).length() > (FILENAME_MAX - 7)) {
                throw SystemError("Invalid credential file name generated");
            }

            // Check if the Proxy Certificate is already there and is valid
            std::string tmpMessage;
            if (isValidProxy(*it, tmpMessage)) {
                return *it;
            }
        }

        // Check if the database contains a valid proxy for this dlg id and DN
        if (DBSingleton::instance().getDBObjectInstance()->isCredentialExpired(id, userDn)) {
            // This looks crazy, but right now I don't know what can happen if I throw here
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Proxy for dlg id "<< id << " and DN " << userDn
                << " has expired in the DB, needs renewal!" << commit;
            return std::string();
        }

        // Create a Temporary File
        TempFile tmp_proxy("cred", TMP_DIRECTORY);

        // Get certificate
        getNewCertificate(userDn, id, tmp_proxy.name());

        // Rename the Temporary File
        tmp_proxy.rename(proxy_filenames.front());

        return proxy_filenames.front();
    }
    catch(const std::exception& exc)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't get The proxy Certificate for the requested user: " << exc.what() << commit;
    }
    catch(...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't get The proxy Certificate for the requested user" << commit;
    }

    return std::string();
}

/*
 * isValidProxy
 *
 * Check if the Proxy Already Exists and is Valid.
 */
bool DelegCred::isValidProxy(const std::string& filename, std::string& message)
{
    //prevent ssl_library_init from getting called by multiple threads
    static boost::mutex qm_cred_service;
    boost::mutex::scoped_lock lock(qm_cred_service);

    // Check if it's valid
    time_t lifetime, voms_lifetime;
    get_proxy_lifetime(filename, &lifetime, &voms_lifetime);

    std::string time1 = boost::lexical_cast<std::string>(lifetime);
    std::string time2 = boost::lexical_cast<std::string>(minValidityTime());
    std::string time3 = boost::lexical_cast<std::string>(voms_lifetime);

    if(lifetime < 0)
        {
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
    if(minValidityTime() >= (unsigned long)lifetime)
        {
            message = " Proxy Certificate ";
            message += filename;
            message += " should be renewed, lifetime is ";
            message += time1;
            message +=  " secs, while min validity time is ";
            message += time2;
            message += " secs";
            return false;
        }

     else if( (voms_lifetime > 0) && (minValidityTime() >= (unsigned long)voms_lifetime))
        {
            message = " VO extensions for certificate ";
            message += filename;
            message += " should be renewed, lifetime is ";
            message += time3;
            message +=  " secs, while min validity time is ";
            message += time2;
            message += " secs";
            return false;
        }

    return true;
}


void DelegCred::getNewCertificate(const std::string &userDn,
    const std::string &credId, const std::string &filename)
{

    try {
        // Get the Cred Id
        boost::optional<UserCredential> cred = DBSingleton::instance().getDBObjectInstance()->findCredential(credId, userDn);
        if (!cred) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Didn't get any credentials from the database!" << commit;
        }

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Get the Cred Id " << credId << " " << userDn << commit;

        // write the content of the certificate property into the file
        std::ofstream ofs(filename.c_str(), std::ios_base::binary);

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
            << "Write the content of the certificate property into the file " << filename << commit;
        if (ofs.bad()) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed open file " << filename << " for writing" << commit;
            return;
        }
        // write the Content of the certificate
        if (cred) {
            ofs << cred->proxy;
        }
        // Close the file
        ofs.close();
    }
    catch (const std::exception& exc) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to get certificate for user <" << userDn << ">. Reason is: " << exc.what() << commit;
    }
}


std::string DelegCred::generateProxyName(const std::string& userDn, const std::string& id, bool legacy)
{
    // Hash the <DN><cred_id>
    size_t h = std::hash<std::string>()(userDn + id);
    std::stringstream ss;
    ss << h;

    std::string hash_str = ss.str();
    std::string encoded;

    if (legacy) {
        encoded = encodeDN(userDn);
    } else {
        encoded += "_" + id;
    }

    // Compute filename maximum length (ignore errors)
    long sys_max_length = pathconf(TMP_DIRECTORY, _PC_NAME_MAX);
    size_t max_length = FILENAME_MAX;

    if (sys_max_length != -1) {
        max_length = sys_max_length - 7 - strlen(PROXY_NAME_PREFIX);

        if (max_length <= 0) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot generate proxy file name: prefix too long" << commit;
            return "";
        }

        if (hash_str.length() > (std::string::size_type) max_length) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot generate proxy file name: hash too long" << commit;
            return "";
        }
    }

    // Generate the filename using the hash and the encoded credential
    std::string filename = std::string(TMP_DIRECTORY) + PROXY_NAME_PREFIX + hash_str;

    if (hash_str.length() < max_length) {
        filename.append(encoded.substr(0, (max_length - hash_str.length())));
    }

    return filename;
}


unsigned long DelegCred::minValidityTime()
{
    return 5400;
}

