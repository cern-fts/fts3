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

#ifndef URLCOPYPARAMS_H
#define URLCOPYPARAMS_H

#include <list>
#include <map>
#include <string>

#include "db/generic/TransferFile.h"
#include "ProtocolResolver.h"

namespace fts3 {
namespace server {

class UrlCopyCmd {
private:
    std::map<std::string, std::string> options;
    std::list<std::string> flags;

    void setFlag(const std::string&, bool);
    void setOption(const std::string&, const std::string&, bool skip_if_empty=true);

    template <typename T>
    void setOption(const std::string& key, const T& value) {
        setOption(key, boost::lexical_cast<std::string>(value));
    }

    bool IPv6Explicit;

public:
    static const std::string Program;
    static std::string prepareMetadataString(const std::string& text);

    UrlCopyCmd(): IPv6Explicit(false) {}

    std::string generateParameters(void);

    void setLogDir(const std::string&);
    void setMonitoring(bool);
    void setManualConfig(bool);
    void setAutoTuned(bool);
    void setInfosystem(const std::string&);
    void setOptimizerLevel(int);
    void setDebugLevel(int);
    void setProxy(const std::string&);
    void setUDT(bool);
    void setIPv6(bool);
    bool isIPv6Explicit(void);
    void setShowUserDn(bool);
    void setFTSName(const std::string&);
    void setOAuthFile(const std::string&);

    void setGlobalTimeout(long);

    void setFromTransfer(const TransferFile&, bool is_multiple=false);

    void setFromProtocol(const ProtocolResolver::protocol& protocol);
    void setSecondsPerMB(long);

    void setNumberOfActive(int);
    void setNumberOfRetries(int);
    void setMaxNumberOfRetries(int);
};

}
}

#endif // URLCOPYPARAMS_H
