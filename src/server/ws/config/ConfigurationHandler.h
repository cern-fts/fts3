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

#ifndef CONFIGURATIONHANDLER_H_
#define CONFIGURATIONHANDLER_H_

#include <string>
#include <vector>
#include <map>
#include <set>

#include <boost/assign.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

#include "common/error.h"
#include "common/logger.h"
#include "common/CfgParser.h"

#include "db/generic/SingleDbInstance.h"

#include "Configuration.h"

namespace fts3
{
namespace ws
{

using namespace boost;
using namespace boost::assign;
using namespace std;
using namespace fts3::common;
using namespace db;


/**
 * The ConfigurationHandler class is used for handling
 * Configuration WebServices' requests
 */
class ConfigurationHandler
{

public:

    /**
     * Constructor
     *
     * initializes the regular expression objects and the 'parameterNameToId' map
     */
    ConfigurationHandler(string dn);

    /**
     * Destructor
     */
    virtual ~ConfigurationHandler();

    /**
     * Parses the configuration string (JSON format)
     * 	Throws an Err_Custom if the given configuration is in wrong format.
     *
     * It has to be called before 'add'!
     *
     * 	@param configuration - string containing the configuration in JSON format
     */
    void parse(string configuration);

    /**
     * Adds a configuration to the DB.
     * 	First the 'parse' method has to be called
     *
     * @see parse
     */
    void add();

    /**
     * Gets the whole configuration regarding all SEs and all SE groups from the DB.
     *
     * @return vector containing single configuration entries in JSON format
     */
    vector<string> get();

    /**
     * Gets the whole configuration regarding all SEs and all SE groups from the DB.
     * 	The configuration can also be restricted to a given SE and/or VO.
     *
     * @param se - SE name
     *
     * @return vector containing single configuration entries in JSON format
     */
    string get(string name);

    /**
     * Gets the whole configuration regarding all SEs and all SE groups from the DB.
     * 	The configuration can also be restricted to a given SE and/or VO.
     *
     * @param se - SE name
     *
     * @return vector containing single configuration entries in JSON format
     */
    vector<string> getAll(string name);

    /**
     *
     */
    string getPair(string src, string dest);

    /**
     *
     */
    string getPair(string symbolic);

    /**
     *
     */
    string getVo(string vo);

    /**
     * Deletes the configuration specified by the argument
     * 	Only share and protocol specific configurations will be deleted.
     *
     * @param cfg - the configuration that should be deleted
     */
    void del ();

private:

    /// Pointer to the 'GenericDbIfce' singleton
    GenericDbIfce* db;

    /// user DN
    string dn;

    scoped_ptr<Configuration> cfg;
};

}
}

#endif /* TEST_H_ */
