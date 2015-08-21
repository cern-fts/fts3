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

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "common/CfgParser.h"

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <vector>
#include <map>

namespace fts3
{
namespace ws
{

using namespace std;
using namespace boost;
using namespace db;
using namespace fts3::common;


/**
 * Configuration is the base class for all configuration types.
 *
 * The class provides an API for configuration handling:
 * - saving configuration into the DB
 * - retrieving the configuration from DB
 * - deleting the configuration from DB
 */
class Configuration
{

public:

    /**
     * Constructor
     *
     * For logging reasons the object needs the user's DN
     */
    Configuration(string dn);

    /**
     * Destructor, it updates the configuration audit traits
     */
    virtual ~Configuration();

    /**
     * Protocol parameters
     */
    struct Protocol
    {
        static const string BANDWIDTH;
        static const string NOSTREAMS;
        static const string TCP_BUFFER_SIZE;
        static const string NOMINAL_THROUGHPUT;
        static const string BLOCKSIZE;
        static const string HTTP_TO;
        static const string URLCOPY_PUT_TO;
        static const string URLCOPY_PUTDONE_TO;
        static const string URLCOPY_GET_TO;
        static const string URLCOPY_GET_DONETO;
        static const string URLCOPY_TX_TO;
        static const string URLCOPY_TXMARKS_TO;
        static const string URLCOPY_FIRST_TXMARK_TO;
        static const string TX_TO_PER_MB;
        static const string NO_TX_ACTIVITY_TO;
        static const string PREPARING_FILES_RATIO;
    };

    /**
     * Returns a configuration in JSON format
     *
     * @return a string with JSON configuration
     */
    virtual string json() = 0;

    /**
     * Saves the current configuration into the DB
     */
    virtual void save() = 0;

    /**
     * Removes the current configuration from the DB
     */
    virtual void del() = 0;

    /**
     * Checks if the configuration concerns a single SE or a group
     */
    virtual bool isgroup() = 0;

    /// the 'any' character used to describe the SE (or SE group) to 'any' relation
    static const string any;
    /// the 'wildcard' string, so called catch-all
    static const string wildcard;
    /// 'on' string
    static const string on;
    /// 'off' string
    static const string off;
    /// the public share
    static const string pub;
    /// 'share_only' string
    static const string share_only;
    /// value of a share pointing that auto should be used
    static const int automatic;

protected:

    /// set of strings that are not allowed as SE or SE group name
    set<string> notAllowed;

    /// Pointer to the 'GenericDbIfce' singleton
    GenericDbIfce* db;

    /**
     * Converts a STL map to JSON configuration string
     *
     * @param params - the parameters to be converted to JSON
     * @return string containing the JSON configuration
     */
    static string json(map<string, int>& params);

    /**
     * Converts a STL map to JSON configuration string
     *
     * @param params - the parameters to be converted to JSON
     * @return string containing the JSON configuration
     */
    static string json(map<string, double>& params);

    /**
     * Converts a STL map to JSON configuration string,
     * 	if the optional is not initialized "auto" value is used
     *
     * @param params - the parameters to be converted to JSON
     * @return string containing the JSON configuration
     */
    static string json(optional< map<string, int> >& params);

    /**
     * Converts a STL vector to JSON configuration string
     *
     * @param members - the vector members to be converted to JSON
     * @return string containing the JSON configuration
     */
    static string json(vector<string>& members);

    /**
     * Gets a map containing the protocol parameter names and the respective values.
     *	If auto-protocol has been used an uninitialized optional is returned.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     *
     * @return map with protocol parameter names and their values
     */
    optional< map<string, int> > getProtocolMap(string source, string destination);

    /**
     * Gets a map containing the protocol parameter names and the respective values
     *
     * @param cfg - link configuration object
     *
     * @return map with protocol parameter names and their values
     */
    optional< map<string, int> > getProtocolMap(LinkConfig* cfg);

    /**
     * Gets a map containing the VO names and the respective share value (for the given source-destination pair).
     *	If auto-protocol has been used an uninitialized optional is returned.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     *
     * @return map with VOs and their share values
     */
    map<string, int> getShareMap(string source, string destination);

    /**
     * Adds a SE to the DB (if not already added)
     *
     * @param se - SE name
     * @param active - the state of the SE (active ('on') by default)
     */
    void addSe(string se, bool active = true);

    /**
     * Changes the SE state to the default one ('on'),
     * 	should be used in case a SE configuration is being removed
     *
     * @param se - SE name
     */
    void eraseSe(string se);

    /**
     * Adds SE group and its members to the DB.
     *
     * @param group - SE group name
     * @param members - SE members of the group
     */
    void addGroup(string group, vector<string>& members);

    /**
     * Checks if the group exists in the DB. Throws an exception if not.
     *
     * @param group - SE group name
     */
    void checkGroup(string group);

    /**
     * Gets link configuration object, and a flag stating whether the object exists in DB or not.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param active - the state
     * @param symbolic_name - the symbolic name describing the link
     */
    pair< std::shared_ptr<LinkConfig>, bool > getLinkConfig(string source, string destination, bool active, string symbolic_name);

    /**
     * Adds a link configuration to the DB.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param active - the state
     * @param symbolic_name - the symbolic name describing the link
     * @param protocol - the protocol parameters and the rrespective values
     */
    void addLinkCfg(string source, string destination, bool active, string symbolic_name, optional< map<string, int> >& protocol);

    /**
     * Adds a share-only link configuration to the DB.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param active - the state
     * @param symbolic_name - the symbolic name describing the link
     * @param protocol - the protocol parameters and the rrespective values
     */
    void addLinkCfg(string source, string destination, bool active, string symbolic_name);

    /**
     * Adds a share configuration to the DB.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param share - VO names and their shares
     */
    void addShareCfg(string source, string destination, map<string, int>& share);

    /**
     * Deletes the link configuration
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     */
    void delLinkCfg(string source, string destination);

    /**
     * Deletes the share configuration
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     */
    void delShareCfg(string source, string destination);

    /// the whole configuration in JSON format
    string all;

    /// number of SQL updates triggered by configuration command
    int updateCount;
    /// number of SQL inserts triggered by configuration command
    int insertCount;
    /// number of SQL deletes triggered by configuration command
    int deleteCount;

private:

    /// client's DN
    string dn;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* CONFIGURATION_H_ */
