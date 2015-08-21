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

#ifndef STANDALONECFG_H_
#define STANDALONECFG_H_

#include "Configuration.h"

#include <string>
#include <map>

namespace fts3
{
namespace ws
{

using namespace std;
using namespace fts3::common;

/**
 * The base class for all 'standalone' configuration,
 * 	it's derived from COnfiguration
 *
 * 	@see Configuration
 */
class StandaloneCfg : public Configuration
{

public:

    /**
     * Constructor. Retrieves the configuration data from DB.
     *
     * @param dn - client's DN
     */
    StandaloneCfg(string dn) : Configuration(dn), active(true) {}

    /**
     * Constructor. Retrieves the configuration data from the given CfgParser.
     *
     * @param dn - client's DN
     * @param parser - the JSON configuration parser
     */
    StandaloneCfg(string dn, CfgParser& parser);

    /**
     * Destructor.
     */
    virtual ~StandaloneCfg();

    /**
     * Creates a string containing the JSON configuration common for all 'standalone' configurations
     */
    virtual string json();

    /**
     * Saves the current configuration into the DB
     */
    virtual void save() = 0;

    /**
     * Removes the configuration from the DB
     */
    virtual void del() = 0;

protected:

    /**
     * Saves the standalone configuration
     *
     * @param name - SE or SE group name
     */
    virtual void save(string name);

    /**
     * Removes the standalone configuration from DB
     */
    virtual void del(string name);

    /**
     * Initializes the in/out shares and protocol parameters
     *
     * @param name - SE or SE group name
     */
    void init(string name);

    /// active state
    bool active;

private:

    /// inbound share
    map<string, int> in_share;
    /// inbound protocol
    optional< map<string, int> > in_protocol;

    /// outbound share
    map<string, int> out_share;
    /// outbound protocol
    optional< map<string, int> > out_protocol;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONESECFG_H_ */
