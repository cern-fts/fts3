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

#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/tokenizer.hpp>
#include <gfal_api.h>

#include "args.h"
#include "common/definitions.h"
#include "errors.h"
#include "file_management.h"
#include "heuristics.h"
#include "common/Logger.h"
#include "monitoring/msg-ifce.h"
#include "reporter.h"
#include "transfer.h"
#include "common/UserProxyEnv.h"
#include "cred/DelegCred.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>

#include "common/DaemonTools.h"
#include "common/panic.h"
#include "version.h"

using boost::thread;
using namespace boost::algorithm;
using namespace fts3::common;

static FileManagement fileManagement;
static transfer_completed transferCompletedMessage;
static bool retry = true;
static std::string errorScope("");
static std::string errorPhase("");
static std::string reasonClass("");
static std::string errorMessage("");
static std::string readFile("");
static volatile bool propagated = false;
static volatile bool terminalState = false;
static std::string globalErrorMessage("");
static std::vector<std::string> turlVector;
static bool inShutdown = false;


//make them global so as to catch unexpected exception before proper reading the arguments
static std::string job_id;
static unsigned file_id;

time_t globalTimeout;

Transfer currentTransfer;

gfal2_context_t handle = NULL;


static std::string replace_dn(std::string& user_dn)
{
    boost::replace_all(user_dn, "?", " ");
    return user_dn;
}


static std::string replaceMetadataString(std::string text)
{
    text = boost::replace_all_copy(text, "?"," ");
    text = boost::replace_all_copy(text, "\\\"","\"");
    return text;
}


static std::vector<std::string> turlList(std::string str)
{
    std::string source_turl;
    std::string destination_turl;
    std::vector<std::string> turl;
    std::string delimiter = "=>";
    std::string shost;
    size_t pos = 0;
    std::string token;

    std::size_t begin = str.find_first_of("(");
    std::size_t end = str.find_first_of(")");
    if (std::string::npos != begin && std::string::npos != end && begin <= end)
        str.erase(begin, end - begin + 1);

    std::size_t begin2 = str.find_first_of("(");
    std::size_t end2 = str.find_first_of(")");
    if (std::string::npos != begin2 && std::string::npos != end2 && begin2 <= end2)
        str.erase(begin2, end2 - begin2 + 1);

    while ((pos = str.find(delimiter)) != std::string::npos) {
        token = str.substr(0, pos);
        source_turl = token;
        str.erase(0, pos + delimiter.length());
        destination_turl = str;
    }

    //source
    if (!source_turl.empty() && source_turl.length() > 4) {
        Uri src_uri = Uri::parse(source_turl);

        if (src_uri.protocol.length() && src_uri.host.length()) {
            shost = src_uri.getSeName();
            trim(shost);
            turl.push_back(shost);
        }
    }

    //destination
    if (!destination_turl.empty() && destination_turl.length() > 4) {
        Uri dst_uri = Uri::parse(destination_turl);

        if (dst_uri.protocol.length() && dst_uri.host.length()) {
            shost = dst_uri.getSeName();
            trim(shost);
            turl.push_back(shost);
        }
    }

    return turl;
}


static void cancelTransfer()
{
    static volatile bool canceled = false;
    if (handle && !canceled)   // finish all transfer in a clean way
    {
        canceled = true;
        gfal2_cancel(handle);
    }
}


static std::string srmVersion(const std::string & url)
{
    if (url.compare(0, 6, "srm://") == 0)
        return std::string("2.2.0");

    return std::string("");
}


static bool bothGsiftp(const std::string & source, const std::string & dest)
{
    if( boost::starts_with(source, "gsiftp://") && boost::starts_with(dest, "gsiftp://"))
        return true;

    return false;
}


static void call_perf(gfalt_transfer_status_t h, const char*, const char*, gpointer)
{
    if (h) {
        size_t avg = gfalt_copy_get_average_baudrate(h, NULL);
        if (avg > 0) {
            avg = avg / 1024;
        }
        else {
            avg = 0;
        }
        size_t inst = gfalt_copy_get_instant_baudrate(h, NULL);
        if (inst > 0) {
            inst = inst / 1024;
        }
        else {
            inst = 0;
        }

        size_t trans = gfalt_copy_get_bytes_transfered(h, NULL);
        time_t elapsed = gfalt_copy_get_elapsed_time(h, NULL);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "bytes: " << trans
        << ", avg KB/sec:" << avg
        << ", inst KB/sec:" << inst
        << ", elapsed:" << elapsed
        << commit;
        currentTransfer.throughput = (double) avg;
        currentTransfer.transferredBytes = trans;
    }
}


std::string getDefaultScope()
{
    return errorScope.length() == 0 ? AGENT : errorScope;
}


std::string getDefaultReasonClass()
{
    return reasonClass.length() == 0 ? ALLOCATION : reasonClass;
}


std::string getDefaultErrorPhase()
{
    return errorPhase.length() == 0 ? GENERAL_FAILURE : errorPhase;
}


void taskTimerCanceler()
{
    boost::this_thread::sleep(boost::posix_time::seconds(600));
    exit(1);
}


