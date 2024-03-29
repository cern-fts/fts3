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

#ifndef STORAGECONFIG_H_
#define STORAGECONFIG_H_

#include <string>
#include <boost/logic/tribool.hpp>

class StorageConfig
{
public:
    StorageConfig(): ipv6(boost::indeterminate), udt(boost::indeterminate),
                     debugLevel(0),
                     inboundMaxActive(0), outboundMaxActive(0),
                     inboundMaxThroughput(0), outboundMaxThroughput(0) {};

    // Merge the configuration from b into this
    // In other words, everything that is not set in this, is picked from b
    void merge(const StorageConfig &b) {
        if (boost::indeterminate(ipv6)) {
            ipv6 = b.ipv6;
        }
        if (boost::indeterminate(udt)) {
            udt = b.udt;
        }
        if (debugLevel <= 0) {
            debugLevel = b.debugLevel;
        }
        if (inboundMaxActive <= 0) {
            inboundMaxActive = b.inboundMaxActive;
        }
        if (outboundMaxActive <= 0) {
            outboundMaxActive = b.outboundMaxActive;
        }
        if (inboundMaxThroughput <= 0) {
            inboundMaxThroughput = b.inboundMaxThroughput;
        }
        if (outboundMaxThroughput <= 0) {
            outboundMaxThroughput = b.outboundMaxThroughput;
        }
    }

    std::string storage;
    std::string site;
    std::string metadata;
    boost::tribool ipv6, udt;
    int debugLevel;
    int inboundMaxActive, outboundMaxActive;
    double inboundMaxThroughput, outboundMaxThroughput;
};

#endif // STORAGECONFIG_H_
