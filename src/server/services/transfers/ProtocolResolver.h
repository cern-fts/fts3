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

#ifndef PROTOCOLRESOLVER_H_
#define PROTOCOLRESOLVER_H_

#include "common/definitions.h"
#include "db/generic/SingleDbInstance.h"

#include <list>
#include <string>
#include <utility>

#include <boost/logic/tribool.hpp>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

namespace fts3 {
namespace server {

using namespace db;

/**
 * The class aims to resolve the protocol parameters for a given transfer job.
 */
class ProtocolResolver
{

public:

    /**
     * protocol data:
     * - number of streams
     * - TCP buffer size
     * - url-copy timeout
     */
    struct protocol
    {
        protocol() : nostreams(DEFAULT_NOSTREAMS), tcp_buffer_size(DEFAULT_BUFFSIZE), urlcopy_tx_to(DEFAULT_TIMEOUT),
                     strict_copy(false)
        { }

        int nostreams;
        int tcp_buffer_size;
        int urlcopy_tx_to;
        bool strict_copy;
        boost::tribool ipv6;
    };

    /**
     * The link type
     */
    enum LinkType
    {
        SE_PAIR = 0, //< SE pair
        GROUP_PAIR, //< SE group pair
        SOURCE_SE, //< standalonoe source SE
        SOURCE_GROUP, //< standalone source SE group
        SOURCE_WILDCARD, //< standalone default source SE
        DESTINATION_SE, //< standalone destination SE
        DESTINATION_GROUP, //< standalone destination SE group
        DESTINATION_WILDCARD //< standalone default destination SE
    };

    /**
     * triplet that uniquely defines share configuration
     * such triplets are assigned to transfer-job at submission time
     */
    enum
    {
        SOURCE = 0,
        DESTINATION,
        VO
    };

    /**
     * Constructor.
     *
     * Loads the configurations assigned to the transfer job from the DB.
     * Adds respective entries to the link table.
     *
     * @param job_id - transfer job ID
     */
    ProtocolResolver(TransferFile const & file, std::vector< std::shared_ptr<ShareConfig> >& cfgs);
    ProtocolResolver(const fts3::server::ProtocolResolver&);

    /**
     * Destructor.
     */
    ~ProtocolResolver();

    /**
     * Resolves the protocol parameters.
     *
     * @return an object containing protocol parameters (the memory has to be released by the user)
     */
    bool resolve();

    /**
     * gets the number of streams that should be used
     *
     * @return number of streams
     */
    int getNoStreams();

    /**
     * gets TCP buffer size
     *
     * @return TCP buffer size
     */
    int getTcpBufferSize();

    /**
     * gets url-copy timeout
     *
     * @return url-copy timeout
     */
    int getUrlCopyTxTo();

    /**
     * returns if strict copy is set
     */
    bool getStrictCopy();

    /**
     * returns if ipv6 is set
     */
    bool getIPv6();

    /**
     * Gets the user defined protocol parameters (submitted with the job)
     *
     * @return an object containing protocol parameters
     */
    static boost::optional<protocol> getUserDefinedProtocol(TransferFile const & file);

private:


    /**
     * Checks if given entity is a SE group
     *
     * @param entity name
     *
     * @return true if the entity is a group, false otherwise
     */
    bool isGr(std::string name);

    /**
     * Gets the first object from the link sublist that was initialized.
     *
     * @param l - sublist of link array
     *
     * @return first initialized object in the sublist, or an uninitialized object if non was found
     */
    boost::optional< std::pair<std::string, std::string> > getFirst(std::list<LinkType> l);

    /**
     * Gets the protocol parameters for the given link
     *
     * @param link - source and destination pair
     *
     * @return an object containing protocol parameters
     */
    boost::optional<protocol> getProtocolCfg(boost::optional< std::pair<std::string, std::string> > link);

    /**
     * Merges two sets of protocol parameters.
     *
     * @param source_ptr - the protocol parameters of the source link
     * @param destination_ptr - the protocol parameters of the destination link
     *
     * @return an object containing protocol parameters (the memory has to be released by the user)
     */
    boost::optional<protocol> merge(boost::optional<protocol> source, boost::optional<protocol> destination);

    /// DB singleton instance
    GenericDbIfce* db;

    /// array containing respective source-destination pairs (corresponds to the LinkType enumeration)
    boost::optional< std::pair<std::string, std::string> > link[8];

    /// stores the protocol parameters that have been resolved
    boost::optional<protocol> prot;

    // the transfer file
    TransferFile const & file;

    std::vector< std::shared_ptr<ShareConfig> >& cfgs;

    /// -1 indicates that for the given protocol parameter the value obtained from auto-tuner should be used
    static const int automatic = -1;

    /// true if auto tuner was used
    bool auto_tuned;
};

} // end namespace server
} // end namespace fts3

#endif /* PROTOCOLRESOLVER_H_ */
