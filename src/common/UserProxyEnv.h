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

#ifndef GLITE_DATA_AGENTS_USERPROXYENV_H_
#define GLITE_DATA_AGENTS_USERPROXYENV_H_

#include <string>
#include <boost/utility.hpp>



/**
 * UserProxyEnv: wrapper class to set the environment variable with the
 * proxy credentail and restore to the previous state when the object goes out
 * of scope.
 * Note: it doesn't work on a multithreaded environment
 */
class UserProxyEnv : boost::noncopyable
{
public:
    /**
     * Constructor
     */
    explicit UserProxyEnv(const std::string& file_name);
    /**
     * Destructor
     */
    ~UserProxyEnv();
private:
    /**
     * Default Constructor. Not Implemented
     */
    UserProxyEnv();
private:
    /**
     * Old value of the environment variable X509_USER_KEY
     */
    std::string m_key;

    /**
     * Old value of the environment variable X509_USER_CERT
     */
    std::string m_cert;

    /**
     * Old value of the environment variable X509_USER_PROXY
     */
    std::string m_proxy;
    /**
     * Flag the indicates that the environment variable has been set
     */
    bool m_isSet;


};


#endif //GLITE_DATA_AGENTS_USERPROXYENV_H_
