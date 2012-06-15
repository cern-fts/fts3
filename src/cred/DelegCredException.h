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

#ifndef CRED_DELEG_DELEGCREDEXCEPTION_H_
#define CRED_DELEG_DELEGCREDEXCEPTION_H_

#include "CredServiceException.h"

    
/**
 * The Exception is thrown when an error happens executing a MyProxyClient method
 */
struct DelegCredException : public CredServiceException {
    /**
     * Constructor
     * @param reason [IN] the reason of the error
     */
    explicit DelegCredException(const std::string& reason):CredServiceException(reason){}

    /**
     * Destructor
     */
    virtual ~DelegCredException() throw() {}
};



#endif //GLITE_DATA_TRANSFER_AGENT_CRED_DELEG_DELEGCREDEXCEPTION_H_
