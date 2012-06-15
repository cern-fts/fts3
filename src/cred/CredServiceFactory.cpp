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

#include "glite/data/agents/cred/CredServiceFactory.h"
#include "glite/data/agents/AgentExceptions.h"
#include "glite/data/agents/cred/CredServiceException.h"

using namespace glite::data::agents::cred;

/*
 * Initialize Static Attirbutes
 */
CredServiceFactory * CredServiceFactory::s_instance = 0;
std::map<std::string,CredServiceFactory *> CredServiceFactory::s_instances;

/*
 * instance
 *
 * Return the registered Instance
 */
CredServiceFactory& CredServiceFactory::instance(const std::string& type) /*throw (LogicError)*/ {
    
    if(true == type.empty()){
        if(0 == s_instance){
            throw LogicError("No CredServiceFactory configured");
        }    
        return (*s_instance);
    } else {
        std::map<std::string,CredServiceFactory *>::const_iterator it = s_instances.find(type);
        if(it == s_instances.end()){
            throw LogicError("No CredServiceFactory configured for teh requested type");
        }
        return *((*it).second);
    }
}

/*
 * CredServiceFactory
 *
 * Constructor
 */
CredServiceFactory::CredServiceFactory(){
    // Register the instance
    if(0 == s_instance){
        s_instance = this;
    }
}
        
/*
 * registerFactory
 *
 * register Factory
 */
void CredServiceFactory::registerFactory(const std::string& type){
    // Register the instance
    s_instances.insert(std::map<std::string,CredServiceFactory *>::value_type(type,this));
}

/*
 * deregisterFactory
 *
 * deregister Factory
 */
void CredServiceFactory::deregisterFactory(const std::string& type){
    // Deregister the instance
    std::map<std::string,CredServiceFactory *>::iterator it = s_instances.find(type);
    if(it != s_instances.end()){
        s_instances.erase(it);
    }        
}

/*
 * ~CredServiceFactory
 *
 * Destructor
 */
CredServiceFactory::~CredServiceFactory(){
    if(this != s_instance){
        s_instance = 0;        
    }
}
