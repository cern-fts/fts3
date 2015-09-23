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

#ifndef DELEGATIONOORCHESTRATOR_H_
#define DELEGATIONOORCHESTRATOR_H_

#include "common/Singleton.h"

#include <boost/thread.hpp>

#include <string>
#include <map>

namespace fts3
{
namespace ws
{

using namespace fts3::common;

/**
 * A DelegationRequestCache orchestrates the proces of proxy certificate delegation
 * in case if two or more clients with the same DN and VO are trying to renew a proxy
 *
 * DelegationOrchestrator is a singleton
 *
 */
class DelegationRequestCache: public Singleton<DelegationRequestCache>
{

    friend class Singleton<DelegationRequestCache>;

public:

    /**
     * The destructor
     */
    virtual ~DelegationRequestCache() {};

    /**
     * Adds new public key request to the cache
     *
     * @param delegationId - delegation ID (unique for a given VO and DN)
     * @param rqst - public key request
     */
    void put(std::string delegationId, std::string rqst)
    {
        pkeys[delegationId] = rqst;
    }

    /**
     * Removes a public key request from the cache
     *
     * @param delegationId - delegation ID (unique for a given VO and DN)
     */
    void remove(std::string delegationId)
    {
        pkeys.erase(delegationId);
    }

    /**
     * Gets the public key request from the cache
     *
     * @param delegationId - delegation ID (unique for a given VO and DN)
     */
    std::string get(std::string delegationId)
    {
        return pkeys[delegationId];
    }

    /**
     * Checks if the public key request is in the cache
     *
     * @param delegationId - delegation ID (unique for a given VO and DN)
     */
    bool check(std::string delegationId)
    {
        return pkeys.find(delegationId) != pkeys.end();
    }

    /**
     * Mutex cast operator, should be used with 'scoped_lock'
     * to lock the cache before any operation is executed
     */
    operator boost::mutex&()
    {
        return m;
    }

private:
    /**
     * Default constructor
     *
     * Private, should not be used
     */
    DelegationRequestCache() {};

    /**
     * Copying constructor
     *
     * Private, should not be used
     */
    DelegationRequestCache(DelegationRequestCache const&);

    /**
     * Assignment operator
     *
     * Private, should not be used
     */
    DelegationRequestCache& operator=(DelegationRequestCache const&);

    /**
     * the map holding a public key to each proxy certificate that is being delegated at the moment
     */
    std::map<std::string, std::string> pkeys;

    boost::mutex m;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* DELEGATIONOORCHESTRATOR_H_ */
