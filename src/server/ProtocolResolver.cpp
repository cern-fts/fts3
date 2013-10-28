/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ProtocolResolver.cpp
 *
 *  Created on: Dec 3, 2012
 *      Author: Michal Simon
 */

#include "ProtocolResolver.h"

#include "ws/config/Configuration.h"

#include "common/OptimizerSample.h"

#include <vector>

#include <boost/assign/list_of.hpp>

FTS3_SERVER_NAMESPACE_START

using namespace fts3::ws;
using namespace fts3::common;
using namespace boost::assign;

ProtocolResolver::ProtocolResolver(TransferFiles* file, vector< boost::shared_ptr<ShareConfig> >& cfgs) :
    db(DBSingleton::instance().getDBObjectInstance()),
    file(file),
    cfgs(cfgs),
    auto_tuned(false)
{

    vector< boost::shared_ptr<ShareConfig> >::iterator it;

    // loop over the assigned configurations
    for (it = cfgs.begin(); it != cfgs.end(); ++it)
        {

            // get the source and destination
            string source = (*it)->source;
            string destination = (*it)->destination;
            // create source-destination pair
            pair<string, string> entry = pair<string, string>(source, destination);

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

bool ProtocolResolver::isGr(string name)
{
    return db->checkGroupExists(name);
}

optional<ProtocolResolver::protocol> ProtocolResolver::getProtocolCfg(optional< pair<string, string> > link)
{
    if (!link) return optional<protocol>();

    string source = (*link).first;
    string destination = (*link).second;

    boost::shared_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    protocol ret;

    // set number of streams
    ret.nostreams = cfg->NOSTREAMS;

    // not used for now but set anyway
    ret.no_tx_activity_to = cfg->NO_TX_ACTIVITY_TO;

    // set TCP buffer size
    ret.tcp_buffer_size = cfg->TCP_BUFFER_SIZE;

    // set the timeout
    ret.urlcopy_tx_to = cfg->URLCOPY_TX_TO;
    
    if(cfg->auto_tuning == "on")
	auto_tuned = true;    

    return ret;
}

optional<ProtocolResolver::protocol> ProtocolResolver::merge(optional<protocol> source, optional<protocol> destination)
{
    // make sure both source and destination protocol exists
    if (!source) return destination;
    if (!destination) return source;

    protocol ret, &src_prot = *source, &dst_prot = *destination, auto_prot = autotune();

    // iterate over all protocol parameters
    for (int i = 0; i < protocol::size; i++)
        {
            // if both are automatic use the automatic value
            if (src_prot[i] == automatic && dst_prot[i] == automatic)
                {
                    ret[i] = auto_prot[i];
                    auto_tuned = true;
                }
            // if only source uses automatic merge automatic value with destination
            else if (src_prot[i] == automatic)
                {
                    if (auto_prot[i] < dst_prot[i])
                        {
                            ret[i] = auto_prot[i];
                            auto_tuned = true;
                        }
                    else
                        {
                            ret[i] = dst_prot[i];
                        }
                }
            // if only destination uses automatic merge source with automatic value
            else if (dst_prot[i] == automatic)
                {
                    if (auto_prot[i] < src_prot[i])
                        {
                            ret[i] = auto_prot[i];
                            auto_tuned = true;
                        }
                    else
                        {
                            ret[i] = src_prot[i];
                        }
                }
            // otherwise automatic is not used at all so merge source with destination
            else
                {
                    ret[i] =
                        src_prot[i] < dst_prot[i] ?
                        src_prot[i] : dst_prot[i]
                        ;
                }
        }

    return ret;
}

optional< pair<string, string> > ProtocolResolver::getFirst(list<LinkType> l)
{
    // look for the first link
    list<LinkType>::iterator it;
    for (it = l.begin(); it != l.end(); ++it)
        {
            // return the first existing link
            if (link[*it]) return link[*it];
        }
    // if nothing was found return empty link
    return optional< pair<string, string> >();
}

bool ProtocolResolver::resolve()
{

    // check if there's a SE pair configuration
    prot = getProtocolCfg(link[SE_PAIR]);

    if (prot.is_initialized()) return true;

    // check if there is a SE group pair configuration
    prot = getProtocolCfg(link[GROUP_PAIR]);
    if (prot.is_initialized()) return true;

    // get the first existing standalone source link from the list
    optional< pair<string, string> > source_link = getFirst(
                list_of (SOURCE_SE) (SOURCE_GROUP) (SOURCE_WILDCARD)
            );
    // get the first existing standalone destination link from the list
    optional< pair<string, string> > destination_link = getFirst(
                list_of (DESTINATION_SE) (DESTINATION_GROUP) (DESTINATION_WILDCARD)
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

    string source = file->SOURCE_SE;
    string destination = file->DEST_SE;

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

int ProtocolResolver::getNoTxActiveTo()
{
    return (*prot).no_tx_activity_to;
}

int ProtocolResolver::getTcpBufferSize()
{
    return (*prot).tcp_buffer_size;
}

int ProtocolResolver::getUrlCopyTxTo()
{
    return (*prot).urlcopy_tx_to;
}

FTS3_SERVER_NAMESPACE_END
