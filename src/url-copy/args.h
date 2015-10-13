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

#pragma once

#include <getopt.h>
#include <iostream>
#include <string>

class UrlCopyOpts
{
public:
    static UrlCopyOpts& getInstance();

    int parse(int argc, char * const argv[]);
    void usage(const std::string& bin);
    std::string getErrorMessage();

    // Flags
    bool monitoringMessages;
    bool autoTunned;
    bool manualConfig;
    bool overwrite;
    bool logToStderr;
    bool reuse;
    bool multihop;
    bool enable_udt;
    bool enable_ipv6;
    bool global_timeout;
    bool strictCopy;
    unsigned debugLevel;
    bool hide_user_dn;
    int level;
    int active;

    enum CompareChecksum
    {
        CHECKSUM_DONT_CHECK = 0, // Do not check checksum
        CHECKSUM_STRICT,          // Strict comparison
        CHECKSUM_RELAXED          // Relaxed comparision. i.e. do not fail on empty checksum on source
    } compareChecksum;

    // Arguments
    std::string infosys;
    std::string tokenBringOnline;
    std::string fileMetadata;
    std::string jobMetadata;
    std::string sourceSiteName;
    std::string destSiteName;
    std::string vo;
    std::string checksumValue;
    std::string jobId;
    std::string sourceUrl;
    std::string destUrl;
    std::string sourceTokenDescription;
    std::string destTokenDescription;
    unsigned    fileId;
    std::string proxy;
    long long   userFileSize;
    int         bringOnline;
    int         copyPinLifetime;
    int         nStreams;
    unsigned    tcpBuffersize;
    unsigned    blockSize;
    unsigned    timeout;
    int     secPerMb;
    std::string user_dn;
    std::string alias;
    std::string oauthFile;
    int retry;
    int retry_max;
    std::string job_m_replica;
    std::string last_replica;
    std::string logDir;

    bool areTransfersOnFile() const
    {
        return reuse || multihop;
    }

private:
    static UrlCopyOpts instance;
    static const option long_options[];
    static const char short_options[];

    UrlCopyOpts();
    UrlCopyOpts(const UrlCopyOpts&);

    std::string errorMessage;
};


std::ostream& operator << (std::ostream& out, const UrlCopyOpts::CompareChecksum& c);
