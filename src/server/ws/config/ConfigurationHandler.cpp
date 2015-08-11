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
 * ConfigurationHandler.cpp
 *
 *  Created on: Jun 26, 2012
 *      Author: Michał Simon
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

ConfigurationHandler::ConfigurationHandler(string dn):
    dn(dn),
    db (DBSingleton::instance().getDBObjectInstance()),
    cfg(0)
{
}

ConfigurationHandler::~ConfigurationHandler()
{

}

void ConfigurationHandler::parse(string configuration)
{

    to_lower(configuration);

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

vector<string> ConfigurationHandler::get()
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

    vector<string> ret;

    // get all standalone configurations (SE names only)
    vector<string> secfgs = db->getAllStandAlloneCfgs();
    vector<string>::iterator it;
    // add the configurations for the SEs to the response
    for (it = secfgs.begin(); it != secfgs.end(); it++)
        {
            // get the SE name
            string se = *it;
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
    vector<string> socfgs = db->getAllShareOnlyCfgs();
    // add the configurations for the SEs to the response
    for (it = socfgs.begin(); it != socfgs.end(); it++)
        {
            // get the SE name
            string se = *it;
            // if it's a wildcard change it to 'any', due to the convention
            if (se == Configuration::wildcard) se = Configuration::any;
            ShareOnlyCfg cfg(dn, se);
            ret.push_back(cfg.json());
        }
    // get all pair configuration (source-destination pairs only)
    vector< std::pair<string, string> > paircfgs = db->getAllPairCfgs();
    vector< std::pair<string, string> >::iterator it2;
    // add the configurations for the pairs to the response
    for (it2 = paircfgs.begin(); it2 != paircfgs.end(); it2++)
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
    vector<string> activityshares = db->getAllActivityShareConf();
    vector<string>::iterator it3;

    for (it3 = activityshares.begin(); it3 != activityshares.end(); it3++)
        {
            ActivityCfg cfg(dn, *it3);
            ret.push_back(cfg.json());
        }

    return ret;
}

string ConfigurationHandler::get(string name)
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

vector<string> ConfigurationHandler::getAll(string name)
{
    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

    vector<string> ret;

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
    vector< pair<string, string> > pairs = db->getPairsForSe(name);
    vector< pair<string, string> >::iterator it;

    for (it = pairs.begin(); it != pairs.end(); it++)
        {
            // get the pair configurations
            ret.push_back(getPair(it->first, it->second));
        }

    // check if it is a member of a group
    string group = db->getGroupForSe(name);

    if (!group.empty())
        {

            // get the standalone group configuration
            ret.push_back(get(group));

            // check if there are pair configurations
            pairs = db->getPairsForSe(group);

            for (it = pairs.begin(); it != pairs.end(); it++)
                {
                    // get the pair configurations
                    ret.push_back(getPair(it->first, it->second));
                }
        }

    return ret;
}

string ConfigurationHandler::getPair(string src, string dest)
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

string ConfigurationHandler::getPair(string symbolic)
{
    scoped_ptr< pair<string, string> > p (
        db->getSourceAndDestination(symbolic)
    );

    if (p.get())
        return getPair(p->first, p->second);
    else
        throw Err_Custom("The symbolic name does not exist!");
}

string ConfigurationHandler::getVo(string vo)
{
    cfg.reset(new ActivityCfg(dn, vo));
    return cfg->json();
}

void ConfigurationHandler::del()
{

    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is deleting configuration" << commit;
    cfg->del();
}

