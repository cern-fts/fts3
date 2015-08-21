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

#ifndef CRED_DELEG_DELEGCRED_H_
#define CRED_DELEG_DELEGCRED_H_

#include "CredService.h"
#include <boost/utility.hpp>


/**
 * DelegCred CredService
 */
class DelegCred : public CredService, boost::noncopyable
{
public:
    /**
     * Constructor
     */
    DelegCred();

    /**
     * Destructor
     */
    virtual ~DelegCred();

    /**
     * Generate a Name for the Certificate Proxy File
     */
    virtual std::string getFileName(const std::string& userDn, const std::string& passphrase) /* throw(LogicError) */;

    /**
     * Get the Cerificate From the MyProxy Server
     */
    virtual void getNewCertificate(const std::string& userDn, const std::string& passphrase, const std::string& filename);

    /**
     * Returns the validity time that the cached copy of the certificate should
     * have.
     */
    virtual unsigned long minValidityTime() /* throw() */;

private:

    /**
     * Encode a name
     */
    std::string encodeName(const std::string& str)/*throw()*/;

};



#endif //CRED_DELEG_DELEGCRED_H_
