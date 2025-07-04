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
#include <boost/algorithm/string.hpp>
#include <filesystem>

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
    gfal2.set("BDII", "ENABLED", false);

    if (!indeterminate(opts.enableIpv6)) {
        gfal2.set("GRIDFTP PLUGIN", "IPV6", static_cast<bool>(opts.enableIpv6));
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


static DestFile createDestFileReport(const Transfer &transfer, Gfal2 &gfal2)
{
    const std::string checksumType = transfer.checksumAlgorithm.empty() ? "ADLER32" :
                                     transfer.checksumAlgorithm;
    const std::string checksum = gfal2.getChecksum(transfer.destination,
                                                   transfer.checksumAlgorithm);
    const uint64_t destFileSize = gfal2.stat(transfer.destination).st_size;
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


void UrlCopyProcess::performDestFileReportWorkflow(Transfer& transfer)
{
    if (!opts.dstFileReport) {
        return;
    }

    refreshExpiredAccessToken(transfer, IS_DEST);

    try {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking integrity of destination tape file: "
                                        << transfer.destination << commit;
        auto destFile = createDestFileReport(transfer, gfal2);
        transfer.fileMetadata = DestFile::appendDestFileToFileMetadata(transfer.fileMetadata, destFile.toJSON());
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file report: " << destFile.toString() << commit;
    } catch (const std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to check integrity of destination tape file: "
                                       << transfer.destination << " (error=" << ex.what() << ")" << commit;
    }
}


void UrlCopyProcess::performOverwriteOnDiskWorkflow(Transfer& transfer)
{
    refreshExpiredAccessToken(transfer, IS_DEST);

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
        gfal2.rm(transfer.destination);
        transfer.stats.overwriteOnDiskRetc = 0;
    } catch (const Gfal2Exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to delete destination file. Aborting transfer! "
                                       << "(overwrite-when-only-on-disk requested) (error= " << ex.what() << ")" << commit;
        transfer.stats.overwriteOnDiskRetc = std::abs(ex.code());
        throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EFAULT,
                           "Failed to delete destination file (overwrite-when-only-on-disk requested)");
    }
}


