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

#include <boost/algorithm/string/replace.hpp>
#include <json/json.h>

#include "UrlCopyCmd.h"


namespace fts3 {
namespace server {

const std::string UrlCopyCmd::Program("fts_url_copy");


UrlCopyCmd::UrlCopyCmd() : IPv6Explicit(false)
{
}


std::string UrlCopyCmd::prepareMetadataString(const std::string &text)
{
    std::string copy = boost::replace_all_copy(text, " ", "?");
    copy = boost::replace_all_copy(copy, "\"", "\\\"");
    return copy;
}


void UrlCopyCmd::setFlag(const std::string &key, bool set)
{
    options.erase(key);

    std::list<std::string>::iterator i = std::find(flags.begin(), flags.end(), key);
    if (set && i == flags.end())
        flags.push_back(key);
    else if (!set && i != flags.end())
        flags.erase(i);
}


void UrlCopyCmd::setOption(const std::string &key, const std::string &value, bool skip_if_empty)
{
    std::list<std::string>::iterator i = std::find(flags.begin(), flags.end(), key);
    if (i != flags.end())
        flags.erase(i);
    if (!value.empty() || !skip_if_empty)
        options[key] = value;
}


std::string UrlCopyCmd::generateParameters()
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


void UrlCopyCmd::setLogDir(const std::string &path)
{
    setOption("logDir", path);
}


void UrlCopyCmd::setMonitoring(bool set, const std::string &msgDir)
{
    setOption("msgDir", msgDir);
    setFlag("monitoring", set);
}


void UrlCopyCmd::setPingInterval(int interval)
{
    setOption("ping-interval", interval);
}


void UrlCopyCmd::setOptimizerLevel(int level)
{
    setOption("level", level);
}


void UrlCopyCmd::setDebugLevel(int level)
{
    setOption("debug", level);
}


void UrlCopyCmd::setProxy(const std::string &path)
{
    setOption("proxy", path);
}


void UrlCopyCmd::setUDT(boost::tribool set)
{
    if (boost::indeterminate(set)) {
        setFlag("udt", false);
    } else {
        bool value = (set.value == true);
        setFlag("udt", value);
    }
}


void UrlCopyCmd::setIPv6(boost::tribool set)
{
    if (boost::indeterminate(set)) {
        IPv6Explicit = false;
        setFlag("ipv6", false);
        setFlag("ipv4", false);
    }
    else {
        bool value = (set.value == true);
        IPv6Explicit = true;
        setFlag("ipv6", value);
        setFlag("ipv4", !value);
    }
}


void UrlCopyCmd::setSkipEvict(boost::tribool set)
{
    if (boost::indeterminate(set)) {
        setFlag("skip-evict", false);
    } else {
        bool value = (set.value == true);
        setFlag("skip-evict", value);
    }
}


void UrlCopyCmd::setOverwriteDiskEnabled(boost::tribool set)
{
    if (boost::indeterminate(set)) {
        setFlag("overwrite-disk-enabled", false);
    } else {
        setFlag("overwrite-disk-enabled", (set.value == true));
    }
}


void UrlCopyCmd::setCopyMode(CopyMode copyMode)
{
    std::string mode;

    switch (copyMode) {
        case (CopyMode::ANY):
            mode = "pull";
            break;
        case (CopyMode::PULL):
            mode = "pull";
            // Explicitly disable fallback because only PULL will work
            setFlag("disable-fallback", true);
            break;
        case (CopyMode::PUSH):
            mode = "push";
            // Explicitly disable fallback because only PUSH will work
            setFlag("disable-fallback", true);
            break;
        case (CopyMode::STREAMING):
            mode = "streamed";
            break;
        default:
            // Do not set copy-mode and let the url-copy use its own default copy mode
            return;
    }

    setOption("copy-mode", mode);
}


bool UrlCopyCmd::isIPv6Explicit()
{
    return IPv6Explicit;
}

void UrlCopyCmd::setFTSName(const std::string &hostname)
{
    setOption("alias", hostname);
}

void UrlCopyCmd::setCloudConfigFile(const std::string &path)
{
    setOption("cloud-config", path);
}


void UrlCopyCmd::setOAuthFile(const std::string &path)
{
    setOption("oauth-file", path);
}


void UrlCopyCmd::setSourceTokenId(const std::string &token_id)
{
    setOption("src-token-id", token_id);
}


void UrlCopyCmd::setDestinationTokenId(const std::string &token_id)
{
    setOption("dst-token-id", token_id);
}


void UrlCopyCmd::setAuthMethod(const std::string &method)
{
    setOption("auth-method", method);
}


void UrlCopyCmd::setSourceTokenUnmanaged(bool unmanaged)
{
    setFlag("src-token-unmanaged", unmanaged);
}


void UrlCopyCmd::setDestinationTokenUnmanaged(bool unmanaged)
{
    setFlag("dst-token-unmanaged", unmanaged);
}


void UrlCopyCmd::setTokenRefreshMarginPeriod(int margin)
{
    setOption("token-refresh-margin", margin);
}


void UrlCopyCmd::setFromTransfer(const TransferFile &transfer,
    bool is_multiple, bool publishUserDn, const std::string &msgDir)
{
    setOption("job-metadata", prepareMetadataString(transfer.jobMetadata));

    switch (transfer.jobType) {
        case Job::kTypeSessionReuse:
            setFlag("reuse", true);
            break;
        case Job::kTypeMultipleReplica:
            setFlag("job-m-replica", true);
            break;
        case Job::kTypeMultiHop:
            setFlag("job-multihop", true);
            break;
        case Job::kTypeRegular:
            break;
    }

    setOption("vo", transfer.voName);
    setOption("job-id", transfer.jobId);
    setFlag("overwrite", transfer.overwriteFlag == "Y");

    if (!transfer.checksumMode.empty()) {
        setOption("checksum-mode", transfer.checksumMode);
    }

    if (transfer.archiveTimeout > 0) {
        setFlag("archiving", true);
        setFlag("dst-file-report", !transfer.dstFileReport.empty());
    }

    setOption("source-space-token", transfer.sourceSpaceToken);
    setOption("dest-space-token", transfer.destinationSpaceToken);

    if (!transfer.fileMetadata.empty()) {
        try {
            Json::Value json;
            std::istringstream ss(transfer.fileMetadata);
            ss >> json;

            for (const auto& issuerType: {"source-issuer", "dest-issuer"}) {
                auto issuer = json.get(issuerType, "").asString();

                if (!issuer.empty()) {
                    setOption(issuerType, issuer);
                }
            }
        } catch (const Json::Exception&) {
            // Ignore for now.
        }
    }

    if (publishUserDn)
        setOption("user-dn", prepareMetadataString(transfer.userDn));

    setFlag("last-replica", transfer.lastReplica);
    setFlag("last-hop", transfer.lastHop);

    // On multiple jobs, this data is per transfer and is passed via a file
    // under /var/lib/fts3/<job-id>, so skip it
    if (!is_multiple) {
        setOption("file-id", transfer.fileId);
        setOption("source", transfer.sourceSurl);
        setOption("destination", transfer.destSurl);
        setOption("checksum", transfer.checksum);
        if (transfer.userFilesize > 0) {
            setOption("user-filesize", transfer.userFilesize);
        }
        if (transfer.scitag > 0) {
            setOption("scitag", transfer.scitag);
        }
        setOption("token-bringonline", transfer.bringOnlineToken);
        setOption("file-metadata", prepareMetadataString(transfer.fileMetadata));
        setOption("archive-metadata", prepareMetadataString(transfer.archiveMetadata));
        if (!transfer.activity.empty()) {
            setOption("activity", transfer.activity);
        }
    }
    else {
        setOption("bulk-file", msgDir + "/" + transfer.jobId);
    }
}


void UrlCopyCmd::setFromProtocol(const TransferFile::ProtocolParameters &protocol)
{
    if (protocol.nostreams > 0) {
        setOption("nstreams", protocol.nostreams);
    }

    if (protocol.timeout > 0) {
        setOption("timeout", protocol.timeout);
    }

    if (protocol.buffersize > 0) {
        setOption("tcp-buffersize", protocol.buffersize);
    }

    if (!indeterminate(protocol.ipv6)) {
        this->setIPv6(protocol.ipv6);
    }
    if (!indeterminate(protocol.udt)) {
        this->setUDT(protocol.udt);
    }

    setFlag("strict-copy", protocol.strictCopy);
    setFlag("disable-cleanup", protocol.disableCleanup);
}


void UrlCopyCmd::setSecondsPerMB(long value)
{
    setOption("sec-per-mb", value);
}


void UrlCopyCmd::setNumberOfRetries(int count)
{
    setOption("retry", count);
}


void UrlCopyCmd::setMaxNumberOfRetries(int retry_max)
{
    setOption("retry_max", retry_max);
}


void UrlCopyCmd::setDisableDelegation(bool disable_delegation)
{
    setFlag("no-delegation", disable_delegation);
}


void UrlCopyCmd::setDisableStreaming(bool disable_streaming)
{
    setFlag("no-streaming", disable_streaming);
}


void UrlCopyCmd::setOverwrite(bool overwrite)
{
    setFlag("overwrite", overwrite);
}


void UrlCopyCmd::setOverwriteOnDisk(bool overwrite)
{
    setFlag("overwrite-on-disk", overwrite);
}


void UrlCopyCmd::setThirdPartyTURL(const std::string& thirdPartyTURL)
{
    setOption("3rd-party-turl", thirdPartyTURL);
}


int UrlCopyCmd::getBuffersize()
{
    auto buffersize = options["tcp-buffersize"];
    if (buffersize.empty()) {
        return 0;
    }
    return boost::lexical_cast<int>(buffersize);
}


int UrlCopyCmd::getNoStreams()
{
    auto nstreams = options["nstreams"];
    if (nstreams.empty()) {
        return 0;
    }
    return boost::lexical_cast<int>(nstreams);
}


int UrlCopyCmd::getTimeout()
{
    auto timeout = options["timeout"];
    if (timeout.empty()) {
        return 0;
    }
    return boost::lexical_cast<int>(timeout);
}


std::ostream &operator<<(std::ostream &os, const UrlCopyCmd &cmd)
{
    os << UrlCopyCmd::Program << " ";

    for (auto flag = cmd.flags.begin(); flag != cmd.flags.end(); ++flag) {
        os << " --" << *flag;
    }

    for (auto option = cmd.options.begin(); option != cmd.options.end(); ++option) {
        os << " --" << option->first << " \"" << option->second << "\"";
    }

    return os;
}

}
}
