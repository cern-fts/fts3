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

#include "Blacklister.h"

#include "common/logger.h"
#include "common/error.h"

#include "ws/CGsiAdapter.h"

#include "ws/SingleTrStateInstance.h"

#include <vector>
#include <set>

namespace fts3
{
namespace ws
{

using namespace fts3::common;

Blacklister::Blacklister(soap* ctx, string name, string status, int timeout, bool blk) :
    name(name),
    status(status),
    timeout(timeout),
    blk(blk),
    db(DBSingleton::instance().getDBObjectInstance())
{

    CGsiAdapter cgsi(ctx);
    adminDn = cgsi.getClientDn();
}

Blacklister::Blacklister(soap* ctx, string name, string vo, string status, int timeout, bool blk) :
    name(name),
    vo(vo),
    status(status),
    timeout(timeout),
    blk(blk),
    db(DBSingleton::instance().getDBObjectInstance())
{

    CGsiAdapter cgsi(ctx);
    adminDn = cgsi.getClientDn();
}

Blacklister::~Blacklister()
{

}

void Blacklister::executeRequest()
{

    // the vo parameter is given only for SE blacklisting
    if (vo.is_initialized())
        {

            handleSeBlacklisting();

        }
    else
        {

            handleDnBlacklisting();
        }
}

void Blacklister::handleSeBlacklisting()
{

    // audit the operation
    string cmd = "fts-set-blacklist se " + name + (blk ? " on" : " off");
    db->auditConfiguration(adminDn, cmd, "blacklist");

    if (blk)
        {
            db->blacklistSe(
                name, *vo, status, timeout, string(), adminDn
            );
            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << adminDn << " had blacklisted the SE: " << name << commit;

            handleJobsInTheQueue();

        }
    else
        {
            db->unblacklistSe(name);
            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << adminDn << " had unblacklisted the SE: " << name << commit;
        }
}

void Blacklister::handleDnBlacklisting()
{

    // audit the operation
    string cmd = "fts-set-blacklist dn " + name + (blk ? " on" : " off");
    db->auditConfiguration(adminDn, cmd, "blacklist");

    if (blk)
        {
            if (name == adminDn)
                {
                    throw Err_Custom ("A user cannot blacklist himself!");
                }

            db->blacklistDn(name, string(), adminDn);
            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << name << " had blacklisted the DN: " << adminDn << commit;

            handleJobsInTheQueue();

        }
    else
        {
            if (name == adminDn)
                {
                    throw Err_Custom ("A user cannot unblacklist himself!");
                }

            db->unblacklistDn(name);
            // log it
            FTS3_COMMON_LOGGER_NEWLOG (INFO) << "User: " << adminDn << " had unblacklisted the DN: " << name << commit;
        }
}

void Blacklister::handleJobsInTheQueue()
{

    // for now cancel only the

    if (status == "CANCEL")
        {


            if (vo.is_initialized())
                {

                    set<string> canceled;

                    db->cancelFilesInTheQueue(name, *vo, canceled);
                    set<string>::const_iterator iter;
                    if(!canceled.empty())
                        {
                            for (iter = canceled.begin(); iter != canceled.end(); ++iter)
                                {
                                    fts3::server::SingleTrStateInstance::instance().sendStateMessage((*iter), -1);
                                }
                            canceled.clear();
                        }
                }
            else
                {

                    vector<string> canceled;

                    db->cancelJobsInTheQueue(name, canceled);
                    vector<string>::const_iterator iter;
                    if(!canceled.empty())
                        {
                            for (iter = canceled.begin(); iter != canceled.end(); ++iter)
                                {
                                    fts3::server::SingleTrStateInstance::instance().sendStateMessage((*iter), -1);
                                }
                            canceled.clear();
                        }
                }
        }
    else if (status == "WAIT" || status == "WAIT_AS")
        {
            if (vo.is_initialized())
                {
                    db->setFilesToWaiting(name, *vo, timeout);
                }
            else
                {
                    db->setFilesToWaiting(name, timeout);
                }
        }
}

} /* namespace ws */
} /* namespace fts3 */