void UrlCopyProcess::refreshExpiredAccessToken(const Transfer& transfer, bool is_source)
{
    // Not OAuth2 authentication
    if (opts.authMethod != "oauth2" || opts.oauthFile.empty()) {
        return;
    }

    // Unmanaged tokens
    if ((is_source && transfer.sourceTokenUnmanaged) ||
        (!is_source && transfer.destTokenUnmanaged)) {
        return;
    }

    auto accessToken = is_source ? gfal2.getSourceToken() : gfal2.getDestinationToken();
    auto tokenId = is_source ? transfer.sourceTokenId : transfer.destTokenId;
    auto target = is_source ? "source" : "destination";
    auto scope = is_source ? SOURCE : DESTINATION;

    std::string expField = extractAccessTokenField(accessToken, "exp");
    auto exp = (!expField.empty()) ? std::stoll(expField) : 0;

    // No "exp" or more than "TokenRefreshMarginPeriod" seconds until expiration
    if (exp == 0 || getTimestampSeconds(opts.tokenRefreshMargin) <= exp) {
        return;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Requesting " << target << " token refresh: token_id=" << tokenId
                                    << " (token_exp=" << exp << ")" << commit;
    auto [token, expiry] = reporter.requestTokenRefresh(tokenId, transfer);

    if (token.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to obtain refreshed token for " << target << "!" << commit;
        std::ostringstream errmsg;
        errmsg << "Failed to obtain refresh token for " << target;
        throw UrlCopyError(scope, TRANSFER_PREPARATION, EFAULT, errmsg.str());
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Successfully refreshed access token for " << target << ": "
                                    << accessTokenPayload(token) << commit;
    if (is_source) { gfal2.setSourceToken(transfer.source, token); }
    else           { gfal2.setDestinationToken(transfer.destination, token); }
}


uint64_t UrlCopyProcess::obtainFileSize(const Transfer& transfer, bool is_source)
{
    auto url = is_source ? transfer.source : transfer.destination;
    auto target = is_source ? "source" : "destination";
    auto scope = is_source ? SOURCE : DESTINATION;
    auto phase = is_source ? TRANSFER_PREPARATION : TRANSFER_FINALIZATION;
    refreshExpiredAccessToken(transfer, is_source);

    try {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Getting " << target << " file size" << commit;
        auto filesize = gfal2.stat(url).st_size;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File size: " << filesize << " (" << target << ")" << commit;
        return filesize;
    } catch (const Gfal2Exception &ex) {
        throw UrlCopyError(scope, phase, ex);
    } catch (const std::exception &ex) {
        throw UrlCopyError(scope, phase, EINVAL, ex.what());
    }
}


std::string UrlCopyProcess::obtainFileChecksum(const Transfer& transfer, bool is_source,
                                               const std::string& algorithm,
                                               const std::string& scope, const std::string& phase)
{
    auto url = is_source ? transfer.source : transfer.destination;
    auto target = is_source ? "source" : "destination";
    refreshExpiredAccessToken(transfer, is_source);

    try {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Getting " << target << " file checksum" << commit;
        auto checksum = gfal2.getChecksum(url, algorithm);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum: " << checksum << " (" << target << ")" << commit;
        return checksum;
    } catch (const Gfal2Exception &ex) {
        throw UrlCopyError(scope, phase, ex);
    } catch (const std::exception &ex) {
        throw UrlCopyError(scope, phase, EINVAL, ex.what());
    }
}


bool UrlCopyProcess::checkFileExists(const Transfer& transfer)
{
    refreshExpiredAccessToken(transfer, IS_DEST);

    try {
        gfal2.access(transfer.destination, F_OK);
        return true;
    } catch (const Gfal2Exception &ex) {
        if (ex.code() != ENOENT) {
            throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, ex);
        }

        return false;
    }
}


void UrlCopyProcess::deleteFile(const Transfer& transfer)
{
    refreshExpiredAccessToken(transfer, IS_DEST);

    try {
        gfal2.rm(transfer.destination);
    } catch (const Gfal2Exception &ex) {
        throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, ex);
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "OVERWRITE Deleted file: " << transfer.destination << commit;
}


void UrlCopyProcess::mkdirRecursive(const Transfer& transfer)
{
    refreshExpiredAccessToken(transfer, IS_DEST);
    const auto& uri = transfer.destination;
    std::filesystem::path dest_path(uri.path);

    if (dest_path.has_parent_path()) {
        std::string parent_uri = uri.protocol + "://" + uri.host;

        if (uri.port != 0) {
            parent_uri += ":" + std::to_string(uri.port);
        }

        parent_uri += dest_path.parent_path().string();

        if (!uri.queryString.empty()) {
            parent_uri += "?" + uri.queryString;
        }

        // Parent directory exists, exit early
        try {
            gfal2.access(parent_uri, F_OK);
            return;
        } catch (const Gfal2Exception &ex) {
            if (ex.code() != ENOENT) {
                throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, ex);
            }
        }

        try {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Destination parent directory does not exist, creating: " + parent_uri << commit;
            gfal2.mkdir_recursive(parent_uri);
        } catch (const Gfal2Exception &ex) {
            if (ex.code() != EEXIST) {
                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Unable to create destination parent directory: " + parent_uri << commit;
                throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, ex);
            }
        }
    }
}

void UrlCopyProcess::performCopy(Gfal2TransferParams& params, Transfer& transfer)
{
    refreshExpiredAccessToken(transfer, IS_SOURCE);
    refreshExpiredAccessToken(transfer, IS_DEST);

    try {
        gfal2.copy(params, transfer.source, transfer.destination);
    } catch (const Gfal2Exception &ex) {
        // Clean-up on a copy failure
        if (ex.code() != EEXIST) {
            cleanupOnFailure(transfer);
        } else {
            // Should only get here due to a race condition at the storage level
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "The transfer failed because the file exists. Do not clean!" << commit;
        }

        int errc = (timeoutExpired ? ETIMEDOUT : ex.code());
        throw UrlCopyError(TRANSFER, TRANSFER, errc, ex.what());
    } catch (const std::exception &ex) {
        throw UrlCopyError(TRANSFER, TRANSFER, EINVAL, ex.what());
    }
}


