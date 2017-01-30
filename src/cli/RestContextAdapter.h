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

#ifndef RESTCONTEXTADAPTER_H_
#define RESTCONTEXTADAPTER_H_

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

#include "ServiceAdapter.h"
#include "exception/rest_client_not_implemented.h"

#include "JobStatus.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace fts3
{
namespace cli
{

class RestContextAdapter : public ServiceAdapter
{

public:
    RestContextAdapter(std::string const & endpoint, std::string const & capath, std::string const & proxy, bool insecure):
        ServiceAdapter(removeTrailingSlash(endpoint)),
        capath(capath),
        proxy(proxy),
        insecure(insecure)
    {
     	if (this->capath.empty()) {
            const char *x509_cert_dir = getenv("X509_CERT_DIR");
            if (x509_cert_dir) {
                this->capath = x509_cert_dir;
            } else {
                this->capath = "/etc/grid-security/certificates";
            }
        }
	if (this->proxy.empty()) {
            const char *x509_user_proxy = getenv("X509_USER_PROXY");
            if (x509_user_proxy) {
                this->proxy = x509_user_proxy;
            } else {
                std::ostringstream proxy_path;
                proxy_path << "/tmp/x509up_u" << geteuid();
                this->proxy = proxy_path.str();
            }
        }
    }

    virtual ~RestContextAdapter() {}

    void authorize(const std::string&, const std::string&) { throw rest_client_not_implemented("authorize"); }
    void blacklistDn(std::string subject, std::string status, int timeout, bool mode);
    void blacklistSe(std::string name, std::string vo, std::string status, int timeout, bool mode);
    std::vector< std::pair<std::string, std::string>  > cancel(std::vector<std::string> const & jobIds);
    boost::tuple<int, int> cancelAll(const std::string& vo);
    void debugSet(std::string source, std::string destination, unsigned level);
    void delConfiguration(std::vector<std::string> const &) { throw rest_client_not_implemented("delConfiguration"); }
    void delegate(std::string const & delegationId, long expirationTime);
    std::string deleteFile (const std::vector<std::string>& filesForDelete);
    void doDrain(bool) { throw rest_client_not_implemented("doDrain"); }
    std::string getBandwidthLimit() { throw rest_client_not_implemented("getBandwidthLimit"); }
    std::vector<std::string> getConfiguration (std::string, std::string, std::string, std::string) { throw rest_client_not_implemented("getConfiguration"); }
    std::vector<FileInfo> getFileStatus (std::string const & jobId, bool archive, int offset, int limit, bool retries);
    long isCertValid();
    JobStatus getTransferJobStatus (std::string const & jobId, bool archive);
    JobStatus getTransferJobSummary (std::string const & jobId, bool archive);
    std::vector<JobStatus> listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination);
    std::vector<JobStatus> listDeletionRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination);
    void optimizerModeSet(int) { throw rest_client_not_implemented("optimizerModeSet"); }
    void prioritySet(std::string jobId, int priority);
    void queueTimeoutSet(unsigned) { throw rest_client_not_implemented("queueTimeoutSet"); }
    void retrySet(std::string, int) { throw rest_client_not_implemented("retrySet"); }
    void revoke(const std::string&, const std::string&) { throw rest_client_not_implemented("revoke"); }
    void setBandwidthLimit(const std::string&, const std::string&, int) { throw rest_client_not_implemented("setBandwidthLimit"); }
    void setConfiguration (std::vector<std::string> const &) { throw rest_client_not_implemented("setConfiguration"); }
    void setDropboxCredential(std::string const &, std::string const &, std::string const &) { throw rest_client_not_implemented("setDropboxCredential"); }
    void setFixActivePerPair(std::string, std::string, int) { throw rest_client_not_implemented("setFixActivePerPair"); }
    void setGlobalLimits(boost::optional<int>, boost::optional<int>) { throw rest_client_not_implemented("setGlobalLimits"); }
    void setGlobalTimeout(int) { throw rest_client_not_implemented("setGlobalTimeout"); }
    void setMaxDstSeActive(std::string, int) { throw rest_client_not_implemented("setMaxDstSeActive"); }
    void setMaxOpt(std::tuple<std::string, int, std::string> const &, std::string const&) { throw rest_client_not_implemented("setMaxOpt"); }
    void setMaxSrcSeActive(std::string, int) { throw rest_client_not_implemented("setMaxSrcSeActive"); }
    void setS3Credential(std::string const &, std::string const &, std::string const &, std::string const &) { throw rest_client_not_implemented("setS3Credential"); }
    void setSecPerMb(int) { throw rest_client_not_implemented("setSecPerMb"); }
    void setSeProtocol(std::string, std::string, std::string) { throw rest_client_not_implemented("setSeProtocol"); }
    void showUserDn(bool) { throw rest_client_not_implemented("showUserDn"); }
    std::string transferSubmit (std::vector<File> const & files,
        std::map<std::string, std::string> const &parameters, boost::property_tree::ptree const& extraParams);


private:

    void getInterfaceDetails();
    static std::string removeTrailingSlash(const std::string &p) {
        std::string r(p);
        if (r.size()>0 && r[r.size()-1]=='/')
            r.erase(r.size()-1);
        return r;
    }

    std::string capath;
    std::string proxy;
    bool insecure;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* RESTCONTEXTADAPTER_H_ */
