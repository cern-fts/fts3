/*
 * Copyright (c) CERN 2022
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

#include "common/Logger.h"
#include "common/ThreadPool.h"
#include "common/DaemonTools.h"

#include "config/ServerConfig.h"
#include "cred/DelegCred.h"

#include "db/generic/SingleDbInstance.h"
#include "db/generic/TransferFile.h"

#include "server/DrainMode.h"

#include "ForceStartTransfersService.h"
#include "FileTransferExecutor.h"

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace server {

ForceStartTransfersService::ForceStartTransfersService(HeartBeat *beat) : BaseService("ForceStartTransfersService"), beat(beat) {
    logDir = config::ServerConfig::instance().get<std::string>("TransferLogDirectory");
    msgDir = config::ServerConfig::instance().get<std::string>("MessagingDirectory");
    execPoolSize = config::ServerConfig::instance().get<int>("InternalThreadPool");
    ftsHostName = config::ServerConfig::instance().get<std::string>("Alias");
    infosys = config::ServerConfig::instance().get<std::string>("Infosys");

    monitoringMessages = config::ServerConfig::instance().get<bool>("MonitoringMessaging");
    pollInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("ForceStartTransfersCheckInterval");
}

void ForceStartTransfersService::forceRunJobs() {

    // Bail out as soon as possible if there are too many fts_url_copy processes
    int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
    int urlCopyCount = countProcessesWithName("fts_url_copy");
    int availableUrlCopySlots = maxUrlCopy - urlCopyCount;

    if (availableUrlCopySlots <= 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Reached limitation of MaxUrlCopyProcesses (ForceStartTransfers)"
                                           << commit;
        return;
    }

    ThreadPool<FileTransferExecutor> execPool(execPoolSize);

    try {
        auto tfs = db::DBSingleton::instance().getDBObjectInstance()->getForceStartTransfers();

        if (tfs.empty()) {
            return;
        }

        std::map<std::pair<std::string, std::string>, std::string> proxies;

        for (auto& tf: tfs) {
            if (boost::this_thread::interruption_requested()) {
                execPool.interrupt();
                return;
            }

            if (tf.fileId == 0 || tf.userDn.empty() || tf.credId.empty()) {
                continue;
            }

            std::pair<std::string, std::string> proxy_key(tf.credId, tf.userDn);

            if (proxies.find(proxy_key) == proxies.end()) {
                proxies[proxy_key] = DelegCred::getProxyFile(tf.userDn, tf.credId);
            }

            FileTransferExecutor *exec = new FileTransferExecutor(tf, monitoringMessages, infosys, ftsHostName,
                                                                  proxies[proxy_key], logDir, msgDir);
            execPool.start(exec);

            if (--availableUrlCopySlots <= 0) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Reached limitation of MaxUrlCopyProcesses (ForceStartTransfers)"
                                                   << commit;
                break;
            }
        }

        // Wait for all the workers to finish
        execPool.join();
        int scheduled = execPool.reduce(std::plus<int>());
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Force Start Threadpool processed: " << tfs.size() << " files ("
                                        << scheduled << " have been scheduled)" << commit;

    } catch (const boost::thread_interrupted &) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in ForceStartTransfersService:forceRunJobs" << commit;
        execPool.interrupt();
        execPool.join();
    } catch (std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in ForceStartTransfersService:forceRunJobs " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in ForceStartTransfersService!" << commit;
    }
}

void ForceStartTransfersService::runService() {
    while (!boost::this_thread::interruption_requested()) {
        try {
            boost::this_thread::sleep(pollInterval);

            // This service intentionally does not follow the drain mechanism

            if (beat->isLeadNode()) {
                forceRunJobs();
            }
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot delete old files " << e.what() << fts3::common::commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot delete old files" << fts3::common::commit;
        }
    }
}

} //end namespace server
} //end namespace fts3
