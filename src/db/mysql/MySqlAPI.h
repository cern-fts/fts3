/*
 * Copyright (c) CERN 2013-2016
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

#include <soci/soci.h>
#include "db/generic/GenericDbIfce.h"
#include "db/generic/StoragePairState.h"
#include "msg-bus/consumer.h"
#include "msg-bus/producer.h"

class MySqlAPI : public GenericDbIfce
{
public:
    MySqlAPI();
    virtual ~MySqlAPI();

    struct HashSegment
    {
        unsigned start;
        unsigned end;

        HashSegment(): start(0), end(0xFFFF) {}
    } hashSegment;

    /// Initialize database connection by providing information from fts3config file
    /// @param nPooledConnections   The number connections to pool
    virtual void init(const std::string &dbtype, const std::string& username, const std::string& password,
        const std::string& connectString, int nPooledConnections);

    /// Return the type of the database
    /// @return The database type
    virtual std::string getDbtype() const;

    /// Recover from the DB transfers marked as ACTIVE for the host 'host'
    virtual std::list<fts3::events::MessageUpdater> getActiveInHost(const std::string &host);

    /// Get a list of transfers ready to go for the given queues
    /// When session reuse is enabled for a job, all the files belonging to that job should run at once
    /// @param queues       Queues for which to check (see getQueuesWithSessionReusePending)
    /// @param[out] files   A map where the key is the VO. The value is a queue of pairs (jobId, list of transfers)
    virtual void getReadySessionReuseTransfers(const std::vector<QueueId>& queues,
        std::map< std::string, std::queue< std::pair<std::string, std::list<TransferFile>>>>& files);

    /// Get a list of transfers ready to go for the given queues
    /// @param queues       Queues for which to check (see getQueuesWithPending)
    /// @param[out] files   A map where the key is the VO. The value is a list of transfers belonging to that VO
    virtual void getReadyTransfers(const std::vector<QueueId>& queues,
        std::map< std::string, std::list<TransferFile>>& files);

    /// Update the status of a transfer
    /// @param jobId            The job ID
    /// @param fileId           The file ID
    /// @param processId        fts_url_copy process ID running the transfer
    /// @param transferState    Transfer statue
    /// @param errorReason      For failed states, the error message
    /// @param filesize         Actual filesize reported by the storage
    /// @param duration         How long (in seconds) took to transfer the file
    /// @param throughput       Transfer throughput
    /// @param retry            If the error is considered recoverable by fts_url_copy
    /// @param fileMetadata     The new file metadata in case it needs to be updated
    /// @return                 true if an updated was done into the DB, false otherwise
    ///                         (i.e. trying to set ACTIVE an already ACTIVE transfer)
    /// @note                   If jobId is empty, or if fileId is 0, then processId will be used to decide
    ///                         which transfers to update
    virtual boost::tuple<bool, std::string> updateTransferStatus(const std::string& jobId, uint64_t fileId, int processId,
                                                                 const std::string& transferState, const std::string& errorReason,
                                                                 uint64_t filesize, double duration, double throughput,
                                                                 bool retry, const std::string& fileMetadata);

    /// Update the status of a job
    /// @param jobId            The job ID
    /// @param jobState         The job state
    virtual bool updateJobStatus(const std::string& jobId, const std::string& jobState);

    /// Get the token associated with the given token ID
    /// @param tokenId          The token ID
    /// @return                 The <token, unmanaged flag> pair for the token ID, if any
    virtual std::pair<std::string, bool> findToken(const std::string& tokenId);

    /// Get the credentials associated with the given delegation ID and user
    /// @param delegationId     Delegation ID. See insertCredentialCache
    /// @param userDn           The user's DN
    /// @return                 The delegated credentials, if any
    virtual boost::optional<UserCredential> findCredential(const std::string& delegationId, const std::string& userDn);

    /// Check if the credential for the given delegation id and user is expired or not
    /// @param delegationId     Delegation ID. See insertCredentialCache
    /// @param userDn           The user's DN
    /// @return                 true if the stored credentials expired or do not exist
    virtual bool isCredentialExpired(const std::string& delegationId, const std::string &userDn);

    /// Get the debug level for the given pair
    /// @param sourceStorage    The source storage as protocol://host
    /// @param destStorage      The destination storage as protocol://host
    /// @return                 An integer with the debug level configured for the pair. 0 = no debug.
    virtual unsigned getDebugLevel(const std::string& sourceStorage, const std::string& destStorage);

    /// Checks if there are available slots to run transfers for the given pair
    /// @param sourceStorage        The source storage  (as protocol://host)
    /// @param destStorage          The destination storage  (as protocol://host)
    virtual bool isTrAllowed(const std::string& sourceStorage, const std::string& destStorage);

    /// Mark a reuse job (and its files) as failed
    /// @param jobId    The job id
    /// @param pid      The PID of the fts_url_copy
    /// @param message  The error message
    /// @param force    Force termination regardless of job_type
    /// @note           If jobId is empty, the implementation may look for the job bound to the pid.
    ///                 Note that I am not completely sure you can get an empty jobId.
    virtual bool terminateReuseProcess(const std::string& jobId, int pid, const std::string& message, bool force);

    /// Goes through transfers marked as 'ACTIVE' and make sure the timeout didn't expire
    /// @param[out] transfers   An array with the expired transfers. Only jobId, fileId and pid are filled
    virtual void reapStalledTransfers(std::vector<TransferFile>& transfers);

    /// Set the PID for all the files inside a reuse or multihop job
    /// @param jobId    The job id for which the files will be updated
    /// @param pid      The process ID
    /// @note           Transfers within reuse and multihop jobs go all together to a single fts_url_copy process
    virtual void setPidForJob(const std::string& jobId, int pid);

    /// Moves old transfer and job records to the archive tables
    /// Delete old entries in other tables (i.e. t_optimize_evolution)
    /// @param[in] intervalDays Jobs older than this many days will be purged
    /// @param[in] bulkSize How many jobs per iteration must be processed
    /// @param[out] nJobs   How many jobs have been moved
    /// @param[out] nFiles  How many files have been moved
    /// @param[out] nDeletions  How many deletions have been moved
    virtual void backup(int intervalDays, long bulkSize, long* nJobs, long* nFiles, long* nDeletions);

    /// Mark all the transfers as failed because the process fork failed
    /// @param jobId    The job id for which url copy failed to fork
    /// @note           This method is used only for reuse jobs
    virtual void forkFailed(const std::string& jobId);

    /// Get the link configuration for the link defined by the source and destination given
    virtual std::unique_ptr<LinkConfig> getLinkConfig(const std::string &source, const std::string &destination);

    /// Get the list of VO share configurations for the given link
    virtual std::vector<ShareConfig> getShareConfig(const std::string &source, const std::string &destination);

    /// Returns how many retries there is configured for the given jobId
    virtual int getRetry(const std::string & jobId);

    /// Returns how many thime the given file has been already retried
    virtual int getRetryTimes(const std::string & jobId, uint64_t fileId);

    /// Set to FAIL jobs that have been in the queue for more than its max in queue time
    /// @param jobs An output parameter, where the set of expired job ids is stored
    virtual void setToFailOldQueuedJobs(std::vector<std::string>& jobs);

    /// Update the protocol parameters used for each transfer
    virtual void updateProtocol(const std::vector<fts3::events::Message>& messages);

    /// Get the state the transfer identified by jobId/fileId
    virtual std::vector<TransferState> getStateOfTransfer(const std::string& jobId, uint64_t fileId);

    /// Run a set of sanity checks over the database, logging potential inconsistencies and fixing them
    virtual void checkSanityState();

    /// Run a sanity check over the database on multihop jobs, logging potential inconsistencies and fixing them
    virtual void multihopSanitySate();

    /// Add a new retry to the transfer identified by fileId
    /// @param jobId    Job identifier
    /// @param fileId   Transfer identifier
    /// @param retryNo  The retry attempt number
    /// @param reason   String representation of the failure
    /// @param errcode  An integer representing the failure
    /// @param logFile  The log file path
    virtual void setRetryTransfer(const std::string& jobId, uint64_t fileId, int retryNo,
                                  const std::string& reason, const std::string& logFile, int errcode);

    /// Bulk update of transfer progress
    virtual void updateFileTransferProgressVector(const std::vector<fts3::events::MessageUpdater> &messages);

    /// Bulk update for log files
    virtual void transferLogFileVector(std::map<int, fts3::events::MessageLog>& messagesLog);

    /**
     * Signals that the server is alive
     * The total number of running (alive) servers is put in count
     * The index of this specific machine is put in index
     * A default implementation is provided, as this is used for optimization,
     * so it is not mandatory.
     * start and end are set to the interval of hash values this host will process
     */
    virtual void updateHeartBeat(unsigned* index, unsigned* count, unsigned* start, unsigned* end,
        std::string service_name);

    /// Update the state of a transfer inside a session reuse job
    virtual long updateFileStatusReuse(const TransferFile &file, const std::string &status);

    /// Puts into requestIDs, jobs that have been cancelled, and for which the running fts_url_copy must be killed
    virtual void getCancelJob(std::vector<int>& requestIDs);

    /// Returns list of transfers that need to be force started
    virtual std::list<TransferFile> getForceStartTransfers();

    /// Returns if this host has been set to drain
    virtual bool getDrain();

    /// Returns if for the given link, UDT has been enabled
    virtual boost::tribool isProtocolUDT(const std::string &source, const std::string &dest);

    /// Returns if for the given link, IPv6 has been enabled
    virtual boost::tribool isProtocolIPv6(const std::string &source, const std::string &dest);

    /// Returns if for the given storage endpoint, skip eviction has been enabled
    virtual boost::tribool getSkipEvictionFlag(const std::string &source);

    /// Returns whether the given storage endpoint is enabled for the "overwrite-when-only-on-disk" feature or not
    virtual boost::tribool getOverwriteDiskEnabledFlag(const std::string &storage);

    /// Returns TPC mode for a pair of source/destination endpoints
    virtual CopyMode getCopyMode(const std::string &source, const std::string &destination);

    /// Returns how many streams must be used for the given link
    virtual int getStreamsOptimization(const std::string &sourceSe, const std::string &destSe);

    /// Returns whether proxy delegation should be disabled for the given link
    virtual bool getDisableDelegationFlag(const std::string &sourceSe, const std::string &destSe);

    /// Returns the 3rd-party TURL for the given link
    virtual std::string getThirdPartyTURL(const std::string &sourceSe, const std::string &destSE);

    /// Returns the globally configured transfer timeout
    virtual int getGlobalTimeout(const std::string &voName);

    /// Returns how many seconds must be added to the timeout per MB to be transferred
    virtual int getSecPerMb(const std::string &voName);

    /// Returns the globally configured disable streaming flag
    virtual bool getDisableStreamingFlag(const std::string &voName);

    /// Puts into the vector queue the Queues for which there are pending transfers
    virtual void getQueuesWithPending(std::vector<QueueId>& queues);

    /// Puts into the vector queues the Queues for which there are session-reuse pending transfers
    virtual void getQueuesWithSessionReusePending(std::vector<QueueId>& queues);

    /// Updates the status for staging operations
    /// @param stagingOpStatus  Update for files in staging or started
    virtual void updateStagingState(const std::vector<MinFileStatus>& stagingOpStatus);

    /// Updates the status for archiving operations
    /// @param archivingOpStatus  Update for files in archiving
    virtual void updateArchivingState(const std::vector<MinFileStatus>& archivingOpStatus);

    /// Updates the start time for archiving operations
    /// @param jobs  A map where the key is the job id, and the value another map where the key is a surl, and the
    ///                     value a file id
    virtual void setArchivingStartTime(const std::map< std::string, std::map<std::string, std::vector<uint64_t> > > &jobs);

    /// Update the bring online token for the given set of transfers
    /// @param jobs     A map where the key is the job id, and the value another map where the key is a surl, and the
    ///                     value a file id
    /// @param token    The SRM token
    virtual void updateBringOnlineToken(const std::map< std::string, std::map<std::string, std::vector<uint64_t> > > &jobs,
        const std::string &token);

    /// Get staging operations ready to be started
    /// @params[out] stagingOps The list of staging operations will be put here
    virtual void getFilesForStaging(std::vector<StagingOperation> &stagingOps);

    /// Get archivingOps operations ready to be polled
    /// @params[out] archivingOps The list of Archiving  operations will be put here
    virtual void getFilesForArchiving(std::vector<ArchivingOperation> &archivingOps);

    /// Get staging operations already started
    /// @params[out] stagingOps The list of started staging operations will be put here
    virtual void getAlreadyStartedStaging(std::vector<StagingOperation> &stagingOps);

    /// Get archiving operations already started
    /// @params[out] archivingOps The list of started archiving operations will be put here
    virtual void getAlreadyStartedArchiving(std::vector<ArchivingOperation> &archivingOps);

    /// Put into files a set of bring online requests that must be cancelled
    /// @param files    Each entry in the set if a pair of surl / token
    virtual void getStagingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files);

    /// Put into files a set of archiving requests that must be cancelled
    /// @param files    Each entry in the set if a pair of jobid / surl
    virtual void getArchivingFilesForCanceling(std::set< std::pair<std::string, std::string> >& files);

    /// Returns list of access tokens without an associated refresh token
    virtual std::list<Token> getAccessTokensWithoutRefresh(int limit);

    /// Store a list of exchanged tokens
    virtual void storeExchangedTokens(const std::set<ExchangedToken>& exchangedTokens);

    /// Mark token-exchange retry timestamp and error message
    virtual void markFailedTokenExchange(const std::set< std::pair<std::string, std::string> >& failedExchanges);

    /// Fail transfers without refresh token due to failed token exchange
    virtual void failTransfersWithFailedTokenExchange(
            const std::set<std::pair<std::string, std::string> >& failedExchanges);

    /// Update all files found in "TOKEN_PREP" state which also have refresh tokens available
    virtual void updateTokenPrepFiles();

    /// Given a list of token ids, return a map of valid <token_id, Token>
    virtual std::map<std::string, Token> getValidAccessTokens(const std::list<std::string>& token_ids);

    /// Given a list of token ids, return a map of failed token-refreshes <token_id, (message, timestamp)>
    virtual std::map<std::string, std::pair<std::string, int64_t>>
        getFailedAccessTokenRefreshes(const std::list<std::string>& token_ids);

    /// Given a list of token ids, mark them for refreshing
    virtual void markTokensForRefresh(const std::list<std::string>& token_ids);

    /// Returns list of access tokens marked for refreshing
    virtual std::list<Token> getAccessTokensForRefresh(int limit);

    /// Store a list of refreshed tokens
    virtual void storeRefreshedTokens(const std::set<RefreshedToken>& refreshedTokens);

    /// Mark token-refresh timestamp and error message
    virtual void markFailedTokenRefresh(const std::set< std::pair<std::string, std::string> >& failedRefreshes);

    /// Retrieve the credentials for a cloud storage endpoint for the given user/VO
    virtual bool getCloudStorageCredentials(const std::string& userDn,
        const std::string& voName,
        const std::string& cloudName,
        CloudStorageAuth& auth);

    /// Get if the user dn should be visible or not in the messaging
    virtual bool publishUserDn(const std::string &vo);

    /// Get the configuration for a given storage
    virtual StorageConfig getStorageConfig(const std::string &storage);

    /// Get the list of Token Providers
    virtual std::map<std::string, TokenProvider> getTokenProviders();

    /// Return a list of pairs with active or submitted transfers
    /// @return A list containing all pairs with files in active state
    virtual std::list<Pair> getActivePairs();

    /// Return the optimizer configuration value
    /// @param  source  The source storage as protocol://host
    /// @param  dest    The destination storage as protocol://host
    /// @return         The configured optimizer mode
    virtual OptimizerMode getOptimizerMode(const std::string &source, const std::string &dest);

    /// Get configured limits
    /// @param      pair    A pair constituted of a source storage and a destination storage
    /// @param[out] range   The configured range for the input pair
    /// @param[out] limits  The configured storage limits for the input pair
    virtual void getPairLimits(const Pair &pair, Range &range, StorageLimits &limits);

    /// Get the stored optimizer value (current value)
    /// @param  pair    A pair constituted of a source storage and a destination storage
    /// @return         The optimizer value for the input pair
    virtual int getOptimizerValue(const Pair &pair);

    /// Get the weighted throughput for the pair
    /// @param  pair                A pair constituted of a source storage and a destination storage
    /// @param  interval            A time interval in seconds to compute the weighted throughput for the input pair
    /// @param[out] throughput      The throughput for the input pair during the input time interval
    /// @param[out] filesizeAvg     The average file size for the input pair during the input time interval
    /// @param[out] filesizeStdDev  The standard deviation of the file size for the input pair during the input time interval
    virtual void getThroughputInfo(const Pair &pair, const boost::posix_time::time_duration &interval,
                                   double *throughput, double *filesizeAvg, double *filesizeStdDev);

    /// Get average transfer duration on a given time interval
    /// @param  pair        A pair constituted of a source storage and a destination storage
    /// @param  interval    A time interval in seconds to compute the weighted throughput for the input pair
    /// return              The average transfer duration
    virtual time_t getAverageDuration(const Pair &pair, const boost::posix_time::time_duration &interval);

    /// Get the success rate for the pair
    /// @param      pair        A pair constituted of a source storage and a destination storage
    /// @param      interval    A time interval in seconds to compute the weighted throughput for the input pair
    /// @param[out] retryCount  The number of file transfers retried for the input pair during the input time interval
    /// @return                 The success rate for the input pair
    virtual double getSuccessRateForPair(const Pair &pair, const boost::posix_time::time_duration &interval,
                                         int *retryCount);

    /// Get the number of transfers in a given state
    /// @param  pair    A pair constituted of a source storage and a destination storage
    /// @param  state   The file state to be counted
    /// @return         The count of files in the input state for the input pair
    virtual int getCountInState(const Pair &pair, const std::string &state);

    /// Get current inbound throughput
    /// @param se   The storage endpoint as protocol://host
    /// @return     The inbound throughput
    virtual double getThroughputAsSource(const std::string &se);

    /// Get current outbound throughput
    /// @param se   The storage endpoint as protocol://host
    /// @return     The outbound throughput
    virtual double getThroughputAsDestination(const std::string &se);

    /// Permanently register the optimizer decision
    /// @param  pair            A pair constituted of a source storage and a destination storage
    /// @param  activeDecision  The optimizer decision taken
    /// @param  newState        The new optimizer state for the pair
    /// @param  diff            The diff in the optimizer decision
    /// @param  rationale       The rationale for the optimizer decision
    virtual void storeOptimizerDecision(const Pair &pair, int activeDecision,
                                        const PairState &newState, int diff, const std::string &rationale);

    /// Permanently register the number of streams per active
    /// @param  pair    A pair constituted of a source storage and a destination storage
    /// @param  streams The number of streams to be registerd for the pair
    virtual void storeOptimizerStreams(const Pair &pair, int streams);

    /// Get the list of scheduled file-transfers
    /// @param maxFiles The maximum number of file-transfers this method should return
    virtual std::list<TransferFile> postgresGetScheduledFileTransfers(const int maxFiles);

    /// Store maxUrlCopyProcesses for the fts_server running on this host into the t_hosts table
    /// @param maxUrlCopyProcesses Maximum number of fts_url_copy processes
    virtual void postgresStoreMaxUrlCopyProcesses(const int maxUrlCopyProcesses);

    /// Put all the transfers assigned to this host that are in the SELECTED state back in the queue
    virtual void recoverSelectedTransfers();