void UrlCopyProcess::releaseSourceFile(Transfer& transfer)
{
    refreshExpiredAccessToken(transfer, IS_SOURCE);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Releasing source file" << commit;
    try {
        gfal2.releaseFile(transfer.source, transfer.tokenBringOnline);
        transfer.stats.evictionRetc = 0;
    } catch (const Gfal2Exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "RELEASE-PIN Failed to release source file: "
                                           << transfer.source << commit;
        transfer.stats.evictionRetc = std::abs(ex.code());
    }
}


void UrlCopyProcess::cleanupOnFailure(Transfer& transfer)
{
    refreshExpiredAccessToken(transfer, IS_DEST);

    if (!opts.disableCleanup) {
        try {
            gfal2.rm(transfer.destination);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file removed (cleanup-on-failure)" << commit;
            transfer.stats.cleanupRetc = 0;
        } catch (const Gfal2Exception &ex) {
            if (ex.code() != ENOENT) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Error when trying to clean the destination: " <<  ex.what() << commit;
            }
            transfer.stats.cleanupRetc = std::abs(ex.code());
        };
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "The cleanup-on-failure has been explicitly disabled" << commit;
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
// The second line contains the bearer token for the destination storage endpoint.
// The rest of the contents in the file are discarded.
static void loadTokensFile(const std::string& path, const Transfer& transfer, Gfal2& gfal2) {
    std::string line;
    std::ifstream infile(path.c_str(), std::ios_base::in);

    // First line contains source bearer token
    if (std::getline(infile, line, '\n') && !line.empty()) {
        gfal2.setSourceToken(transfer.source, line);
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load OAuth2 source bearer token from credentials file" << commit;
    }

    // Second line contains destination bearer token
    if (std::getline(infile, line, '\n') && !line.empty()) {
        gfal2.setDestinationToken(transfer.destination, line);
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load OAuth2 destination bearer token from credentials file" << commit;
    }

    // Close the file as we are only interested in the first two lines
    infile.close();
    unlink(path.c_str());
}


static void setupTokenConfig(const UrlCopyOpts &opts, const Transfer &transfer, Gfal2 &gfal2)
{
    // Load Cloud + OIDC credentials
    if (!opts.cloudStorageConfig.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Loading cloud storage credentials" << commit;
        try {
            gfal2.loadConfigFile(opts.cloudStorageConfig);
        } catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load cloud storage credentials file: " << ex.what() << commit;
        }
        unlink(opts.cloudStorageConfig.c_str());
    }

    if (opts.authMethod == "oauth2" && !opts.oauthFile.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Loading OAuth2 token credentials" << commit;
        // OAuth2 tokens have been passed in the OAuthFile
        loadTokensFile(opts.oauthFile, transfer, gfal2);
        return;
    }

    // SE-issued tokens are obtained in two ways (only HTTPs/DAVs endpoints):
    // 1. Issued by the SE itself (colloquially called macaroons)
    //    - Needs a validity time (in minutes) and list of activities
    // 2. Issued from a dedicated token endpoint of that storage (JWT-like tokens)
    //    - Token endpoint discovered via the .well-known/openid-configuration
    //
    // Gfal2 can retrieve bearer tokens via both options

    bool macaroonEnabledSource = (transfer.source.protocol.find("davs") == 0) || (transfer.source.protocol.find("https") == 0);
    bool macaroonEnabledDestination = (transfer.destination.protocol.find("davs") == 0) || (transfer.destination.protocol.find("https") == 0);
    unsigned macaroonValidity = 180;

    // Disable macaroon retrieval at Gfal2 level only when both endpoints are HTTP/DAV
    // Note: this still allows SRM transfers to use macaroons (obtained by Gfal2 after TURL resolution)
    if (macaroonEnabledSource && macaroonEnabledDestination) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Setting Gfal2 configuration: RETRIEVE_BEARER_TOKEN=false" << commit;
        gfal2.set("HTTP PLUGIN", "RETRIEVE_BEARER_TOKEN", false);
    }

    // Compute macaroon lifetime (minutes) long enough for full transfer duration
    if (opts.timeout) {
        macaroonValidity = (opts.timeout / 60) + 10 ;
    } else if (transfer.userFileSize) {
        macaroonValidity = (adjustTimeoutBasedOnSize(transfer.userFileSize, opts.addSecPerMb) / 60) + 10;
    }

    if (macaroonEnabledSource) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Will attempt retrieval of \"SE-issued token (macaroon)\" for source" << commit;
        try {
            auto token = gfal2.tokenRetrieve(transfer.source, "",
                                             macaroonValidity, {"DOWNLOAD", "LIST"});
            gfal2.setSourceToken(transfer.source, token);
        } catch (const Gfal2Exception& ex) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed to retrieve \"SE-issued token (macaroon)\" for source: " << ex.what() << commit;
        }
    }

    if (macaroonEnabledDestination) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Will attempt retrieval of \"SE-issued token (macaroon)\" for destination" << commit;
        try {
            auto token = gfal2.tokenRetrieve(transfer.destination, "",
                                             macaroonValidity, {"MANAGE", "UPLOAD", "DELETE", "LIST"});
            gfal2.setDestinationToken(transfer.destination, token, false);
        } catch (const Gfal2Exception& ex) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed to retrieve \"SE-issued token (macaroon)\" for destination: " << ex.what() << commit;
        }
    }
}


