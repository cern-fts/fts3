/*
 * Copyright (c) CERN 2024
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

#include <string>

#include <db/generic/Pair.h>
#include <monitoring/msg-ifce.h>

// Virtual class used by the optimizer to notify decisions
class OptimizerCallbacks {
public:
    virtual ~OptimizerCallbacks() {};
    virtual void notifyDecision(const Pair &pair, int decision, const PairState &current,
                                int diff, const std::string &rationale) = 0;
};

// Implementation of the virtual OptimizerCallbacks class defined above
class OptimizerNotifier : public OptimizerCallbacks {
protected:
    bool enabled;
    Producer msgProducer;

public:
    OptimizerNotifier(bool enabled, const std::string &msgDir): enabled(enabled), msgProducer(msgDir)
    {}

    OptimizerNotifier(const OptimizerNotifier &) = delete;

    void notifyDecision(const Pair& pair, int decision, const PairState& current,
                        int diff, const std::string& rationale)
    {
        if (!enabled) {
            return;
        }

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
        msg.diff = diff;

        MsgIfce::getInstance()->SendOptimizer(msgProducer, msg);
    }
};
