/*
 * Copyright (c) CERN 2023
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

//
// Created by aalvarez on 11/20/15.
//

#pragma once
#ifndef STORAGEPAIRSTATE_H
#define STORAGEPAIRSTATE_H

#include <string>


struct StoragePairState
{
    StoragePairState(const std::string &source, const std::string &destination,
        double successRate, double throughput, double retryThroughput,
        int active, int throughputSamples, int throughputSamplesEqual):
        source(source), destination(destination),
        successRate(successRate), throughput(throughput), retryThroughput(retryThroughput),
        active(active), throughputSamples(throughputSamples), throughputSamplesEqual(throughputSamplesEqual)
    {
    }

    std::string source;
    std::string destination;
    double successRate;
    double throughput;
    double retryThroughput;
    int active;
    int throughputSamples;
    int throughputSamplesEqual;
};

#endif //STORAGEPAIRSTATE_H
