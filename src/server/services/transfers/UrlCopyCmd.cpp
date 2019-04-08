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
#include <cajun/json/elements.h>
#include <cajun/json/reader.h>
#include <boost/algorithm/string/replace.hpp>

namespace fts3 {
namespace server {

const std::string UrlCopyCmd::Program("fts_url_copy");


UrlCopyCmd::UrlCopyCmd() : IPv6Explicit(false)
{
}


std::string UrlCopyCmd::prepareMetadataString(const std::string &text)
{
    std::string copy(text);
    copy = boost::replace_all_copy(copy, " ", "?");
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


void UrlCopyCmd::setLogDir(const std::string &path)
{
    setOption("logDir", path);
}


void UrlCopyCmd::setMonitoring(bool set, const std::string &msgDir)
{
    setOption("msgDir", msgDir);
    setFlag("monitoring", set);
}


void UrlCopyCmd::setInfosystem(const std::string &infosys)
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


bool UrlCopyCmd::isIPv6Explicit(void)
{
    return IPv6Explicit;
}


void UrlCopyCmd::setFTSName(const std::string &hostname)
{
    setOption("alias", hostname);
}


void UrlCopyCmd::setOAuthFile(const std::string &path)
{
    setOption("oauth", path);
}

void UrlCopyCmd::setAuthMethod(const std::string &method)
{
    setOption("authMethod", method);
}

void UrlCopyCmd::setFromTransfer(const TransferFile &transfer,
    bool is_multiple, bool publishUserDn, const std::string &msgDir)
{
    setOption("file-metadata", prepareMetadataString(transfer.fileMetadata));
    setOption("job-metadata", prepareMetadataString(transfer.jobMetadata));

    switch (transfer.jobType) {
        case Job::kTypeSessionReuse:
            setFlag("reuse", true);
            break;
        case Job::kTypeMultipleReplica:
            setFlag("job_m_replica", true);
            break;
    }



    // setOption("source-site", std::string());
    // setOption("dest-site", std::string());
    setOption("vo", transfer.voName);
    if (!transfer.checksumMode.empty())
        setOption("checksum-mode", transfer.checksumMode);
    setOption("job-id", transfer.jobId);
    setFlag("overwrite", !transfer.overwriteFlag.empty());
    setOption("dest-token-desc", transfer.destinationSpaceToken);
    setOption("source-token-desc", transfer.sourceSpaceToken);

    if (!transfer.fileMetadata.empty())
    {
        std::istringstream ss(transfer.fileMetadata);
        json::Object obj;
        try {
            json::Reader::Read(obj, ss);
            json::Object::const_iterator it = obj.Find("source-issuer");
            if (it != obj.End())
            {
                const json::String issuer = it->element;
                setOption("source-issuer", issuer.Value());
            }
            it = obj.Find("dest-issuer");
            if (it != obj.End())
            {
                const json::String issuer = it->element;
                setOption("dest-issuer", issuer.Value());
            }
        } catch (json::Exception) {
            // Ignore for now.
        }
    }

    if (publishUserDn)
        setOption("user-dn", prepareMetadataString(transfer.userDn));

    setFlag("last_replica", transfer.lastReplica);

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
