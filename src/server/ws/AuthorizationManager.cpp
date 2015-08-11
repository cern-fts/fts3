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
 * AuthorizationManager.cpp
 *
 *  Created on: Jul 18, 2012
 *      Author: Michal Simon
 */

#include "AuthorizationManager.h"
#include "CGsiAdapter.h"

#include "common/error.h"
#include "common/logger.h"

#include "config/serverconfig.h"

#include "db/generic/SingleDbInstance.h"

#include <boost/algorithm/string.hpp>


using namespace fts3::config;
using namespace db;

namespace fts3
{
namespace ws
{

const string AuthorizationManager::ALL_LVL = "all";
const string AuthorizationManager::VO_LVL = "vo";
const string AuthorizationManager::PRV_LVL;

const string AuthorizationManager::PUBLIC_ACCESS = "roles.Public";

const string AuthorizationManager::DELEG_OP = "deleg";
const string AuthorizationManager::TRANSFER_OP = "transfer";
const string AuthorizationManager::CONFIG_OP = "config";

const string AuthorizationManager::WILD_CARD = "*";

const string AuthorizationManager::ROLES_SECTION_PREFIX = "roles.";

const OwnedResource *AuthorizationManager::dummy = NULL;

template<>
vector<string> AuthorizationManager::get< vector<string> >(string cfg)   // TODO same code is in serverconfig.h, Michail is using it!
{

    char_separator<char> sep(";");
    tokenizer< char_separator<char> > tokens(cfg, sep);
    tokenizer< char_separator<char> >::iterator it;

    std::vector<string> ret;
    for (it = tokens.begin(); it != tokens.end(); ++it)
        {
            ret.push_back(*it);
        }

    return ret;
}

template<>
string AuthorizationManager::get<string>(string cfg)
{
    size_t pos = cfg.find(':');
    if (pos != string::npos)
        {
            return cfg.substr(pos + 1);
        }
    else
        {
            return cfg;
        }
}

template<>
AuthorizationManager::Level AuthorizationManager::get<AuthorizationManager::Level>(string cfg)
{
    size_t pos = cfg.find(':');
    if (pos != string::npos)
        {
            return stringToLvl(cfg.substr(0, pos));
        }
    else
        {
            return PRV;
        }
}

set<string> AuthorizationManager::vostInit()
{

    // parse the authorized vo list
    vector<string> voNameList = theServerConfig().get< vector<string> >("AuthorizedVO");
    return set <string> (voNameList.begin(), voNameList.end());
}

map<string, map<string, AuthorizationManager::Level> > AuthorizationManager::accessInit()
{

    map<string, map<string, Level> > ret;

    // roles.* is a regular expression for all role entries
    map<string, string> rolerights = theServerConfig().get< map<string, string> > (ROLES_SECTION_PREFIX + WILD_CARD);
    if (!rolerights.empty())
        {
            map<string, string>::iterator it;
            for (it = rolerights.begin(); it != rolerights.end(); it++)
                {

                    map<string, Level> rights;

                    vector<string> r = get< vector<string> >(it->second);
                    vector<string>::iterator r_it;
                    for (r_it = r.begin(); r_it != r.end(); r_it++)
                        {
                            string op = get<string>(*r_it);
                            Level lvl = get<Level>(*r_it);
                            rights[op] = lvl;
                            if (op == TRANSFER_OP)
                                {
                                    // if someone has transfer rights, he automatically gets delegation rights
                                    // how ever we need a distinction for the Root user who is not allowed to delegate
                                    rights[DELEG_OP] = lvl;
                                }
                        }

                    ret[it->first] = rights;
                }
        }

    return ret;
}

AuthorizationManager::AuthorizationManager() :
    vos(vostInit()),
    access(accessInit()),
    cfgReadTime(theServerConfig().getReadTime())
{

}

AuthorizationManager::~AuthorizationManager()
{

}

AuthorizationManager::Level AuthorizationManager::stringToLvl(string s)
{

    if (s == ALL_LVL) return ALL;
    if (s == VO_LVL) return VO;
    return PRV;
}

string AuthorizationManager::lvlToString(Level lvl)
{

    switch (lvl)
        {
        case NONE:
            return "none";
        case PRV:
            return "private";
        case VO:
            return "vo";
        case ALL:
            return "all";
        default:
            return string();
        }
}

string AuthorizationManager::operationToStr(Operation op)
{

    switch(op)
        {
        case DELEG:
            return DELEG_OP;
        case TRANSFER:
            return TRANSFER_OP;
        case CONFIG:
            return CONFIG_OP;
        default:
            return string();
        }
}


AuthorizationManager::Level AuthorizationManager::check(string role, string operation)
{

    map< string, map<string, Level> >::const_iterator a_it;

    // check if the role is specified in fts3config file
    a_it = access.find(role);
    if (a_it == access.end()) return NONE;

    map<string, Level>::const_iterator l_it;

    Level ret = NONE;

    // check is there is a wild card
    l_it = a_it->second.find(WILD_CARD);
    if (l_it != a_it->second.end())
        {
            ret = l_it->second;
        }

    // check if the operation is given directly
    l_it = a_it->second.find(operation);
    if (l_it != a_it->second.end())
        {
            if (l_it->second > ret) ret = l_it->second;
        }

    // return the higher access level
    return ret;
}

AuthorizationManager::Level AuthorizationManager::getGrantedLvl(soap* ctx, Operation op)
{

    CGsiAdapter cgsi(ctx);

    // root is authorized to do anything but delegations
    if(cgsi.isRoot())
        {
            if (op != DELEG) return ALL;
            string msg = "Authorization failed, a host certificate has been used to submit a transfer!";
            throw Err_Custom(msg);
        }

    // if the VO authorization list was specified and a wildcard was not used ...
//	if (!vos.empty() && !vos.count("*")) {
//
//		string vo = cgsi.getClientVo();
//		to_lower(vo);
//
//		if (!vos.count(vo)) {
//
//			string msg = "Authorization failed, access was not granted. ";
//			msg += "(Please check if the fts3 configuration file contains the VO: '";
//			msg += vo;
//			msg += "' and if the right delimiter was used!)";
//
//			throw Err_Custom(msg);
//		}
//	}

    // get operation string
    string op_str = operationToStr(op);

    // check if the access is public
    Level lvl = check(PUBLIC_ACCESS, op_str);

    // check if the user has a role that is granting him the access
    vector<string> roles = cgsi.getClientRoles();
    if (!roles.empty())
        {
            vector<string>::iterator it;
            for (it = roles.begin(); it != roles.end(); ++it)
                {
                    Level tmp = check(ROLES_SECTION_PREFIX + *it, op_str);
                    if (tmp > lvl) lvl = tmp;
                }
        }

    // if access was not granted throw an exception
    if (lvl != NONE) return lvl;
    else
        {
            string msg = "Authorisation failed, access was not granted. ";
            msg += "(The user: ";
            msg += cgsi.getClientDn();
            msg += ") has not the right Role to perform '";
            msg += op_str;
            msg += 	"' operation)";
            throw Err_Custom(msg);
        }
}

AuthorizationManager::Level AuthorizationManager::getRequiredLvl(soap* ctx, Operation op, const OwnedResource* rsc)
{

    CGsiAdapter cgsi(ctx);

    // if the resource is not specified we don't need any access level
    // this can happen for example in case of fts-transfer-list where
    // the resources (transfer-jobs) are listed depending on the granted level
    if (!rsc) return NONE;

    switch(op)
        {
        case DELEG:
            return PRV; // it is only possible to remove someone else's proxy-certificate so it's always 'PRV'
        case TRANSFER:
        {
            if (rsc->getUserDn() == cgsi.getClientDn()) return PRV; // it is user's job
            if (rsc->getVo() == cgsi.getClientVo()) return VO; // it is a job that has been created within user's VO
            return ALL; // it needs global access
        }
        case CONFIG:
            return ALL; // so far only global admins will be able to configure
        default:
            return ALL; // in case of a bug return the highest possible level
        }
}

AuthorizationManager::Level AuthorizationManager::authorize(soap* ctx, Operation op,  const OwnedResource* rsc)
{

    // check if the configuration file has been modified
    if (cfgReadTime != theServerConfig().getReadTime())
        {
            // if yes we have to update the data
            vos = vostInit();
            access = accessInit();
            cfgReadTime = theServerConfig().getReadTime();
        }

    Level grantedLvl = getGrantedLvl(ctx, op);
    Level requiredLvl = getRequiredLvl(ctx, op, rsc);

    if (grantedLvl < requiredLvl)
        {
            string msg = "Authorisation failed, access was not granted. ";

            switch(grantedLvl)
                {
                case PRV:
                    msg += "(the user is only authorised to manage his own transfer-jobs)";
                    break;

                case VO:
                    msg += "(the user is authorised to manage resources only within his VO)";
                    break;
                }

            throw Err_Custom(msg);
        }

    return grantedLvl;

}

}
}



