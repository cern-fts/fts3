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

#include "ConfigurationHandler.h"

#include <sstream>
#include <algorithm>

#include <boost/optional.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include "StandaloneSeCfg.h"
#include "StandaloneGrCfg.h"
#include "SePairCfg.h"
#include "GrPairCfg.h"
#include "ShareOnlyCfg.h"
#include "ActivityCfg.h"

using namespace fts3::ws;

ConfigurationHandler::ConfigurationHandler(std::string dn):
    dn(dn),
    db (DBSingleton::instance().getDBObjectInstance()),
    cfg()
{
}

ConfigurationHandler::~ConfigurationHandler()
{

}

void ConfigurationHandler::parse(std::string configuration)
{
    boost::to_lower(configuration);

    CfgParser parser (configuration);

    switch(parser.getCfgType())
        {
        case CfgParser::STANDALONE_SE_CFG:
            cfg.reset(
                new StandaloneSeCfg(dn, parser)
            );
            break;
        case CfgParser::STANDALONE_GR_CFG:
            cfg.reset(
                new StandaloneGrCfg(dn, parser)
            );
            break;
        case CfgParser::SE_PAIR_CFG:
            cfg.reset(
                new SePairCfg(dn, parser)
            );
            break;
        case CfgParser::GR_PAIR_CFG:
            cfg.reset(
                new GrPairCfg(dn, parser)
            );
            break;
        case CfgParser::SHARE_ONLY_CFG:
            cfg.reset(
                new ShareOnlyCfg(dn, parser)
            );
            break;
        case CfgParser::ACTIVITY_SHARE_CFG:
            cfg.reset(
                new ActivityCfg(dn, parser)
            );
            break;
        case CfgParser::NOT_A_CFG:
        default:
            throw Err_Custom("Wrong configuration format!");
        }
}

void ConfigurationHandler::add()
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is adding configuration" << commit;
    cfg->save();
}

std::vector<std::string> ConfigurationHandler::get()
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

    std::vector<std::string> ret;

    // get all standalone configurations (SE names only)
    std::vector<std::string> secfgs = db->getAllStandAlloneCfgs();
    // add the configurations for the SEs to the response
    for (auto it = secfgs.begin(); it != secfgs.end(); it++)
        {
            // get the SE name
            std::string se = *it;
            // if it's a wildcard change it to 'any', due to the convention
            if (se == Configuration::wildcard) se = Configuration::any;
            // check if it's a group or a SE
            if (db->checkGroupExists(*it))
                {
                    StandaloneGrCfg cfg(dn, se);
                    ret.push_back(cfg.json());
                }
            else
                {
                    StandaloneSeCfg cfg(dn, se);
                    ret.push_back(cfg.json());
                }
        }
    // get all share only configurations
    std::vector<std::string> socfgs = db->getAllShareOnlyCfgs();
    // add the configurations for the SEs to the response
    for (auto it = socfgs.begin(); it != socfgs.end(); it++)
        {
            // get the SE name
            std::string se = *it;
            // if it's a wildcard change it to 'any', due to the convention
            if (se == Configuration::wildcard) se = Configuration::any;
            ShareOnlyCfg cfg(dn, se);
            ret.push_back(cfg.json());
        }
    // get all pair configuration (source-destination pairs only)
    std::vector< std::pair<std::string, std::string> > paircfgs = db->getAllPairCfgs();
    // add the configurations for the pairs to the response
    for (auto it2 = paircfgs.begin(); it2 != paircfgs.end(); it2++)
        {

            bool grPair = db->checkGroupExists(it2->first) && db->checkGroupExists(it2->second);

            if (grPair)
                {
                    GrPairCfg cfg(dn, it2->first, it2->second);
                    ret.push_back(cfg.json());
                }
            else
                {
                    SePairCfg cfg(dn, it2->first, it2->second);
                    ret.push_back(cfg.json());
                }
        }

    // get all activity configs ...
    std::vector<std::string> activityshares = db->getAllActivityShareConf();

    for (auto it3 = activityshares.begin(); it3 != activityshares.end(); it3++)
        {
            ActivityCfg cfg(dn, *it3);
            ret.push_back(cfg.json());
        }

    return ret;
}

std::string ConfigurationHandler::get(std::string name)
{
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

    if (db->isShareOnly(name))
        {

            cfg.reset(
                new ShareOnlyCfg(dn, name)
            );

        }
    else
        {
            if (db->checkGroupExists(name))
                {
                    cfg.reset(
                        new StandaloneGrCfg(dn, name)
                    );
                }
            else
                {
                    cfg.reset(
                        new StandaloneSeCfg(dn, name)
                    );
                }
        }

    return cfg->json();
}

std::vector<std::string> ConfigurationHandler::getAll(std::string name)
{
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

    std::vector<std::string> ret;

    try
        {
            // get the standalone configuration
            ret.push_back(get(name));
        }
    catch (Err& ex)
        {
            // we don't care if nothing has been found
        }

    // check if there are pair configurations
    std::vector< std::pair<std::string, std::string> > pairs = db->getPairsForSe(name);

    for (auto it = pairs.begin(); it != pairs.end(); it++)
        {
            // get the pair configurations
            ret.push_back(getPair(it->first, it->second));
        }

    // check if it is a member of a group
    std::string group = db->getGroupForSe(name);

    if (!group.empty())
        {

            // get the standalone group configuration
            ret.push_back(get(group));

            // check if there are pair configurations
            pairs = db->getPairsForSe(group);

            for (auto it = pairs.begin(); it != pairs.end(); it++)
                {
                    // get the pair configurations
                    ret.push_back(getPair(it->first, it->second));
                }
        }

    return ret;
}

std::string ConfigurationHandler::getPair(std::string src, std::string dest)
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

    bool grPair = db->checkGroupExists(src) && db->checkGroupExists(dest);
    bool sePair = !db->checkGroupExists(src) && !db->checkGroupExists(dest);

    if (grPair)
        {
            cfg.reset(
                new GrPairCfg(dn, src, dest)
            );
        }
    else if (sePair)
        {
            cfg.reset(
                new SePairCfg(dn, src, dest)
            );
        }
    else
        throw Err_Custom("The source and destination have to be either two SEs or two SE groups!");

    return cfg->json();
}

std::string ConfigurationHandler::getPair(std::string symbolic)
{
    std::unique_ptr< std::pair<std::string, std::string> > p (
        db->getSourceAndDestination(symbolic)
    );

    if (p.get())
        return getPair(p->first, p->second);
    else
        throw Err_Custom("The symbolic name does not exist!");
}

std::string ConfigurationHandler::getVo(std::string vo)
{
    cfg.reset(new ActivityCfg(dn, vo));
    return cfg->json();
}

void ConfigurationHandler::del()
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is deleting configuration" << commit;
    cfg->del();
}

