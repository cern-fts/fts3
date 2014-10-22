/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 * GSoapContextAdapter.h
 *
 *  Created on: May 30, 2012
 *      Author: Michał Simon
 */

#ifndef GSOAPCONTEXADAPTER_H_
#define GSOAPCONTEXADAPTER_H_

#include "ServiceAdapter.h"

#include "JobStatus.h"
#include "Snapshot.h"
#include "File.h"

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <vector>
#include <map>
#include <boost/property_tree/ptree.hpp>

using namespace std;
using namespace boost::property_tree;

namespace fts3
{
namespace cli
{

/**
 * The adapter class for the GSoap context
 *
 * This one is used instead of GSoap generated proxy classes because of GSoap bug ID: 3511337
 * (see example http://sourceforge.net/tracker/?func=detail&atid=468021&aid=3511337&group_id=52781)
 *
 * Provides all the functionalities of transfer and configuration web service
 */
class GSoapContextAdapter : public ServiceAdapter
{

    struct Cleaner
    {
        Cleaner(GSoapContextAdapter* me) : me(me) {}

        void operator() ()
        {
            me->clean();
        }

        GSoapContextAdapter* me;
    };

public:

    /**
     * Constructor
     *
     * Creates and initializes GSoap context
     */
    GSoapContextAdapter(string endpoint);

    /**
     * Destructor
     *
     * Deallocates GSoap context
     */
    virtual ~GSoapContextAdapter();

    /**
     * Type cast operator.
     *
     * @return pointer do gsoap context (soap*)
     */
    operator soap*()
    {
        return ctx;
    }

    /**
     * Remote call that will be transferSubmit2 or transferSubmit3
     *
     * @param elements - job elements to be executed
     * @param parameters - parameters for the job that is being submitted
     * @param checksum - flag indicating whether the checksum should be used
     * 	(if false transferSubmit2 is used, otherwise transferSubmit3 is used)
     *
     * @return the job ID
     */
    std::string transferSubmit (std::vector<File> const & files, std::map<std::string, std::string> const & parameters);

    /**
     * Delegates the credential
     *
     * @param delegationId : delegation ID
     * @param expirationTime : requested expiration time
     */
    void delegate(std::string const & delegationId, long expirationTime);

    /**
     * Remote call to getTransferJobStatus
     *
     * @param jobId   the job id
     * @param archive if true, the archive will be queried
     *
     * @return an object holding the job status
     */
    JobStatus getTransferJobStatus (std::string const & jobId, bool archive);

    /**
     * Remote call to getRoles
     *
     * @param resp server response (roles)
     */
    void getRoles (impltns__getRolesResponse& resp);

    /**
     * Remote call to getRolesOf
     *
     * @param resp server response (roles)
     */
    void getRolesOf (string dn, impltns__getRolesOfResponse& resp);

    /**
     * Remote call to cancel
     *
     * @param jobIds list of job IDs
     */
    std::vector< std::pair<std::string, std::string>  > cancel(std::vector<std::string> const & jobIds);

    /**
     * Remote call to listRequests
     * Internally is listRequests2
     *
     * @param dn user dn
     * @param vo vo name
     * @param array statuses of interest
     * @param resp server response
     */
    std::vector<JobStatus> listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination);

    /**
     * Remote call to listDeletionRequests
     *
     * @param dn user dn
     * @param vo vo name
     * @param array statuses of interest
     * @param resp server response
     */
    virtual std::vector<JobStatus> listDeletionRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & source, std::string const & destination);

    /**
     * Remote call to listVOManagers
     *
     * @param vo vo name
     * @param resp server response
     */
    void listVoManagers (string vo, impltns__listVOManagersResponse& resp);

    /**
     * Remote call to getTransferJobSummary
     * Internally it is getTransferJobSummary3
     *
     * @param jobId   id of the job
     * @param archive if true, the archive will be queried
     *
     * @return an object containing job summary
     */
    JobStatus getTransferJobSummary (std::string const & jobId, bool archive);

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
    std::vector<FileInfo> getFileStatus (std::string const & jobId, bool archive, int offset, int limit, bool retries);