void abnormalTermination(Reporter &reporter, std::string classification, const std::string&,
    const std::string &finalState, bool exitProc=true)
{
    terminalState = true;

    //stop reporting progress
    inShutdown = true;

    if (globalErrorMessage.length() > 0) {
        errorMessage += " " + globalErrorMessage;
    }

    if (classification != "CANCELED")
        retry = true;

    FTS3_COMMON_LOGGER_NEWLOG(CRIT) << errorMessage << commit;

    transferCompletedMessage.transfer_error_scope = getDefaultScope();
    transferCompletedMessage.transfer_error_category = getDefaultReasonClass();
    transferCompletedMessage.is_recoverable = retry;
    transferCompletedMessage.failure_phase = getDefaultErrorPhase();
    transferCompletedMessage.transfer_error_message = errorMessage;
    transferCompletedMessage.final_transfer_state = finalState;
    transferCompletedMessage.tr_timestamp_complete = getTimestampStr();
    transferCompletedMessage.job_m_replica = currentTransfer.job_m_replica;

    if (currentTransfer.job_m_replica == "true") {
        if (classification == "CANCELED") {
            transferCompletedMessage.job_state = "CANCELED";
        }
        else {
            if (currentTransfer.last_replica == "true") {
                transferCompletedMessage.job_state = "FAILED";
            }
            else {
                transferCompletedMessage.job_state = "ACTIVE";
            }
        }
    }
    else {
        transferCompletedMessage.job_state = "UNKNOWN";
    }


    if (classification == "CANCELED")
        classification = "FAILED";

    if (UrlCopyOpts::getInstance().monitoringMessages) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Send monitoring complete message" << commit;
        std::string msgReturnValue = msg_ifce::getInstance()->SendTransferFinishMessage(reporter.producer,
            transferCompletedMessage);
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Complete message content: " << msgReturnValue << commit;
    }

    reporter.timeout = UrlCopyOpts::getInstance().timeout;
    reporter.nostreams = UrlCopyOpts::getInstance().nStreams;
    reporter.buffersize = UrlCopyOpts::getInstance().tcpBuffersize;

    if (currentTransfer.fileId == 0 && file_id > 0)
        currentTransfer.fileId = file_id;

    reporter.sendTerminal(currentTransfer.throughput, retry,
        currentTransfer.jobId, currentTransfer.fileId,
        classification, errorMessage,
        currentTransfer.getTransferDurationInSeconds(),
        currentTransfer.fileSize);

    std::string moveFile = fileManagement.archive();

    reporter.sendLog(currentTransfer.jobId, currentTransfer.fileId, fileManagement._getLogArchivedFileFullPath(),
        UrlCopyOpts::getInstance().debugLevel);

    if (moveFile.length() != 0) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "INIT Failed to archive file: " << moveFile
        << commit;
    }
    if (UrlCopyOpts::getInstance().areTransfersOnFile() && readFile.length() > 0)
        unlink(readFile.c_str());

    try {
        boost::thread bt(taskTimerCanceler);
    }
    catch (std::exception &e) {
        globalErrorMessage = e.what();
    }
    catch (...) {
        globalErrorMessage = "INIT Failed to create boost thread, boost::thread_resource_error";
    }

    //send a ping here in order to flag this transfer's state as terminal to store into t_turl
    if (turlVector.size() == 2) //make sure it has values
    {
        double throughputTurl = 0.0;

        if (currentTransfer.throughput > 0.0) {
            throughputTurl = convertKbToMb(currentTransfer.throughput);
        }

        reporter.sendPing(currentTransfer.jobId,
            currentTransfer.fileId,
            throughputTurl,
            currentTransfer.transferredBytes,
            reporter.source_se,
            reporter.dest_se,
            turlVector[0],
            turlVector[1],
            "FAILED");
    }

    cancelTransfer();
    sleep(1);
    if (exitProc)
        exit(1);
}


void canceler(Reporter &reporter)
{
    errorMessage = "INIT Transfer " + currentTransfer.jobId + " was canceled because it was not responding";
    abnormalTermination(reporter, "FAILED", errorMessage, "Abort");
}

/// This thread reduces one by one the value pointed by timeout,
/// until it reaches 0.
/// Using a pointer allow us to reset the timeout if we happen to hit
/// a file bigger than initially expected.
void taskTimer(time_t* timeout, Reporter *reporter)
{
    while (*timeout) {
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        *timeout -= 1;
    }
    canceler(*reporter);
}


void taskStatusUpdater(int time, Reporter *reporter)
{
    // Do not send url_copy heartbeat right after thread creation, wait a bit
    sleep(30);

    while (time && !inShutdown) {
        double throughputTurl = 0.0;

        if (currentTransfer.throughput > 0.0) {
            throughputTurl = convertKbToMb(currentTransfer.throughput);
        }

        reporter->sendPing(currentTransfer.jobId,
            currentTransfer.fileId,
            throughputTurl,
            currentTransfer.transferredBytes,
            reporter->source_se,
            reporter->dest_se,
            "gsiftp:://fake",
            "gsiftp:://fake",
            "ACTIVE");

        boost::this_thread::sleep(boost::posix_time::seconds(time));
    }
}


void shutdown_callback(int signum, void *reporterPtr)
{
    Reporter *reporter = (Reporter*)reporterPtr;
    //stop reporting progress to the server if a signal is received
    inShutdown = true;

    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Received signal " << signum << " (" << strsignal(signum) << ")" << commit;


    if (signum == SIGABRT || signum == SIGSEGV || signum == SIGILL || signum == SIGFPE || signum == SIGBUS ||
        signum == SIGTRAP || signum == SIGSYS) {
        if (propagated == false) {
            std::string stackTrace = fts3::common::panic::stack_dump(fts3::common::panic::stack_backtrace,
                fts3::common::panic::stack_backtrace_size);
            propagated = true;
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "TRANSFER process died: " << currentTransfer.jobId << commit;
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Received signal: " << signum << commit;
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Stacktrace: " << stackTrace << commit;

            errorMessage = "Transfer process died with: " + stackTrace;
            abnormalTermination(*reporter, "FAILED", errorMessage, "Error", false);
        }
    }
    else if (signum == SIGINT || signum == SIGTERM) {
        if (propagated == false) {
            propagated = true;
            errorMessage = "TRANSFER " + currentTransfer.jobId + " canceled by the user";
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << errorMessage << commit;
            abnormalTermination(*reporter, "CANCELED", errorMessage, "Abort", true);
        }
    }
    else if (signum == SIGUSR1) {
        if (propagated == false) {
            propagated = true;
            errorMessage = "TRANSFER " + currentTransfer.jobId + " has been forced-canceled because it was stalled";
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << errorMessage << commit;
            abnormalTermination(*reporter, "FAILED", errorMessage, "Abort", true);
        }
    }
    else {
        if (propagated == false) {
            propagated = true;
            errorMessage =
                "TRANSFER " + currentTransfer.jobId + " aborted, check log file for details, received signum " +
                boost::lexical_cast<std::string>(signum);
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << errorMessage << commit;
            abnormalTermination(*reporter, "FAILED", errorMessage, "Abort", false);
        }
    }
}

