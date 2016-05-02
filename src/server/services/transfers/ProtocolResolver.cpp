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

#include "ProtocolResolver.h"

#include "common/definitions.h"
#include "server/services/webservice/ws/config/Configuration.h"

#include <boost/assign/list_of.hpp>

using namespace fts3::server;
using namespace fts3::ws;
using namespace fts3::common;
using namespace boost::assign;

ProtocolResolver::ProtocolResolver(TransferFile const & file, std::vector< std::shared_ptr<ShareConfig> >& cfgs) :
    db(DBSingleton::instance().getDBObjectInstance()),
    file(file),
    cfgs(cfgs),
    auto_tuned(false)
{

    std::vector< std::shared_ptr<ShareConfig> >::iterator it;

    // loop over the assigned configurations
    for (it = cfgs.begin(); it != cfgs.end(); ++it)
        {

            // get the source and destination
            std::string source = (*it)->source;
            std::string destination = (*it)->destination;
            // create source-destination pair
            std::pair<std::string, std::string> entry = std::make_pair(source, destination);

            // check if it is default configuration for destination SE
            if (destination == Configuration::wildcard && source == Configuration::any)
                {
                    link[DESTINATION_WILDCARD] = entry;
                    continue;
                }
            // check if it is default configuration for source SE
            if (source == Configuration::wildcard && destination == Configuration::any)
                {
                    link[SOURCE_WILDCARD] = entry;
                    continue;
                }

            // check if we are dealing with groups or SEs
            if (isGr(source) || isGr(destination))
                {
                    // check if it's standalone group configuration of the destination
                    if (destination != Configuration::any && source == Configuration::any)
                        {
                            link[DESTINATION_GROUP] = entry;
                            continue;
                        }
                    // check if it's standalone group configuration of the source
                    if (source != Configuration::any && destination == Configuration::any)
                        {
                            link[SOURCE_GROUP] = entry;
                            continue;
                        }
                    // if it's neither of the above it has to be a pair
                    link[GROUP_PAIR] = entry;

                }
            else
                {
                    // check if it's standalone SE configuration of the destination
                    if (destination != Configuration::any && source == Configuration::any)
                        {
                            link[DESTINATION_SE] = entry;
                            continue;
                        }
                    // check if it's standalone SE configuration of the source
                    if (source != Configuration::any && destination == Configuration::any)
                        {
                            link[SOURCE_SE] = entry;
                            continue;
                        }
                    // if it's neither of the above it has to be a pair
                    link[SE_PAIR] = entry;
                }
        }
}

ProtocolResolver::~ProtocolResolver()
{

}

bool ProtocolResolver::isGr(std::string name)
{
    return db->checkGroupExists(name);
}

boost::optional<ProtocolResolver::protocol> ProtocolResolver::getProtocolCfg(boost::optional< std::pair<std::string, std::string> > link)
{
    if (!link) return boost::optional<protocol>();

    std::string source = (*link).first;
    std::string destination = (*link).second;

    std::shared_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    protocol ret;

    // set number of streams
    ret.nostreams = cfg->numberOfStreams;

    // set TCP buffer size
    ret.tcp_buffer_size = cfg->tcpBufferSize;

    // set the timeout
    ret.urlcopy_tx_to = cfg->transferTimeout;

    if(cfg->autoTuning == "on")
        auto_tuned = true;

    return ret;
}

boost::optional<ProtocolResolver::protocol> ProtocolResolver::getUserDefinedProtocol(TransferFile const & file)
{
    if (file.internalFileParams.empty()) return boost::optional<protocol>();

    auto params = file.getProtocolParameters();
    protocol ret;

    ret.ipv6 = params.ipv6;
    ret.nostreams = params.nostreams;
    ret.strict_copy = params.strictCopy;
    ret.tcp_buffer_size = params.buffersize;
    ret.urlcopy_tx_to = params.timeout;

    return ret;
}

boost::optional<ProtocolResolver::protocol> ProtocolResolver::merge(boost::optional<protocol> source, boost::optional<protocol> destination)
{
    // make sure both source and destination protocol exists
    if (!source) return destination;
    if (!destination) return source;

    protocol ret, &src_prot = *source, &dst_prot = *destination;

    ret.nostreams = std::min(src_prot.nostreams, dst_prot.nostreams);
    ret.tcp_buffer_size = std::min(src_prot.tcp_buffer_size, dst_prot.tcp_buffer_size);
    ret.urlcopy_tx_to = std::max(src_prot.urlcopy_tx_to, dst_prot.urlcopy_tx_to);
    ret.strict_copy = (src_prot.strict_copy || dst_prot.strict_copy);
    ret.ipv6 = (src_prot.ipv6 || dst_prot.ipv6);

    return ret;
}

boost::optional< std::pair<std::string, std::string> > ProtocolResolver::getFirst(std::list<LinkType> l)
{
    // look for the first link
    for (auto it = l.begin(); it != l.end(); ++it)
        {
            // return the first existing link
            if (link[*it]) return link[*it];
        }
    // if nothing was found return empty link
    return boost::optional< std::pair<std::string, std::string> >();
}

bool ProtocolResolver::resolve()
{
    // check if the user specified the protocol parameters while submitting
    prot = getUserDefinedProtocol(file);
    if (prot.is_initialized()) return true;

    // check if there's a SE pair configuration
    prot = getProtocolCfg(link[SE_PAIR]);
    if (prot.is_initialized()) return true;

    // check if there is a SE group pair configuration
    prot = getProtocolCfg(link[GROUP_PAIR]);
    if (prot.is_initialized()) return true;

    // get the first existing standalone source link from the list
    boost::optional< std::pair<std::string, std::string> > source_link = getFirst(
                boost::assign::list_of (SOURCE_SE) (SOURCE_GROUP) (SOURCE_WILDCARD)
            );
    // get the first existing standalone destination link from the list
    boost::optional< std::pair<std::string, std::string> > destination_link = getFirst(
                boost::assign::list_of (DESTINATION_SE) (DESTINATION_GROUP) (DESTINATION_WILDCARD)
            );

    // merge the configuration of the most specific standalone source and destination links
    prot = merge(
               getProtocolCfg(source_link),
               getProtocolCfg(destination_link)
           );

    return prot.is_initialized();
}

int ProtocolResolver::getNoStreams()
{
    return (*prot).nostreams;
}

int ProtocolResolver::getTcpBufferSize()
{
    return (*prot).tcp_buffer_size;
}

int ProtocolResolver::getUrlCopyTxTo()
{
    return (*prot).urlcopy_tx_to;
}

bool ProtocolResolver::getStrictCopy()
{
    return (*prot).strict_copy;
}

bool ProtocolResolver::getIPv6()
{
    return (*prot).ipv6;
}

