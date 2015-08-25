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

#ifndef STANDALONESECFG_H_
#define STANDALONESECFG_H_

#include "StandaloneCfg.h"

#include <string>

namespace fts3
{
namespace ws
{

using namespace fts3::common;

/**
 * The standalone SE configuration
 * 	it is derived from StandaloneCfg
 *
 * @see StandaloneCfg
 */
class StandaloneSeCfg : public StandaloneCfg
{

public:

    /**
     * Constructor.
     *
     * Retrieves the configuration data from the DB for the given SE name.
     *
     * @param dn - client's DN
     * @param name - SE name
     */
    StandaloneSeCfg(std::string dn, std::string name);

    /**
     * Constructor.
     *
     * Retrieves the configuration data from the given CfgParser
     *
     * @param dn - client's DN
     * @param parser - the parser object
     */
    StandaloneSeCfg(std::string dn, CfgParser& parser);

    /**
     * Destructor.
     */
    virtual ~StandaloneSeCfg();

    /**
     * Creates a std::string containing the JSON configuration common for a SE group 'standalone' configurations
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
     * SE name
     */
    std::string se;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONESECFG_H_ */
