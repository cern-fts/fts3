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

#include <dlfcn.h>

#include <cstdlib>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include "common/Logger.h"

#include "LogHelper.h"
#include "heuristics.h"
#include "AutoInterruptThread.h"
#include "UrlCopyProcess.h"
#include "version.h"

using fts3::common::commit;

// dlopen-style handle to the token issuer library.  Allows us to
// simplify the build-time dependencies.
int (*g_x509_scitokens_issuer_init_p)(char **) = NULL;
char* (*g_x509_scitokens_issuer_get_token_p)(const char *, const char *, const char *,
                                             char**) = NULL;
char *(*g_x509_macaroon_issuer_retrieve_p)(const char *, const char *, const char *, int,
                                          const char **, char **) = NULL;
void *g_x509_scitokens_issuer_handle = NULL;


static void setupGlobalGfal2Config(const UrlCopyOpts &opts, Gfal2 &gfal2)
{
    if (!opts.oauthFile.empty()) {
        try {
            gfal2.loadConfigFile(opts.oauthFile);
            unlink(opts.oauthFile.c_str());
        }
        catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load OAuth config file: " << ex.what() << commit;
        }
    }

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


UrlCopyProcess::UrlCopyProcess(const UrlCopyOpts &opts, Reporter &reporter):
    opts(opts), reporter(reporter), canceled(false), timeoutExpired(false)
{
    todoTransfers = opts.transfers;
    setupGlobalGfal2Config(opts, gfal2);
}


static void initTokenLibrary()
{
    if (g_x509_scitokens_issuer_handle)
    {
        return;
    }

    char *error;
    g_x509_scitokens_issuer_handle = dlopen("libX509SciTokensIssuer.so",
                                            RTLD_NOW|RTLD_GLOBAL);
    if (!g_x509_scitokens_issuer_handle)
    {
        char *error = dlerror();
        std::stringstream ss;
        ss <<  "Failed to load the token issuer library: " <<
            (error ? error : "(unknown)");
        throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, EINVAL, ss.str());
    }
    // Clear any potential error message
    dlerror();

    *(void **)(&g_x509_scitokens_issuer_init_p) = dlsym(g_x509_scitokens_issuer_handle,
                                                        "x509_scitokens_issuer_init");
    if ((error = dlerror()) != NULL)
    {
        std::stringstream ss;
        ss << "Failed to load the initializer handle: " << error;
        dlclose(g_x509_scitokens_issuer_handle);
        g_x509_scitokens_issuer_handle = NULL;
        throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, EINVAL, ss.str());
    }
    dlerror();

    *(void **)(&g_x509_scitokens_issuer_get_token_p) =
        dlsym(g_x509_scitokens_issuer_handle, "x509_scitokens_issuer_retrieve");
    if ((error = dlerror()) != NULL)
    {
        std::stringstream ss;
        ss << "Failed to load the token retrieval handle: " <<  error;
        g_x509_scitokens_issuer_init_p = NULL;
        dlclose(g_x509_scitokens_issuer_handle);
        g_x509_scitokens_issuer_handle = NULL;
        throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, EINVAL, ss.str());
    }
    dlerror();

    *(void **)(&g_x509_macaroon_issuer_retrieve_p) =
        dlsym(g_x509_scitokens_issuer_handle, "x509_macaroon_issuer_retrieve");
    if ((error = dlerror()) != NULL)
    {
        std::stringstream ss;
        ss << "Failed to load the macaroon retrieval handle: " <<  error;
        g_x509_scitokens_issuer_init_p = NULL;
        g_x509_scitokens_issuer_get_token_p = NULL;
        dlclose(g_x509_scitokens_issuer_handle);
        g_x509_scitokens_issuer_handle = NULL;
        throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, EINVAL, ss.str());
    }
    dlerror();

    char *err = NULL;
    if ((*g_x509_scitokens_issuer_init_p)(&err))
    {
        std::stringstream ss;
        ss << "Failed to initialize the client issuer library: " << err;
        g_x509_scitokens_issuer_init_p = NULL;
        g_x509_scitokens_issuer_get_token_p = NULL;
        g_x509_macaroon_issuer_retrieve_p = NULL;
        free(err);
        dlclose(g_x509_scitokens_issuer_handle);
        throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, EINVAL, ss.str());
    }
}


