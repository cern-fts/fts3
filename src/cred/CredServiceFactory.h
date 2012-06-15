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

#ifndef GLITE_DATA_AGENTS_CRED_CREDSERVICEFACTORY_H_
#define GLITE_DATA_AGENTS_CRED_CREDSERVICEFACTORY_H_

#include "glite/data/agents/cred/CredServiceException.h"
#include "glite/data/agents/cred/CredService.h"
#include "glite/data/agents/sd/sd-utility.h"

#include <string>
#include <map>

namespace glite {
namespace data  {
namespace agents{
namespace cred  {

/**
 * CredServiceFactory interface
 * @interface
 */
class CredServiceFactory {
public:

    /**
     * Return the CredService Type.
     * @return the type of the CredService
     */
    virtual const std::string& getType() /*throw ()*/ = 0;

    /**
     * Return the CredService Service Type to be used to query ServiceDiscovery.
     * @return the CredService Service Type to be used to query ServiceDiscovery 
     */
    virtual const std::string& getServiceType() /*throw ()*/ = 0;

    /**
     * Return the SD Selector predicate for choosing the correct CredService Service
     * when more than one are available
     * @return the SD Service Selector. 0 means use the default
     */
    virtual sd::SelectPred * getServiceSelector(const std::string& vo_name) /*throw()*/ {return 0;}

    /**
     * Create a CredService instance
     * @param endpoint [IN] the service hostname and port to contact. If this 
     * parameter is empty, ServiceDiscovery will be used to find the endpoint 
     * of the Server to contact
     * @return a new instance of a CredService
     * @throws LogicError in case of error
     */
    virtual CredService * create(const std::string& endpoint) /*throw (LogicError)*/ = 0; 

    /**
     * Return the configured CredServiceFactory Instance. Please note that 
     * this method doesn't allocate the instance (it's not a singleton)
     * @return the configued CredServiceFactory 
     * @throws LogicError in case of no CredServiceFactory is configured
     */
    static CredServiceFactory& instance(const std::string& type = "") /*throw (LogicError)*/; 
   
protected:

    /**
     * Constructor
     */ 
    CredServiceFactory();

    /**
     * register a Factory
     */ 
    void registerFactory(const std::string& type);

    /**
     * deregister a Factory
     */ 
    void deregisterFactory(const std::string& type);

    /**
     * Destructor
     */ 
    virtual ~CredServiceFactory();

private:

    /**
     * The default Instance
     */
    static CredServiceFactory * s_instance;

    /**
     * The registered Instance
     */
    static std::map<std::string, CredServiceFactory *> s_instances;

    /** @link dependency 
     * @label creates*/
    /*# CredService lnkCredService; */

    /** @link dependency 
     * @label throws*/
    /*# CredServiceException lnkCredServiceException; */
};

} // end namespace cred
} // end namespace agents
} // end namespace data
} // end namespace glite

#endif //GLITE_DATA_AGENTS_CRED_CREDSERVICEFACTORY_H_
