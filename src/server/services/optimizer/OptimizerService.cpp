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
#include "OptimizerService.h"
#include "Optimizer.h"

#include "db/generic/SingleDbInstance.h"
#include "common/Logger.h"


namespace fts3 {
namespace server {

using optimizer::Optimizer;


OptimizerService::OptimizerService(HeartBeat *beat): BaseService("OptimizerService"), beat(beat)
{
}


void OptimizerService::runService()
{
    auto optimizerInterval = config::ServerConfig::instance().get<int>("OptimizerInterval");
    auto optimizerSteadyInterval = config::ServerConfig::instance().get<int>("OptimizerSteadyInterval");
    auto maxNumberOfStreams = config::ServerConfig::instance().get<int>("OptimizerMaxStreams");

    Optimizer optimizer(db::DBSingleton::instance().getDBObjectInstance()->getOptimizerDataSource());
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
        boost::this_thread::sleep(boost::posix_time::seconds(optimizerInterval));
    }
}


} // end namespace server
} // end namespace fts3
