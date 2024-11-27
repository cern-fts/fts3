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
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>

#include "LogHelper.h"
#include "heuristics.h"
#include "AutoInterruptThread.h"
#include "UrlCopyProcess.h"
#include "version.h"
#include "DestFile.h"
#include "common/Logger.h"
#include "common/TimeUtils.h"


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


static void performDestFileReportWorkflow(const UrlCopyOpts &opts, Transfer &transfer,
                                          Gfal2 &gfal2, Gfal2TransferParams &params)
{
    if (!opts.dstFileReport) {
        return;
    }

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


static void performOverwriteOnDiskWorkflow(const UrlCopyOpts &opts, Transfer &transfer,
                                           Gfal2 &gfal2, Gfal2TransferParams &params)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Entering \"overwrite-when-only-on-disk\" workflow" << commit;
    std::string xattrLocality;

    if (!opts.overwriteDiskEnabled) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination endpoint not configured with \"overwrite-disk-enabled\". "
                                        << "Aborting transfer! (overwrite-when-only-on-disk requested)" << commit;
        throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EINVAL,
                           "Destination endpoint not configured with \"overwrite-disk-enabled\" (overwrite-when-only-on-disk requested)");
    }

    try {
        // Check file locality
        xattrLocality = gfal2.getXattr(transfer.destination, GFAL_XATTR_STATUS);
    } catch (const Gfal2Exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to check destination file locality. Aborting transfer! "
                                       << "(overwrite-when-only-on-disk requested) (error= " << ex.what() << ")" << commit;
        throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EFAULT,
                           "Could not check destination file locality (overwrite-when-only-on-disk requested)");
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Identified destination file locality: " << xattrLocality << commit;

    bool file_on_tape = (xattrLocality == GFAL_XATTR_STATUS_NEARLINE ||
                         xattrLocality == GFAL_XATTR_STATUS_NEARLINE_ONLINE);
    bool file_only_on_disk = (xattrLocality == GFAL_XATTR_STATUS_ONLINE);

    if (file_on_tape) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file on tape. Aborting transfer! "
                                        << "(overwrite-when-only-on-disk requested)" << commit;
        throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EROFS,
                           "Destination file exists and is on tape (overwrite-when-only-on-disk requested)");
    }

    if (!file_only_on_disk) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file location unknown: " << xattrLocality << ". Aborting transfer! "
                                        << "(overwrite-when-only-on-disk requested)" << commit;
        std::ostringstream errmsg;
        errmsg << "Destination file location unknown: " + xattrLocality
               << " (overwrite-when-only-on-disk requested)";
        throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EFAULT, errmsg.str());
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file not on tape. Removing file "
                                    << "(overwrite-when-only-on-disk requested)" << commit;
    try {
        gfal2.rm(params, transfer.destination, false);
        transfer.stats.overwriteOnDiskRetc = 0;
    } catch (const Gfal2Exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to delete destination file. Aborting transfer! "
                                       << "(overwrite-when-only-on-disk requested) (error= " << ex.what() << ")" << commit;
        transfer.stats.overwriteOnDiskRetc = std::abs(ex.code());
        throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EFAULT,
                           "Failed to delete destination file (overwrite-when-only-on-disk requested)");
    }
}


UrlCopyProcess::UrlCopyProcess(const UrlCopyOpts &opts, Reporter &reporter):
    opts(opts), reporter(reporter), canceled(false), timeoutExpired(false)
{
    todoTransfers = opts.transfers;
    setupGlobalGfal2Config(opts, gfal2);
}


// Read source and destination bearer tokens form a configuration file and set them
// in the Gfal2 transfer parameters object to be loaded in the credentials map.
// The first line contains the bearer token for the source storage endpoint.
// The second line  contains the bearer token for the destination storage endpoint.
// The rest of the contents in the file are discarded.
static void loadTokensFile(const std::string& path, Gfal2TransferParams &params) {
    std::string line;
    std::ifstream infile(path.c_str(), std::ios_base::in);

    // First line contains source bearer token
    if (std::getline(infile, line, '\n') && !line.empty()) {
        params.setSourceBearerToken(line);
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load source bearer token form credentials file" << commit;
    }

    // Second line contains destination bearer token
    if (std::getline(infile, line, '\n') && !line.empty()) {
        params.setDestBearerToken(line);
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load destination bearer token form credentials file" << commit;
    }

    // Close the file as we are only interested in the first two lines
    infile.close();

    unlink(path.c_str());
}


