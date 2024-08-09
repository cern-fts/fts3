/*
 * Copyright (c) CERN 2013-2024
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

#pragma once
#ifndef OPTIMIZERSERVICE_H_
#define OPTIMIZERSERVICE_H_

#include "common/BaseService.h"
#include "server/services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace server {


class OptimizerService: public fts3::common::BaseService
{
public:
    OptimizerService(const std::shared_ptr<HeartBeat>& beat);
    void optimizeAllPairs();
    virtual void runService();

protected:
    std::shared_ptr<HeartBeat> beat;
    int optimizerPoolSize;
};

} // end namespace server
} // end namespace fts3

#endif // OPTIMIZERSERVICE_H_