// Callback used to populate the messaging with the different stages
static void event_logger(const gfalt_event_t e, gpointer /*udata*/)
{
    static const char *sideStr[] = {"SRC", "DST", "BTH"};
    static const GQuark SRM_DOMAIN = g_quark_from_static_string("SRM");

    std::string timestampStr = boost::lexical_cast<std::string>(e->timestamp);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << '[' << timestampStr << "] "
        << sideStr[e->side] << ' '
        << g_quark_to_string(e->domain) << '\t'
        << g_quark_to_string(e->stage) << '\t'
        << e->description << commit;

    std::string quark = std::string(g_quark_to_string(e->stage));
    std::string source = currentTransfer.sourceUrl;
    std::string dest = currentTransfer.destUrl;

    if (quark == "TRANSFER:ENTER" && ((source.compare(0, 6, "srm://") == 0) || (dest.compare(0, 6, "srm://") == 0))) {
        turlVector = turlList(std::string(e->description));
    }

    if (e->stage == GFAL_EVENT_TRANSFER_ENTER) {
        transferCompletedMessage.timestamp_transfer_started = timestampStr;
        currentTransfer.startTime = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_TRANSFER_EXIT) {
        transferCompletedMessage.timestamp_transfer_completed = timestampStr;
        currentTransfer.finishTime = e->timestamp;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_SOURCE) {
        transferCompletedMessage.timestamp_checksum_source_started = timestampStr;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_SOURCE) {
        transferCompletedMessage.timestamp_checksum_source_ended = timestampStr;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_DESTINATION) {
        transferCompletedMessage.timestamp_checksum_dest_started = timestampStr;
    }
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_DESTINATION) {
        transferCompletedMessage.timestamp_checksum_dest_ended = timestampStr;
    }
    else if (e->stage == GFAL_EVENT_PREPARE_ENTER && e->domain == SRM_DOMAIN) {
        transferCompletedMessage.time_spent_in_srm_preparation_start = timestampStr;
    }
    else if (e->stage == GFAL_EVENT_PREPARE_EXIT && e->domain == SRM_DOMAIN) {
        transferCompletedMessage.time_spent_in_srm_preparation_end = timestampStr;
    }
    else if (e->stage == GFAL_EVENT_CLOSE_ENTER && e->domain == SRM_DOMAIN) {
        transferCompletedMessage.time_spent_in_srm_finalization_start = timestampStr;
    }
    else if (e->stage == GFAL_EVENT_CLOSE_EXIT && e->domain == SRM_DOMAIN) {
        transferCompletedMessage.time_spent_in_srm_finalization_end = timestampStr;
    }
}


static void log_func(const gchar *, GLogLevelFlags, const gchar *message, gpointer)
{
    if (message) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << message << commit;
    }
}


int statWithRetries(gfal2_context_t handle, const std::string &category, const std::string &url, off_t *size,
    std::string *errMsg)
{
    struct stat statBuffer;
    GError *statError = NULL;
    bool canBeRetried = false;
    int errorCode = 0;
    errMsg->clear();

    for (int attempt = 0; attempt < 4; attempt++) {
        if (gfal2_stat(handle, url.c_str(), &statBuffer, &statError) < 0) {
            errorCode = statError->code;
            errMsg->assign(statError->message);
            g_clear_error(&statError);

            canBeRetried = retryTransfer(errorCode, category, std::string(*errMsg));
            if (!canBeRetried)
                return errorCode;
        }
        else {
            *size = statBuffer.st_size;
            errMsg->clear();
            return 0;
        }

        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << category << " Stat failed with " << *errMsg << "(" << errorCode << ")" <<
        commit;
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << category << " Stat the file will be retried" << commit;
        sleep(3); //give it some time to breath
    }

    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "No more retries for stat the file" << commit;
    return errorCode;
}


void setRemainingTransfersToFailed(Reporter &reporter, const std::vector<Transfer> &transferList, unsigned currentIndex)
{
    for (unsigned i = currentIndex; i < transferList.size(); ++i) {
        const Transfer &t = transferList[i];
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Report FAILED back to the server for " << t.fileId << commit;

        transferCompletedMessage.source_srm_version = srmVersion(t.sourceUrl);
        transferCompletedMessage.destination_srm_version = srmVersion(t.destUrl);
        transferCompletedMessage.source_url = t.sourceUrl;
        transferCompletedMessage.dest_url = t.destUrl;
        transferCompletedMessage.transfer_error_scope = TRANSFER;
        transferCompletedMessage.transfer_error_category = GENERAL_FAILURE;
        transferCompletedMessage.is_recoverable = retry;
        transferCompletedMessage.failure_phase = TRANSFER;
        transferCompletedMessage.transfer_error_message = "Not executed because a previous hop failed";
        transferCompletedMessage.job_state = "UNKNOWN";

        if (UrlCopyOpts::getInstance().monitoringMessages) {
            msg_ifce::getInstance()->SendTransferFinishMessage(reporter.producer, transferCompletedMessage, true);
        }

        reporter.sendTerminal(0, false, t.jobId, t.fileId,
            "FAILED", "Not executed because a previous hop failed",
            0, 0);
    }
}


static void validateRunningPrivileges()
{
    // Make sure we do not run as root
    if (getuid() == 0 || geteuid() == 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Running as root! This is not recommended." << commit;
    }
}