    /**
     * Remote call to setConfiguration
     *
     * @param config th configuration to be set
     * @param resp server response
     */
    void setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp);

    /**
     * Remote call to getConfiguration
     *
     * @param vo - vo name that will be used to filter the response
     * @param name - SE or SE group name that will be used to filter the response
     * @param resp - server response
     */
    void getConfiguration (string src, string dest, string all, string name, implcfg__getConfigurationResponse& resp);

    /**
     * Remote call to delConfiguration
     *
     * @param cfg - the configuration that will be deleted
     * @param resp - server response
     */
    void delConfiguration(config__Configuration *config, implcfg__delConfigurationResponse &resp);

    /**
     * Remote call to setBringOnline
     *
     * @param triplet - se name, max number staging files, vo name
     * @param operation - 'staging' or 'delete'
     */
    void setMaxOpt(std::tuple<std::string, int, std::string> const &, std::string const&);

    string deleteFile (std::vector<std::string>& filesForDelete);

    /**
     * Remote call to setBandwidthLimit
     */
    void setBandwidthLimit(const std::string& source_se, const std::string& dest_se, int limit);

    /**
     * Remote call to getBandwidthLimit
     */
    void getBandwidthLimit(implcfg__getBandwidthLimitResponse& resp);

    /**
     * Splits the given string, and sets:
     * 		- major number
     * 		- minor number
     * 		- patch number
     *
     * @param interface - interface version of FTS3 service
     */
    void setInterfaceVersion(string interface);

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
    void debugSet(string source, string destination, unsigned level);

    /**
     * Remote call to blacklistDN
     *
     * @param subject - the DN that will be added/removed from blacklist
     * @param status  - either CANCEL or WAIT
     * @param timeout - the timeout for the jobs already in queue
     * @param mode    - on/off
     */
    void blacklistDn(string subject, string status, int timeout, bool mode);

    /**
     * Remote call to blacklistSe
     *
     * @param name    - name of the SE
     * @param vo      - name of the VO for whom the SE should be blacklisted (optional)
     * @param status  - either CANCEL or WAIT
     * @param timeout - the timeout for the jobs already in queue
     * @param mode    - on/off
     */
    void blacklistSe(string name, string vo, string status, int timeout, bool mode);

    /**
     * Remote call to doDrain
     *
     * switches the drain mode
     *
     * @param  drain - on/off
     */
    void doDrain(bool drain);


    /**
     * Remote call to showUserDn
     *
     * switches the show-user-DN mode
     *
     * @param  show - on/off
     */
    void showUserDn(bool show);

    /**
     * Remote call to prioritySet
     *
     * Sets priority for the given job
     *
     * @param jobId - the id of the job
     * @param priority - the priority to be set
     */
    void prioritySet(string jobId, int priority);

    /**
     * Sets the protocol (UDT) for given SE
     *
     * @param protocol - for now only 'udt' is supported
     * @param se - the name of the SE in question
     * @param state - either 'on' or 'off'
     */
    void setSeProtocol(string protocol, string se, string state);

    /**
     * Remote call to retrySet
     *
     * @param retry - number of retries to be set
     */
    void retrySet(string vo, int retry);

    /**
     * Remote call to optimizerModeSet
     *
     * @param mode - optimizer mode
     */
    void optimizerModeSet(int mode);

    /**
     * Remote call to queueTimeoutSet
     */
    void queueTimeoutSet(unsigned timeout);

    /**
     * Remote call to setGlobalTimeout
     */
    void setGlobalTimeout(int timeout);

    /**
     * Remote call to setSecPerMb
     */
    void setSecPerMb(int secPerMb);

    /**
     * Remote call to setMaxDstSeActive
     */
    void setMaxDstSeActive(string se, int active);

    /**
     * Remote call to setMaxSrcSeActive
     */
    void setMaxSrcSeActive(string se, int active);

    /**
     * Remote call to fixActivePerPair
     */
    void setFixActivePerPair(string source, string destination, int active);

    /**
     * Remote call to setS3Ceredential
     *
     * @param accessKey : S3 access key
     * @param secretKey : S3 secret key
     * @param vo        : VO name
     * @param storage   : storage name (e.g. s3://hostname.com)
     */
    void setS3Credential(std::string const & accessKey, std::string const & secretKey, std::string const & vo, std::string const & storage);

    /**
     * Remote call to setDropboxCeredential
     *
     * @param appKey    : S3 app key
     * @param appSecret : S3 app secret
     * @param apiUrl    : service API URL (usually https://www.dropbox.com/1)
     */
    void setDropboxCredential(std::string const & appKey, std::string const & appSecret, std::string const & apiUrl);

    /**
     * @param vo : user VO name
     * @param src : source SE
     * @param dst : destination SE
     *
     * @return the snapshot
     */
    std::vector<Snapshot> getSnapShot(std::string const & vo, std::string const & src, std::string const & dst);

    /**
     * @param jobId : job ID
     * @return : vector containing detailed information about files in the given job (including file ID)
     */
    std::vector<DetailedFileStatus> getDetailedJobStatus(std::string const & jobId);

    std::string getVersion()
    {
        return version;
    }

private:

    void getInterfaceDeatailes();

    void clean();

    static void signalCallback(int signum);
    static vector<Cleaner> cleaners;

    ///
    soap* ctx;

    ///@{
    /**
     * Interface Version components
     */
    long major;
    long minor;
    long patch;
    ///@}
};

}
}

#endif /* GSOAPCONTEXADAPTER_H_ */
