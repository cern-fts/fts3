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
#include "common/error.h"
#include "common/definitions.h"

#include <set>

#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

namespace fts3
{
namespace ws
{

using namespace boost;

const string Configuration::Protocol::BANDWIDTH = "bandwidth";
const string Configuration::Protocol::NOSTREAMS = "nostreams";
const string Configuration::Protocol::TCP_BUFFER_SIZE = "tcp_buffer_size";
const string Configuration::Protocol::NOMINAL_THROUGHPUT = "nominal_throughput";
const string Configuration::Protocol::BLOCKSIZE = "blocksize";
const string Configuration::Protocol::HTTP_TO = "http_to";
const string Configuration::Protocol::URLCOPY_PUT_TO = "urlcopy_put_to";
const string Configuration::Protocol::URLCOPY_PUTDONE_TO = "urlcopy_putdone_to";
const string Configuration::Protocol::URLCOPY_GET_TO = "urlcopy_get_to";
const string Configuration::Protocol::URLCOPY_GET_DONETO = "urlcopy_get_doneto";
const string Configuration::Protocol::URLCOPY_TX_TO = "urlcopy_tx_to";
const string Configuration::Protocol::URLCOPY_TXMARKS_TO = "urlcopy_txmarks_to";
const string Configuration::Protocol::URLCOPY_FIRST_TXMARK_TO = "urlcopy_first_txmark_to";
const string Configuration::Protocol::TX_TO_PER_MB = "tx_to_per_mb";
const string Configuration::Protocol::NO_TX_ACTIVITY_TO = "no_tx_activity_to";
const string Configuration::Protocol::PREPARING_FILES_RATIO = "preparing_files_ratio";

const string Configuration::any = "*";
const string Configuration::wildcard = "(*)";
const string Configuration::on = "on";
const string Configuration::off = "off";
const string Configuration::pub = "public";
const string Configuration::share_only = "all";
const int    Configuration::automatic = -1;

Configuration::Configuration(string dn) :
    dn(dn),
    db (DBSingleton::instance().getDBObjectInstance()),
    insertCount(0),
    updateCount(0),
    deleteCount(0)
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

string Configuration::json(map<string, int>& params)
{

    stringstream ss;

    ss << "[";

    map<string, int>::iterator it;
    for (it = params.begin(); it != params.end();)
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

string Configuration::json(map<string, double>& params)
{

    stringstream ss;

    ss << "[";

    map<string, double>::iterator it;
    for (it = params.begin(); it != params.end();)
        {
            ss << "{\"" << it->first << "\":" << it->second << "}";
            it++;
            if (it != params.end()) ss << ",";
        }

    ss << "]";

    return ss.str();
}

string Configuration::json(optional< map<string, int> >& params)
{

    stringstream ss;

    if (!params.is_initialized())
        {
            ss << "\"" << CfgParser::auto_value << "\"";
            return ss.str();
        }

    return json(params.get());
}

string Configuration::json(vector<string>& members)
{

    stringstream ss;

    ss << "[";

    vector<string>::iterator it;
    for (it = members.begin(); it != members.end();)
        {
            ss << "\"" << *it << "\"";
            it++;
            if (it != members.end()) ss << ",";
        }

    ss << "]";

    return ss.str();
}

void Configuration::addSe(string se, bool active)
{
    static const regex re(".+://[a-zA-Z0-9\\.-]+");

    if (se != wildcard && !regex_match(se, re))
        throw Err_Custom("The SE name should be complaint with the following convention: 'protocol://hostname' !");

    //check if SE exists
    Se* ptr = 0;
    db->getSe(ptr, se);
    if (!ptr)
        {
            // if not add it to the DB
            db->addSe(string(), string(), string(), se, active ? on : off, string(), string(), string(), string(), string(), string());
            insertCount++;
        }
    else
        db->updateSe(string(), string(), string(), se, active ? on : off, string(), string(), string(), string(), string(), string());
    delete ptr;
}

void Configuration::eraseSe(string se)
{
    db->updateSe(string(), string(), string(), se, on, string(), string(), string(), string(), string(), string());
    updateCount++;
}

void Configuration::addGroup(string group, vector<string>& members)
{
    // first check if the new group members are correct
    vector<string>::iterator it;
    for (it = members.begin(); it != members.end(); it++)
        {
            if (db->checkIfSeIsMemberOfAnotherGroup(*it))
                throw Err_Custom("The SE: " + *it + " is already a member of another SE group!");
        }
    // if the group already exists delete it
    if (db->checkGroupExists(group))
        {
            // if the group exists remove it!
            vector<string> tmp;
            db->getGroupMembers(group, tmp);
            db->deleteMembersFromGroup(group, tmp);
            deleteCount++;

            // remove also assigned cfgs
            vector<string>::iterator it;
            for (it = tmp.begin(); it != tmp.end(); it++)
                db->delFileShareConfig(group, *it);
        }
    // add new members to the group
    for (it = members.begin(); it != members.end(); it++)
        {
            addSe(*it);
        }

    db->addMemberToGroup(group, members);
    insertCount++;
}

void Configuration::checkGroup(string group)
{
    // check if the group exists
    if (!db->checkGroupExists(group))
        {
            throw Err_Custom(
                "The group: " +  group + " does not exist!"
            );
        }
}

pair< std::shared_ptr<LinkConfig>, bool > Configuration::getLinkConfig(string source, string destination, bool active, string symbolic_name)
{

    scoped_ptr< pair<string, string> > p (
        db->getSourceAndDestination(symbolic_name)
    );

    if (p.get())
        {
            if (source != p->first || destination != p->second)
                throw Err_Custom("A 'pair' with the same symbolic name exists already!");
        }

    std::shared_ptr<LinkConfig> cfg (
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
    cfg->symbolic_name = symbolic_name;

    return make_pair(cfg, update);
}

void Configuration::addLinkCfg(string source, string destination, bool active, string symbolic_name, optional< map<string, int> >& protocol)
{

    pair< std::shared_ptr<LinkConfig>, bool > cfg = getLinkConfig(source, destination, active, symbolic_name);

    // not used for now therefore set to 0
    cfg.first->NO_TX_ACTIVITY_TO = 0;

    if (protocol.is_initialized())
        {
            int value = protocol.get()[Protocol::NOSTREAMS];
            cfg.first->NOSTREAMS = value ? value : DEFAULT_NOSTREAMS;

            value = protocol.get()[Protocol::TCP_BUFFER_SIZE];
            cfg.first->TCP_BUFFER_SIZE = value ? value : DEFAULT_BUFFSIZE;

            value = protocol.get()[Protocol::URLCOPY_TX_TO];
            cfg.first->URLCOPY_TX_TO = value ? value : DEFAULT_TIMEOUT;

            cfg.first->auto_tuning = off;
        }
    else
        {
            cfg.first->NOSTREAMS = -1;
            cfg.first->TCP_BUFFER_SIZE = -1;
            cfg.first->URLCOPY_TX_TO = -1;
            cfg.first->auto_tuning = on;
        }

    if (cfg.second)
        {
            db->updateLinkConfig(cfg.first.get());
            updateCount++;
        }
    else
        {
            db->addLinkConfig(cfg.first.get());
            insertCount++;
        }
}

void Configuration::addLinkCfg(string source, string destination, bool active, string symbolic_name)
{

    pair< std::shared_ptr<LinkConfig>, bool > cfg = getLinkConfig(source, destination, active, symbolic_name);

    // not used therefore set to 0
    cfg.first->NO_TX_ACTIVITY_TO = 0;
    // not used in case of share-only configuration therefore set to -1
    cfg.first->NOSTREAMS = -1;
    cfg.first->TCP_BUFFER_SIZE = -1;
    cfg.first->URLCOPY_TX_TO = -1;

    // mark it as share only
    cfg.first->auto_tuning = share_only;

    if (cfg.second)
        {
            db->updateLinkConfig(cfg.first.get());
            updateCount++;
        }
    else
        {
            db->addLinkConfig(cfg.first.get());
            insertCount++;
        }
}

void Configuration::addShareCfg(string source, string destination, map<string, int>& share)
{
    if (share.empty())
        {
            // if no share was defined it means that autotuner should take care if all the transfers
            // so it is a public auto-share
            share.insert(make_pair(pub, automatic));
        }

    // set with VOs that need an update
    set<string> update;
    // find all share configuration for source and destination
    vector<ShareConfig*> vec = db->getShareConfig(source, destination);
    vector<ShareConfig*>::iterator iv;
    // loop over share configuration
    for (iv = vec.begin(); iv != vec.end(); iv++)
        {
            scoped_ptr<ShareConfig> cfg (*iv);
            if (share.find(cfg->vo) == share.end())
                {
                    // if the VO was not in the new configuration remove the record
                    db->deleteShareConfig(source, destination, cfg->vo);
                    // Although it is a deletion it is done due to an update
                    updateCount++;
                }
            else
                {
                    // otherwise schedule it for update
                    update.insert(cfg->vo);
                }
        }
    // save the configuration in DB
    map<string, int>::iterator it;
    for (it = share.begin(); it != share.end(); it++)
        {
            std::string vo = it->first;
            // create new share configuration
            scoped_ptr<ShareConfig> cfg(new ShareConfig);
            cfg->source = source;
            cfg->destination = destination;
            cfg->vo = vo;
            cfg->active_transfers = it->second;
            // check if the configuration should use insert or update
            if (update.count(it->first))
                {
                    db->updateShareConfig(cfg.get());
                    updateCount++;
                }
            else
                {
                    db->addShareConfig(cfg.get());
                    insertCount++;
                }

            // now lets update t_file_share_config !!!

            // get all scheduled files that are affected by this configuration
            vector<int> files;
            vector<int>::iterator it_f;

            if (isgroup())
                {
                    db->getFilesForNewGrCfg(source, destination, vo, files);

                    // update t_file_share_config
                    for (it_f = files.begin(); it_f != files.end(); it_f++)
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
                    for (it_f = files.begin(); it_f != files.end(); it_f++)
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

optional< map<string, int> > Configuration::getProtocolMap(string source, string destination)
{

    scoped_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    if (cfg->auto_tuning == on) return optional< map<string, int> >();

    return getProtocolMap(cfg.get());
}

optional< map<string, int> > Configuration::getProtocolMap(LinkConfig* cfg)
{

    map<string, int> ret;
    ret[Protocol::NOSTREAMS] = cfg->NOSTREAMS;
    ret[Protocol::TCP_BUFFER_SIZE] = cfg->TCP_BUFFER_SIZE;
    ret[Protocol::URLCOPY_TX_TO] = cfg->URLCOPY_TX_TO;
    ret[Protocol::NO_TX_ACTIVITY_TO] = cfg->NO_TX_ACTIVITY_TO;

    return ret;
}

map<string, int> Configuration::getShareMap(string source, string destination)
{

    vector<ShareConfig*> vec = db->getShareConfig(source, destination);

    if (vec.empty())
        {
            throw Err_Custom(
                "A configuration for source: '" + source + "' and destination: '" + destination + "' does not exist!"
            );
        }

    map<string, int> ret;

    vector<ShareConfig*>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++)
        {
            scoped_ptr<ShareConfig> cfg(*it);
            ret[cfg->vo] = cfg->active_transfers;
        }

    return ret;
}

void Configuration::delLinkCfg(string source, string destination)
{

    scoped_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    if (!cfg.get())
        {

            if (source == wildcard || destination == wildcard)
                {
                    throw Err_Custom("The default configuration does not exist!");
                }

            string msg;
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

            throw Err_Custom(msg);
        }


    db->deleteLinkConfig(source, destination);
    deleteCount++;
}

void Configuration::delShareCfg(string source, string destination)
{

    db->deleteShareConfig(source, destination);
    deleteCount++;
}

}
}
