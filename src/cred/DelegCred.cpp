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

#include "DelegCred.h"
#include "Cred.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fstream>
#include "utility.h"
#include "SingleDbInstance.h"
#include "common/logger.h"
#include "common/error.h"
#include <sys/types.h>
#include <grp.h>
#include <sys/stat.h>
#include <pwd.h>
#include "name_to_uid.h"

using namespace FTS3_COMMON_NAMESPACE;
using namespace db;

namespace
{
const char * const PROXY_NAME_PREFIX         = "x509up_h";
}

const std::string repository = "/tmp/";


/*
 * DelegCred
 *
 * Constructor
 */
DelegCred::DelegCred()
{
}

/*
 * ~DelegCred
 *
 * Destructor
 */
DelegCred::~DelegCred()
{
}

/*
 * getNewCertificate
 *
 * Get The proxy Certificate for the requested user
 */
void DelegCred::getNewCertificate(const std::string& userDn, const std::string& cred_id, const std::string& filename) /*throw (LogicError, DelegCredException)*/
{

    Cred* cred = NULL;
    try
        {
            // Get the Cred Id
            cred = DBSingleton::instance().getDBObjectInstance()->findCredential(cred_id, userDn);

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Get the Cred Id " << cred_id << " " << userDn << commit;


            // write the content of the certificate property into the file
            std::ofstream ofs(filename.c_str(), std::ios_base::binary);

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "write the content of the certificate property into the file " << filename << commit;
            if(ofs.bad())
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed open file " << filename << " for writing" << commit;
                    if(cred)
                        delete cred;
                    return;
                }
            // write the Content of the certificate
            if(cred)
                ofs << cred->proxy.c_str();
            // Close the file
            ofs.close();
            uid_t pw_uid;
            pw_uid = name_to_uid();
            int checkChown = chown(filename.c_str(), pw_uid, getgid());
            if(checkChown!=0)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to chmod for proxy" << filename << commit;
                }

            if(cred)
                delete cred;
        }
    catch(const std::exception& exc)
        {
            if(cred)
                delete cred;
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to get certificate for user <" << userDn << ">. Reason is: " << exc.what() << commit;
        }
}

/*
 * getFileName
 *
 * Generate a Name for the Certificate Proxy File
 */
std::string DelegCred::getFileName(const std::string& userDn, const std::string& id) /*throw(LogicError)*/
{

    std::string filename;

    // Hash the DN:id
    unsigned long h = hash_string(userDn + id);
    std::stringstream ss;
    ss << h;
    std::string h_str = ss.str();

    // Encode the DN
    std::string encoded_dn = encodeName(userDn);

    // Compute Max length
    unsigned long filename_max = static_cast<unsigned long>(pathconf(repository.c_str(), _PC_NAME_MAX));
    size_t max_length = (filename_max - 7 - strlen(PROXY_NAME_PREFIX));
    if(max_length == 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot generate proxy file name: prefix too long" << commit;
            return std::string("");
        }
    if(h_str.length() > (std::string::size_type)max_length)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot generate proxy file name: has too long" << commit;
            return std::string("");
        }
    // Generate the filename using the has
    filename = repository + PROXY_NAME_PREFIX + h_str;
    if(h_str.length() < max_length)
        {
            filename.append(encoded_dn.substr(0,(max_length - h_str.length())));
        }
    return filename;
}

/*
 * minValidityTime
 *
 * Returns the validity time that the cached copy of the certificate should
 * have.
 */
unsigned long DelegCred::minValidityTime() /* throw() */
{
    return 3600;
}

/*
 * encodeName
 *
 * Encode a string, is not url encoded since the hash is alweays prefixed
 */
std::string DelegCred::encodeName(const std::string& str) /*throw()*/
{

    std::string encoded;
    encoded.reserve(str.length());

    // for each character
    std::string::const_iterator s_it;
    for(s_it = str.begin(); s_it != str.end(); ++s_it)
        {
            if (isalnum((*s_it)))
                {
                    encoded.push_back(static_cast<char>(tolower((*s_it))));
                }
            else
                {
                    encoded.push_back('X');
                }
        }
    return encoded;
}
