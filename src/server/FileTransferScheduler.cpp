/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * FileTransferScheduler.cpp
 *
 *  Created on: May 7, 2012
 *      Author: Michał Simon
 */


#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include <sstream>

#include "FileTransferScheduler.h"

#include "ws/config/Configuration.h"

#include "common/logger.h"
#include "common/JobStatusHandler.h"

#include "ws/SingleTrStateInstance.h"


using namespace db;
using namespace fts3::ws;


FileTransferScheduler::FileTransferScheduler(
    TransferFiles& file,
    vector< boost::shared_ptr<ShareConfig> > cfgs,
    set<string> inses,
    set<string> outses,
    set<string> invos,
    set<string> outvos
) :
    file (file),
    db (DBSingleton::instance().getDBObjectInstance())
{

    srcSeName = file.SOURCE_SE;
    destSeName = file.DEST_SE;

    vector< boost::shared_ptr<ShareConfig> > no_auto_share;

    vector< boost::shared_ptr<ShareConfig> >::iterator it;
    for (it = cfgs.begin(); it != cfgs.end(); it++)
        {

            boost::shared_ptr<ShareConfig>& cfg = *it;

            if (cfg->share_only)
                {
                    double tmp = 0;
                    int sum = 0;


                    if (cfg->source == Configuration::any)
                        {
                            // calculate for destination
                            tmp = db->getSeIn(outses, cfg->destination);
                            sum = db->sumUpVoShares(cfg->source, cfg->destination, invos);
                        }
                    else if (cfg->destination == Configuration::any)
                        {
                            // calculate for source
                            tmp = db->getSeOut(cfg->source, inses);
                            sum = db->sumUpVoShares(cfg->source, cfg->destination, outvos);
                        }


                    if (sum == 0)
                        {
                            // if sum == 0 it means that there was no share defined for the submitting VO
                            tmp = 0;

                        }
                    else if (tmp > 1)
                        {
                            // if tmp == 1 it means that there are no active transfer and auto-tuner allocated the first credit
                            // calculate the share
                            tmp = (tmp * cfg->active_transfers) / sum;
                        }
                    // round up the result
                    cfg->active_transfers = static_cast<int>(tmp + 0.5);
                    cfg->share_only = false; // now the value has been set
                }

            if (cfg->active_transfers != Configuration::automatic) no_auto_share.push_back(cfg);
        }

    this->cfgs = no_auto_share;
}

FileTransferScheduler::~FileTransferScheduler()
{

}

bool FileTransferScheduler::schedule()
{
    try
        {
            if(cfgs.empty())
                {
                    bool allowed = db->isTrAllowed(srcSeName, destSeName);
                    if(allowed)
                        {
                            return true;
                        }
                    return false;
                }

            vector< boost::shared_ptr<ShareConfig> >::iterator it;

            for (it = cfgs.begin(); it != cfgs.end(); ++it)
                {

                    string source = (*it)->source;
                    string destination = (*it)->destination;
                    string vo = (*it)->vo;

                    boost::shared_ptr<ShareConfig>& cfg = *it;

                    if (!cfg.get()) continue; // if the configuration has been deleted in the meanwhile continue

                    // check if the configuration allows this type of transfer-job
                    if (!cfg->active_transfers)
                        {
                            // failed to allocate active transfers credits to transfer-job
                            string msg = getNoCreditsErrMsg(cfg.get());
                            // set file status to failed
                            db->updateFileTransferStatus(
                                0.0,
                                file.JOB_ID,
                                file.FILE_ID,
                                JobStatusHandler::FTS3_STATUS_FAILED,
                                msg,
                                0,
                                0,
                                0,
                                false
                            );
                            // set job states if necessary
                            db->updateJobTransferStatus(
                                file.JOB_ID,
                                JobStatusHandler::FTS3_STATUS_FAILED,0
                            );
                            // log it
                            FTS3_COMMON_LOGGER_NEWLOG (ERR) << msg << commit;
                            // the file has been resolved as FAILED, it won't be scheduled
                            return false;
                        }

                    int active_transfers = 0;

                    if (source == Configuration::wildcard)
                        {
                            active_transfers = db->countActiveOutboundTransfersUsingDefaultCfg(srcSeName, vo);
                            if (cfg->active_transfers - active_transfers > 0) continue;
                            return false;
                        }

                    if (destination == Configuration::wildcard)
                        {
                            active_transfers = db->countActiveInboundTransfersUsingDefaultCfg(destSeName, vo);
                            if (cfg->active_transfers - active_transfers > 0) continue;
                            return false;
                        }

                    active_transfers = db->countActiveTransfers(source, destination, vo);
                    if (cfg->active_transfers - active_transfers > 0) continue;
                    return false;
                }
        }
    catch(std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread exception " << e.what() <<  commit;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread exception unknown" <<  commit;
        }

    return true;
}

string FileTransferScheduler::getNoCreditsErrMsg(ShareConfig* cfg)
{
    stringstream ss;

    ss << "Failed to allocate active transfer credits to transfer job due to ";

    if (cfg->source == Configuration::wildcard || cfg->destination == Configuration::wildcard)
        {

            ss << "the default standalone SE configuration.";

        }
    else if (cfg->source != Configuration::any && cfg->destination != Configuration::any)
        {

            ss << "a pair configuration (" << cfg->source << " -> " << cfg->destination << ").";

        }
    else if (cfg->source != Configuration::any)
        {

            ss << "a standalone out-bound configuration (" << cfg->source << ").";

        }
    else if (cfg->destination != Configuration::any)
        {

            ss << "a standalone in-bound configuration (" << cfg->destination << ").";

        }
    else
        {

            ss << "configuration constraints.";
        }

    ss << "Only the following VOs are allowed: ";

    vector<ShareConfig*> cfgs = db->getShareConfig(cfg->source, cfg->destination);
    vector<ShareConfig*>::iterator it;

    for (it = cfgs.begin(); it != cfgs.end(); ++it)
        {
            boost::shared_ptr<ShareConfig> ptr (*it);
            if (ptr->active_transfers)
                {
                    if (it != cfgs.begin()) ss << ", ";
                    ss << ptr->vo;
                }
        }

    return ss.str();
}