static void setupTokenConfig(const UrlCopyOpts &opts, const Transfer &transfer,
                             Gfal2 &gfal2, Gfal2TransferParams &params)
{
    // Load Cloud + OIDC credentials
    if (!opts.cloudStorageConfig.empty()) {
        try {
            gfal2.loadConfigFile(opts.cloudStorageConfig);
        } catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load cloud storage credentials file: " << ex.what() << commit;
        }
        unlink(opts.cloudStorageConfig.c_str());
        return;
    }

    if (opts.authMethod == "oauth2" && !opts.oauthFile.empty()) {
        loadTokensFile(opts.oauthFile, params);
        // OIDC tokens have been passed in the OauthFile
        // to be loaded by Gfal2 in its credentials map
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
    params.setTransferCleanUp(!opts.disableCleanup);

    // SciTag value should always be in the [65, 65535] range
    if (transfer.scitag > 0) {
        params.setScitag(transfer.scitag);
    }

    if (!transfer.archiveMetadata.empty()) {
        params.setArchiveMetadata(transfer.archiveMetadata);
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
    } else if (opts.authMethod == "oauth2" && !opts.oauthFile.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source token: " << accessTokenPayload(params.getSrcToken()) << commit;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination token: " << accessTokenPayload(params.getDstToken()) << commit;
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Running without any authentication!" << commit;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "VO: " << opts.voName << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id: " << transfer.jobId << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File id: " << transfer.fileId << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source url: " << transfer.source << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest url: " << transfer.destination << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Overwrite enabled: " << opts.overwrite << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Overwrite when only on disk: " << opts.overwriteOnDisk << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Disable delegation: " << opts.noDelegation << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Disable local streaming: " << opts.noStreaming << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Skip eviction of source file: " << opts.skipEvict << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Disable cleanup: " << opts.disableCleanup << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest space token: " << transfer.destTokenDescription << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source space token: " << transfer.sourceTokenDescription << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum: " << transfer.checksumValue << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum enabled: " << transfer.checksumMode << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User filesize: " << transfer.userFileSize << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Scitag: " << transfer.scitag << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File metadata: " << transfer.fileMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Archive metadata: " << transfer.archiveMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job metadata: " << opts.jobMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Bringonline token: " << transfer.tokenBringOnline << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "UDT: " << opts.enableUdt << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BDII:" << opts.infosys << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source token issuer: " << transfer.sourceTokenIssuer << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination token issuer: " << transfer.destTokenIssuer << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Report on the destination tape file: " << opts.dstFileReport << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Third Party TURL protocol list: " << gfal2.get("SRM PLUGIN", "TURL_3RD_PARTY_PROTOCOLS")
                                    << ((!opts.thirdPartyTURL.empty()) ? " (database configuration)" : "") << commit;

    if (opts.authMethod == "oauth2" && !opts.oauthFile.empty()) {
        auto [token, expiry] = reporter.requestTokenRefresh(transfer.sourceTokenId, transfer);

        if (token.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to obtain refreshed token for source!" << commit;
        } else {
            params.setSourceBearerToken(token);
        }
    }

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

                // File exists
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file exists" << commit;
                bool overwrite_disk_performed = false;
                std::string errmsg = "Destination file exists and overwrite is not enabled";

                if (opts.overwriteOnDisk) {
                    try {
                        performOverwriteOnDiskWorkflow(opts, transfer, gfal2, params);
                        overwrite_disk_performed = true;
                    } catch(const UrlCopyError &ex) {
                        // Dealing with file on tape endpoint, allow DestFileReport
                        if (ex.code() == EROFS) {
                            errmsg = ex.what();
                        } else {
                            throw;
                        }
                    }
                }

                if (!overwrite_disk_performed) {
                    performDestFileReportWorkflow(opts, transfer, gfal2, params);
                    throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EEXIST, errmsg);
                }
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
        transfer.stats.process.start = getTimestampMilliseconds();
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
        transfer.stats.process.end = getTimestampMilliseconds();
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
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "UrlCopyProcess panic... " << msg << commit;
    boost::lock_guard<boost::mutex> lock(transfersMutex);
    for (auto transfer = todoTransfers.begin(); transfer != todoTransfers.end(); ++transfer) {
        Gfal2TransferParams params;
        transfer->error.reset(new UrlCopyError(AGENT, TRANSFER_SERVICE, EINTR, msg));
        reporter.sendTransferCompleted(*transfer, params);
    }
    todoTransfers.clear();
}
