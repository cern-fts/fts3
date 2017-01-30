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

#ifndef SERVICEADAPTER_H_
#define SERVICEADAPTER_H_

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>

#include "JobStatus.h"
#include "File.h"

#include <string>
#include <vector>
#include <map>
#include <tuple>

namespace fts3
{
namespace cli
{

class ServiceAdapter
{
    friend class ServiceAdapterFallbackFacade;

public:
    ServiceAdapter(std::string const & endpoint) : endpoint(endpoint) {}
    virtual ~ServiceAdapter() {}

    /**
     * Handle static authorization/revocation
     */
    virtual void authorize(const std::string& op, const std::string& dn) = 0;

    /**
     * Remote call to blacklistDn
     *
     * @param subject - the DN that will be added/removed from blacklist
     * @param status  - either CANCEL, WAIT or WAIT_AS
     * @param timeout - the timeout for the jobs already in queue
     * @param mode    - set or unset the blacklisting
     */
    virtual void blacklistDn(std::string subject, std::string status, int timeout, bool mode) = 0;

    /**
     * Remote call to blacklistSe
     *
     * @param name    - name of the SE
     * @param vo      - name of the VO for whom the SE should be blacklisted (optional)
     * @param status  - either CANCEL, WAIT or WAIT_AS
     * @param timeout - the timeout for the jobs already in queue
     * @param mode    - set or unset the blacklisting
     */
    virtual void blacklistSe(std::string name, std::string vo, std::string status, int timeout, bool mode) = 0;

    /**
     * Remote call to cancel
     *
     * @param jobIds : list of job IDs
     */
    virtual std::vector< std::pair<std::string, std::string>  > cancel(std::vector<std::string> const & jobIds) = 0;

    /**
     * Remote call to cancelAll
     */
    virtual boost::tuple<int, int> cancelAll(const std::string& vo) = 0;

    /**
     * Remote call to debugSet
     *
     * set the debug mode to on/off for
     * a given pair of SEs or a single SE
     *
     * @param source - source se (or the single SE
     * @param destination - destination se (might be empty)
     * @param level - debug level
     */
    virtual void debugSet(std::string source, std::string destination, unsigned level) = 0;

    /**
     * Remote call to delConfiguration
     *
     * @param cfgs - vector of configurations to delete
     */
    virtual void delConfiguration(std::vector<std::string> const &cfgs) = 0;

    /**
     * Delegates a proxy certificate
     *
     * @param delegationId : delegation ID
     * @param expirationTime : user defined expiration time
     */
    virtual void delegate(std::string const & delegationId, long expirationTime) = 0;

    /**
     * Remote call for the removal of files
     */
    virtual std::string deleteFile (const std::vector<std::string>& filesForDelete) = 0;

    /**
     * Remote call to doDrain
     *
     * switches the drain mode
     *
     * @param  drain - on/off
     */
    virtual void doDrain(bool drain) = 0;

    /**
     * Remote call to listRequests
     * Internally is listRequests2
     *
     * @param dn user dn
     * @param vo vo name
     * @param array statuses of interest
     * @param resp server response
     */
    virtual std::vector<JobStatus> listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination) = 0;

    /**
     * Remote call to listDeletionRequests
     *
     * @param dn user dn
     * @param vo vo name
     * @param array statuses of interest
     * @param resp server response
     */
    virtual std::vector<JobStatus> listDeletionRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination) = 0;

    /**
     * Remote call to getBandwidthLimit
     */
    virtual std::string getBandwidthLimit() = 0;

    /**
     * Remote call to getConfiguration
     *
     * @param vo - vo name that will be used to filter the response
     * @param name - SE or SE group name that will be used to filter the response
     */
    virtual std::vector<std::string> getConfiguration (std::string src, std::string dest, std::string all, std::string name) = 0;

    /**
     * Remote call to getFileStatus
     *
     * @param jobId   id of the job
     * @param archive if true, the archive will be queried
     * @param offset  query starting from this offset (i.e. files 100 in advance)
     * @param limit   query a limited number of files (i.e. only 50 results)
     * @param retries get file retries
     * @param resp server response
     * @return The number of files returned
     */
    virtual std::vector<FileInfo> getFileStatus (std::string const & jobId, bool archive, int offset, int limit, bool retries) = 0;

    /**
     * Remote call to getTransferJobStatus
     *
     * @param jobId   the job id
     * @param archive if true, the archive will be queried
     *
     * @return an object holding the job status
     */
    virtual JobStatus getTransferJobStatus (std::string const & jobId, bool archive) = 0;

