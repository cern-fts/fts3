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

#ifndef CRED_CRED_UTILITY_H_
#define CRED_CRED_UTILITY_H_

#include <string>

/**
 * Get Proxy DN
 */
std::string get_proxy_dn(const std::string& proxy_file = "") /*throw (CredServiceException)*/;

/**
 * Get Proxy Certificate File Name for the given user
 */
std::string get_proxy_cert(
    const std::string&  user_dn,
    const std::string&  user_cred,
    const std::string&  vo_name,
    const std::string&  cred_service_endpoint,
    const std::string&  assoc_service,
    const std::string&  assoc_service_type,
    bool                disable_delegation,
    const std::string&  cred_service_type = "") /*throw (LogicError, CredServiceException)*/;

/**
 * Return the proxy lifetime. In case of other errors, returns -1
 */
void get_proxy_lifetime(const std::string& filename, time_t *lifetime, time_t *vo_lifetime);



#endif //CRED_CRED_UTILITY_H_
