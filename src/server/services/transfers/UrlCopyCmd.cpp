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

/**
 * See url-copy/args.cpp to see the list of supported arguments
 */

#include "UrlCopyCmd.h"
#include <boost/algorithm/string/replace.hpp>

using namespace fts3::server;


const std::string UrlCopyCmd::Program("fts_url_copy");


std::string UrlCopyCmd::prepareMetadataString(const std::string& text)
{
    std::string copy(text);
    copy = boost::replace_all_copy(copy, " ", "?");
    copy = boost::replace_all_copy(copy, "\"", "\\\"");
    return copy;
}


void UrlCopyCmd::setFlag(const std::string& key, bool set)
{
    options.erase(key);

    std::list<std::string>::iterator i = std::find(flags.begin(), flags.end(), key);
    if (set && i == flags.end())
        flags.push_back(key);
    else if (!set && i != flags.end())
        flags.erase(i);
}


void UrlCopyCmd::setOption(const std::string& key, const std::string& value, bool skip_if_empty)
{
    std::list<std::string>::iterator i = std::find(flags.begin(), flags.end(), key);
    if (i != flags.end())
        flags.erase(i);
    if (!value.empty() || !skip_if_empty)
        options[key] = value;
}


std::string UrlCopyCmd::generateParameters(void)
{
    std::ostringstream cmd;

    for (auto flag = flags.begin(); flag != flags.end(); ++flag) {
        cmd << " --" << *flag;
    }

    for (auto option = options.begin(); option != options.end(); ++option) {
        cmd << " --" << option->first << " " << option->second;
    }

    return cmd.str();
}


void UrlCopyCmd::setLogDir(const std::string& path)
{
    setOption("logDir", path);
}


void UrlCopyCmd::setMonitoring(bool set)
{
    setFlag("monitoring", set);
}


void UrlCopyCmd::setManualConfig(bool set)
{
    setFlag("manual-config", set);
}


void UrlCopyCmd::setAutoTuned(bool set)
{
    setFlag("auto-tunned", set);
}


void UrlCopyCmd::setInfosystem(const std::string& infosys)
{
    setOption("infosystem", infosys);
}


void UrlCopyCmd::setOptimizerLevel(int level)
{
    setOption("level", level);
}


void UrlCopyCmd::setDebugLevel(int level)
{
    setOption("debug", level);
}


void UrlCopyCmd::setProxy(const std::string& path)
{
    setOption("proxy", path);
}


void UrlCopyCmd::setUDT(bool set)
{
    setFlag("udt", set);
}


void UrlCopyCmd::setIPv6(bool set)
{
    IPv6Explicit = true;
    setFlag("ipv6", set);
}


bool UrlCopyCmd::isIPv6Explicit(void)
{
    return IPv6Explicit;
}


void UrlCopyCmd::setShowUserDn(bool show)
{
    setFlag("hide-user-dn", !show);
}


void UrlCopyCmd::setFTSName(const std::string& hostname)
{
    setOption("alias", hostname);
}


void UrlCopyCmd::setOAuthFile(const std::string& path)
{
    setOption("oauth", path);
}


void UrlCopyCmd::setGlobalTimeout(long timeout)
{
    // I have no clue why is this done in this way, but that's how it was before the refactoring
    // Need to figure it out, eventually
    // Have a look at url-copy/heuristics.cpp, it is used as a flag for actually using the value of --timeout,
    // but I do not know why
    setFlag("global-timeout", timeout > 0);
}


void UrlCopyCmd::setFromTransfer(const TransferFile& transfer, bool is_multiple)
{
    setOption("file-metadata", prepareMetadataString(transfer.fileMetadata));
    setOption("job-metadata", prepareMetadataString(transfer.jobMetadata));
    if (transfer.bringOnline > 0)
        setOption("bringonline", transfer.bringOnline);
    setFlag("reuse", transfer.reuseJob == "Y");
    setFlag("multi-hop", transfer.reuseJob == "H");
    // setOption("source-site", std::string());
    // setOption("dest-site", std::string());
    setOption("vo", transfer.voName);
    if (!transfer.checksumMethod.empty())
        setOption("compare-checksum", transfer.checksumMethod);
    if (transfer.pinLifetime > 0)
        setOption("pin-lifetime", transfer.pinLifetime);
    setOption("job-id", transfer.jobId);
    setFlag("overwrite", !transfer.overwriteFlag.empty());
    setOption("dest-token-desc", transfer.destinationSpaceToken);
    setOption("source-token-desc", transfer.sourceSpaceToken);
    setOption("user-dn", prepareMetadataString(transfer.userDn));
    if (transfer.reuseJob == "R")
        setOption("job_m_replica", "true");
    setOption("last_replica", transfer.lastReplica == 1? "true": "false");

    // On multiple jobs, this data is per transfer and is passed via a file
    // under /var/lib/fts3/<job-id>, so skip it
    if (!is_multiple) {
        setOption("file-id", transfer.fileId);
        setOption("source", transfer.sourceSurl);
        setOption("destination", transfer.destSurl);
        setOption("checksum", transfer.checksum);
        if (transfer.userFilesize > 0)
            setOption("user-filesize", transfer.userFilesize);
        setOption("token-bringonline", transfer.bringOnlineToken);
    }
}


void UrlCopyCmd::setFromProtocol(const ProtocolResolver::protocol& protocol)
{
    if (protocol.nostreams >= 0) {
        setOption("nstreams", protocol.nostreams);
    }
    else {
        setOption("nstreams", DEFAULT_NOSTREAMS);
    }

    if (protocol.urlcopy_tx_to >= 0) {
        setOption("timeout", protocol.urlcopy_tx_to);
    }
    else {
        setOption("timeout", DEFAULT_TIMEOUT);
    }

    if (protocol.tcp_buffer_size > 0) {
        setOption("tcp-buffersize", protocol.tcp_buffer_size);
    }
    else {
        setOption("tcp-buffersize", DEFAULT_BUFFSIZE);
    }


    if (!indeterminate(protocol.ipv6)) {
        this->setIPv6(protocol.ipv6);
    }

    setFlag("strict-copy", protocol.strict_copy);
}


void UrlCopyCmd::setSecondsPerMB(long value)
{
    setOption("sec-per-mb", value);
}


void UrlCopyCmd::setNumberOfActive(int active)
{
    setOption("active", active);
}


void UrlCopyCmd::setNumberOfRetries(int count)
{
    setOption("retry", count);
}


void UrlCopyCmd::setMaxNumberOfRetries(int retry_max)
{
    setOption("retry_max", retry_max);
}
