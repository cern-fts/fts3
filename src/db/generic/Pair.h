/*
 * Copyright (c) CERN 2013-2017
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
#ifndef PAIR_H
#define PAIR_H

#include <iostream>
#include <string>
#include "common/Uri.h"

struct Pair {
    std::string source, destination;

    Pair(const std::string &s, const std::string &d): source(s), destination(d) {
    }

    bool isLanTransfer() const {
        return fts3::common::isLanTransfer(source, destination);
    }
};

// Required so it can be used as a key on a std::map
inline bool operator < (const Pair &a, const Pair &b) {
    return a.source < b.source || (a.source == b.source && a.destination < b.destination);
}

inline std::ostream& operator << (std::ostream &os, const Pair &pair) {
    return (os << pair.source << " => " << pair.destination);
}

struct Range {
    int min, max;
    // Set to true if min,max is configured specifically, or is a *->* configuration
    bool specific;
    // Set to true if min,max is configured with SE limits instead of link configuration
    bool storageSpecific;

    Range(): min(0), max(0), specific(false), storageSpecific(false) {}
};

inline std::ostream& operator << (std::ostream &os, const Range &range) {
    return (os << "[" << range.min << ", " << range.max << "]");
}

struct StorageLimits {
    int source, destination;
    double throughputSource, throughputDestination;

    StorageLimits(): source(0), destination(0),
                     throughputSource(0), throughputDestination(0) {}
};



struct PairState {
    time_t timestamp;
    double throughput;
    time_t avgDuration;
    double successRate;
    int retryCount;
    int activeCount;
    int queueSize;
    // Exponential Moving Average
    double ema;
    // Filesize statistics
    double filesizeAvg;
    double filesizeStdDev;
    // Optimizer last decision
    int connections;

    PairState(): timestamp(0), throughput(0), avgDuration(0), successRate(0), retryCount(0), activeCount(0),
                 queueSize(0), ema(0), filesizeAvg(0), filesizeStdDev(0), connections(1) {}

    PairState(time_t ts, double thr, time_t ad, double sr, int rc, int ac, int qs, double ema, int conn):
            timestamp(ts), throughput(thr), avgDuration(ad), successRate(sr), retryCount(rc),
            activeCount(ac), queueSize(qs), ema(ema), filesizeAvg(0), filesizeStdDev(0), connections(conn) {}
};

#endif // PAIR_H
