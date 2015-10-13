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

#ifndef SHAREONLYCFG_H_
#define SHAREONLYCFG_H_

#include "Configuration.h"

namespace fts3
{
namespace ws
{

/**
 * The share-only configuration
 *  it is derived from Configuration
 *
 *  @see Configuration
 */
class ShareOnlyCfg : public Configuration
{

public:

    /**
     * Constructor. Retrieves the configuration data from DB.
     *
     * @param dn - client's DN
     */
    ShareOnlyCfg(std::string dn, std::string name);

    /**
     * Constructor. Retrieves the configuration data from the given CfgParser.
     *
     * @param dn - client's DN
     * @param parser - the JSON configuration parser
     */
    ShareOnlyCfg(std::string dn, CfgParser& parser);

    /**
     * Destructor.
     */
    virtual ~ShareOnlyCfg();

    /**
     * Creates a std::string containing the JSON configuration common for all 'standalone' configurations
     */
    virtual std::string json();

    /**
     * Saves the current configuration into the DB
     */
    virtual void save();

    /**
     * Removes the configuration from the DB
     */
    virtual void del();

    /**
     * Checks if the configuration concerns a single SE or a group
     */
    virtual bool isgroup()
    {
        return false;
    }

private:

    /**
     * Initializes the object from DB
     */
    void init(std::string se);

    /**
     * Make sure that shares sum up to 100%
     */
    void checkShare(std::map<std::string, int>& share);

    /// active state
    bool active;

    /// SE name
    std::string se;

    /// inbound share
    std::map<std::string, int> in_share;

    std::map<std::string, int> out_share;
    /// outbound protocol
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* SHAREONLYCFG_H_ */
