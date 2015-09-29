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

#include "Configuration.h"
#include "common/definitions.h"

#include <set>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>
#include "../../../../../common/Exceptions.h"

namespace fts3
{
namespace ws
{


const std::string Configuration::Protocol::BANDWIDTH = "bandwidth";
const std::string Configuration::Protocol::NOSTREAMS = "nostreams";
const std::string Configuration::Protocol::TCP_BUFFER_SIZE = "tcp_buffer_size";
const std::string Configuration::Protocol::NOMINAL_THROUGHPUT = "nominal_throughput";
const std::string Configuration::Protocol::BLOCKSIZE = "blocksize";
const std::string Configuration::Protocol::HTTP_TO = "http_to";
const std::string Configuration::Protocol::URLCOPY_PUT_TO = "urlcopy_put_to";
const std::string Configuration::Protocol::URLCOPY_PUTDONE_TO = "urlcopy_putdone_to";
const std::string Configuration::Protocol::URLCOPY_GET_TO = "urlcopy_get_to";
const std::string Configuration::Protocol::URLCOPY_GET_DONETO = "urlcopy_get_doneto";
const std::string Configuration::Protocol::URLCOPY_TX_TO = "urlcopy_tx_to";
const std::string Configuration::Protocol::URLCOPY_TXMARKS_TO = "urlcopy_txmarks_to";
const std::string Configuration::Protocol::URLCOPY_FIRST_TXMARK_TO = "urlcopy_first_txmark_to";
const std::string Configuration::Protocol::TX_TO_PER_MB = "tx_to_per_mb";
const std::string Configuration::Protocol::NO_TX_ACTIVITY_TO = "no_tx_activity_to";
const std::string Configuration::Protocol::PREPARING_FILES_RATIO = "preparing_files_ratio";

const std::string Configuration::any = "*";
const std::string Configuration::wildcard = "(*)";
const std::string Configuration::on = "on";
const std::string Configuration::off = "off";
const std::string Configuration::pub = "public";
const std::string Configuration::share_only = "all";
const int    Configuration::automatic = -1;


Configuration::Configuration(std::string dn) :
    db (DBSingleton::instance().getDBObjectInstance()),
    updateCount(0),
    insertCount(0),
    deleteCount(0),
    dn(dn)
{

    notAllowed.insert(wildcard);

}

Configuration::~Configuration()
{

    if (deleteCount)
        db->auditConfiguration(dn, all, "delete");

    if (insertCount)
        db->auditConfiguration(dn, all, "insert");

    if (updateCount)
        db->auditConfiguration(dn, all, "update");
}

std::string Configuration::json(std::map<std::string, int>& params)
{

    std::stringstream ss;

    ss << "[";

    for (auto it = params.begin(); it != params.end();)
        {
            if (it->second == automatic)
                {
                    // it is auto
                    ss << "{\"" << it->first << "\":\"" << CfgParser::auto_value << "\"}";
                }
            else
                {
                    // it is a normal value
                    ss << "{\"" << it->first << "\":" << it->second << "}";
                }
            it++;
            if (it != params.end()) ss << ",";
        }

    ss << "]";

    return ss.str();
}

std::string Configuration::json(std::map<std::string, double>& params)
{

    std::stringstream ss;

    ss << "[";

    for (auto it = params.begin(); it != params.end();)
        {
            ss << "{\"" << it->first << "\":" << it->second << "}";
            it++;
            if (it != params.end()) ss << ",";
        }

    ss << "]";

    return ss.str();
}

std::string Configuration::json(boost::optional< std::map<std::string, int> >& params)
{

    std::stringstream ss;

    if (!params.is_initialized())
        {
            ss << "\"" << CfgParser::auto_value << "\"";
            return ss.str();
        }

    return json(params.get());
}

std::string Configuration::json(std::vector<std::string>& members)
{

    std::stringstream ss;

    ss << "[";

    for (auto it = members.begin(); it != members.end();)
        {
            ss << "\"" << *it << "\"";
            it++;
            if (it != members.end()) ss << ",";
        }

    ss << "]";

    return ss.str();
}

void Configuration::addSe(std::string seName, bool active)
{
    static const boost::regex re(".+://[a-zA-Z0-9\\.-]+");

    if (seName != wildcard && !boost::regex_match(seName, re))
        throw UserError("The SE name should be compliant with the following convention: 'protocol://hostname' !");

    //check if SE exists
    boost::optional<StorageElement> storage(db->getStorageElement(seName));
    if (!storage)
        {
            db->addStorageElement(seName, active ? on : off);
            insertCount++;
        }
    else
        {
            db->updateStorageElement(seName, active ? on : off);
        }
}

void Configuration::eraseSe(std::string se)
{
    db->updateStorageElement(se, off);
    updateCount++;
}

void Configuration::addGroup(std::string group, std::vector<std::string>& members)
{
    // first check if the new group members are correct
    for (auto it = members.begin(); it != members.end(); it++)
        {
            if (db->checkIfSeIsMemberOfAnotherGroup(*it))
                throw UserError("The SE: " + *it + " is already a member of another SE group!");
        }
    // if the group already exists delete it
    if (db->checkGroupExists(group))
        {
            // if the group exists remove it!
            std::vector<std::string> tmp;
            db->getGroupMembers(group, tmp);
            db->deleteMembersFromGroup(group, tmp);
            deleteCount++;

            // remove also assigned cfgs
            for (auto it = tmp.begin(); it != tmp.end(); it++)
                db->delFileShareConfig(group, *it);
        }
    // add new members to the group
    for (auto it = members.begin(); it != members.end(); it++)
        {
            addSe(*it);
        }

    db->addMemberToGroup(group, members);
    insertCount++;
}

void Configuration::checkGroup(std::string group)
{
    // check if the group exists
    if (!db->checkGroupExists(group))
        {
            throw UserError(
                "The group: " +  group + " does not exist!"
            );
        }
}

std::pair< LinkConfig, bool > Configuration::getLinkConfig(std::string source, std::string destination, bool active, std::string symbolic_name)
{

    std::unique_ptr< std::pair<std::string, std::string> > p (
        db->getSourceAndDestination(symbolic_name)
    );

    if (p.get())
        {
            if (source != p->first || destination != p->second)
                throw UserError("A 'pair' with the same symbolic name exists already!");
        }

    std::unique_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    bool update = true;
    if (!cfg.get())
        {
            cfg.reset(new LinkConfig);
            update = false;
        }

    cfg->source = source;
    cfg->destination = destination;
    cfg->state = active ? on : off;
    cfg->symbolicName = symbolic_name;

    return std::make_pair(*cfg, update);
}

void Configuration::addLinkCfg(std::string source, std::string destination, bool active, std::string symbolic_name, boost::optional< std::map<std::string, int> >& protocol)
{

    std::pair< LinkConfig, bool > cfg = getLinkConfig(source, destination, active, symbolic_name);

    if (protocol.is_initialized())
        {
            int value = protocol.get()[Protocol::NOSTREAMS];
            cfg.first.numberOfStreams = value ? value : DEFAULT_NOSTREAMS;

            value = protocol.get()[Protocol::TCP_BUFFER_SIZE];
            cfg.first.tcpBufferSize = value ? value : DEFAULT_BUFFSIZE;

            value = protocol.get()[Protocol::URLCOPY_TX_TO];
            cfg.first.transferTimeout = value ? value : DEFAULT_TIMEOUT;

            cfg.first.autoTuning = off;
        }
    else
        {
            cfg.first.numberOfStreams = -1;
            cfg.first.tcpBufferSize = -1;
            cfg.first.transferTimeout = -1;
            cfg.first.autoTuning = on;
        }

    if (cfg.second)
        {
            db->updateLinkConfig(cfg.first);
            updateCount++;
        }
    else
        {
            db->addLinkConfig(cfg.first);
            insertCount++;
        }
}

void Configuration::addLinkCfg(std::string source, std::string destination, bool active, std::string symbolic_name)
{

    std::pair< LinkConfig, bool > cfg = getLinkConfig(source, destination, active, symbolic_name);

    // not used in case of share-only configuration therefore set to -1
    cfg.first.numberOfStreams = -1;
    cfg.first.tcpBufferSize = -1;
    cfg.first.transferTimeout = -1;

    // mark it as share only
    cfg.first.autoTuning = share_only;

    if (cfg.second)
        {
            db->updateLinkConfig(cfg.first);
            updateCount++;
        }
    else
        {
            db->addLinkConfig(cfg.first);
            insertCount++;
        }
}

void Configuration::addShareCfg(std::string source, std::string destination, std::map<std::string, int>& share)
{
    if (share.empty())
        {
            // if no share was defined it means that autotuner should take care if all the transfers
            // so it is a public auto-share
            share.insert(make_pair(pub, automatic));
        }

    // set with VOs that need an update
    std::set<std::string> update;
    // find all share configuration for source and destination
    std::vector<ShareConfig> vec = db->getShareConfig(source, destination);
    // loop over share configuration
    for (auto iv = vec.begin(); iv != vec.end(); iv++)
        {
            if (share.find(iv->vo) == share.end())
                {
                    // if the VO was not in the new configuration remove the record
                    db->deleteShareConfig(source, destination, iv->vo);
                    // Although it is a deletion it is done due to an update
                    updateCount++;
                }
            else
                {
                    // otherwise schedule it for update
                    update.insert(iv->vo);
                }
        }
    // save the configuration in DB
    for (auto it = share.begin(); it != share.end(); it++)
        {
            std::string vo = it->first;
            // create new share configuration
            ShareConfig cfg;
            cfg.source = source;
            cfg.destination = destination;
            cfg.vo = vo;
            cfg.activeTransfers = it->second;
            // check if the configuration should use insert or update
            if (update.count(it->first))
                {
                    db->updateShareConfig(cfg);
                    updateCount++;
                }
            else
                {
                    db->addShareConfig(cfg);
                    insertCount++;
                }

            // now lets update t_file_share_config !!!

            // get all scheduled files that are affected by this configuration
            std::vector<int> files;

            if (isgroup())
                {
                    db->getFilesForNewGrCfg(source, destination, vo, files);

                    // update t_file_share_config
                    for (auto it_f = files.begin(); it_f != files.end(); it_f++)
                        {

                            // check if it is a pair
                            bool pair = source != Configuration::any && destination != Configuration::any;

                            // if it is not a pair it is a standalone configuration
                            if (!pair)
                                {
                                    // if there is already a pair cfg this one does not apply
                                    if (db->hasPairGrCfgAssigned(*it_f, vo)) continue;

                                }

                            // if it is a pair cfg ..
                            if (pair)
                                {
                                    // the default or standalone cfg should be withdraw from use
                                    db->delFileShareConfig(*it_f, source, Configuration::any, vo);
                                    db->delFileShareConfig(*it_f, Configuration::any, destination, vo);
                                }

                            db->addFileShareConfig(*it_f, source, destination, vo);
                        }


                }
            else
                {
                    db->getFilesForNewSeCfg(source, destination, vo, files);

                    // update t_file_share_config
                    for (auto it_f = files.begin(); it_f != files.end(); it_f++)
                        {
                            // first check the configuration type
                            bool pair =
                                source != Configuration::wildcard &&
                                source != Configuration::any &&
                                destination != Configuration::wildcard &&
                                destination != Configuration::any
                                ;

                            bool standalone =
                                (source != Configuration::wildcard && destination == Configuration::any) ||
                                (source == Configuration::any && destination != Configuration::wildcard)
                                ;

                            // if it is not a pair it is either standalone or default cfg
                            if (!pair)
                                {
                                    // if there is already a pair cfg this one does not apply
                                    if (db->hasPairSeCfgAssigned(*it_f, vo)) continue;
                                }

                            // if it is not a pair or standalone cfg it is a default cfg
                            if (!pair && !standalone)
                                {
                                    // if there is already a standalone cfg the default does not apply
                                    if (db->hasStandAloneSeCfgAssigned(*it_f, vo)) continue;
                                }

                            // if it is either a pair or standalone ..
                            if (pair || standalone)
                                {
                                    // the default cfg should be withdraw from use
                                    db->delFileShareConfig(*it_f, Configuration::wildcard, Configuration::any, vo);
                                    db->delFileShareConfig(*it_f, Configuration::any, Configuration::wildcard, vo);
                                }

                            // if it is a pair cfg ..
                            if (pair)
                                {
                                    // the default or standalone cfg should be withdraw from use
                                    db->delFileShareConfig(*it_f, source, Configuration::any, vo);
                                    db->delFileShareConfig(*it_f, Configuration::any, destination, vo);
                                }

                            db->addFileShareConfig(*it_f, source, destination, vo);
                        }
                }
        }
}

boost::optional< std::map<std::string, int> > Configuration::getProtocolMap(std::string source, std::string destination)
{

    std::unique_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    if (cfg->autoTuning == on)
        return boost::optional< std::map<std::string, int> >();

    return getProtocolMap(cfg.get());
}

boost::optional< std::map<std::string, int> > Configuration::getProtocolMap(LinkConfig* cfg)
{

    std::map<std::string, int> ret;
    ret[Protocol::NOSTREAMS] = cfg->numberOfStreams;
    ret[Protocol::TCP_BUFFER_SIZE] = cfg->tcpBufferSize;
    ret[Protocol::URLCOPY_TX_TO] = cfg->transferTimeout;

    return ret;
}

std::map<std::string, int> Configuration::getShareMap(std::string source, std::string destination)
{

    std::vector<ShareConfig> vec = db->getShareConfig(source, destination);

    if (vec.empty())
        {
            throw UserError(
                "A configuration for source: '" + source + "' and destination: '" + destination + "' does not exist!"
            );
        }

    std::map<std::string, int> ret;

    for (auto it = vec.begin(); it != vec.end(); it++)
        {
            ret[it->vo] = it->activeTransfers;
        }

    return ret;
}

void Configuration::delLinkCfg(std::string source, std::string destination)
{

    std::unique_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    if (!cfg.get())
        {

            if (source == wildcard || destination == wildcard)
                {
                    throw UserError("The default configuration does not exist!");
                }

            std::string msg;
            if (destination == any)
                {
                    msg += "A standalone configuration for " + source;
                }
            else if (source == any)
                {
                    msg += "A standloane configuration for " + destination;
                }
            else
                {
                    msg += "A pair configuration for " + source + " and " + destination;
                }

            msg += " does not exist!";

            throw UserError(msg);
        }


    db->deleteLinkConfig(source, destination);
    deleteCount++;
}

void Configuration::delShareCfg(std::string source, std::string destination)
{

    db->deleteShareConfig(source, destination);
    deleteCount++;
}

}
}