int main(int argc, char **argv)
{
    setenv("GLOBUS_THREAD_MODEL", "pthread", 1);
    validateRunningPrivileges();

    UrlCopyOpts &opts = UrlCopyOpts::getInstance();
    if (opts.parse(argc, argv) < 0) {
        std::cerr << opts.getErrorMessage() << std::endl;
        errorMessage = "TRANSFER process died with: " + opts.getErrorMessage();
        return 1;
    }

    // Create reporter
    Reporter reporter(opts.msgDir);

    //make sure to skip file_id for multi-hop and reuse jobs
    if (opts.multihop || opts.reuse)
        file_id = 0;
    else
        file_id = opts.fileId;

    // register signals handler
    fts3::common::panic::setup_signal_handlers(shutdown_callback, &reporter);

    fileManagement.init(opts.logDir);

    currentTransfer.jobId = opts.jobId;

    UserProxyEnv* cert = NULL;

    if (argc < 4) {
        errorMessage = "INIT Failed to read url-copy process arguments";
        abnormalTermination(reporter, "FAILED", errorMessage, "Error");
    }

    try {
        // Send an update message back to the server to indicate it's alive
        boost::thread btUpdater(taskStatusUpdater, 60, &reporter);
    }
    catch (std::exception &e) {
        globalErrorMessage = "INIT " + std::string(e.what());
        throw;
    }
    catch (...) {
        globalErrorMessage = "INIT Failed to create boost thread, boost::thread_resource_error";
        throw;
    }

    if (opts.proxy.length() > 0) {
        // Set Proxy Env
        cert = new UserProxyEnv(opts.proxy);
    }

    // Populate the transfer list
    std::vector<Transfer> transferList;

    if (opts.areTransfersOnFile()) {
        readFile = opts.msgDir + "/" + opts.jobId;
        Transfer::initListFromFile(opts.jobId, readFile, &transferList);
    }
    else {
        transferList.push_back(Transfer::createFromOptions(opts));
    }

    //cancelation point
    long unsigned int numberOfFiles = transferList.size();
    globalTimeout = numberOfFiles * 6000;

    try {
        boost::thread bt(taskTimer, &globalTimeout, &reporter);
    }
    catch (std::exception &e) {
        errorMessage = e.what();
        abnormalTermination(reporter, "FAILED", errorMessage, "Abort");
    }
    catch (...) {
        errorMessage = "INIT Failed to create boost thread, boost::thread_resource_error";
        abnormalTermination(reporter, "FAILED", errorMessage, "Abort");
    }

    if (opts.areTransfersOnFile() && transferList.empty() == true) {
        errorMessage =
            "INIT Transfer " + currentTransfer.jobId + " contains no urls with session reuse/multihop enabled";

        retry = false;

        abnormalTermination(reporter, "FAILED", errorMessage, "Error");
    }

    // gfal2 debug logging
    gfal2_log_set_handler((GLogFunc) log_func, NULL);
    if (opts.debugLevel > 0) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set the transfer to debug level " << opts.debugLevel << commit;
        gfal2_log_set_level(G_LOG_LEVEL_DEBUG);

        if (opts.debugLevel >= 2) {
            setenv("CGSI_TRACE", "1", 1);
            setenv("GLOBUS_FTP_CLIENT_DEBUG_LEVEL", "255", 1);
            setenv("GLOBUS_FTP_CONTROL_DEBUG_LEVEL", "10", 1);
            setenv("GFAL2_GRIDFTP_DEBUG", "1", 1);
        }
        if (opts.debugLevel >= 3) {
            setenv("CGSI_TRACE", "1", 1);
            setenv("GLOBUS_FTP_CLIENT_DEBUG_LEVEL", "255", 1);
            setenv("GLOBUS_FTP_CONTROL_DEBUG_LEVEL", "10", 1);
            setenv("GLOBUS_GSI_AUTHZ_DEBUG_LEVEL", "2", 1);
            setenv("GLOBUS_CALLOUT_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_GSI_CERT_UTILS_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_GSI_CRED_DEBUG_LEVEL", "10", 1);
            setenv("GLOBUS_GSI_PROXY_DEBUG_LEVEL", "10", 1);
            setenv("GLOBUS_GSI_SYSCONFIG_DEBUG_LEVEL", "1", 1);
            setenv("GLOBUS_GSI_GSS_ASSIST_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_GSSAPI_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_NEXUS_DEBUG_LEVEL", "1", 1);
            setenv("GLOBUS_GIS_OPENSSL_ERROR_DEBUG_LEVEL", "1", 1);
            setenv("XRD_LOGLEVEL", "Dump", 1);
            setenv("GFAL2_GRIDFTP_DEBUG", "1", 1);
        }
    }
    else {
        gfal2_log_set_level(G_LOG_LEVEL_MESSAGE);
    }

    GError* handleError = NULL;
    GError *tmp_err = NULL;
    gfalt_params_t params;
    handle = gfal2_context_new(&handleError);
    params = gfalt_params_handle_new(NULL);
    gfalt_add_event_callback(params, event_logger, NULL, NULL, NULL);

    if (!handle) {
        errorMessage = "Failed to create the gfal2 handle: ";
        if (handleError && handleError->message) {
            errorMessage += "INIT " + std::string(handleError->message);
            abnormalTermination(reporter, "FAILED", errorMessage, "Error");
        }
    }

    //reuse session
    if (opts.areTransfersOnFile()) {
        gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "SESSION_REUSE", TRUE, NULL);
    }

    // Enable UDT
    if (opts.enable_udt) {
        gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "ENABLE_UDT", TRUE, NULL);
    }

    // Enable IPv6
    if (opts.enable_ipv6) {
        gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "IPV6", TRUE, NULL);
    }

    // Load OAuth credentials, if any
    if (!opts.oauthFile.empty()) {
        if (gfal2_load_opts_from_file(handle, opts.oauthFile.c_str(), &handleError) < 0) {
            errorMessage = "OAUTH " + std::string(handleError->message);
            abnormalTermination(reporter, "FAILED", errorMessage, "Error");
        }
        unlink(opts.oauthFile.c_str());
    }

    // Identify the client
    gfal2_set_user_agent(handle, "fts_url_copy", VERSION, NULL);

    for (register unsigned int ii = 0; ii < numberOfFiles; ii++) {
        // New transfer, new message
        transferCompletedMessage = transfer_completed();

        errorScope = std::string("");
        reasonClass = std::string("");
        errorPhase = std::string("");
        retry = true;
        errorMessage = std::string("");
        currentTransfer.throughput = 0.0;

        currentTransfer = transferList[ii];

        // Set custom information in gfal2
        gfal2_add_client_info(handle, "job-id", currentTransfer.jobId.c_str(), NULL);
        gfal2_add_client_info(handle, "file-id", boost::lexical_cast<std::string>(currentTransfer.fileId).c_str(),
            NULL);
        gfal2_add_client_info(handle, "retry", boost::lexical_cast<std::string>(opts.retry).c_str(), NULL);

        fileManagement.setSourceUrl(currentTransfer.sourceUrl);
        fileManagement.setDestUrl(currentTransfer.destUrl);
        fileManagement.setFileId(currentTransfer.fileId);
        fileManagement.setJobId(currentTransfer.jobId);

        reporter.timeout = opts.timeout;
        reporter.nostreams = opts.nStreams;
        reporter.buffersize = opts.tcpBuffersize;
        reporter.source_se = fileManagement.getSourceHostname();
        reporter.dest_se = fileManagement.getDestHostname();
        fileManagement.generateLogFile();


        transferCompletedMessage.tr_timestamp_start = getTimestampStr();
        transferCompletedMessage.agent_fqdn = opts.alias;
        transferCompletedMessage.endpoint = opts.alias;
        transferCompletedMessage.t_channel = fileManagement.getSePair();
        transferCompletedMessage.transfer_id = fileManagement.getLogFileName();
        transferCompletedMessage.source_srm_version = srmVersion(currentTransfer.sourceUrl);
        transferCompletedMessage.destination_srm_version = srmVersion(currentTransfer.destUrl);
        transferCompletedMessage.source_url = currentTransfer.sourceUrl;
        transferCompletedMessage.dest_url = currentTransfer.destUrl;
        transferCompletedMessage.source_hostname = fileManagement.getSourceHostnameFile();
        transferCompletedMessage.dest_hostname = fileManagement.getDestHostnameFile();
        transferCompletedMessage.channel_type = "urlcopy";
        transferCompletedMessage.vo = opts.vo;
        transferCompletedMessage.source_site_name = opts.sourceSiteName;
        transferCompletedMessage.dest_site_name = opts.destSiteName;
        transferCompletedMessage.block_size = opts.blockSize;
        transferCompletedMessage.srm_space_token_dest = opts.destTokenDescription;
        transferCompletedMessage.srm_space_token_source = opts.sourceTokenDescription;

        transferCompletedMessage.retry = opts.retry;
        transferCompletedMessage.retry_max = opts.retry_max;

        if (opts.hide_user_dn) {
            transferCompletedMessage.user_dn = std::string();
        }
        else {
            transferCompletedMessage.user_dn = opts.user_dn;
        }

        transferCompletedMessage.file_metadata = replaceMetadataString(currentTransfer.fileMetadata);
        transferCompletedMessage.job_metadata = replaceMetadataString(opts.jobMetadata);

        if (!opts.logToStderr) {
            std::string debugOutput = "/dev/null";
            if (opts.debugLevel)
                debugOutput = fileManagement.getLogFilePath() + ".debug";
            int checkError = theLogger().redirect(fileManagement.getLogFilePath(), debugOutput);
            if (checkError != 0) {
                std::string message = mapErrnoToString(checkError);
                errorMessage = "INIT Failed to create transfer log file, error was: " + message;
                goto stop;
            }
        }

        if (opts.monitoringMessages) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Send monitoring start message " << commit;
            std::string msgReturnValue = msg_ifce::getInstance()->SendTransferStartMessage(reporter.producer,
                transferCompletedMessage);
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Start message content: " << msgReturnValue << commit;
        }

        //also reuse session when both url's are gsiftp
        if (true == bothGsiftp(currentTransfer.sourceUrl, currentTransfer.destUrl)) {
            gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "SESSION_REUSE", TRUE, NULL);
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "GridFTP session reuse enabled since both uri's are gsiftp" << commit;
        }

        // Scope
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer accepted" << commit;
            if (opts.hide_user_dn) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy: Hidden" << commit;
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User DN Hidden:" << commit;
            }
            else {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Proxy:" << opts.proxy << commit;
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User DN:" << replace_dn(opts.user_dn) << commit;
            }
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "VO:" << opts.vo << commit; //a
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id:" << opts.jobId << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File id:" << currentTransfer.fileId << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer number: " << (ii + 1) << "/" << numberOfFiles << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source url:" << currentTransfer.sourceUrl << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest url:" << currentTransfer.destUrl << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Overwrite enabled:" << opts.overwrite << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Dest space token:" << opts.destTokenDescription << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source space token:" << opts.sourceTokenDescription << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Pin lifetime:" << opts.copyPinLifetime << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BringOnline:" << opts.bringOnline << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum:" << currentTransfer.checksumValue << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum enabled:" << currentTransfer.checksumMethod << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User filesize:" << currentTransfer.userFileSize << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File metadata:" <<
                replaceMetadataString(currentTransfer.fileMetadata) << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job metadata:" << replaceMetadataString(opts.jobMetadata) << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Bringonline token:" << currentTransfer.tokenBringOnline << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Multihop: " << opts.multihop << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "UDT: " << opts.enable_udt << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Active: " << opts.active << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Debug level: " << opts.debugLevel << commit;

            if (opts.strictCopy) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Copy only transfer!" << commit;
            }

            //set to active only for reuse
            if (opts.areTransfersOnFile()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set the transfer to ACTIVE, report back to the server" << commit;
                reporter.setMultipleTransfers(true);
                reporter.sendMessage(currentTransfer.throughput, false,
                    opts.jobId, currentTransfer.fileId,
                    "ACTIVE", "", 0,
                    currentTransfer.fileSize);
            }

            if (opts.proxy.empty() || access(opts.proxy.c_str(), F_OK) != 0) {
                errorMessage = "INIT Proxy error, valid proxy not found. Probably expired";
                errorScope = SOURCE;
                reasonClass = mapErrnoToString(EINVAL);
                errorPhase = TRANSFER_PREPARATION;
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                goto stop;
            }
            else if (access(opts.proxy.c_str(), R_OK) != 0) {
                errorMessage = "INIT Proxy error, check if user fts3 can read " + opts.proxy;
                errorMessage += " (" + mapErrnoToString(errno) + ")";
                errorScope = SOURCE;
                reasonClass = mapErrnoToString(errno);
                errorPhase = TRANSFER_PREPARATION;
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                goto stop;
            }

            // set infosys to gfal2
            if (handle) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BDII:" << opts.infosys << commit;
                if (opts.infosys.compare("false") == 0) {
                    gfal2_set_opt_boolean(handle, "BDII", "ENABLED", false, NULL);
                }
                else {
                    gfal2_set_opt_string(handle, "BDII", "LCG_GFAL_INFOSYS", (char *) opts.infosys.c_str(), NULL);
                }
            }

            if (!opts.sourceTokenDescription.empty())
                gfalt_set_src_spacetoken(params, opts.sourceTokenDescription.c_str(), NULL);

            if (!opts.destTokenDescription.empty())
                gfalt_set_dst_spacetoken(params, opts.destTokenDescription.c_str(), NULL);

            if (opts.strictCopy)
                gfalt_set_strict_copy_mode(params, TRUE, NULL);

            gfalt_set_create_parent_dir(params, TRUE, NULL);

            //get checksum timeout from gfal2
            int checksumTimeout = gfal2_get_opt_integer(handle, "GRIDFTP PLUGIN", "CHECKSUM_CALC_TIMEOUT", NULL);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Checksum timeout " << checksumTimeout << commit;
            transferCompletedMessage.checksum_timeout = checksumTimeout;

            // Checksums
            if (currentTransfer.checksumMethod) {
                // Set checksum check
                gfalt_set_checksum_check(params, TRUE, NULL);
                if (currentTransfer.checksumMethod == UrlCopyOpts::CompareChecksum::CHECKSUM_RELAXED) {
                    gfal2_set_opt_boolean(handle, "SRM PLUGIN", "ALLOW_EMPTY_SOURCE_CHECKSUM", TRUE, NULL);
                    gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "SKIP_SOURCE_CHECKSUM", TRUE, NULL);
                    gfal2_set_opt_string(handle, "XROOTD PLUGIN", "COPY_CHECKSUM_MODE", "target", NULL);
                }
                else {
                    gfal2_set_opt_string(handle, "XROOTD PLUGIN", "COPY_CHECKSUM_MODE", "end2end", NULL);
                }

                if (!currentTransfer.checksumValue.empty() &&
                    currentTransfer.checksumValue != "x")   //user provided checksum
                {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "User  provided checksum" << commit;
                    gfalt_set_user_defined_checksum(params,
                        currentTransfer.checksumAlgorithm.c_str(),
                        currentTransfer.checksumValue.c_str(),
                        NULL);
                }
                else    //use auto checksum
                {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Calculate checksum auto" << commit;
                }
            }


            // Before any operation, check if the proxy is valid
            std::string message;
            bool isValid = DelegCred::isValidProxy(opts.proxy, message);
            if (!isValid) {
                errorMessage = "INIT" + message;
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                errorScope = SOURCE;
                reasonClass = mapErrnoToString(gfal_posix_code_error());
                errorPhase = TRANSFER_PREPARATION;
                retry = false;
                goto stop;
            }

            // Stat source file
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "SOURCE Stat the source surl start" << commit;
            int errorCode = statWithRetries(handle, "SOURCE", currentTransfer.sourceUrl, &currentTransfer.fileSize,
                &errorMessage);
            if (errorCode != 0) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "SOURCE Failed to get source file size, errno:"
                << errorCode << ", " << errorMessage << commit;

                errorMessage = "SOURCE Failed to get source file size: " + errorMessage;
                errorScope = SOURCE;
                reasonClass = mapErrnoToString(errorCode);
                errorPhase = TRANSFER_PREPARATION;
                retry = retryTransfer(errorCode, "SOURCE", errorMessage);
                goto stop;
            }

            if (currentTransfer.fileSize == 0) {
                errorMessage = "SOURCE file size is 0";
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                errorScope = SOURCE;
                reasonClass = mapErrnoToString(gfal_posix_code_error());
                errorPhase = TRANSFER_PREPARATION;
                retry = true;
                goto stop;
            }

            if (currentTransfer.userFileSize != 0 && currentTransfer.userFileSize != currentTransfer.fileSize) {
                std::stringstream error_;
                error_ << "SOURCE User specified source file size is " << currentTransfer.userFileSize <<
                " but stat returned " << currentTransfer.fileSize;
                errorMessage = error_.str();
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                errorScope = SOURCE;
                reasonClass = mapErrnoToString(gfal_posix_code_error());
                errorPhase = TRANSFER_PREPARATION;
                retry = true;
                goto stop;
            }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source file size: " << currentTransfer.fileSize << commit;
            transferCompletedMessage.file_size = currentTransfer.fileSize;

            //overwrite dest file if exists
            if (opts.overwrite) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Overwrite is enabled" << commit;
                gfalt_set_replace_existing_file(params, TRUE, NULL);
            }
            else if (opts.strictCopy) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Overwrite is not enabled, but this is a copy-only transfer" <<
                commit;
            }
            else {
                struct stat statbufdestOver;
                // if overwrite is not enabled, check if  exists and stop the transfer if it does
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Stat the dest surl to check if file already exists" << commit;
                errorMessage = ""; //reset
                if (gfal2_stat(handle, (currentTransfer.destUrl).c_str(), &statbufdestOver, &tmp_err) == 0) {
                    double dest_sizeOver = (double) statbufdestOver.st_size;
                    if (dest_sizeOver > 0) {
                        errorMessage = "DESTINATION file already exists and overwrite is not enabled";
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                        errorScope = DESTINATION;
                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                        errorPhase = TRANSFER_PREPARATION;
                        retry = false;
                        goto stop;
                    }
                }
                else {
                    g_clear_error(&tmp_err); //don't do anything
                }
            }

            unsigned int experimentalTimeout = adjustTimeoutBasedOnSize(currentTransfer.fileSize, opts.timeout,
                opts.secPerMb, opts.global_timeout);
            if (!opts.manualConfig || opts.autoTunned || opts.timeout == 0)
                opts.timeout = experimentalTimeout;

            if (opts.global_timeout) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer timeout is set globally:" << opts.timeout << commit;
            }
            else {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer timeout:" << opts.timeout << commit;
            }
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Add " << opts.secPerMb << " seconds per MB transfer timeout " << commit;

            gfalt_set_timeout(params, opts.timeout, NULL);
            transferCompletedMessage.transfer_timeout = opts.timeout;
            globalTimeout = experimentalTimeout + 3600;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Resetting global timeout thread to " << globalTimeout << " seconds" <<
            commit;

            //tune streams based on levels and/or session reuse
            if (!opts.multihop && opts.reuse) {
                opts.nStreams = 1;
                gfalt_set_nbstreams(params, opts.nStreams, NULL);
                gfalt_set_tcp_buffer_size(params, 0, NULL);
            }
            else if ((!opts.manualConfig || opts.autoTunned) && opts.level == 3) {
                int tcp_buffer_size = 8388608; //8MB
                int tcp_streams_max = 16;

                if (currentTransfer.fileSize <= tcp_buffer_size) {
                    opts.nStreams = 2;
                    opts.tcpBuffersize = tcp_buffer_size / 2;
                    gfalt_set_nbstreams(params, opts.nStreams, NULL);
                    gfalt_set_tcp_buffer_size(params, opts.tcpBuffersize, NULL);

                }
                else {
                    if ((currentTransfer.fileSize / tcp_buffer_size) > tcp_streams_max) {
                        opts.nStreams = adjustStreamsBasedOnSize(currentTransfer.fileSize, opts.nStreams);
                        opts.tcpBuffersize = tcp_buffer_size;
                        gfalt_set_nbstreams(params, opts.nStreams, NULL);
                        gfalt_set_tcp_buffer_size(params, opts.tcpBuffersize, NULL);
                    }
                    else {
                        opts.nStreams = static_cast<int>(
                            (ceil((currentTransfer.fileSize / static_cast<double>(tcp_buffer_size)))) + 1);
                        opts.tcpBuffersize = tcp_buffer_size;
                        gfalt_set_nbstreams(params, opts.nStreams, NULL);
                        gfalt_set_tcp_buffer_size(params, opts.tcpBuffersize, NULL);
                    }
                }
            }
            else if ((!opts.manualConfig || opts.autoTunned) && opts.level == 2) {
                gfalt_set_nbstreams(params, opts.nStreams, NULL);
                gfalt_set_tcp_buffer_size(params, opts.tcpBuffersize, NULL);
            }
            else if ((!opts.manualConfig || opts.autoTunned) && opts.level == 1) {
                unsigned int experimentalNstreams = adjustStreamsBasedOnSize(currentTransfer.fileSize, opts.nStreams);
                opts.nStreams = experimentalNstreams;
                gfalt_set_nbstreams(params, opts.nStreams, NULL);
                gfalt_set_tcp_buffer_size(params, opts.tcpBuffersize, NULL);
            }
            else {
                gfalt_set_nbstreams(params, opts.nStreams, NULL);
                gfalt_set_tcp_buffer_size(params, opts.tcpBuffersize, NULL);
            }

            transferCompletedMessage.number_of_streams = opts.nStreams;
            transferCompletedMessage.tcp_buffer_size = opts.tcpBuffersize;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TCP streams: " << opts.nStreams << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TCP buffer size: " << opts.tcpBuffersize << commit;

            //update protocol stuff
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Update protocol stuff, report back to the server" << commit;
            reporter.timeout = opts.timeout;
            reporter.nostreams = opts.nStreams;
            reporter.buffersize = opts.tcpBuffersize;

            reporter.sendMessage(currentTransfer.throughput, false,
                opts.jobId, currentTransfer.fileId,
                "UPDATE", "",
                0, currentTransfer.fileSize);

            gfalt_add_monitor_callback(params, &call_perf, NULL, NULL, NULL);

            //check all params before passed to gfal2
            if ((currentTransfer.sourceUrl).c_str() == NULL || (currentTransfer.destUrl).c_str() == NULL) {
                errorMessage = "INIT Failed to get source or dest surl";
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                errorScope = TRANSFER;
                reasonClass = GENERAL_FAILURE;
                errorPhase = TRANSFER;
                retry = false;
                goto stop;
            }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer Starting" << commit;
            reporter.sendLog(opts.jobId, currentTransfer.fileId, fileManagement.getLogFilePath(), opts.debugLevel);

            if (gfalt_copy_file(handle, params, (currentTransfer.sourceUrl).c_str(), (currentTransfer.destUrl).c_str(),
                &tmp_err) != 0) {
                if (tmp_err != NULL && tmp_err->message != NULL) {
                    FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << std::string(tmp_err->message) << commit;
                    if (tmp_err->code == ETIMEDOUT) {
                        errorMessage = std::string(tmp_err->message);
                        errorMessage += ", operation timeout";
                    }
                    else {
                        errorMessage = std::string(tmp_err->message);
                    }
                    errorScope = TRANSFER;
                    reasonClass = mapErrnoToString(tmp_err->code);
                    errorPhase = TRANSFER;
                }
                else {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "TRANSFER failed - Error message: Unresolved error" << commit;
                    errorMessage = std::string("Unresolved error");
                    errorScope = TRANSFER;
                    reasonClass = GENERAL_FAILURE;
                    errorPhase = TRANSFER;
                }
                if (tmp_err) {
                    std::string message;
                    if (tmp_err->message)
                        message.assign(tmp_err->message);
                    retry = retryTransfer(tmp_err->code, "TRANSFER", message);
                }
                g_clear_error(&tmp_err);
                goto stop;
            }
            else {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "Transfer completed successfully" << commit;
            }

            currentTransfer.transferredBytes = currentTransfer.fileSize;
            transferCompletedMessage.total_bytes_transfered = currentTransfer.fileSize;

            if (!opts.strictCopy) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DESTINATION Stat the dest surl start" << commit;
                off_t dest_size;
                errorCode = statWithRetries(handle, "DESTINATION", currentTransfer.destUrl, &dest_size, &errorMessage);
                if (errorCode != 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DESTINATION Failed to get dest file size, errno:" << errorCode <<
                    ", "
                    << errorMessage << commit;
                    errorMessage = "DESTINATION Failed to get dest file size: " + errorMessage;
                    errorScope = DESTINATION;
                    reasonClass = mapErrnoToString(errorCode);
                    errorPhase = TRANSFER_FINALIZATION;
                    retry = retryTransfer(errorCode, "DESTINATION", errorMessage);
                    goto stop;
                }

                if (dest_size <= 0) {
                    errorMessage = "DESTINATION file size is 0";
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                    errorScope = DESTINATION;
                    reasonClass = mapErrnoToString(gfal_posix_code_error());
                    errorPhase = TRANSFER_FINALIZATION;
                    retry = true;
                    goto stop;
                }

                if (currentTransfer.userFileSize != 0 && currentTransfer.userFileSize != dest_size) {
                    std::stringstream error_;
                    error_ << "DESTINATION User specified destination file size is " << currentTransfer.userFileSize <<
                    " but stat returned " << dest_size;
                    errorMessage = error_.str();
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                    errorScope = DESTINATION;
                    reasonClass = mapErrnoToString(gfal_posix_code_error());
                    errorPhase = TRANSFER_FINALIZATION;
                    retry = true;
                    goto stop;
                }

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DESTINATION  file size: " << dest_size << commit;

                //check source and dest file sizes
                if (currentTransfer.fileSize == dest_size) {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DESTINATION Source and destination file size matching" <<
                    commit;
                    errorMessage = "";
                }
                else {
                    errorMessage = "DESTINATION Source and destination file size mismatch ";
                    errorMessage += boost::lexical_cast<std::string>(currentTransfer.fileSize);
                    errorMessage += " <> ";
                    errorMessage += boost::lexical_cast<std::string>(dest_size);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << errorMessage << commit;
                    errorScope = DESTINATION;
                    reasonClass = mapErrnoToString(gfal_posix_code_error());
                    errorPhase = TRANSFER_FINALIZATION;
                    goto stop;
                }
            }
            else {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Skipping destination file size check" << commit;
            }
        } //logStream
