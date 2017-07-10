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

#include <config/ServerConfig.h>
#include <monitoring/msg-ifce.h>
#include "OptimizerService.h"
#include "Optimizer.h"

#include "db/generic/SingleDbInstance.h"


namespace fts3 {
namespace server {

using optimizer::Optimizer;
using optimizer::OptimizerCallbacks;
using optimizer::PairState;


class OptimizerNotifier : public OptimizerCallbacks {
protected:
    Producer msgProducer;

public:
    OptimizerNotifier(const std::string &msgDir) : msgProducer(msgDir)
    {}

    OptimizerNotifier(const OptimizerNotifier &) = delete;

    void notifyDecision(const Pair &pair, int decision, const PairState &current,
        int diff, const std::string &rationale)
    {
        // Broadcast the decision
        OptimizerInfo msg;

        msg.source_se = pair.source;
        msg.dest_se = pair.destination;

        msg.timestamp = millisecondsSinceEpoch();

        msg.throughput = current.throughput;
        msg.avgDuration = current.avgDuration;
        msg.successRate = current.successRate;
        msg.retryCount = current.retryCount;
        msg.activeCount = current.activeCount;
        msg.queueSize = current.queueSize;
        msg.ema = current.ema;
        msg.filesizeAvg = current.filesizeAvg;
        msg.filesizeStdDev = current.filesizeStdDev;
        msg.connections = decision;
        msg.rationale = rationale;

        MsgIfce::getInstance()->SendOptimizer(msgProducer, msg);
    }
};


OptimizerService::OptimizerService(HeartBeat *beat): BaseService("OptimizerService"), beat(beat)
{
}


void OptimizerService::runService()
{
    typedef boost::posix_time::time_duration TDuration;

    auto optimizerInterval = config::ServerConfig::instance().get<TDuration>("OptimizerInterval");
    auto optimizerSteadyInterval = config::ServerConfig::instance().get<TDuration>("OptimizerSteadyInterval");
    auto maxNumberOfStreams = config::ServerConfig::instance().get<int>("OptimizerMaxStreams");
    auto maxSuccessRate = config::ServerConfig::instance().get<int>("OptimizerMaxSuccessRate");
    auto medSuccessRate = config::ServerConfig::instance().get<int>("OptimizerMedSuccessRate");
    auto lowSuccessRate = config::ServerConfig::instance().get<int>("OptimizerLowSuccessRate");
    auto baseSuccessRate = config::ServerConfig::instance().get<int>("OptimizerBaseSuccessRate");

    OptimizerNotifier optimizerCallbacks(
        config::ServerConfig::instance().get<std::string>("MessagingDirectory")
    );

    Optimizer optimizer(
        db::DBSingleton::instance().getDBObjectInstance()->getOptimizerDataSource(),
        &optimizerCallbacks
    );
    optimizer.setSteadyInterval(optimizerSteadyInterval);
    optimizer.setMaxNumberOfStreams(maxNumberOfStreams);

    while (!boost::this_thread::interruption_requested()) {
        try {
            if (beat->isLeadNode()) {
                optimizer.run();
            }
        }
        catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread OptimizerService " << e.what() <<
            fts3::common::commit;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread OptimizerService unknown" << fts3::common::commit;
        }
        boost::this_thread::sleep(optimizerInterval);
    }
}


} // end namespace server
} // end namespace fts3
