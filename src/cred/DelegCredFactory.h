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

#ifndef CRED_DELEG_DELEGCREDFACTORY_H_
#define CRED_DELEG_DELEGCREDFACTORY_H_

#include "CredServiceFactory.h"
#include "common/logger.h"
#include "common/error.h"

using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;


/**
 * Configure DelegCred library
 */
class DelegCredFactory : CredServiceFactory, 
                         public glite::config::ComponentConfiguration{
public:

    /**
     * Constructor
     */
    DelegCredFactory();
        
    /**
     * Destructor
     */
    virtual ~DelegCredFactory();

    // =========================================================================
    // CertProxyServiceFactory Methods
    // =========================================================================

    /**
     * Return the CredService Type.
     * @return the type of the CredService
     */
    virtual const std::string& getType() /*throw ()*/; 

    /**
     * Return the CertProxy Service Type to be used to query ServiceDiscovery.
     * @return the CertProxy Service Type to be used to query ServiceDiscovery 
     */
    virtual const std::string& getServiceType() /*throw ()*/;

    /**
     * Create a CredService instance
     * @param endpoint [IN] the service hostname and port to contact. If this 
     * parameter is empty, ServiceDiscovery will be used to find the endpoint 
     * of the Server to contact
     * @return a new instance of a CredService
     * @throws LogicError in case of error
     */
    virtual glite::data::agents::cred::CredService * create(const std::string& endpoint) /*throw (LogicError)*/; 

    // =========================================================================
    // ComponentConfiguration Methods
    // =========================================================================

    /**
     * Initialize the Component
     */
    virtual int init(const Params& params);
                                                                                                                           
    /**
     * Configure the Component
     */
    virtual int config(const Params& params);
                                                                                                                           
    /**
     * Start the Component
     */
    virtual int start(){return 0;}
                                                                                                                           
    /**
     * Stop the Component
     */
    virtual int stop(){return 0;}
                                                                                                                           
    /**
     * Finalize the Component
     */
    virtual int fini();

    // =========================================================================
    // Properties
    // =========================================================================
    
    /**
     * Get Minimum Valibity Time for a Proxy Certificate 
     */
    unsigned long minValidityTime() const { return m_minValidityTime;}
    
    /**
     * Get Proxy  Certificates Repository Property
     */
    const std::string& repository() const { return m_repository;}
    
    /**
     * Get the logger object
     */
    const glite::config::Logger& logger() const {return m_logger;}
    
private:
    glite::config::Logger m_logger;
    /**
     * the Deleg Service Type
     */
    std::string m_delegType;

    /**
     * the dummy Service Type 
     */
    std::string m_fakeServiceType;
    
    /**
     * Minimum Valibity Time for a Proxy Certificate 
     */
    unsigned long m_minValidityTime;
    
    /**
     * Proxy Certificates Repository
     */
    std::string m_repository;

    /** @link dependency 
     * @label creates*/
    /*# DelegCred lnkDelegCred; */
};

} // end namespace deleg
} // end namespace cred
} // end namespace agent
} // end namespace transfer
} // end namespace data
} // end namespace glite

#endif //CRED_DELEG_DELEGCREDFACTORY_H_