static void setupTransferConfig(const UrlCopyOpts &opts, const Transfer &transfer,
                                Gfal2 &gfal2, Gfal2TransferParams &params)
{
    params.setStrictCopy(true);
    params.setCreateParentDir(false);
    params.setTransferCleanUp(false);
    params.setReplaceExistingFile(false);
    params.setDelegationFlag(!opts.noDelegation);
    params.setStreamingFlag(!opts.noStreaming);

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

    setupTokenConfig(opts, transfer, gfal2);

    if (!transfer.sourceSpaceToken.empty()) {
        params.setSourceSpacetoken(transfer.sourceSpaceToken);
    }

    if (!transfer.destSpaceToken.empty()) {
        params.setDestSpacetoken(transfer.destSpaceToken);
    }

    // Set HTTP copy mode
    if (!opts.copyMode.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Setting Gfal2 configuration: DEFAULT_COPY_MODE=" << opts.copyMode << commit;
        gfal2.set("HTTP PLUGIN", "DEFAULT_COPY_MODE", opts.copyMode);
    }

    // Disable Gfal TPC copy fallback
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Setting Gfal2 configuration: ENABLE_FALLBACK_TPC_COPY=false" << commit;
    gfal2.set("HTTP PLUGIN", "ENABLE_FALLBACK_TPC_COPY", false);

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


static void timeoutTask(const boost::posix_time::time_duration& duration, UrlCopyProcess *urlCopyProcess)
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


static bool compare_checksum(std::string source, std::string target)
{
    source.erase(0, source.find_first_not_of('0'));
    target.erase(0, target.find_first_not_of('0'));

    return boost::iequals(source, target);
}


void UrlCopyProcess::runTransfer(Transfer &transfer, Gfal2TransferParams &params)
{
    if (!opts.proxy.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy: " << opts.proxy << commit;
    } else if (opts.authMethod == "oauth2" && !opts.oauthFile.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source token: " << accessTokenPayload(gfal2.getSourceToken()) << commit;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination token: " << accessTokenPayload(gfal2.getDestinationToken()) << commit;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source token id: " << transfer.sourceTokenId
                                        << " (" << (transfer.sourceTokenUnmanaged ? "unmanaged" : "managed") << ")" << commit;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination token id: " << transfer.destTokenId
                                        << " (" << (transfer.destTokenUnmanaged ? "unmanaged" : "managed") << ")" << commit;
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
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source space token: " << transfer.sourceSpaceToken << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest space token: " << transfer.destSpaceToken << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum: " << transfer.checksumValue << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum mode: " << transfer.checksumMode << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User filesize: " << transfer.userFileSize << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Scitag: " << transfer.scitag << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File metadata: " << transfer.fileMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Archive metadata: " << transfer.archiveMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job metadata: " << opts.jobMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Bringonline token: " << transfer.tokenBringOnline << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "UDT: " << opts.enableUdt << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Report on the destination tape file: " << opts.dstFileReport << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Third Party TURL protocol list: " << gfal2.get("SRM PLUGIN", "TURL_3RD_PARTY_PROTOCOLS")
                                    << ((!opts.thirdPartyTURL.empty()) ? " (database configuration)" : "") << commit;

    ////////////////////////////
    /// Source file verification
    ////////////////////////////

    if (!opts.strictCopy) {
        transfer.fileSize = obtainFileSize(transfer, IS_SOURCE);
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Copy only transfer!" << commit;
        transfer.fileSize = transfer.userFileSize;
    }

    /////////////////////////
    /// Internal housekeeping
    /////////////////////////

    // Timeout
    unsigned timeout = opts.timeout;
    if (timeout == 0) {
        timeout = adjustTimeoutBasedOnSize(transfer.fileSize, opts.addSecPerMb);
    }

    // Set protocol parameters
    params.setNumberOfStreams(opts.nStreams);
    params.setTcpBuffersize(opts.tcpBuffersize);
    params.setTimeout(timeout);
    // Send filesize and other parameters to server
    reporter.sendProtocol(transfer, params);

    ////////////////////////////////
    /// Source checksum verification
    ////////////////////////////////

    if (!opts.strictCopy) {
        // Checksum-mode "source" or "end-to-end"
        if (transfer.checksumMode & Transfer::CHECKSUM_SOURCE) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Obtaining source checksum for checksum verification"
                                            << " (" << transfer.checksumMode << ")" << commit;
            transfer.sourceChecksumValue = obtainFileChecksum(transfer, IS_SOURCE, transfer.checksumAlgorithm,
                                                              SOURCE, TRANSFER_PREPARATION);

            if (!transfer.checksumValue.empty()) { // User-set checksum available
                if (!compare_checksum(transfer.sourceChecksumValue, transfer.checksumValue)) {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed source and user-defined checksum verification! "
                                                    << "(" << transfer.sourceChecksumValue << " != " << transfer.checksumValue << ")" << commit;
                    throw UrlCopyError(SOURCE, TRANSFER_PREPARATION, EIO,
                        "Source and user-defined " + transfer.checksumAlgorithm + " checksum do not match "
                            + "(" + transfer.sourceChecksumValue + " != " + transfer.checksumValue + ")");
                }

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source and user-defined checksum match! (" << transfer.checksumValue << ")" << commit;
            } else if (transfer.checksumMode == Transfer::CHECKSUM_SOURCE) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Checksum-mode \"" << transfer.checksumMode << "\" requested, "
                                                   << "but no user-defined checksum provided!" << commit;
            }
        }
    }

    ////////////////////////////////
    /// Destination file preparation
    ////////////////////////////////

    if (!opts.strictCopy) {
        // Check if destination exists
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking existence of destination file" << commit;
        bool destination_exists = checkFileExists(transfer);

        // Apply the overwrite workflows
        if (destination_exists) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file exists!" << commit;

            if (opts.overwrite) {
                deleteFile(transfer);
            } else {
                std::string errmsg = "Destination file exists and overwrite is not enabled";
                bool overwrite_disk_performed = false;

                if (opts.overwriteOnDisk) {
                    try {
                        performOverwriteOnDiskWorkflow(transfer);
                        overwrite_disk_performed = true;
                    } catch (const UrlCopyError &ex) {
                        // Dealing with file on tape endpoint, allow DestFileReport
                        if (ex.code() == EROFS) {
                            errmsg = ex.what();
                        } else {
                            throw;
                        }
                    }
                }

                if (!overwrite_disk_performed) {
                    performDestFileReportWorkflow(transfer);
                    throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EEXIST, errmsg);
                }
            }
        } else {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination file does not exist!" << commit;
            mkdirRecursive(transfer);
        }
    }

    /////////////////////////////////
    /// Transfer
    /////////////////////////////////

    // Install callbacks
    params.addEventCallback(eventCallback, &transfer);
    params.addMonitorCallback(performanceCallback, &transfer);

    // Timeout thread
    timeoutExpired = false;
    AutoInterruptThread timeoutThread(
        boost::bind(&timeoutTask, boost::posix_time::seconds(timeout + 60), this)
    );

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "IPv6: " << (boost::indeterminate(opts.enableIpv6) ? "indeterminate" :
                                                   (opts.enableIpv6 ? "true" : "false")) << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TCP streams: " << params.getNumberOfStreams() << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TCP buffer size: " << opts.tcpBuffersize << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Timeout set to: " << timeout << commit;

    // Ping thread
    AutoInterruptThread pingThread(boost::bind(&pingTask, &transfer, &reporter, opts.pingInterval));
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Setting ping interval to: " << opts.pingInterval << commit;

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Starting transfer" << commit;
    performCopy(params, transfer);

    /////////////////////////////////
    /// Destination file verification
    /////////////////////////////////

    if (!opts.strictCopy) {
        auto destSize = obtainFileSize(transfer, IS_DEST);

        if (transfer.fileSize != destSize) {
            cleanupOnFailure(transfer);
            throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, EINVAL,
                               "Source and destination file size mismatch");
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source and destination file size matching" << commit;

        // Checksum-mode "target" or "end-to-end"
        if (transfer.checksumMode & Transfer::CHECKSUM_TARGET) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Obtaining destination checksum for checksum verification"
                                            << " (" << transfer.checksumMode << ")" << commit;
            transfer.destChecksumValue = obtainFileChecksum(transfer, IS_DEST, transfer.checksumAlgorithm,
                                                            DESTINATION, TRANSFER_FINALIZATION);

            if (!transfer.checksumValue.empty()) { // User-set checksum available
                if (!compare_checksum(transfer.checksumValue, transfer.destChecksumValue)) {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed user-defined and destination checksum verification! "
                                                    << "(" << transfer.checksumValue << " != " << transfer.destChecksumValue << ")" << commit;
                    cleanupOnFailure(transfer);
                    throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, EIO,
                        "User-defined and destination " + transfer.checksumAlgorithm + " checksum do not match "
                            + "(" + transfer.checksumValue + " != " + transfer.destChecksumValue + ")");
                }

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User-defined and destination checksum match! (" << transfer.checksumValue << ")" << commit;
            } else if (transfer.checksumMode == Transfer::CHECKSUM_TARGET) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Checksum-mode \"" << transfer.checksumMode << "\" requested, "
                                                   << "but no user-defined checksum provided!" << commit;
            }
        }

        // Checksum-mode exactly "end-to-end"
        if (transfer.checksumMode == Transfer::CHECKSUM_BOTH) {
            if (!compare_checksum(transfer.sourceChecksumValue, transfer.destChecksumValue)) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Failed end-to-end checksum verification! "
                                                << "(" << transfer.sourceChecksumValue << " != " << transfer.destChecksumValue << ")" << commit;
                cleanupOnFailure(transfer);
                throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, EIO,
                    "Source and destination " + transfer.checksumAlgorithm + " checksum do not match "
                        + "(" + transfer.sourceChecksumValue + " != " + transfer.destChecksumValue + ")");
            }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "End-to-end checksum match! (" << transfer.destChecksumValue << ")" << commit;
        }
    }

    /////////////////////////////////
    /// Release source file
    /////////////////////////////////

    if (!transfer.tokenBringOnline.empty() && !opts.skipEvict) {
        releaseSourceFile(transfer);
    }
}


void UrlCopyProcess::run()
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


void UrlCopyProcess::cancel()
{
    canceled = true;
    gfal2.cancel();
}


void UrlCopyProcess::timeout()
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
