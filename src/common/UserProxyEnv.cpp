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

#include "UserProxyEnv.h"
#include <iostream>


namespace
{
const char * const KEY_ENV_VAR   = "X509_USER_KEY";
const char * const CERT_ENV_VAR  = "X509_USER_CERT";
const char * const PROXY_ENV_VAR = "X509_USER_PROXY";
}

/*
 * UserProxyEnv
 *
 * Constructor
 */
UserProxyEnv::UserProxyEnv(const std::string& file_name):
    m_isSet(false)
{

    if(false == file_name.empty())
        {
            char * k = getenv(KEY_ENV_VAR);
            if(0 != k)
                {
                    m_key = k;
                }
            char * c = getenv(CERT_ENV_VAR);
            if(0 != c)
                {
                    m_cert = c;
                }
            char * p = getenv(PROXY_ENV_VAR);
            if(0 != p)
                {
                    m_proxy = p;
                }

            setenv(PROXY_ENV_VAR,file_name.c_str(), 1);
            setenv(CERT_ENV_VAR, file_name.c_str(), 1);
            setenv(KEY_ENV_VAR, file_name.c_str(), 1);
            m_isSet = true;

        }
    else
        {
            std::cerr << "Delegated credentials not found" << std::endl;
        }
}

/*
 * ~UserProxyEnv
 *
 * Destructor
 */
UserProxyEnv::~UserProxyEnv()
{
    // Reset the Environament Variable to the previous value
    if(m_isSet)
        {
            if(!m_proxy.empty())
                {
                    setenv(PROXY_ENV_VAR, m_proxy.c_str(),1);
                }
            else
                {
                    unsetenv(PROXY_ENV_VAR);
                }
            if(!m_key.empty())
                {
                    setenv(KEY_ENV_VAR, m_key.c_str(), 1);
                }
            if(!m_cert.empty())
                {
                    setenv(CERT_ENV_VAR, m_cert.c_str(),1);
                }
            //FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy Environment Restored" << commit;
        }
}
