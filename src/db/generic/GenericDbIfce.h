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
#ifndef GENERICDBIFCE_H_
#define GENERICDBIFCE_H_

#include <list>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "JobStatus.h"
#include "common/JobParameterHandler.h"
#include "FileTransferStatus.h"
#include "SeConfig.h"
#include "SeGroup.h"
#include "SeProtocolConfig.h"
#include "QueueId.h"
#include "LinkConfig.h"
#include "ShareConfig.h"
#include "CloudStorageAuth.h"
#include "TransferState.h"

#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>

#include "DeleteOperation.h"
#include "Job.h"
#include "MinFileStatus.h"
#include "StagingOperation.h"
#include "StorageElement.h"
#include "TransferFile.h"
#include "UserCredential.h"
#include "UserCredentialCache.h"

#include "msg-bus/events.h"

#include "server/services/optimizer/Optimizer.h"


/// Hold information about individual submitted transfers
struct SubmittedTransfer
{
    SubmittedTransfer(): filesize(0), fileIndex(0), hashedId(0) {}
    std::string source;
    std::string destination;
    std::string sourceSe;
    std::string destSe;
    std::string checksum;
    double filesize;
    std::string metadata;
    std::string selectionStrategy;
    int fileIndex;
    boost::optional<int> waitTimeout;
    std::string activity;
    std::string state;
    unsigned hashedId;
};

///
class GenericDbIfce
{
public:

    virtual ~GenericDbIfce() {};

    /// Initialize database connection by providing information from fts3config file
    /// @param nPooledConnections   The number connections to pool
    virtual void init(const std::string& username, const std::string& password,
            const std::string& connectString, int nPooledConnections) = 0;

    /// Recover from the DB transfers marked as ACTIVE for the host 'host'
    virtual std::list<fts3::events::MessageUpdater> getActiveInHost(const std::string &host) = 0;

    /// Get a list of transfers ready to go for the given queues
    /// When session reuse is enabled for a job, all the files belonging to that job should run at once
    /// @param queues       Queues for which to check (see getQueuesWithSessionReusePending)
    /// @param[out] files   A map where the key is the VO. The value is a queue of pairs (jobId, list of transfers)
    virtual void getReadySessionReuseTransfers(const std::vector<QueueId>& queues,
            std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile>>>>& files) = 0;

    /// Get a list of transfers ready to go for the given queues
    /// @param queues       Queues for which to check (see getQueuesWithPending)
    /// @param[out] files   A map where the key is the VO. The value is a list of transfers belonging to that VO
    virtual void getReadyTransfers(const std::vector<QueueId>& queues,
            std::map< std::string, std::list<TransferFile>>& files) = 0;

    /// Get a list of multihop jobs ready to go
    /// @param[out] files   A map where the key is the VO. The value is a queue of pairs (jobId, list of transfers)
    virtual void getMultihopJobs(std::map< std::string, std::queue<std::pair<std::string, std::list<TransferFile>>>>& files) = 0;

    /// Update the status of a transfer
    /// @param jobId            The job ID
    /// @param fileId           The file ID
    /// @param throughput       Transfer throughput
    /// @param transferState    Transfer statue
    /// @param errorReason      For failed states, the error message
    /// @param processId        fts_url_copy process ID running the transfer
    /// @param filesize         Actual filesize reported by the storage
    /// @param duration         How long (in seconds) took to transfer the file
    /// @param retry            If the error is considered recoverable by fts_url_copy
    /// @return                 true if an updated was done into the DB, false otherwise
    ///                         (i.e. trying to set ACTIVE an already ACTIVE transfer)
    /// @note                   If jobId is empty, or if fileId is 0, then processId will be used to decide
    ///                         which transfers to update
    virtual bool updateTransferStatus(const std::string& jobId, int fileId, double throughput,
            const std::string& transferState, const std::string& errorReason,
            int processId, double filesize, double duration, bool retry) = 0;

    /// Update the status of a job
    /// @param jobId            The job ID
    /// @param jobState         The job state
    /// @param pid              The PID of the fts_url_copy process
    /// @note                   If jobId is empty, the pid will be used to decide which job to update
    virtual bool updateJobStatus(const std::string& jobId, const std::string& jobState, int pid) = 0;

    /// Get the credentials associated with the given delegation ID and user
    /// @param delegationId     Delegation ID. See insertCredentialCache
    /// @param userDn           The user's DN
    /// @return                 The delegated credentials, if any
    virtual boost::optional<UserCredential> findCredential(const std::string& delegationId, const std::string& userDn) = 0;