private:
    size_t                poolSize;
    soci::connection_pool* connectionPool;
    std::string           hostname;
    std::string username_;
    std::map<std::string, boost::posix_time::ptime> queuedStagingFiles;
    std::string m_dbtype;

    void updateHeartBeatInternal(soci::session& sql, unsigned* index, unsigned* count, unsigned* start, unsigned* end,
                                 const std::string& serviceName);

    std::map<std::string, int> getFilesNumPerActivity(soci::session& sql,
        std::string src, std::string dst, std::string vo, int filesNum,
        std::set<std::string> & defaultActivities);

    std::map<std::string, long long> getActivitiesInQueue(soci::session& sql, std::string src,
        std::string dst, std::string vo);

    std::map<std::string, double> getActivityShareConf(soci::session& sql, std::string vo);

    void updateArchivingStateInternal(soci::session& sql, const std::vector<MinFileStatus> &archivingOpsStatus);

    void updateStagingStateInternal(soci::session& sql, const std::vector<MinFileStatus> &stagingOpsStatus);

    boost::tuple<bool, std::string> updateFileTransferStatusInternal(soci::session& sql,
                                                                     std::string jobId, uint64_t fileId, int processId,
                                                                     std::string newFileState, const std::string& errorReason,
                                                                     uint64_t filesize, double duration, double throughput,
                                                                     bool retry, const std::string& fileMetadata);

    bool updateJobTransferStatusInternal(soci::session& sql, std::string jobId, const std::string& state);

    bool resetForRetryStaging(soci::session& sql, uint64_t fileId, const std::string & jobId, bool retry, int& times);

    uint64_t getBestNextReplica(soci::session& sql, const std::string & jobId, const std::string & voName);

    std::vector<TransferState> getStateOfTransferInternal(soci::session& sql, const std::string& jobId, uint64_t fileId);

    void useFileReplica(soci::session& sql, std::string jobId, uint64_t fileId, std::string destSurlUuid, soci::indicator destSurlUuidInd);

    uint64_t getNextHop(soci::session& sql, const std::string& jobId);

    void useNextHop(soci::session& sql, std::string jobId);

    void setNullDestSURLMultiHop(soci::session& sql, std::string jobId);

    bool isArchivingTransfer(soci::session& sql, const std::string& jobId,
                             const Job::JobType& jobType, int archiveTimeout);

    bool getDrainInternal(soci::session& sql);

    int getMaxTimeInQueue(const std::string &voName);

    bool publishUserDnInternal(soci::session& sql, const std::string &vo);

    // Sanity checks
    void fixJobNonTerminallAllFilesTerminal(soci::session &sql);
    void fixJobTerminalFileNonTerminal(soci::session &sql);
    void recoverFromDeadHosts(soci::session &sql);
    void recoverStalledStaging(soci::session &sql);
    void recoverStalledArchiving(soci::session &sql);

    void fixEmptyJob(soci::session &sql, const std::string &jobId);
    void fixNonTerminalJob(soci::session &sql, const std::string &jobId,
        uint64_t filesInJob, uint64_t cancelCount, uint64_t finishedCount, uint64_t failedCount);

    std::vector<std::string> getVos(void);
    void cancelExpiredJobsForVo(std::vector<std::string>& jobs, int maxTime, const std::string &vo);

    // Multihop Check
    void fixFilesInNotUsedState(soci::session &sql);
};