    /**
     * Remote call to getTransferJobSummary
     * Internally it is getTransferJobSummary3
     *
     * @param jobId   id of the job
     * @param archive if true, the archive will be queried
     *
     * @return an object containing job summary
     */
    virtual JobStatus getTransferJobSummary (std::string const & jobId, bool archive) = 0;

    virtual const std::string &getVersion() const { return version; }

    /**
     * Checks the expiration date of the local proxy certificate
     * @return expiration date of the proxy certificate
     */
    virtual long isCertValid() = 0;

    /**
     * Remote call to optimizerModeSet
     *
     * @param mode - optimizer mode
     */
    virtual void optimizerModeSet(int mode) = 0;

    void printServiceDetails();

    /**
     * Remote call to prioritySet
     *
     * Sets priority for the given job
     *
     * @param jobId - the id of the job
     * @param priority - the priority to be set
     */
    virtual void prioritySet(std::string jobId, int priority) = 0;

    /**
     * Remote call to queueTimeoutSet
     */
    virtual void queueTimeoutSet(unsigned timeout) = 0;

    /**
     * Remote call to retrySet
     *
     * @param retry - number of retries to be set
     */
    virtual void retrySet(std::string vo, int retry) = 0;

    /**
     * Handle static authorization/revocation
     */
    virtual void revoke(const std::string& op, const std::string& dn) = 0;

    /**
     * Remote call to setBandwidthLimit
     */
    virtual void setBandwidthLimit(const std::string& source_se, const std::string& dest_se, int limit) = 0;

    /**
     * Remote call to setConfiguration
     *
     * @param cfgs - vector of configurations to be set
     */
    virtual void setConfiguration (std::vector<std::string> const &cfgs) = 0;

    /**
     * Remote call to setDropboxCeredential
     *
     * @param appKey    : S3 app key
     * @param appSecret : S3 app secret
     * @param apiUrl    : service API URL (usually https://www.dropbox.com/1)
     */
    virtual void setDropboxCredential(std::string const & appKey, std::string const & appSecret, std::string const & apiUrl) = 0;

    /**
     * Remote call to fixActivePerPair
     */
    virtual void setFixActivePerPair(std::string source, std::string destination, int active) = 0;

    /**
     * Remote call to setGlobalLimits
     */
    virtual void setGlobalLimits(boost::optional<int> maxActivePerLink, boost::optional<int> maxActivePerSe) = 0;

    /**
     * Remote call to setGlobalTimeout
     */
    virtual void setGlobalTimeout(int timeout) = 0;

    /**
     * Remote call to setMaxDstSeActive
     */
    virtual void setMaxDstSeActive(std::string se, int active) = 0;

    /**
     * Remote call to setBringOnline
     *
     * @param triplet - se name, max number staging files, vo name
     * @param operation - 'staging' or 'delete'
     */
    virtual void setMaxOpt(std::tuple<std::string, int, std::string> const &triplet, std::string const &opt) = 0;

    /**
     * Remote call to setMaxSrcSeActive
     */
    virtual void setMaxSrcSeActive(std::string se, int active) = 0;

    /**
     * Remote call to setS3Ceredential
     *
     * @param accessKey : S3 access key
     * @param secretKey : S3 secret key
     * @param vo        : VO name
     * @param storage   : storage name (e.g. s3://hostname.com)
     */
    virtual void setS3Credential(std::string const & accessKey, std::string const & secretKey, std::string const & vo, std::string const & storage) = 0;

    /**
     * Remote call to setSecPerMb
     */
    virtual void setSecPerMb(int secPerMb) = 0;

    /**
     * Sets the protocol (UDT) for given SE
     *
     * @param protocol - for now only 'udt' is supported
     * @param se - the name of the SE in question
     * @param state - either 'on' or 'off'
     */
    virtual void setSeProtocol(std::string protocol, std::string se, std::string state) = 0;

    /**
     * Remote call to showUserDn
     *
     * switches the show-user-DN mode
     *
     * @param  show - on/off
     */
    virtual void showUserDn(bool show) = 0;

    /**
     * Remote call that will be transferSubmit2 or transferSubmit3
     *
     * @param elements - job elements to be executed
     * @param parameters - parameters for the job that is being submitted
     * @param extraParameters - raw json with the extra parameters
     *
     * @return the job ID
     */
    virtual std::string transferSubmit (std::vector<File> const & files,
        std::map<std::string, std::string> const & parameters, boost::property_tree::ptree const & extraParameters) = 0;

protected:

    virtual void getInterfaceDetails() = 0;

    std::string const endpoint;

    ///@{
    /**
     * general informations about the FTS3 service
     */
    std::string interface;
    std::string version;
    std::string schema;
    std::string metadata;
    ///@}
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* SERVICEADAPTER_H_ */
