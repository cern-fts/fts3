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

#ifndef PAIRCFG_H_
#define PAIRCFG_H_

#include "Configuration.h"

#include <string>
#include <map>

#include <boost/optional.hpp>

namespace fts3
{
namespace ws
{

using namespace std;
using namespace boost;
using namespace fts3::common;


/**
 * It is the base class for 'pair' configurations,
 * 	it is derived from Configuration
 *
 * @see Configuration
 */
class PairCfg : public Configuration
{

public:

    /**
     * Constructor. It retrieves configuration data from DB for the given source and destination
     *
     * @param dn - client's DN
     * @param source - the source (SE or SE group)
     * @param destination - the destination (SE or SE group)
     */
    PairCfg(string dn, string source, string destination);

    /**
     * Constructor. It retrieves configuration data from the given CfgParser
     *
     * @param dn - client's DN
     * @param parser - the parser that has been used to parser the JSON configuration
     */
    PairCfg(string dn, CfgParser& parser);

    /**
     * Destructor.
     */
    virtual ~PairCfg();

    /**
     * Creates a string containing the JSON configuration common for all 'pair' configurations
     */
    virtual string json();

    /**
     * Saves the configuration into the DB.
     */
    virtual void save();

    /**
     * Removes the configuration from the DB.
     */
    virtual void del();

protected:
    /// source
    string source;
    /// destination
    string destination;
    /// optional symbolic name
    optional<string> symbolic_name_opt;
    /// symbolic name (given by user or generated)
    string symbolic_name;

private:
    /// active state
    bool active;
    /// the share
    map<string, int> share;
    /// the protocol parameters
    optional< map<string, int> > protocol;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* PAIRCFG_H_ */
