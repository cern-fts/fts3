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

#include <boost/lexical_cast.hpp>

#include "common/Logger.h"

#include "ConfigurationAssigner.h"
#include "FileTransferScheduler.h"
#include "SingleTrStateInstance.h"


using namespace db;
using namespace fts3::common;
using namespace fts3::server;


FileTransferScheduler::FileTransferScheduler(
    TransferFile const & file,
    std::vector< std::shared_ptr<ShareConfig> > cfgs,
    std::set<std::string> inses,
    std::set<std::string> outses,
    std::set<std::string> invos,
    std::set<std::string> outvos
) :
    file (file),
    db (DBSingleton::instance().getDBObjectInstance())
{

    srcSeName = file.sourceSe;
    destSeName = file.destSe;

    std::vector< std::shared_ptr<ShareConfig> > no_auto_share;

    for (auto it = cfgs.begin(); it != cfgs.end(); it++)
        {

            std::shared_ptr<ShareConfig>& cfg = *it;

            if (cfg->shareOnly)
                {
                    double tmp = 0;
                    int sum = 0;


                    if (cfg->source == ConfigurationAssigner::any)
                        {
                            // calculate for destination
                            tmp = db->getSeIn(outses, cfg->destination);
                            sum = db->sumUpVoShares(cfg->source, cfg->destination, invos);
                        }
                    else if (cfg->destination == ConfigurationAssigner::any)
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
                            tmp = (tmp * cfg->activeTransfers) / sum;
                        }
                    // round up the result
                    cfg->activeTransfers = static_cast<int>(tmp + 0.5);
                    cfg->shareOnly = false; // now the value has been set
                }

            if (cfg->activeTransfers != ConfigurationAssigner::automatic) no_auto_share.push_back(cfg);
        }

    this->cfgs = no_auto_share;
}

FileTransferScheduler::~FileTransferScheduler()
{

}

bool FileTransferScheduler::schedule(int &currentActive)
{
    try
        {
            if(cfgs.empty())
                {
                    bool allowed = db->isTrAllowed(srcSeName, destSeName, currentActive);
                    if(allowed)
                        {
                            return true;
                        }
                    return false;
                }

            for (auto it = cfgs.begin(); it != cfgs.end(); ++it)
                {

                    std::string source = (*it)->source;
                    std::string destination = (*it)->destination;
                    std::string vo = (*it)->vo;

                    std::shared_ptr<ShareConfig>& cfg = *it;

                    if (!cfg.get()) continue; // if the configuration has been deleted in the meanwhile continue

                    // check if the configuration allows this type of transfer-job
                    if (!cfg->activeTransfers)
                        {
                            // failed to allocate active transfers credits to transfer-job
                            std::string msg = getNoCreditsErrMsg(cfg.get());
                            // set file status to failed
                            db->updateTransferStatus(
                                file.jobId, file.fileId, 0.0,
                                "FAILED", msg,
                                0, 0.0, 0.0, false
                            );
                            // set job states if necessary
                            db->updateJobStatus(
                                file.jobId, "FAILED", 0
                            );
                            // log it
                            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << msg << commit;
                            // the file has been resolved as FAILED, it won't be scheduled
                            return false;
                        }

                    int active_transfers = 0;

                    if (source == ConfigurationAssigner::wildcard)
                        {
                            active_transfers = db->countActiveOutboundTransfersUsingDefaultCfg(srcSeName, vo);
                            if (cfg->activeTransfers - active_transfers > 0) continue;
                            return false;
                        }

                    if (destination == ConfigurationAssigner::wildcard)
                        {
                            active_transfers = db->countActiveInboundTransfersUsingDefaultCfg(destSeName, vo);
                            if (cfg->activeTransfers - active_transfers > 0) continue;
                            return false;
                        }

                    active_transfers = db->countActiveTransfers(source, destination, vo);
                    if (cfg->activeTransfers - active_transfers > 0) continue;
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

std::string FileTransferScheduler::getNoCreditsErrMsg(ShareConfig* cfg)
{
    std::stringstream ss;

    ss << "Failed to allocate active transfer credits to transfer job due to ";

    if (cfg->source == ConfigurationAssigner::wildcard || cfg->destination == ConfigurationAssigner::wildcard)
        {

            ss << "the default standalone SE configuration.";

        }
    else if (cfg->source != ConfigurationAssigner::any && cfg->destination != ConfigurationAssigner::any)
        {

            ss << "a pair configuration (" << cfg->source << " -> " << cfg->destination << ").";

        }
    else if (cfg->source != ConfigurationAssigner::any)
        {

            ss << "a standalone out-bound configuration (" << cfg->source << ").";

        }
    else if (cfg->destination != ConfigurationAssigner::any)
        {

            ss << "a standalone in-bound configuration (" << cfg->destination << ").";

        }
    else
        {

            ss << "configuration constraints.";
        }

    ss << "Only the following VOs are allowed: ";

    std::vector<ShareConfig> cfgs = db->getShareConfig(cfg->source, cfg->destination);

    for (auto it = cfgs.begin(); it != cfgs.end(); ++it)
        {
            if (it->activeTransfers)
                {
                    if (it != cfgs.begin())
                        ss << ", ";
                    ss << it->vo;
                }
        }

    return ss.str();
}

