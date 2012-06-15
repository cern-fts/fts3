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

#include "DelegCredFactory.h"
#include "DelegCred.h"
#include "DelegCredException.h"
#include <glite/data/config/service/ParamReader.hxx>
#include <glite/data/agents/AgentExceptions.h>
#include <glite/data/agents/sd/sd-utility.h>
#include <glite/data/agents/url-utility.h>
#include <glite/data/agents/sd/SDConfig.h>

// Fix Warning
#ifdef IOV_MAX 
#undef IOV_MAX
#endif //IOV_MAX
#include "globus_gsi_credential.h"

#include <boost/scoped_ptr.hpp>

using namespace glite::data::agents;
using namespace glite::data::agents::cred;
using namespace glite::data::transfer::agent::cred::deleg;
using namespace glite::config;
using boost::scoped_ptr;

/*
 * Constants
 */
namespace {
const char * const DELEG_CRED_NAME               = "cred-deleg";
                                                                                                                          
// Properties Names
const char * const MINVALIDITYTIME_PROPERTY_NAME = "MinValidityTime";
const char * const REPOSITORY_PROPERTY_NAME      = "Repository";
                                                                                                                          
// Properties Default Values
const unsigned long DEFAULT_MINVALIDITYTIME = 60 * 60;      // 1 hour
const char * const DEFAULT_REPOSITORY            = "/tmp/";
    
const char * const DELEG_CREDSERVICETYPE          = "deleg";
const char * const DELEG_FAKE_SERVICETYPE         = "(none)";
}

extern "C" {
/*
 * create_glite_component
 *
 * Create Component instance
 */
ComponentConfiguration * create_glite_component(){
    return new DelegCredFactory();
}
                                                                                
/*
 * destroy_glite_component
 *
 * Destroy Component instance
 */
void destroy_glite_component(ComponentConfiguration * component){
    DelegCredFactory * factory = dynamic_cast<DelegCredFactory*>(component);
    if(0 != factory){
        delete factory;
    }
}
                                                                                
} // End extern "C"
    
/*
 * DelegCredFactory
 *
 * Constructor
 */
DelegCredFactory::DelegCredFactory():CredServiceFactory(),
        ComponentConfiguration(DELEG_CRED_NAME), 
        m_logger(DELEG_CRED_NAME),
        m_delegType(DELEG_CREDSERVICETYPE),
        m_fakeServiceType(DELEG_FAKE_SERVICETYPE),
        m_minValidityTime(DEFAULT_MINVALIDITYTIME),
        m_repository(DEFAULT_REPOSITORY){
    this->registerFactory(DELEG_CREDSERVICETYPE);
}
                                                                                                                          
/*
 * ~DelegCredConfig
 *
 * Destructor
 */
DelegCredFactory::~DelegCredFactory(){
    this->deregisterFactory(DELEG_CREDSERVICETYPE);
}

/*
 * init
 *
 * Initialize the Component
 */
int DelegCredFactory::init(const Params& params){
                                                                                                                          
    try{
        ParamReader reader(getName(),params);

        // Get Min Validity time
        reader.getParameter(MINVALIDITYTIME_PROPERTY_NAME,m_minValidityTime);

        // Get Proxy Repository
        reader.getParameter(REPOSITORY_PROPERTY_NAME,m_repository);
        // Check That Repository is Valid
        DIR * dir = opendir(m_repository.c_str());
        if(0 == dir){
            m_log_error("Initialization parameter " << REPOSITORY_PROPERTY_NAME << " specifies invalid directory " << m_repository);
            return -1;
        } else {
            closedir(dir);
        }
        // Check thet repository end with a slash
        if('/' != *(m_repository.rbegin())){
            m_repository.push_back('/');
        }
        
        // Log Configuration Values
        m_log_info(getName() << " Initialized. Initialization Parameters are");
        m_log_info("MinValidityTime : " << m_minValidityTime);
        m_log_info("Repository      : " << m_repository);
        
        // Activate Globus GSI Credential Module
        globus_module_activate(GLOBUS_GSI_CREDENTIAL_MODULE);    
    } catch(const LogicError& exc) {
        m_log_error(getName() << " configuration Error: " << exc.what());
        return -1;
    } catch(const std::exception& exc) {
        m_log_error(getName() << " configuration Unexpected Exception: " << exc.what());
        return -1;
    } catch(...) {
        m_log_error(getName() << " configuration Unhandled Exception");
        return -1;
    }
    return 0;
}

/*
 * config
 *
 * Configure the Component
 */
int DelegCredFactory::config(const Params& params){
    // Nothing to do
    return 0;
}

/*
 * fini
 *
 * Finalize the Component
 */
int DelegCredFactory::fini(){
    // Deativate Globus GSI Credential Module
    globus_module_deactivate(GLOBUS_GSI_CREDENTIAL_MODULE);

    return 0;
}

/*
 * getTag
 *
 * Return the tag to be used to search the myproxy server in the job param
 */
const std::string& DelegCredFactory::getType() /*throw ()*/{
    return m_delegType;    
}

/*
 * getServiceType
 *
 * Return the Cred Service Type to be used to query ServiceDiscovery.
 */
const std::string& DelegCredFactory::getServiceType() /*throw ()*/{
    return m_fakeServiceType;
}


/*
 * create
 *
 * Create a CredService instance
 */
CredService * DelegCredFactory::create(const std::string& endpoint) /*throw (LogicError)*/{
    // Return new Instance
    return new DelegCred(*this);
}
