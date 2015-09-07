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

#include "ws/config/Configuration.h"

#include "common/OptimizerSample.h"

#include <vector>

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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

    std::vector<std::string> params;
    boost::split(params, file.internalFileParams, boost::is_any_of(","));

    protocol ret;

    for (auto i = params.begin(); i != params.end(); ++i)
        {
            if (boost::starts_with(*i, "nostreams:"))
                {
                    ret.nostreams = boost::lexical_cast<int>(i->substr(10));
                }
            else if (boost::starts_with(*i, "timeout:"))
                {
                    ret.urlcopy_tx_to = boost::lexical_cast<int>(i->substr(8));
                }
            else if (boost::starts_with(*i, "buffersize:"))
                {
                    ret.tcp_buffer_size = boost::lexical_cast<int>(i->substr(11));
                }
            else if (*i == "strict")
                {
                    ret.strict_copy = true;
                }
            else if (*i == "ipv4")
                {
                    ret.ipv6 = false;
                }
            else if (*i == "ipv6")
                {
                    ret.ipv6 = true;
                }
        }

    return ret;
}

void ProtocolResolver::fillAuto(boost::optional<protocol>& source, boost::optional<protocol>& destination)
{

    if (!source && !destination) return;

    protocol auto_prot = autotune();

    // iterate over all protocol parameters
    for (int i = 0; i < protocol::size; i++)
        {
            // source and destination auto flags
            bool src_auto_tuned = false, dst_auto_tuned = false;
            // check the source
            if (source.is_initialized() && (*source)[i] == automatic)
                {
                    // set the flag
                    src_auto_tuned = true;
                    (*source)[i] = auto_prot[i];
                }
            // check the destination
            if (destination.is_initialized() && (*destination)[i] == automatic)
                {
                    // set the flag
                    dst_auto_tuned = true;
                    (*destination)[i] = auto_prot[i];
                }
            // auto_tuned is set to true if:
            // - both source and destination use auto
            // - or source uses auto and destination does not exist
            // - or destination uses auto and source does not exist
            auto_tuned |= (src_auto_tuned && dst_auto_tuned) || (src_auto_tuned && !destination) || (dst_auto_tuned && !source);
        }
}

boost::optional<ProtocolResolver::protocol> ProtocolResolver::merge(boost::optional<protocol> source, boost::optional<protocol> destination)
{
    // replace the 'automatic' marker (-1) with autotuner values
    fillAuto(source, destination);

    // make sure both source and destination protocol exists
    if (!source) return destination;
    if (!destination) return source;

    protocol ret, &src_prot = *source, &dst_prot = *destination;

    // iterate over all protocol parameters
    for (int i = 0; i < protocol::size; i++)
        {
            ret[i] =
                src_prot[i] < dst_prot[i] ?
                src_prot[i] : dst_prot[i]
                ;
        }

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

    // merge the configuration of the most specific standlone source and destination links
    prot = merge(
               getProtocolCfg(source_link),
               getProtocolCfg(destination_link)
           );

    return prot.is_initialized();
}

ProtocolResolver::protocol ProtocolResolver::autotune()
{
    protocol ret;

    std::string source = file.sourceSe;
    std::string destination = file.destSe;

    OptimizerSample opt_config;
    DBSingleton::instance().getDBObjectInstance()->fetchOptimizationConfig2(&opt_config, source, destination);
    ret.tcp_buffer_size = opt_config.getBufSize();
    ret.nostreams = opt_config.getStreamsperFile();
    ret.urlcopy_tx_to = opt_config.getTimeout();

    return ret;
}

bool ProtocolResolver::isAuto()
{
    return auto_tuned;
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