static std::string setupBearerToken(const std::string &issuer, const std::string &proxy)
{
    initTokenLibrary();

    char *err = NULL;
    char *token = (*g_x509_scitokens_issuer_get_token_p)(issuer.c_str(),
        proxy.c_str(), proxy.c_str(), &err);
    if (token)
    {
        std::string token_retval(token);
        free(token);
        return token_retval;
    }
    std::stringstream ss;
    ss << "Failed to retrieve token: " << err;
    free(err);

    throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, EIO, ss.str());
}


static std::string setupMacaroon(const std::string &url, const std::string &proxy,
                                 const std::vector<std::string> &activity,
				 unsigned validity)
{
    initTokenLibrary();

    std::vector<const char*> activity_list;
    activity_list.reserve(activity.size() + 1);
    for (std::vector<std::string>::const_iterator iter = activity.begin();
         iter != activity.end();
         iter++)
    {
        activity_list.push_back(iter->c_str());
    }
    activity_list.push_back(NULL);

    char *err = NULL;
    char *token = (*g_x509_macaroon_issuer_retrieve_p)(url.c_str(),
                                                     proxy.c_str(), proxy.c_str(),
                                                     validity,
                                                     &activity_list[0],
                                                     &err);
    if (token)
    {
        std::string token_retval(token);
        free(token);
        return token_retval;
    }
    std::stringstream ss;
    ss << "Failed to retrieve macaroon: " << err;
    free(err);

    throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, EIO, ss.str());
}


