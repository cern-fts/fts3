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

#include "ConfigurationAssigner.h"

#include "common/JobStatusHandler.h"

#include <boost/assign.hpp>
#include <boost/regex.hpp>

namespace fts3
{
namespace server
{

using namespace fts3::common;
using namespace boost::assign;

const std::string ConfigurationAssigner::any = "*";
const std::string ConfigurationAssigner::wildcard = "(*)";
const std::string ConfigurationAssigner::on = "on";
const std::string ConfigurationAssigner::off = "off";
const std::string ConfigurationAssigner::pub = "public";
const std::string ConfigurationAssigner::share_only = "all";
const int ConfigurationAssigner::automatic = -1;


ConfigurationAssigner::ConfigurationAssigner(TransferFile const & file) :
    file(file),
    db (DBSingleton::instance().getDBObjectInstance()),
    assign_count(0)
{

}

ConfigurationAssigner::~ConfigurationAssigner()
{
}

void ConfigurationAssigner::assign(std::vector< std::shared_ptr<ShareConfig> >& out)
{
    std::string source = file.sourceSe;
    std::string destination = file.destSe;
    std::string vo = file.voName;

    // possible configurations for SE
    std::list<cfg_type> se_cfgs = list_of
                             ( cfg_type( share(source, destination, vo), content(true, true) ) )
                             ( cfg_type( share(source, ConfigurationAssigner::any, vo), content(true, false) ) )
                             ( cfg_type( share(ConfigurationAssigner::wildcard, ConfigurationAssigner::any, vo), content(true, false) ) )
                             ( cfg_type( share(ConfigurationAssigner::any, destination, vo), content(false, true) ) )
                             ( cfg_type( share(ConfigurationAssigner::any, ConfigurationAssigner::wildcard, vo), content(false, true) ) )
                             ;

    assignShareCfg(se_cfgs, out);

    // get group names for source and destination SEs
    std::string sourceGr = db->getGroupForSe(source);
    std::string destinationGr = db->getGroupForSe(destination);

    // possible configuration for SE group
    std::list<cfg_type> gr_cfgs;
    if (!sourceGr.empty() && !destinationGr.empty()) {
        gr_cfgs.push_back(cfg_type(share(sourceGr, destinationGr, vo), content(true, true)));
    }
    if (!sourceGr.empty()) {
        gr_cfgs.push_back(cfg_type(share(sourceGr, ConfigurationAssigner::any, vo), content(true, false)));
    }
    if (!destinationGr.empty()) {
        gr_cfgs.push_back(cfg_type(share(ConfigurationAssigner::any, destinationGr, vo), content(false, true)));
    }

    assignShareCfg(gr_cfgs, out);
}

void ConfigurationAssigner::assignShareCfg(std::list<cfg_type> arg, std::vector< std::shared_ptr<ShareConfig> >& out)
{
    content both (false, false);

    for (auto it = arg.begin(); it != arg.end(); ++it) {
        share s = boost::get<SHARE>(*it);
        content c = boost::get<CONTENT>(*it);

        // check if configuration for the given side has not been assigned already
        if ((c.first && both.first) || (c.second && both.second)) {
            continue;
        }

        std::string source = boost::get<SOURCE>(s);
        std::string destination = boost::get<DESTINATION>(s);
        std::string vo = boost::get<VO>(s);

        // get the link configuration
        std::unique_ptr<LinkConfig> link(db->getLinkConfig(source, destination));

        // if there is no link there will be no share
        // (also if the link configuration state is 'off' we don't care about the share)
        if (!link.get() || link->state == ConfigurationAssigner::off) {
            continue;
        }

        // check if there is a VO share
        std::shared_ptr<ShareConfig> ptr(
                db->getShareConfig(source, destination, vo)
        );

        if (ptr.get()) {
            // set the share only status
            ptr->shareOnly = link->autoTuning == ConfigurationAssigner::share_only;
            // add to out
            out.push_back(ptr);
            // add to DB
            db->addFileShareConfig(file.fileId, ptr->source, ptr->destination, ptr->vo);
            // a configuration has been assigned
            assign_count++;
            // set the respective flags
            both.first |= c.first;
            both.second |= c.second;
            // if both source and destination are covert break;
            if (both.first && both.second) break;
            // otherwise continue
            continue;
        }

        // check if there is a public share
        ptr = db->getShareConfig(source, destination, ConfigurationAssigner::pub);

        // if not create a public share with 0 active transfer (equivalent)
        if (!ptr.get()) {
            // create the object
            ptr.reset(
                    new ShareConfig
            );
            // fill in the respective values
            ptr->source = source;
            ptr->destination = destination;
            ptr->vo = ConfigurationAssigner::pub;
            ptr->activeTransfers = 0;
            // insert into DB
            db->addShareConfig(*ptr.get());
        }

        // set the share only status
        ptr->shareOnly = link->autoTuning == ConfigurationAssigner::share_only;
        // add to out
        out.push_back(ptr);
        // add to DB
        db->addFileShareConfig(file.fileId, ptr->source, ptr->destination, ptr->vo);
        // a configuration has been assigned
        assign_count++;
        // set the respective flags
        both.first |= c.first;
        both.second |= c.second;
        // if both source and destination are covert break;
        if (both.first && both.second) break;
    }
}

} /* namespace server */
} /* namespace fts3 */
