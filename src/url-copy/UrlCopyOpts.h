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

#ifndef URLCOPYOPTS_H
#define URLCOPYOPTS_H

#include <getopt.h>
#include <iostream>
#include <string>
#include <boost/logic/tribool.hpp>


class UrlCopyOpts
{
public:
    // Default constructor
    UrlCopyOpts();

    // Parse arguments. Exit immediately if invalid, after printing a message.
    void parse(int argc, char * const argv[]);

    // Print usage message and exit
    void usage(const std::string& bin);

    // Parameters
    std::string bulkFile;

    bool isSessionReuse;
    bool isMultipleReplicaJob;

    bool strictCopy;

    std::string voName;
    std::string userDn;
    std::string proxy;
    std::string oauthFile;

    std::string infosys;
    std::string alias;

    std::string jobMetadata;

    std::string authMethod;

    unsigned optimizerLevel;
    bool     overwrite;
    bool     noDelegation;
    unsigned nStreams;
    unsigned tcpBuffersize;
    unsigned timeout;
    bool     enableUdt;
    boost::tribool enableIpv6;
    unsigned addSecPerMb;
    bool     enableMonitoring; // Legacy option
    unsigned active; // Legacy option

    unsigned retry;
    unsigned retryMax;

    std::string logDir;
    std::string msgDir;

    unsigned debugLevel;
    bool     logToStderr;

    Transfer::TransferList transfers;

private:
    void validateRequired();

    static const option long_options[];
    static const char short_options[];
};


#endif // URLCOPYOPTS_H