static void setupTransferConfig(const UrlCopyOpts &opts, const Transfer &transfer,
    Gfal2 &gfal2, Gfal2TransferParams &params)
{
    params.setStrictCopy(opts.strictCopy);
    params.setCreateParentDir(true);
    params.setReplaceExistingFile(opts.overwrite);
    bool macaroonRequestEnabledSource = false;
    bool macaroonRequestEnabledDestination = false;
    unsigned macaroonValidity = 180;

    if (opts.timeout) {
        macaroonValidity = (unsigned) opts.timeout/60;
    } else if (transfer.userFileSize) {
        macaroonValidity = (unsigned) adjustTimeoutBasedOnSize(transfer.userFileSize, opts.addSecPerMb)/60;
    }
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Source protocol: " << transfer.source.protocol << commit;

    //check source and destination protocol, so to enable macaroons
    if ((transfer.source.protocol.find("dav")==0)  ||  (transfer.source.protocol.find("http") == 0)) {
        macaroonRequestEnabledSource = true;
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Destination protocol: " << transfer.destination.protocol << commit;

    if ((transfer.destination.protocol.find("dav")==0)  ||  (transfer.destination.protocol.find("http") == 0)) {
        macaroonRequestEnabledDestination = true;
    }
  
    // Attempt to retrieve an oauth token from the VO's issuer; if not,
    // then try to retrieve a token from the SE itself.
    if (!transfer.sourceTokenIssuer.empty()) {
        params.setSourceBearerToken(setupBearerToken(transfer.sourceTokenIssuer, opts.proxy));
    }
    else if (macaroonRequestEnabledSource) 
    {
        try
        {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Will attempt to generate a macaroon for source" << commit;
            std::vector<std::string> activity_list;
            activity_list.reserve(2);
            activity_list.push_back("DOWNLOAD");
	    activity_list.push_back("LIST");
            params.setSourceBearerToken(setupMacaroon(transfer.source, opts.proxy, activity_list, macaroonValidity));
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Will use generated macaroon for source." << commit;
        }
        catch (const UrlCopyError &ex)
        {
            // As we always try for a macaroon, do not fail on error.
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Macaroon issuing failed for source; will use GSI proxy for authorization: " << ex.what() << commit;
        }
    }
    if (!transfer.destTokenIssuer.empty()) {
        params.setDestBearerToken(setupBearerToken(transfer.destTokenIssuer, opts.proxy));
    }
    else if (macaroonRequestEnabledDestination)
    {
        try
        {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Will attempt to generate a macaroon for destination" << commit;
            std::vector<std::string> activity_list;
            activity_list.reserve(4);
            activity_list.push_back("MANAGE");
            activity_list.push_back("UPLOAD");
            activity_list.push_back("DELETE");
	    activity_list.push_back("LIST");
            params.setDestBearerToken(setupMacaroon(transfer.destination, opts.proxy, activity_list, macaroonValidity));
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Will use generated macaroon for destination." << commit;
        }
        catch (const UrlCopyError &ex)
        {
            // As we always try for a macaroon, do not fail on error.
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Macaroon issuing failed for destination; will use GSI proxy for authorization: " << ex.what() << commit;
        }
    }

    if (!transfer.sourceTokenDescription.empty()) {
        params.setSourceSpacetoken(transfer.sourceTokenDescription);
    }
    if (!transfer.destTokenDescription.empty()) {
        params.setDestSpacetoken(transfer.destTokenDescription);
    }
    if (!transfer.checksumAlgorithm.empty()) {
    	try	{
    		params.setChecksum(transfer.checksumMode, transfer.checksumAlgorithm, transfer.checksumValue);
    	}
    	catch (const Gfal2Exception &ex) {
    		if (transfer.checksumMode == Transfer::CHECKSUM_SOURCE) {
    			throw UrlCopyError(SOURCE, TRANSFER_PREPARATION, ex);
    		}
    		else if (transfer.checksumMode == Transfer::CHECKSUM_TARGET) {
    			throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, ex);
    		}
    		else
    		    throw UrlCopyError(TRANSFER, TRANSFER_PREPARATION, ex);
    	}
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
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Timeout expired" << commit;
        urlCopyProcess->timeout();
    }
    catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Timeout stopped" << commit;
    }
    catch (const std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception in the timeout task: " << ex.what() << commit;
    }
}


static void pingTask(Transfer *transfer, Reporter *reporter)
{
    try {
        while (!boost::this_thread::interruption_requested()) {
            boost::this_thread::sleep(boost::posix_time::seconds(60));
            reporter->sendPing(*transfer);
        }
    }
    catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Ping stopped" << commit;
    }
    catch (const std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception in the ping task: " << ex.what() << commit;
    }
}


