//
// Created by Tom Hepworth on 29/08/2022.
//

#include "ForceStartTransferService.h"

#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"

#include "server/DrainMode.h"

#include "FileTransferExecutor.h"

#include "common/ThreadPool.h"

#include "common/DaemonTools.h"

#include "cred/DelegCred.h"

#include "db/generic/TransferFile.h"

using namespace fts3::config;
using namespace fts3::common;
using namespace db;

namespace fts3 {
namespace server {

ForceStartTransferService::ForceStartTransferService(HeartBeat *beat): BaseService("ForceStartTransferService"), beat(beat) {
    logDir = config::ServerConfig::instance().get<std::string>("TransferLogDirectory");
    msgDir = config::ServerConfig::instance().get<std::string>("MessagingDirectory");
    execPoolSize = config::ServerConfig::instance().get<int>("InternalThreadPool");
    ftsHostName = config::ServerConfig::instance().get<std::string>("Alias");
    infosys = config::ServerConfig::instance().get<std::string>("Infosys");

    monitoringMessages = config::ServerConfig::instance().get<bool>("MonitoringMessaging");
    pollInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("ForceStartTransferCheckInterval");
}

void ForceStartTransferService::forceRunJobs() {

    // Bail out as soon as possible if there are too many url-copy processes
    int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
    int urlCopyCount = countProcessesWithName("fts_url_copy");
    int availableUrlCopySlots = maxUrlCopy - urlCopyCount;

    if (availableUrlCopySlots <= 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Reached limitation of MaxUrlCopyProcesses" << commit;
        return;
    }

    ThreadPool<FileTransferExecutor> execPool(execPoolSize);

    try {
        std::list<TransferFile> tfs = db::DBSingleton::instance().getDBObjectInstance()->getForceStartTransfers();

        std::map<std::pair<std::string, std::string>, std::string> proxies;

        for (auto & tf : tfs) {
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

            FileTransferExecutor *exec = new FileTransferExecutor(tf, monitoringMessages, infosys,
                                                                  ftsHostName,proxies[proxy_key], logDir, msgDir);
            execPool.start(exec);

            if (--availableUrlCopySlots <= 0) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Reached limitation of MaxUrlCopyProcesses" << commit;
                break;
            }
        }

        // wait for all the workers to finish
        execPool.join();
        int scheduled = execPool.reduce(std::plus<int>());
        FTS3_COMMON_LOGGER_NEWLOG(INFO) <<"Force Start Threadpool processed: " << tfs.size()
                                        << " files (" << scheduled << " have been scheduled)" << commit;

    } catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TransfersService:getFiles" << commit;
        execPool.interrupt();
        execPool.join();
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService:getFiles " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
    }
}

void ForceStartTransferService::runService() {
    while (!boost::this_thread::interruption_requested()) {
        try {
            boost::this_thread::sleep(pollInterval);

            if (DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more force start transfers for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            if (beat->isLeadNode()) {
                forceRunJobs();
            }
        } catch(std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot delete old files " << e.what() <<  fts3::common::commit;
        } catch(...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot delete old files" <<  fts3::common::commit;
        }
    }
}

} //end namespace server
} //end namespace fts3