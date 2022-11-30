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

    friend std::ostream& operator << (std::ostream& os, const UrlCopyCmd&);

public:
    static const std::string Program;
    static std::string prepareMetadataString(const std::string& text);

    UrlCopyCmd();

    std::string generateParameters(void);

    void setLogDir(const std::string&);
    void setMonitoring(bool, const std::string&);
    void setPingInterval(int interval);
    void setInfosystem(const std::string&);
    void setOptimizerLevel(int);
    void setDebugLevel(int);
    void setProxy(const std::string&);
    void setUDT(boost::tribool);
    void setIPv6(boost::tribool);
    void setEvict(boost::tribool);
    bool isIPv6Explicit(void);
    void setFTSName(const std::string&);
    void setOAuthFile(const std::string&);
    void setAuthMethod(const std::string&);
    void setRetrieveSEToken(bool);

    void setFromTransfer(const TransferFile&, bool isMultiple, bool publishUserDn, const std::string &msgDir);

    void setFromProtocol(const TransferFile::ProtocolParameters& protocol);
    void setSecondsPerMB(long);

    void setNumberOfActive(int);
    void setNumberOfRetries(int);
    void setMaxNumberOfRetries(int);
    void setDisableDelegation(bool);
    void setDisableStreaming(bool);
    void setOverwrite(bool);
    void setThirdPartyTURL(const std::string&);

    // Observers
    int getBuffersize();
    int getNoStreams();
    int getTimeout();
};

std::ostream& operator << (std::ostream& os, const fts3::server::UrlCopyCmd&);

}
}

#endif // URLCOPYPARAMS_H
