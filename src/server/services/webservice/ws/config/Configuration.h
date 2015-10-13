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
    Configuration(std::string dn);

    /**
     * Destructor, it updates the configuration audit traits
     */
    virtual ~Configuration();

    /**
     * Protocol parameters
     */
    struct Protocol
    {
        static const std::string BANDWIDTH;
        static const std::string NOSTREAMS;
        static const std::string TCP_BUFFER_SIZE;
        static const std::string NOMINAL_THROUGHPUT;
        static const std::string BLOCKSIZE;
        static const std::string HTTP_TO;
        static const std::string URLCOPY_PUT_TO;
        static const std::string URLCOPY_PUTDONE_TO;
        static const std::string URLCOPY_GET_TO;
        static const std::string URLCOPY_GET_DONETO;
        static const std::string URLCOPY_TX_TO;
        static const std::string URLCOPY_TXMARKS_TO;
        static const std::string URLCOPY_FIRST_TXMARK_TO;
        static const std::string TX_TO_PER_MB;
        static const std::string NO_TX_ACTIVITY_TO;
        static const std::string PREPARING_FILES_RATIO;
    };

    /**
     * Returns a configuration in JSON format
     *
     * @return a std::string with JSON configuration
     */
    virtual std::string json() = 0;

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
    static const std::string any;
    /// the 'wildcard' std::string, so called catch-all
    static const std::string wildcard;
    /// 'on' std::string
    static const std::string on;
    /// 'off' std::string
    static const std::string off;
    /// the public share
    static const std::string pub;
    /// 'share_only' std::string
    static const std::string share_only;
    /// value of a share pointing that auto should be used
    static const int automatic;

protected:

    /// Pointer to the 'GenericDbIfce' singleton
    GenericDbIfce* db;

    /// set of std::strings that are not allowed as SE or SE group name
    std::set<std::string> notAllowed;

    /**
     * Converts a STL map to JSON configuration std::string
     *
     * @param params - the parameters to be converted to JSON
     * @return std::string containing the JSON configuration
     */
    static std::string json(std::map<std::string, int>& params);

    /**
     * Converts a STL map to JSON configuration std::string
     *
     * @param params - the parameters to be converted to JSON
     * @return std::string containing the JSON configuration
     */
    static std::string json(std::map<std::string, double>& params);

    /**
     * Converts a STL map to JSON configuration std::string,
     *  if the optional is not initialized "auto" value is used
     *
     * @param params - the parameters to be converted to JSON
     * @return std::string containing the JSON configuration
     */
    static std::string json(boost::optional< std::map<std::string, int> >& params);

    /**
     * Converts a STL vector to JSON configuration std::string
     *
     * @param members - the vector members to be converted to JSON
     * @return std::string containing the JSON configuration
     */
    static std::string json(std::vector<std::string>& members);

    /**
     * Gets a map containing the protocol parameter names and the respective values.
     *  If auto-protocol has been used an uninitialized optional is returned.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     *
     * @return map with protocol parameter names and their values
     */
    boost::optional< std::map<std::string, int> > getProtocolMap(std::string source, std::string destination);

    /**
     * Gets a map containing the protocol parameter names and the respective values
     *
     * @param cfg - link configuration object
     *
     * @return map with protocol parameter names and their values
     */
    boost::optional< std::map<std::string, int> > getProtocolMap(LinkConfig* cfg);

    /**
     * Gets a map containing the VO names and the respective share value (for the given source-destination pair).
     *  If auto-protocol has been used an uninitialized optional is returned.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     *
     * @return map with VOs and their share values
     */
    std::map<std::string, int> getShareMap(std::string source, std::string destination);

    /**
     * Adds a SE to the DB (if not already added)
     *
     * @param se - SE name
     * @param active - the state of the SE (active ('on') by default)
     */
    void addSe(std::string se, bool active = true);

    /**
     * Changes the SE state to the default one ('on'),
     *  should be used in case a SE configuration is being removed
     *
     * @param se - SE name
     */
    void eraseSe(std::string se);

    /**
     * Adds SE group and its members to the DB.
     *
     * @param group - SE group name
     * @param members - SE members of the group
     */
    void addGroup(std::string group, std::vector<std::string>& members);

    /**
     * Checks if the group exists in the DB. Throws an exception if not.
     *
     * @param group - SE group name
     */
    void checkGroup(std::string group);

    /**
     * Gets link configuration object, and a flag stating whether the object exists in DB or not.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param active - the state
     * @param symbolic_name - the symbolic name describing the link
     */
    std::pair< LinkConfig, bool > getLinkConfig(std::string source, std::string destination, bool active, std::string symbolic_name);

    /**
     * Adds a link configuration to the DB.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param active - the state
     * @param symbolic_name - the symbolic name describing the link
     * @param protocol - the protocol parameters and the rrespective values
     */
    void addLinkCfg(std::string source, std::string destination, bool active, std::string symbolic_name, boost::optional< std::map<std::string, int> >& protocol);

    /**
     * Adds a share-only link configuration to the DB.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param active - the state
     * @param symbolic_name - the symbolic name describing the link
     * @param protocol - the protocol parameters and the rrespective values
     */
    void addLinkCfg(std::string source, std::string destination, bool active, std::string symbolic_name);

    /**
     * Adds a share configuration to the DB.
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     * @param share - VO names and their shares
     */
    void addShareCfg(std::string source, std::string destination, std::map<std::string, int>& share);

    /**
     * Deletes the link configuration
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     */
    void delLinkCfg(std::string source, std::string destination);

    /**
     * Deletes the share configuration
     *
     * @param source - the source (SE, SE group or 'any')
     * @param destination - the destination (SE, SE group or 'any')
     */
    void delShareCfg(std::string source, std::string destination);

    /// the whole configuration in JSON format
    std::string all;

    /// number of SQL updates triggered by configuration command
    int updateCount;
    /// number of SQL inserts triggered by configuration command
    int insertCount;
    /// number of SQL deletes triggered by configuration command
    int deleteCount;

private:

    /// client's DN
    std::string dn;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* CONFIGURATION_H_ */