void UrlCopyProcess::runTransfer(Transfer &transfer, Gfal2TransferParams &params)
{
    // Log info
    if (!opts.proxy.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy: " << opts.proxy << commit;
    }
    else {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Running without proxy" << commit;
    }
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "VO: " << opts.voName << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id: " << transfer.jobId << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File id: " << transfer.fileId << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source url: " << transfer.source << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest url: " << transfer.destination << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Overwrite enabled: " << opts.overwrite << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest space token: " << transfer.destTokenDescription << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source space token: " << transfer.sourceTokenDescription << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum: " << transfer.checksumValue << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum enabled: " << transfer.checksumMode << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User filesize: " << transfer.userFileSize << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File metadata: " << transfer.fileMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job metadata: " << opts.jobMetadata << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Bringonline token: " << transfer.tokenBringOnline << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "UDT: " << opts.enableUdt << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BDII:" << opts.infosys << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source token issuer: " << transfer.sourceTokenIssuer << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destination token issuer: " << transfer.destTokenIssuer << commit;

    if (opts.strictCopy) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Copy only transfer!" << commit;
        transfer.fileSize = transfer.userFileSize;
    }
    else {
        try {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Getting source file size" << commit;
            transfer.fileSize = gfal2.stat(params, transfer.source, true).st_size;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File size: " << transfer.fileSize << commit;
        }
        catch (const Gfal2Exception &ex) {
            throw UrlCopyError(SOURCE, TRANSFER_PREPARATION, ex);
        }
        catch (const std::exception &ex) {
            throw UrlCopyError(SOURCE, TRANSFER_PREPARATION, EINVAL, ex.what());
        }

        if (!opts.overwrite) {
            try {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checking existence of destination file" << commit;
                gfal2.stat(params, transfer.destination, false);
                throw UrlCopyError(DESTINATION, TRANSFER_PREPARATION, EEXIST,
                    "Destination file exists and overwrite is not enabled");
            }
            catch (const Gfal2Exception &ex) {
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
    AutoInterruptThread pingThread(boost::bind(&pingTask, &transfer, &reporter));

    // Transfer
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Starting transfer" << commit;
    try {
        gfal2.copy(params, transfer.source, transfer.destination);
    }
    catch (const Gfal2Exception &ex) {
        if (timeoutExpired) {
            throw UrlCopyError(TRANSFER, TRANSFER, ETIMEDOUT, ex.what());
        }
        else {
            throw UrlCopyError(TRANSFER, TRANSFER, ex);
        }
    }
    catch (const std::exception &ex) {
        throw UrlCopyError(TRANSFER, TRANSFER, EINVAL, ex.what());
    }

    // Validate destination size
    if (!opts.strictCopy) {
        uint64_t destSize;
        try {
            destSize = gfal2.stat(params, transfer.destination, false).st_size;
        }
        catch (const Gfal2Exception &ex) {
            throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, ex);
        }
        catch (const std::exception &ex) {
            throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, EINVAL, ex.what());
        }

        if (destSize != transfer.fileSize) {
            throw UrlCopyError(DESTINATION, TRANSFER_FINALIZATION, EINVAL,
                "Source and destination file size mismatch");
        }
        else {
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
        }
        else {
            transfer.debugLogFile = "/dev/null";
        }

        if (!opts.logToStderr) {
            fts3::common::theLogger().redirect(transfer.logFile, transfer.debugLogFile);
        }

        // Prepare gfal2 transfer parameters
        Gfal2TransferParams params;
        try {
                setupTransferConfig(opts, transfer, gfal2, params);
        }
        catch (const UrlCopyError &ex) {
                transfer.error.reset(new UrlCopyError(ex));
        }

        // Notify we got it
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer accepted" << commit;
        reporter.sendTransferStart(transfer, params);

        // Run the transfer
        try {
            runTransfer(transfer, params);
        }
        catch (const UrlCopyError &ex) {
            transfer.error.reset(new UrlCopyError(ex));
        }
        catch (const std::exception &ex) {
            transfer.error.reset(new UrlCopyError(AGENT, TRANSFER_SERVICE, EINVAL, ex.what()));
        }
        catch (...) {
            transfer.error.reset(new UrlCopyError(AGENT, TRANSFER_SERVICE, EINVAL, "Unknown exception"));
        }

        // Log error if any
        if (transfer.error) {
            if (transfer.error->isRecoverable()) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "Recoverable error: [" << transfer.error->code() << "] "
                    << transfer.error->what()
                    << commit;
            }
            else {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                << "Non recoverable error: [" << transfer.error->code() << "] "
                << transfer.error->what()
                << commit;
            }
        }
        else {
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

    // On cancellation, todoTransfers will not be empty, and a termination message must be sent
    // for them
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
    }
    catch (const std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to archive the log: "
            << e.what() << commit;
    }

    try {
        if (opts.debugLevel > 0) {
            std::string archivedDebugLogFile = archivedLogFile + ".debug";
            boost::filesystem::rename(transfer.debugLogFile, archivedDebugLogFile);
            transfer.debugLogFile = archivedDebugLogFile;
        }
    }
    catch (const std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to archive the debug log: "
            << e.what() << commit;
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