    /// Check if the credential for the given delegation id and user is expired or not
    /// @param delegationId     Delegation ID. See insertCredentialCache
    /// @param userDn           The user's DN
    /// @return                 true if the stored credentials expired or do not exist
    virtual bool isCredentialExpired(const std::string& delegationId, const std::string &userDn) = 0;

    /// Get the debug level for the given pair
    /// @param sourceStorage    The source storage as protocol://host
    /// @param destStorage      The destination storage as protocol://host
    /// @return                 An integer with the debug level configured for the pair. 0 = no debug.
    virtual unsigned getDebugLevel(const std::string& sourceStorage, const std::string& destStorage) = 0;

    /// Check whether submission is allowed for the given storage
    /// @param storage          The storage to blacklist (as protocol://host)
    /// @param voName           The submitting VO
    virtual bool allowSubmit(const std::string& storage, const std::string& voName) = 0;

    /// Check is the user has been banned
    virtual bool isDnBlacklisted(const std::string& userDn) = 0;

    /// Optimizer data source
    virtual fts3::optimizer::OptimizerDataSource* getOptimizerDataSource() = 0;

    /// Checks if there are available slots to run transfers for the given pair
    /// @param sourceStorage        The source storage  (as protocol://host)
    /// @param destStorage          The destination storage  (as protocol://host)
    /// @param[out] currentActive   The current number of running transfers is put here
    virtual bool isTrAllowed(const std::string& sourceStorage, const std::string& destStorage, int &currentActive) = 0;

    virtual int getSeOut(const std::string & source, const std::set<std::string> & destination) = 0;

    virtual int getSeIn(const std::set<std::string> & source, const std::string & destination) = 0;

    /// Mark a reuse or multihop job (and its files) as failed
    /// @param jobId    The job id
    /// @param pid      The PID of the fts_url_copy
    /// @param message  The error message
    /// @note           If jobId is empty, the implementation may look for the job bound to the pid.
    ///                 Note that I am not completely sure you can get an empty jobId.
    virtual bool terminateReuseProcess(const std::string & jobId, int pid, const std::string & message) = 0;

    /// Goes through transfers marked as 'ACTIVE' and make sure the timeout didn't expire
    /// @param[out] collectJobs A map of fileId with its corresponding jobId that have been cancelled
    virtual void forceFailTransfers(std::map<int, std::string>& collectJobs) = 0;

    /// Set the PID for all the files inside a reuse or multihop job
    /// @param jobId    The job id for which the files will be updated
    /// @param pid      The process ID
    /// @note           Transfers within reuse and multihop jobs go all together to a single fts_url_copy process
    virtual void setPidForJob(const std::string& jobId, int pid) = 0;

    /// Search for transfers stuck in 'READY' state and revert them to 'SUBMITTED'
    /// @note   AFAIK 'READY' only applies for reuse and multihop, but inside
    ///         MySQL reuse seems to be explicitly filtered out, so I am not sure
    ///         how much is this method actually doing
    virtual void revertToSubmitted() = 0;

    /// Moves old transfer and job records to the archive tables
    /// Delete old entries in other tables (i.e. t_optimize_evolution)
    /// @param[in]          How many jobs per iteration must be processed
    /// @param[out] nJobs   How many jobs have been moved
    /// @param[out] nFiles  How many files have been moved
    virtual void backup(long bulkSize, long* nJobs, long* nFiles) = 0;

    /// Mark all the transfers as failed because the process fork failed
    /// @param jobId    The job id for which url copy failed to fork
    /// @note           This method is used only for reuse and multihop jobs
    virtual void forkFailed(const std::string& jobId) = 0;

    /// Mark the files contained in 'messages' as stalled (FAILED)
    /// @param messages Only file_id, job_id and process_id from this is used
    /// @param diskFull Set to true if there are no messages because the disk is full
    virtual bool markAsStalled(const std::vector<fts3::events::MessageUpdater>& messages, bool diskFull) = 0;

    /// Return true if the group 'groupName' exists
    virtual bool checkGroupExists(const std::string & groupName) = 0;

    /// @return the group to which storage belong
    /// @note   It will be the empty string if there is no group
    virtual std::string getGroupForSe(const std::string storage) = 0;

    //t_config_symbolic
    virtual std::unique_ptr<LinkConfig> getLinkConfig(std::string source, std::string destination) = 0;

    virtual void addShareConfig(const ShareConfig& cfg) = 0;
    virtual std::unique_ptr<ShareConfig> getShareConfig(std::string source, std::string destination, std::string vo) = 0;
    virtual std::vector<ShareConfig> getShareConfig(std::string source, std::string destination) = 0;

