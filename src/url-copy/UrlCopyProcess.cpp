/*
 * Copyright (c) CERN 2016
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

#include <cstdlib>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>

#include "LogHelper.h"
#include "heuristics.h"
#include "AutoInterruptThread.h"
#include "UrlCopyProcess.h"
#include "version.h"
#include "DestFile.h"
#include "common/Logger.h"


using fts3::common::commit;


static void setupGlobalGfal2Config(const UrlCopyOpts &opts, Gfal2 &gfal2)
{
    gfal2.set("GRIDFTP PLUGIN", "SESSION_REUSE", true);
    gfal2.set("GRIDFTP PLUGIN", "ENABLE_UDT", opts.enableUdt);

    if (!indeterminate(opts.enableIpv6)) {
        gfal2.set("GRIDFTP PLUGIN", "IPV6", static_cast<bool>(opts.enableIpv6));
    }

    if (opts.infosys.compare("false") == 0) {
        gfal2.set("BDII", "ENABLED",false);
    }
    else {
        gfal2.set("BDII", "ENABLED",true);
        gfal2.set("BDII", "LCG_GFAL_INFOSYS", opts.infosys);
    }

    if (!opts.thirdPartyTURL.empty()) {
        gfal2.set("SRM PLUGIN", "TURL_3RD_PARTY_PROTOCOLS", opts.thirdPartyTURL);
    }

    gfal2.setUserAgent("fts_url_copy", VERSION);

    if (!opts.proxy.empty()) {
        gfal2.set("X509", "CERT", opts.proxy);
        gfal2.set("X509", "KEY", opts.proxy);

        // Most, if not all, gfal2 plugins support the previous way of setting credentials,
        // but just in case for backwards compatibility
        setenv("X509_USER_CERT", opts.proxy.c_str(), 1);
        setenv("X509_USER_KEY", opts.proxy.c_str(), 1);
    }
}


static DestFile createDestFileReport(const Transfer &transfer, Gfal2 &gfal2, Gfal2TransferParams &params)
{
    const std::string checksumType = transfer.checksumAlgorithm.empty() ? "ADLER32" :
                                     transfer.checksumAlgorithm;
    const std::string checksum = gfal2.getChecksum(transfer.destination,
                                                   transfer.checksumAlgorithm);
    const uint64_t destFileSize = gfal2.stat(params, transfer.destination, false).st_size;
    const std::string userStatus = gfal2.getXattr(transfer.destination, GFAL_XATTR_STATUS);

    DestFile destFile;
    destFile.fileSize = destFileSize;
    destFile.checksumType = checksumType;
    destFile.checksumValue = checksum;

    // Set file and disk locality booleans to true if on a valid userStatus
    if (userStatus == GFAL_XATTR_STATUS_ONLINE) {
        destFile.fileOnDisk = true;
    } else if (userStatus == GFAL_XATTR_STATUS_NEARLINE) {
        destFile.fileOnTape = true;
    } else if (userStatus == GFAL_XATTR_STATUS_NEARLINE_ONLINE) {
        destFile.fileOnDisk = true;
        destFile.fileOnTape = true;
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Could not determine file locality from"
                                              " the Gfal2 user.status extended attribute : " << userStatus << commit;
    }

    return destFile;
} 


UrlCopyProcess::UrlCopyProcess(const UrlCopyOpts &opts, Reporter &reporter):
    opts(opts), reporter(reporter), canceled(false), timeoutExpired(false)
{
    todoTransfers = opts.transfers;
    setupGlobalGfal2Config(opts, gfal2);
}


static void setupTokenConfig(const UrlCopyOpts &opts, const Transfer &transfer,
                             Gfal2 &gfal2, Gfal2TransferParams &params)
{
    // Load Cloud + OIDC credentials
    if (!opts.oauthFile.empty()) {
        try {
            gfal2.loadConfigFile(opts.oauthFile);
        } catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load OAuth config file: " << ex.what() << commit;
        }
        unlink(opts.oauthFile.c_str());
    }

    // OIDC token has been passed already in the OauthFile
    // and loaded by Gfal2 as the default BEARER token credential
    if ("oauth2" == opts.authMethod) {
        return;
    }

    // Check if allowed to retrieve Storage Element issued tokens
    if (!opts.retrieveSEToken) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Configured to skip retrieval of SE-issued tokens. "
                                        << "Retrieval delegated to downstream Gfal2 client" << commit;
        return;
    }

    // Bearer tokens can be issued in two ways:
    // 1. Issued by a dedicated TokenIssuer
    //    - Needs only the TokenIssuer endpoint
    // 2. Issued by the SE itself (colloquially called macaroons)
    //    - Can only be done against a https/davs endpoint
    //    - Needs a validity time (in minutes) and list of activities
    //
    // Gfal2 can retrieve bearer tokens from both a TokenIssuer (first choice),
    // then fallback to the SE itself

    bool macaroonEnabledSource = ((transfer.source.protocol.find("davs") == 0) || (transfer.source.protocol.find("https") == 0));
    bool macaroonEnabledDestination = ((transfer.destination.protocol.find("davs") == 0) || (transfer.destination.protocol.find("https") == 0));
    unsigned macaroonValidity = 180;

    // Request a macaroon longer twice the timeout as we could run both push and pull mode
    if (opts.timeout) {
        macaroonValidity = ((unsigned) (2 * opts.timeout) / 60) + 10 ;
    } else if (transfer.userFileSize) {
        macaroonValidity = ((unsigned) (2 * adjustTimeoutBasedOnSize(transfer.userFileSize, opts.addSecPerMb)) / 60) + 10;
    }

    if (!transfer.sourceTokenIssuer.empty() || macaroonEnabledSource) {
        std::string tokenType = (!transfer.sourceTokenIssuer.empty()) ? "bearer token" : "macaroon";
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Will attempt retrieval of " << tokenType << " for source" << commit;
        try {
            params.setSourceBearerToken(gfal2.tokenRetrieve(transfer.source, transfer.sourceTokenIssuer,
                                                            macaroonValidity, {"DOWNLOAD", "LIST"}));
        } catch (const Gfal2Exception& ex) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed to retrieve " << tokenType << " for source: " << ex.what() << commit;
        }
    }

    if (!transfer.destTokenIssuer.empty() || macaroonEnabledDestination) {
        std::string tokenType = (!transfer.destTokenIssuer.empty()) ? "bearer token" : "macaroon";
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Will attempt retrieval of " << tokenType << " for destination" << commit;
        try {
            params.setDestBearerToken(gfal2.tokenRetrieve(transfer.destination, transfer.destTokenIssuer,
                                                          macaroonValidity, {"MANAGE", "UPLOAD", "DELETE", "LIST"}));
        } catch (const Gfal2Exception& ex) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed to retrieve " << tokenType << " for destination: " << ex.what() << commit;
        }
    }
}


static void setupTransferConfig(const UrlCopyOpts &opts, const Transfer &transfer,
                                Gfal2 &gfal2, Gfal2TransferParams &params)
{
    params.setStrictCopy(opts.strictCopy);
    params.setCreateParentDir(true);
    params.setReplaceExistingFile(opts.overwrite);
    params.setDelegationFlag(!opts.noDelegation);
    params.setStreamingFlag(!opts.noStreaming);

    if (!transfer.transferMetadata.empty()) {
        params.setTransferMetadata(transfer.transferMetadata);
    }

    // Set useful log verbosity for HTTP transfers
    if (transfer.source.protocol.find("http") == 0 ||
        transfer.source.protocol.find("dav") == 0 ||
        transfer.destination.protocol.find("http") == 0 ||
        transfer.destination.protocol.find("dav") == 0) {
        gfal2_log_set_level(std::max(gfal2_log_get_level(), G_LOG_LEVEL_INFO));
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Source protocol: " << transfer.source.protocol << commit;
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Destination protocol: " << transfer.destination.protocol << commit;

    setupTokenConfig(opts, transfer, gfal2, params);

    if (!transfer.sourceTokenDescription.empty()) {
        params.setSourceSpacetoken(transfer.sourceTokenDescription);
    }

    if (!transfer.destTokenDescription.empty()) {
        params.setDestSpacetoken(transfer.destTokenDescription);
    }

    if (!transfer.checksumAlgorithm.empty()) {
    	try	{
    		params.setChecksum(transfer.checksumMode, transfer.checksumAlgorithm, transfer.checksumValue);
    	} catch (const Gfal2Exception &ex) {
    		if (transfer.checksumMode == Transfer::CHECKSUM_SOURCE) {
    			throw UrlCopyError(SOURCE, TRANSFER_PREPARATION, ex);
    		} else if (transfer.checksumMode == Transfer::CHECKSUM_TARGET) {
    			throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, ex);
    		} else {
                throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, ex);
            }
    	}
    }

    // Set HTTP copy mode
    if (!opts.copyMode.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Setting Gfal2 configuration: DEFAULT_COPY_MODE=" << opts.copyMode << commit;
        gfal2.set("HTTP PLUGIN", "DEFAULT_COPY_MODE", opts.copyMode);
    }

    // Disable TPC copy fallback
    if (opts.disableCopyFallback) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Setting Gfal2 configuration: ENABLE_FALLBACK_TPC_COPY=false" << commit;
        gfal2.set("HTTP PLUGIN", "ENABLE_FALLBACK_TPC_COPY", false);
    }

    // Avoid TPC attempts in S3 to S3 transfers
    if ((transfer.source.protocol.find("s3") == 0) && (transfer.destination.protocol.find("s3") == 0)) {
        gfal2.set("HTTP PLUGIN", "ENABLE_REMOTE_COPY", false);
    }

    // Enable HTTP sensitive log scope
    if (opts.debugLevel == 3) {
        gfal2.set("HTTP PLUGIN", "LOG_SENSITIVE", true);
    }

    // Additional metadata
    gfal2.addClientInfo("job-id", transfer.jobId);
    gfal2.addClientInfo("file-id", boost::lexical_cast<std::string>(transfer.fileId));
    gfal2.addClientInfo("retry", boost::lexical_cast<std::string>(opts.retry));
}


static void timeoutTask(boost::posix_time::time_duration &duration, UrlCopyProcess *urlCopyProcess)
{
    try {
        boost::this_thread::sleep(duration);
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Timeout expired!" << commit;
        urlCopyProcess->timeout();
    } catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Timeout thread stopped" << commit;
    } catch (const std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception in the timeout task: " << ex.what() << commit;
    }
}


static void pingTask(Transfer *transfer, Reporter *reporter, unsigned pingInterval)
{
    try {
        while (!boost::this_thread::interruption_requested()) {
            boost::this_thread::sleep(boost::posix_time::seconds(pingInterval));
            reporter->sendPing(*transfer);
        }
    } catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Ping thread stopped" << commit;
    } catch (const std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception in the ping task: " << ex.what() << commit;
    }
}


void UrlCopyProcess::runTransfer(Transfer &transfer, Gfal2TransferParams &params)
{
    if (!opts.proxy.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy: " << opts.proxy << commit;
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Running without proxy" << commit;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "VO: " << opts.voName << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id: " << transfer.jobId << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File id: " << transfer.fileId << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source url: " << transfer.source << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest url: " << transfer.destination << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Overwrite enabled: " << opts.overwrite << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Disable delegation: " << opts.noDelegation << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Disable local streaming: " << opts.noStreaming << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Skip eviction of source file: " << opts.skipEvict << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest space token: " << transfer.destTokenDescription << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source space token: " << transfer.sourceTokenDescription << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum: " << transfer.checksumValue << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum enabled: " << transfer.checksumMode << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User filesize: " << transfer.userFileSize << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File metadata: " << transfer.fileMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer metadata: " << transfer.transferMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job metadata: " << opts.jobMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Bringonline token: " << transfer.tokenBringOnline << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "UDT: " << opts.enableUdt << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BDII:" << opts.infosys << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source token issuer: " << transfer.sourceTokenIssuer << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination token issuer: " << transfer.destTokenIssuer << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Report on the destination tape file: " << opts.dstFileReport << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Third Party TURL protocol list: " << gfal2.get("SRM PLUGIN", "TURL_3RD_PARTY_PROTOCOLS")
                                    << ((!opts.thirdPartyTURL.empty()) ? " (database configuration)" : "") << commit;

    if (opts.strictCopy) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Copy only transfer!" << commit;
        transfer.fileSize = transfer.userFileSize;
    } else {
        try {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Getting source file size" << commit;
            transfer.fileSize = gfal2.stat(params, transfer.source, true).st_size;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File size: " << transfer.fileSize << commit;
        } catch (const Gfal2Exception &ex) {
            throw UrlCopyError(SOURCE, TRANSFER_PREPARATION, ex);
        } catch (const std::exception &ex) {
            throw UrlCopyError(SOURCE, TRANSFER_PREPARATION, EINVAL, ex.what());
        }

        if (!opts.overwrite) {
            try {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking existence of destination file" << commit;
                gfal2.stat(params, transfer.destination, false);

                if (opts.dstFileReport) {
                    try {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking integrity of destination tape file: "
                                                        << transfer.destination << commit;
                        auto destFile = createDestFileReport(transfer, gfal2, params);
                        transfer.fileMetadata = DestFile::appendDestFileToFileMetadata(transfer.fileMetadata, destFile.toJSON());
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file report: " << destFile.toString() << commit;
                    } catch (const std::exception &ex) {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to check integrity of destination tape file: "
                                                       << transfer.destination << " (error=" << ex.what() << ")" << commit;
                    }
                }

                throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EEXIST,
                                   "Destination file exists and overwrite is not enabled");
            } catch (const Gfal2Exception &ex) {
                if (ex.code() != ENOENT) {
                    throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, ex);
                }
            }
        }
    }

    // Timeout
    unsigned timeout = opts.timeout;
    if (timeout == 0) {
        timeout = adjustTimeoutBasedOnSize(transfer.fileSize, opts.addSecPerMb);
    }

    // Set protocol parameters
    params.setNumberOfStreams(opts.nStreams);
    params.setTcpBuffersize(opts.tcpBuffersize);
    params.setTimeout(timeout);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "IPv6: " << (boost::indeterminate(opts.enableIpv6) ? "indeterminate" :
                                                   (opts.enableIpv6 ? "true" : "false")) << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TCP streams: " << params.getNumberOfStreams() << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TCP buffer size: " << opts.tcpBuffersize << commit;

    reporter.sendProtocol(transfer, params);

    // Install callbacks
    params.addEventCallback(eventCallback, &transfer);
    params.addMonitorCallback(performanceCallback, &transfer);

    timeoutExpired = false;
    AutoInterruptThread timeoutThread(
        boost::bind(&timeoutTask, boost::posix_time::seconds(timeout + 60), this)
    );
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Timeout set to: " << timeout << commit;

    // Ping thread
    AutoInterruptThread pingThread(boost::bind(&pingTask, &transfer, &reporter, opts.pingInterval));
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Setting ping interval to: " << opts.pingInterval << commit;

    // Transfer
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Starting transfer" << commit;
    try {
        gfal2.copy(params, transfer.source, transfer.destination);
    } catch (const Gfal2Exception &ex) {
        if (timeoutExpired) {
            throw UrlCopyError(TRANSFER, TRANSFER, ETIMEDOUT, ex.what());
        } else {
            throw UrlCopyError(TRANSFER, TRANSFER, ex);
        }
    } catch (const std::exception &ex) {
        throw UrlCopyError(TRANSFER, TRANSFER, EINVAL, ex.what());
    }

    // Release source file if we have a bring-online token
    if (!transfer.tokenBringOnline.empty() && !opts.skipEvict) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Releasing source file" << commit;
        try {
            gfal2.releaseFile(params, transfer.source, transfer.tokenBringOnline, true);
            transfer.stats.evictionRetc = 0;
        } catch (const Gfal2Exception &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "RELEASE-PIN Failed to release source file: "
                                               << transfer.source << commit;
            transfer.stats.evictionRetc = std::abs(ex.code());
        }
    }

    // Validate destination size
    if (!opts.strictCopy) {
        uint64_t destSize;
        try {
            destSize = gfal2.stat(params, transfer.destination, false).st_size;
        } catch (const Gfal2Exception &ex) {
            throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, ex);
        } catch (const std::exception &ex) {
            throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, EINVAL, ex.what());
        }

        if (destSize != transfer.fileSize) {
            throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, EINVAL,
                               "Source and destination file size mismatch");
        } else {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DESTINATION Source and destination file size matching" << commit;
        }
    }
}


void UrlCopyProcess::run(void)
{
    while (!todoTransfers.empty() && !canceled) {
        Transfer transfer;
        {
            boost::lock_guard<boost::mutex> lock(transfersMutex);
            transfer = todoTransfers.front();
        }

        // Prepare logging
        transfer.stats.process.start = millisecondsSinceEpoch();
        transfer.logFile = generateLogPath(opts.logDir, transfer);

        if (opts.debugLevel) {
            transfer.debugLogFile = transfer.logFile + ".debug";
        } else {
            transfer.debugLogFile = "/dev/null";
        }

        if (!opts.logToStderr) {
            fts3::common::theLogger().redirect(transfer.logFile, transfer.debugLogFile);
        }

        // Prepare Gfal2 transfer parameters
        Gfal2TransferParams params;
        try {
            setupTransferConfig(opts, transfer, gfal2, params);
        } catch (const UrlCopyError &ex) {
            transfer.error.reset(new UrlCopyError(ex));
        }

        // Notify we got it
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer accepted" << commit;
        reporter.sendTransferStart(transfer, params);

        // Run the transfer
        try {
            runTransfer(transfer, params);
        } catch (const UrlCopyError &ex) {
            transfer.error.reset(new UrlCopyError(ex));
        } catch (const std::exception &ex) {
            transfer.error.reset(new UrlCopyError(AGENT, TRANSFER_SERVICE, EINVAL, ex.what()));
        } catch (...) {
            transfer.error.reset(new UrlCopyError(AGENT, TRANSFER_SERVICE, EINVAL, "Unknown exception"));
        }

        // Log error if any
        if (transfer.error) {
            if (transfer.error->isRecoverable()) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Recoverable error: [" << transfer.error->code() << "] "
                                               << transfer.error->what() << commit;
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Non recoverable error: [" << transfer.error->code() << "] "
                                               << transfer.error->what() << commit;
            }
        } else {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer finished successfully" << commit;
        }

        // Archive log
        archiveLogs(transfer);

        // Notify back the final state
        transfer.stats.process.end = millisecondsSinceEpoch();
        {
            boost::lock_guard<boost::mutex> lock(transfersMutex);
            doneTransfers.push_back(transfer);

            // todoTransfers may have been emptied by panic()
            if (!todoTransfers.empty()) {
                todoTransfers.pop_front();
                reporter.sendTransferCompleted(transfer, params);
            }
        }
    }

    // On cancellation, todoTransfers will not be empty
    // and a termination message must be sent for them
    for (auto transfer = todoTransfers.begin(); transfer != todoTransfers.end(); ++transfer) {
        Gfal2TransferParams params;
        transfer->error.reset(new UrlCopyError(TRANSFER, TRANSFER_PREPARATION, ECANCELED, "Transfer canceled"));
        reporter.sendTransferCompleted(*transfer, params);
    }
}


void UrlCopyProcess::archiveLogs(Transfer &transfer)
{
    std::string archivedLogFile;

    try {
        archivedLogFile = generateArchiveLogPath(opts.logDir, transfer);
        boost::filesystem::rename(transfer.logFile, archivedLogFile);
        transfer.logFile = archivedLogFile;
    } catch (const std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to archive the log: " << e.what() << commit;
    }

    if (opts.debugLevel > 0) {
        try {
            std::string archivedDebugLogFile = archivedLogFile + ".debug";
            boost::filesystem::rename(transfer.debugLogFile, archivedDebugLogFile);
            transfer.debugLogFile = archivedDebugLogFile;
        } catch (const std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to archive the debug log: " << e.what() << commit;
        }
    }

}


void UrlCopyProcess::cancel(void)
{
    canceled = true;
    gfal2.cancel();
}


void UrlCopyProcess::timeout(void)
{
    timeoutExpired = true;
    gfal2.cancel();
}


void UrlCopyProcess::panic(const std::string &msg)
{
    boost::lock_guard<boost::mutex> lock(transfersMutex);
    for (auto transfer = todoTransfers.begin(); transfer != todoTransfers.end(); ++transfer) {
        Gfal2TransferParams params;
        transfer->error.reset(new UrlCopyError(AGENT, TRANSFER_SERVICE, EINTR, msg));
        reporter.sendTransferCompleted(*transfer, params);
    }
    todoTransfers.clear();
}
