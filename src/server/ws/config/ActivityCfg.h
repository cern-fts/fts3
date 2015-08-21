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

#ifndef ACTIVITYCFG_H_
#define ACTIVITYCFG_H_

#include "Configuration.h"

#include <string>
#include <map>

namespace fts3
{
namespace ws
{



class ActivityCfg  : public Configuration
{
public:

    ActivityCfg(string dn, string name);

    ActivityCfg(string dn, CfgParser& parser);

    virtual ~ActivityCfg();

    /**
     * Creates a string containing the JSON configuration common for all 'standalone' configurations
     */
    virtual string json();

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
    void init(string vo);

    string vo;
    map<string, double> shares;
    bool active;

};

} /* namespace server */
} /* namespace fts3 */
#endif /* ACTIVITYCFG_H_ */