    virtual void addFileShareConfig(int file_id, std::string source, std::string destination, std::string vo) = 0;

    virtual int countActiveTransfers(std::string source, std::string destination, std::string vo) = 0;

    virtual int countActiveOutboundTransfersUsingDefaultCfg(std::string se, std::string vo) = 0;

    virtual int countActiveInboundTransfersUsingDefaultCfg(std::string se, std::string vo) = 0;

    virtual int sumUpVoShares (std::string source, std::string destination, std::set<std::string> vos) = 0;

    virtual int getRetry(const std::string & jobId) = 0;

    virtual int getRetryTimes(const std::string & jobId, int fileId) = 0;

    virtual void setToFailOldQueuedJobs(std::vector<std::string>& jobs) = 0;

    virtual void updateProtocol(const std::vector<fts3::events::Message>& tempProtocol) = 0;

    virtual std::vector<TransferState> getStateOfTransfer(const std::string& jobId, int file_id) = 0;

    virtual void cancelWaitingFiles(std::set<std::string>& jobs) = 0;

    virtual void revertNotUsedFiles() = 0;

    virtual void checkSanityState() = 0;

    virtual void checkSchemaLoaded() = 0;

    virtual void setRetryTransfer(const std::string & jobId, int fileId, int retry, const std::string& reason,
        int errcode) = 0;

    virtual void updateFileTransferProgressVector(std::vector<fts3::events::MessageUpdater>& messages) = 0;

    virtual void transferLogFileVector(std::map<int, fts3::events::MessageLog>& messagesLog) = 0;

    /**
     * Signals that the server is alive
     * The total number of running (alive) servers is put in count
     * The index of this specific machine is put in index
     * A default implementation is provided, as this is used for optimization,
     * so it is not mandatory.
     * start and end are set to the interval of hash values this host will process
     */
    virtual void updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end, std::string service_name)
    {
        *index = 0;
        *count = 1;
        *start = 0x0000;
        *end   = 0xFFFF;
        service_name = std::string("");
    }

    virtual unsigned int updateFileStatusReuse(TransferFile const & file, const std::string status) = 0;

    virtual void getCancelJob(std::vector<int>& requestIDs) = 0;

    virtual bool getDrain() = 0;

    virtual bool isProtocolUDT(const std::string & source_hostname, const std::string & destination_hostname) = 0;

    virtual bool isProtocolIPv6(const std::string & source_hostname, const std::string & destination_hostname) = 0;

    virtual int getStreamsOptimization(const std::string & source_hostname, const std::string & destination_hostname) = 0;

    virtual int getGlobalTimeout() = 0;

    virtual int getSecPerMb() = 0;

    virtual int getBufferOptimization() = 0;

    virtual void getQueuesWithPending(std::vector<QueueId>& queues) = 0;

    virtual void getQueuesWithSessionReusePending(std::vector<QueueId>& queued) = 0;

    /// Updates the status for delete operations
    /// @param delOpsStatus  Update for files in delete or started
    virtual void updateDeletionsState(const std::vector<MinFileStatus>& delOpsStatus) = 0;

    /// Gets a list of delete operations in the queue
    /// @params[out] delOps A list of namespace operations (deletion)
    virtual void getFilesForDeletion(std::vector<DeleteOperation>& delOps) = 0;

    /// Revert namespace operations already in 'STARTED' back to the 'DELETE'
    /// state, so they re-enter the queue
    virtual void requeueStartedDeletes() = 0;

    /// Updates the status for staging operations
    /// @param stagingOpStatus  Update for files in staging or started
    virtual void updateStagingState(const std::vector<MinFileStatus>& stagingOpStatus) = 0;

    //  job_id / file_id / token
    virtual void updateBringOnlineToken(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token) = 0;

    /// Get staging operations ready to be started
    /// @params[out] stagingOps The list of staging operations will be put here
    virtual void getFilesForStaging(std::vector<StagingOperation> &stagingOps) = 0;

    /// Get staging operations already started
    /// @params[out] stagingOps The list of started staging operations will be put here
    virtual void getAlreadyStartedStaging(std::vector<StagingOperation> &stagingOps) = 0;

    //file_id / surl / token
    virtual void getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files) = 0;

    /// Retrieve the credentials for a cloud storage endpoint for the given user/VO
    virtual bool getCloudStorageCredentials(const std::string& userDn,
                                     const std::string& voName,
                                     const std::string& cloudName,
                                     CloudStorageAuth& auth) = 0;

    /// Get if the user dn should be visible or not in the logs
    virtual bool getUserDnVisible() = 0;
};

#endif // GENERICDBIFCE_H_