stop:
        transferCompletedMessage.transfer_error_scope = errorScope;
        transferCompletedMessage.transfer_error_category = reasonClass;
        transferCompletedMessage.is_recoverable = retry;
        transferCompletedMessage.failure_phase = errorPhase;
        transferCompletedMessage.transfer_error_message = errorMessage;

        double throughputTurl = 0.0;

        if (currentTransfer.throughput > 0.0) {
            throughputTurl = convertKbToMb(currentTransfer.throughput);
        }
        else {
            if (currentTransfer.finishTime > 0 && currentTransfer.startTime > 0) {
                uint64_t totalTimeInSecs = (currentTransfer.finishTime - currentTransfer.startTime) / 1000;
                if (totalTimeInSecs <= 1)
                    totalTimeInSecs = 1;

                throughputTurl = convertBtoM(currentTransfer.fileSize, totalTimeInSecs);
            }
            else {
                throughputTurl = convertBtoM(currentTransfer.fileSize, 1);
            }
        }

        if (errorMessage.length() > 0) {
            transferCompletedMessage.job_m_replica = opts.job_m_replica;

            if (opts.job_m_replica == "true") {
                if (opts.last_replica == "true") {
                    transferCompletedMessage.job_state = "FAILED";
                }
                else {
                    transferCompletedMessage.job_state = "ACTIVE";
                }
            }
            else {
                transferCompletedMessage.job_state = "UNKNOWN";
            }

            transferCompletedMessage.final_transfer_state = "Error";
            reporter.timeout = opts.timeout;
            reporter.nostreams = opts.nStreams;
            reporter.buffersize = opts.tcpBuffersize;
            if (!terminalState) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Report FAILED back to the server" << commit;
                reporter.sendTerminal(currentTransfer.throughput, retry,
                    opts.jobId, currentTransfer.fileId,
                    "FAILED", errorMessage,
                    currentTransfer.getTransferDurationInSeconds(),
                    currentTransfer.fileSize);

                //send a ping here in order to flag this transfer's state as terminal to store into t_turl
                if (turlVector.size() == 2) //make sure it has values
                {
                    reporter.sendPing(currentTransfer.jobId,
                        currentTransfer.fileId,
                        throughputTurl,
                        currentTransfer.transferredBytes,
                        reporter.source_se,
                        reporter.dest_se,
                        turlVector[0],
                        turlVector[1],
                        "FAILED");
                }
                turlVector.clear();
            }

            // In case of failure, if this is a multihop transfer, set to fail
            // all the remaining transfers
            if (opts.multihop) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Setting to fail the remaining transfers" << commit;
                setRemainingTransfersToFailed(reporter, transferList, ii);

                std::string archiveErr = fileManagement.archive();
                if (!archiveErr.empty())
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not archive: " << archiveErr << commit;
                reporter.sendLog(opts.jobId, currentTransfer.fileId, fileManagement._getLogArchivedFileFullPath(),
                    opts.debugLevel);
                break; // exit the loop
            }
        }
        else {
            transferCompletedMessage.job_m_replica = opts.job_m_replica;
            if (opts.job_m_replica == "true") {
                transferCompletedMessage.job_state = "FINISHED";
            }
            else {
                transferCompletedMessage.job_state = "UNKNOWN";
            }

            transferCompletedMessage.final_transfer_state = "Ok";
            reporter.timeout = opts.timeout;
            reporter.nostreams = opts.nStreams;
            reporter.buffersize = opts.tcpBuffersize;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Report FINISHED back to the server" << commit;
            reporter.sendTerminal(currentTransfer.throughput, false,
                opts.jobId, currentTransfer.fileId,
                "FINISHED", errorMessage,
                currentTransfer.getTransferDurationInSeconds(),
                currentTransfer.fileSize);
            g_clear_error(&tmp_err);

            // Send a ping here in order to flag this transfer's state as terminal to store into t_turl
            if (turlVector.size() == 2) //make sure it has values
            {
                reporter.sendPing(currentTransfer.jobId,
                    currentTransfer.fileId,
                    throughputTurl,
                    currentTransfer.transferredBytes,
                    reporter.source_se,
                    reporter.dest_se,
                    turlVector[0],
                    turlVector[1],
                    "FINISHED");
            }
            turlVector.clear();
        }

        transferCompletedMessage.tr_timestamp_complete = getTimestampStr();

        if (opts.monitoringMessages) {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Send monitoring complete message" << commit;
            std::string msgReturnValue = msg_ifce::getInstance()->SendTransferFinishMessage(reporter.producer,
                transferCompletedMessage);
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Complete message content: " << msgReturnValue << commit;
        }

        inShutdown = true;
        std::string archiveErr = fileManagement.archive();

        if (!archiveErr.empty())
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not archive: " << archiveErr << commit;
        reporter.sendLog(opts.jobId, currentTransfer.fileId,
            fileManagement._getLogArchivedFileFullPath(), opts.debugLevel);
    } //end for reuse loop

    if (params) {
        gfalt_params_handle_delete(params, NULL);
        params = NULL;
    }
    if (handle) {
        gfal2_context_free(handle);
        handle = NULL;
    }

    if (cert) {
        delete cert;
        cert = NULL;
    }

    if (opts.areTransfersOnFile() && readFile.length() > 0) {
        unlink(readFile.c_str());
    }

    sleep(1);
    return EXIT_SUCCESS;
}
