/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include "args.h"
#include "heuristics.h"
#include <iostream>
#include <algorithm>
#include <ctype.h>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <stdio.h>
#include <pwd.h>
#include <transfer/gfal_transfer.h>
#include <fstream>
#include "boost/date_time/gregorian/gregorian.hpp"
#include <vector>
#include <gfal_api.h>
#include <memory>
#include "file_management.h"
#include "reporter.h"
#include "logger.h"
#include "msg-ifce.h"
#include "errors.h"
#include "signal_logger.h"
#include "UserProxyEnv.h"
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <boost/tokenizer.hpp>
#include <cstdio>
#include <ctime>
#include "definitions.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <exception>
#include "monitoring/utility_routines.h"
#include <ctime>
#include "name_to_uid.h"
#include <boost/thread.hpp>
#include "StaticSslLocking.h"
#include <sys/resource.h>
#include <boost/algorithm/string/replace.hpp>
#include <execinfo.h>

using namespace std;
using boost::thread;

FileManagement* fileManagement = NULL;
static Reporter reporter;
static transfer_completed tr_completed;
static bool retry = true;
static std::string strArray[7];
static std::string g_file_id("0");
static std::string g_job_id("");
static std::string errorScope("");
static std::string errorPhase("");
static std::string reasonClass("");
static std::string errorMessage("");
static std::string readFile("");
static double source_size = 0.0;
static double dest_size = 0.0;
static double diff = 0.0;
static double transfer_start = 0.0;
static double transfer_complete = 0.0;
static uid_t pw_uid;
static char hostname[1024] = {0};
static volatile bool propagated = false;
static volatile bool canceled = false;
static volatile bool terminalState = false;
static std::string nstream_to_string("");
static std::string tcpbuffer_to_string("");
static std::string block_to_string("");
static std::string timeout_to_string("");
static std::string globalErrorMessage("");
static double throughput = 0.0;
static double transferred_bytes = 0;

extern std::string stackTrace;
gfal_context_t handle = NULL;


//convert milli to secs
static double transferDuration(double start , double complete)
{
    return (start==0 || complete==0)? 0.0: (complete - start) / 1000;
}

static std::string replaceMetadataString(std::string text)
{
    text = boost::replace_all_copy(text, "?"," ");
    text = boost::replace_all_copy(text, "\\\"","");
    return text;
}



static void cancelTransfer()
{
    if (handle && !canceled)   // finish all transfer in a clean way
        {
            canceled = true;
            gfal2_cancel(handle);
        }
}

static int fexists(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

static std::string srmVersion(const std::string & url)
{
    if (url.compare(0, 6, "srm://") == 0)
        return std::string("2.2.0");

    return std::string("");
}


static std::vector<std::string> split(const char *str, char c = ':')
{
    std::vector<std::string> result;

    while (1)
        {
            const char *begin = str;

            while (*str != c && *str)
                str++;

            result.push_back(string(begin, str));

            if (0 == *str++)
                break;
        }

    return result;
}

static void call_perf(gfalt_transfer_status_t h, const char*, const char*, gpointer)
{
    if (h)
        {
            size_t avg = gfalt_copy_get_average_baudrate(h, NULL);
            if (avg > 0)
                {
                    avg = avg / 1024;
                }
            else
                {
                    avg = 0;
                }
            size_t inst = gfalt_copy_get_instant_baudrate(h, NULL);
            if (inst > 0)
                {
                    inst = inst / 1024;
                }
            else
                {
                    inst = 0;
                }

            size_t trans = gfalt_copy_get_bytes_transfered(h, NULL);
            time_t elapsed = gfalt_copy_get_elapsed_time(h, NULL);
            Logger::getInstance().INFO() << "bytes: " << trans
                                         << ", avg KB/sec:" << avg
                                         << ", inst KB/sec:" << inst
                                         << ", elapsed:" << elapsed
                                         << std::endl;
            throughput        = (double) avg;
            transferred_bytes = (double) trans;
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

void abnormalTermination(const std::string& classification, const std::string&, const std::string& finalState)
{
    terminalState = true;

    if(globalErrorMessage.length() > 0)
        {
            errorMessage += " " + globalErrorMessage;
        }

    diff = transferDuration(transfer_start, transfer_complete);
    msg_ifce::getInstance()->set_transfer_error_scope(&tr_completed, getDefaultScope());
    msg_ifce::getInstance()->set_transfer_error_category(&tr_completed, getDefaultReasonClass());
    msg_ifce::getInstance()->set_failure_phase(&tr_completed, getDefaultErrorPhase());
    msg_ifce::getInstance()->set_transfer_error_message(&tr_completed, errorMessage);
    msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, finalState);
    msg_ifce::getInstance()->set_tr_timestamp_complete(&tr_completed, msg_ifce::getInstance()->getTimestamp());
    if(UrlCopyOpts::getInstance().monitoringMessages)
        msg_ifce::getInstance()->SendTransferFinishMessage(&tr_completed);

    reporter.timeout = UrlCopyOpts::getInstance().timeout;
    reporter.nostreams = UrlCopyOpts::getInstance().nStreams;
    reporter.buffersize = UrlCopyOpts::getInstance().tcpBuffersize;

    if (strArray[0].length() > 0)
        reporter.sendTerminal(throughput, retry, g_job_id, strArray[0], classification, errorMessage, diff, source_size);
    else
        reporter.sendTerminal(throughput, retry, g_job_id, g_file_id, classification, errorMessage, diff, source_size);

    std::string moveFile = fileManagement->archive();
    if (strArray[0].length() > 0)
        reporter.sendLog(g_job_id, strArray[0], fileManagement->_getLogArchivedFileFullPath(),
                         UrlCopyOpts::getInstance().debug);
    else
        reporter.sendLog(g_job_id, g_file_id, fileManagement->_getLogArchivedFileFullPath(),
                         UrlCopyOpts::getInstance().debug);

    if (moveFile.length() != 0)
        {
            Logger::getInstance().ERROR() << "Failed to archive file: " << moveFile
                                          << std::endl;
        }
    if (UrlCopyOpts::getInstance().reuseFile && readFile.length() > 0)
        unlink(readFile.c_str());

    cancelTransfer();
    sleep(1);
    exit(1);
}

void canceler()
{
    errorMessage = "Transfer " + g_job_id + " was canceled because it was not responding";

    Logger::getInstance().WARNING() << errorMessage << std::endl;

    abnormalTermination("FAILED", errorMessage, "Abort");
}

/**
 * This thread reduces one by one the value pointed by timeout,
 * until it reaches 0.
 * Using a pointer allow us to reset the timeout if we happen to hit
 * a file bigger than initially expected.
 */
void taskTimer(time_t* timeout)
{
    while (*timeout)
        {
            boost::this_thread::sleep(boost::posix_time::seconds(1));
            *timeout -= 1;
        }
    canceler();
}

void taskStatusUpdater(int time)
{
    while (time)
        {
            if (strArray[0].length() > 0)
                {
                    Logger::getInstance().INFO() << "Sending back to the server url-copy is still alive : " <<  throughput << "  " <<  transferred_bytes
                                                 << std::endl;
                    reporter.sendPing(UrlCopyOpts::getInstance().jobId, strArray[0], throughput, transferred_bytes);
                }
            else
                {
                    Logger::getInstance().INFO() << "Sending back to the server url-copy is still alive : "  <<  throughput << "  " <<  transferred_bytes
                                                 << std::endl;
                    reporter.sendPing(UrlCopyOpts::getInstance().jobId, UrlCopyOpts::getInstance().fileId,
                                      throughput, transferred_bytes);
                }
            boost::this_thread::sleep(boost::posix_time::seconds(time));
        }
}


void log_stack(int sig)
{
    if(sig == SIGSEGV || sig == SIGBUS || sig == SIGABRT)
        {
            const int stack_size = 25;
            void * array[stack_size]= {0};
            int nSize = backtrace(array, stack_size);
            char ** symbols = backtrace_symbols(array, nSize);
            for (register int i = 0; i < nSize; ++i)
                {
                    if(symbols && symbols[i])
                        {
                            stackTrace+=std::string(symbols[i]) + '\n';
                        }
                }
            if(symbols)
                {
                    free(symbols);
                }
        }
}


void signalHandler(int signum)
{
    Logger& logger = Logger::getInstance();

    logger.WARNING() << "Received signal " << signum << std::endl;

    log_stack(signum);
    if (stackTrace.length() > 0)
        {
            propagated = true;

            logger.ERROR() << "Transfer process died " << g_job_id << std::endl;
            logger.ERROR() << "Received signal " << signum << std::endl;
            logger.ERROR() << stackTrace << std::endl;

            errorMessage = "Transfer process died " + g_job_id;
            errorMessage += stackTrace;
            abnormalTermination("FAILED", errorMessage, "Error");
        }
    else if (signum == SIGINT || signum == SIGTERM)
        {
            if (propagated == false)
                {
                    propagated = true;
                    errorMessage = "Transfer " + g_job_id + " canceled by the user";
                    logger.WARNING() << errorMessage << std::endl;
                    abnormalTermination("CANCELED", errorMessage, "Abort");
                }
        }
    else if (signum == SIGUSR1)
        {
            if (propagated == false)
                {
                    propagated = true;
                    errorMessage = "Transfer " + g_job_id + " has been forced-canceled because it was stalled";
                    logger.WARNING() << errorMessage << std::endl;
                    abnormalTermination("FAILED", errorMessage, "Abort");
                }
        }
    else
        {
            if (propagated == false)
                {
                    propagated = true;
                    errorMessage = "Transfer " + g_job_id + " aborted, check log file for details, received signum " + boost::lexical_cast<std::string>(signum);
                    logger.WARNING() << errorMessage << std::endl;
                    abnormalTermination("FAILED", errorMessage, "Abort");
                }
        }
}

// Callback used to populate the messaging with the different stages

static void event_logger(const gfalt_event_t e, gpointer /*udata*/)
{
    static const char* sideStr[] = {"SRC", "DST", "BTH"};
    static const GQuark SRM_DOMAIN = g_quark_from_static_string("SRM");

    msg_ifce* msg = msg_ifce::getInstance();
    std::string timestampStr = boost::lexical_cast<std::string>(e->timestamp);

    Logger::getInstance().INFO() << '[' << timestampStr << "] "
                                 << sideStr[e->side] << ' '
                                 << g_quark_to_string(e->domain) << '\t'
                                 << g_quark_to_string(e->stage) << '\t'
                                 << e->description << std::endl;

    if (e->stage == GFAL_EVENT_TRANSFER_ENTER)
        {
            msg->set_timestamp_transfer_started(&tr_completed, timestampStr);
            transfer_start = boost::lexical_cast<double>(e->timestamp);
        }
    else if (e->stage == GFAL_EVENT_TRANSFER_EXIT)
        {
            msg->set_timestamp_transfer_completed(&tr_completed, timestampStr);
            transfer_complete = boost::lexical_cast<double>(e->timestamp);
        }

    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_SOURCE)
        msg->set_timestamp_checksum_source_started(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_SOURCE)
        msg->set_timestamp_checksum_source_ended(&tr_completed, timestampStr);

    else if (e->stage == GFAL_EVENT_CHECKSUM_ENTER && e->side == GFAL_EVENT_DESTINATION)
        msg->set_timestamp_checksum_dest_started(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_CHECKSUM_EXIT && e->side == GFAL_EVENT_DESTINATION)
        msg->set_timestamp_checksum_dest_ended(&tr_completed, timestampStr);

    else if (e->stage == GFAL_EVENT_PREPARE_ENTER && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_preparation_start(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_PREPARE_EXIT && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_preparation_end(&tr_completed, timestampStr);

    else if (e->stage == GFAL_EVENT_CLOSE_ENTER && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_finalization_start(&tr_completed, timestampStr);
    else if (e->stage == GFAL_EVENT_CLOSE_EXIT && e->domain == SRM_DOMAIN)
        msg->set_time_spent_in_srm_finalization_end(&tr_completed, timestampStr);
}

static void log_func(const gchar *, GLogLevelFlags, const gchar *message, gpointer)
{
    if (message)
        {
            Logger::getInstance().DEBUG() << message << std::endl;
        }
}

void myunexpected()
{
    if (propagated == false)
        {
            propagated = true;
            errorMessage = "Transfer unexpected handler called " + g_job_id;
            errorMessage += " Source: " + UrlCopyOpts::getInstance().sourceUrl;
            errorMessage += " Dest: " + UrlCopyOpts::getInstance().destUrl;
            Logger::getInstance().ERROR() << errorMessage << std::endl;

            abnormalTermination("FAILED", errorMessage, "Abort");
        }
}

void myterminate()
{
    if (propagated == false)
        {
            propagated = true;
            errorMessage = "Transfer terminate handler called:" + g_job_id;
            errorMessage += " Source: " + UrlCopyOpts::getInstance().sourceUrl;
            errorMessage += " Dest: " + UrlCopyOpts::getInstance().destUrl;
            Logger::getInstance().ERROR() << errorMessage << std::endl;

            abnormalTermination("FAILED", errorMessage, "Abort");
        }
}



__attribute__((constructor)) void begin(void)
{
    //switch to non-priviledged user to avoid reading the hostcert
    pw_uid = name_to_uid();
    setuid(pw_uid);
    seteuid(pw_uid);
    StaticSslLocking::init_locks();
    fileManagement = new FileManagement();
}

int main(int argc, char **argv)
{
    Logger &logger = Logger::getInstance();

    // register signals handler
    signal(SIGINT, signalHandler);
    signal(SIGUSR1, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGILL, signalHandler);
    signal(SIGBUS, signalHandler);
    signal(SIGTRAP, signalHandler);
    signal(SIGSYS, signalHandler);

    //set_terminate(myterminate);
    //set_unexpected(myunexpected);

    UrlCopyOpts &opts = UrlCopyOpts::getInstance();
    if (opts.parse(argc, argv) < 0)
        {
            std::cerr << opts.getErrorMessage() << std::endl;
            return 1;
        }

    g_file_id = opts.fileId;
    g_job_id = opts.jobId;

    std::string bytes_to_string("");
    struct stat statbufsrc;
    struct stat statbufdest;
    struct stat statbufdestOver;
    int ret = -1;
    UserProxyEnv* cert = NULL;

    hostname[1023] = '\0';
    gethostname(hostname, 1023);


    /*TODO: until we find a way to calculate RTT(perfsonar) accurately, OS tcp auto-tuning does a better job*/
    opts.tcpBuffersize = DEFAULT_BUFFSIZE;

    if(argc < 4)
        {
            errorMessage = "Failed to read url-copy process arguments";
            abnormalTermination("FAILED", errorMessage, "Abort");
        }


    try
        {
            /*send an update message back to the server to indicate it's alive*/
            boost::thread btUpdater(taskStatusUpdater, 60);
        }
    catch (std::exception& e)
        {
            globalErrorMessage = e.what();
            throw;
        }
    catch(...)
        {
            globalErrorMessage = "Failed to create boost thread, boost::thread_resource_error";
            throw;
        }

    if (opts.proxy.length() > 0)
        {
            // Set Proxy Env
            cert = new UserProxyEnv(opts.proxy);
        }

    std::vector<std::string> urlsFile;
    std::string line("");
    readFile = "/var/lib/fts3/" + opts.jobId;
    if (opts.reuseFile)
        {
            std::ifstream infile(readFile.c_str(), std::ios_base::in);
            while (getline(infile, line, '\n'))
                {
                    urlsFile.push_back(line);
                }
            infile.close();
            if(readFile.length() > 0)
                unlink(readFile.c_str());
        }

    //cancelation point
    long unsigned int reuseOrNot = (urlsFile.empty() == true) ? 1 : urlsFile.size();
    time_t globalTimeout = reuseOrNot * 6000;

    try
        {
            boost::thread bt(taskTimer, &globalTimeout);
        }
    catch (std::exception& e)
        {
            globalErrorMessage = e.what();
            throw;
        }
    catch(...)
        {
            globalErrorMessage = "Failed to create boost thread, boost::thread_resource_error";
            throw;
        }

    if (opts.reuseFile && urlsFile.empty() == true)
        {
            errorMessage = "Transfer " + g_job_id + " containes no urls with session reuse enabled";

            abnormalTermination("FAILED", errorMessage, "Error");
        }


    GError* handleError = NULL;
    GError *tmp_err = NULL;
    gfalt_params_t params;
    handle = gfal_context_new(&handleError);
    params = gfalt_params_handle_new(NULL);
    gfalt_set_event_callback(params, event_logger, NULL);


    //reuse session
    if (opts.reuseFile)
        {
            gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "SESSION_REUSE", TRUE, NULL);
        }


    if (!handle)
        {
            errorMessage = "Failed to create the gfal2 handle: ";
            if (handleError && handleError->message)
                {
                    errorMessage += handleError->message;
                    abnormalTermination("FAILED", errorMessage, "Error");
                }
        }


    for (register unsigned int ii = 0; ii < reuseOrNot; ii++)
        {
            errorScope = std::string("");
            reasonClass = std::string("");
            errorPhase = std::string("");
            retry = true;
            errorMessage = std::string("");
            throughput = 0.0;

            if (opts.reuseFile)
                {
                    std::string mid_str(urlsFile[ii]);
                    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
                    tokenizer tokens(mid_str, boost::char_separator<char> (" "));
                    std::copy(tokens.begin(), tokens.end(), strArray);
                }
            else
                {
                    strArray[0] = opts.fileId;
                    strArray[1] = opts.sourceUrl;
                    strArray[2] = opts.destUrl;
                    strArray[3] = opts.checksumValue;
                    if(opts.userFileSize > 0)
                        strArray[4] = to_string<double >(opts.userFileSize, std::dec);
                    else
                        strArray[4] = "0";
                    strArray[5] = opts.fileMetadata;
                    strArray[6] = opts.tokenBringOnline;
                }

            fileManagement->setSourceUrl(strArray[1]);
            fileManagement->setDestUrl(strArray[2]);
            fileManagement->setFileId(strArray[0]);
            fileManagement->setJobId(opts.jobId);
            g_file_id = strArray[0];
            g_job_id = opts.jobId;

            reporter.timeout = opts.timeout;
            reporter.nostreams = opts.nStreams;
            reporter.buffersize = opts.tcpBuffersize;
            reporter.source_se = fileManagement->getSourceHostname();
            reporter.dest_se = fileManagement->getDestHostname();
            fileManagement->generateLogFile();

            msg_ifce::getInstance()->set_tr_timestamp_start(&tr_completed, msg_ifce::getInstance()->getTimestamp());
            msg_ifce::getInstance()->set_agent_fqdn(&tr_completed, hostname);
            msg_ifce::getInstance()->set_t_channel(&tr_completed, fileManagement->getSePair());
            msg_ifce::getInstance()->set_transfer_id(&tr_completed, fileManagement->getLogFileName());
            msg_ifce::getInstance()->set_source_srm_version(&tr_completed, srmVersion(strArray[1]));
            msg_ifce::getInstance()->set_destination_srm_version(&tr_completed, srmVersion(strArray[2]));
            msg_ifce::getInstance()->set_source_url(&tr_completed, strArray[1]);
            msg_ifce::getInstance()->set_dest_url(&tr_completed, strArray[2]);
            msg_ifce::getInstance()->set_source_hostname(&tr_completed, fileManagement->getSourceHostnameFile());
            msg_ifce::getInstance()->set_dest_hostname(&tr_completed, fileManagement->getDestHostnameFile());
            msg_ifce::getInstance()->set_channel_type(&tr_completed, "urlcopy");
            msg_ifce::getInstance()->set_vo(&tr_completed, opts.vo);
            msg_ifce::getInstance()->set_source_site_name(&tr_completed, opts.sourceSiteName);
            msg_ifce::getInstance()->set_dest_site_name(&tr_completed, opts.destSiteName);
            nstream_to_string = to_string<unsigned int>(opts.nStreams, std::dec);
            msg_ifce::getInstance()->set_number_of_streams(&tr_completed, nstream_to_string.c_str());
            tcpbuffer_to_string = to_string<unsigned int>(opts.tcpBuffersize, std::dec);
            msg_ifce::getInstance()->set_tcp_buffer_size(&tr_completed, tcpbuffer_to_string.c_str());
            block_to_string = to_string<unsigned int>(opts.blockSize, std::dec);
            msg_ifce::getInstance()->set_block_size(&tr_completed, block_to_string.c_str());
            msg_ifce::getInstance()->set_srm_space_token_dest(&tr_completed, opts.destTokenDescription);
            msg_ifce::getInstance()->set_srm_space_token_source(&tr_completed, opts.sourceTokenDescription);

            if(opts.monitoringMessages)
                msg_ifce::getInstance()->SendTransferStartMessage(&tr_completed);

            if (!opts.logToStderr)
                {
                    int checkError = Logger::getInstance().redirectTo(fileManagement->getLogFilePath(), opts.debug);
                    if (checkError != 0)
                        {
                            std::string message = mapErrnoToString(checkError);
                            errorMessage = "Failed to create transfer log file, error was: " + message;
                            goto stop;
                        }
                }

            // Scope
            {
                reporter.sendLog(opts.jobId, strArray[0], fileManagement->getLogFilePath(),
                                 opts.debug);

                gfalt_set_user_data(params, NULL, NULL);

                logger.INFO() << "Transfer accepted" << std::endl;
                logger.INFO() << "Proxy:" << opts.proxy << std::endl;
                logger.INFO() << "VO:" << opts.vo << std::endl; //a
                logger.INFO() << "Job id:" << opts.jobId << std::endl;
                logger.INFO() << "File id:" << strArray[0] << std::endl;
                logger.INFO() << "Source url:" << strArray[1] << std::endl;
                logger.INFO() << "Dest url:" << strArray[2] << std::endl;
                logger.INFO() << "Overwrite enabled:" << opts.overwrite << std::endl;
                logger.INFO() << "Tcp buffer size:" << opts.tcpBuffersize << std::endl;
                logger.INFO() << "Dest space token:" << opts.destTokenDescription << std::endl;
                logger.INFO() << "Source space token:" << opts.sourceTokenDescription << std::endl;
                logger.INFO() << "Pin lifetime:" << opts.copyPinLifetime << std::endl;
                logger.INFO() << "BringOnline:" << opts.bringOnline << std::endl;
                logger.INFO() << "Checksum:" << strArray[3] << std::endl;
                logger.INFO() << "Checksum enabled:" << opts.compareChecksum << std::endl;
                logger.INFO() << "User filesize:" << strArray[4] << std::endl;
                logger.INFO() << "File metadata:" << replaceMetadataString(strArray[5]) << std::endl;
                logger.INFO() << "Job metadata:" << replaceMetadataString(opts.jobMetadata) << std::endl;
                logger.INFO() << "Bringonline token:" << strArray[6] << std::endl;

                //set to active only for reuse
                if (opts.reuseFile)
                    {
                        logger.INFO() << "Set the transfer to ACTIVE, report back to the server" << std::endl;
                        reporter.setReuseTransfer(true);
                        reporter.sendMessage(throughput, false, opts.jobId, strArray[0], "ACTIVE", "", diff, source_size);
                    }

                if (fexists(opts.proxy.c_str()) != 0)
                    {
                        errorMessage = "Proxy doesn't exist, probably expired and not renewed " + opts.proxy;
                        errorScope = SOURCE;
                        reasonClass = mapErrnoToString(errno);
                        errorPhase = TRANSFER_PREPARATION;
                        logger.ERROR() << errorMessage << std::endl;
                        goto stop;
                    }

                /*set infosys to gfal2*/
                if (handle)
                    {
                        logger.INFO() << "BDII:" << opts.infosys << std::endl;
                        if (opts.infosys.compare("false") == 0)
                            {
                                gfal2_set_opt_boolean(handle, "BDII", "ENABLED", false, NULL);
                            }
                        else
                            {
                                gfal2_set_opt_string(handle, "BDII", "LCG_GFAL_INFOSYS", (char *) opts.infosys.c_str(), NULL);
                            }
                    }

                /*gfal2 debug logging*/
                if (opts.debug == true)
                    {
                        logger.INFO() << "Set the transfer to debug mode" << std::endl;
                        gfal_set_verbose(GFAL_VERBOSE_TRACE | GFAL_VERBOSE_VERBOSE | GFAL_VERBOSE_TRACE_PLUGIN);
                        gfal_log_set_handler((GLogFunc) log_func, NULL);
                    }

                if (!opts.sourceTokenDescription.empty())
                    gfalt_set_src_spacetoken(params, opts.sourceTokenDescription.c_str(), NULL);

                if (!opts.destTokenDescription.empty())
                    gfalt_set_dst_spacetoken(params, opts.destTokenDescription.c_str(), NULL);

                gfalt_set_create_parent_dir(params, TRUE, NULL);

                //get checksum timeout from gfal2
                logger.INFO() << "Get checksum timeout" << std::endl;
                int checksumTimeout = gfal2_get_opt_integer(handle, "GRIDFTP PLUGIN", "CHECKSUM_CALC_TIMEOUT", NULL);
                msg_ifce::getInstance()->set_checksum_timeout(&tr_completed, boost::lexical_cast<std::string > (checksumTimeout));

                /*Checksuming*/
                if (opts.compareChecksum)
                    {
                        // Set checksum check
                        gfalt_set_checksum_check(params, TRUE, NULL);
                        if (opts.compareChecksum == UrlCopyOpts::CompareChecksum::CHECKSUM_RELAXED)
                            {
                                gfal2_set_opt_boolean(handle, "SRM PLUGIN", "ALLOW_EMPTY_SOURCE_CHECKSUM", TRUE, NULL);
                                gfal2_set_opt_boolean(handle, "GRIDFTP PLUGIN", "SKIP_SOURCE_CHECKSUM", TRUE, NULL);
                            }

                        if (!opts.checksumValue.empty() && opts.checksumValue != "x")   //user provided checksum
                            {
                                logger.INFO() << "User  provided checksum" << std::endl;
                                //check if only alg is specified
                                if (std::string::npos == (strArray[3]).find(":"))
                                    {
                                        gfalt_set_user_defined_checksum(params, (strArray[3]).c_str(), NULL, NULL);
                                    }
                                else
                                    {
                                        std::vector<std::string> token = split((strArray[3]).c_str());
                                        std::string checkAlg = token[0];
                                        std::string csk = token[1];
                                        gfalt_set_user_defined_checksum(params, checkAlg.c_str(), csk.c_str(), NULL);
                                    }
                            }
                        else    //use auto checksum
                            {
                                logger.INFO() << "Calculate checksum auto" << std::endl;
                            }
                    }

                logger.INFO() << "Stat the source surl start" << std::endl;
                for (int sourceStatRetry = 0; sourceStatRetry < 4; sourceStatRetry++)
                    {
                        errorMessage = ""; //reset
                        if (gfal2_stat(handle, (strArray[1]).c_str(), &statbufsrc, &tmp_err) < 0)
                            {
                                std::string tempError(tmp_err->message);
                                const int errCode = tmp_err->code;
                                logger.ERROR() << "Failed to get source file size, errno:"
                                               << errCode << ", " << tempError
                                               << std::endl;
                                errorMessage = "Failed to get source file size: " + tempError;
                                errorScope = SOURCE;
                                reasonClass = mapErrnoToString(errCode);
                                errorPhase = TRANSFER_PREPARATION;
                                retry = retryTransfer(tmp_err->code, "SOURCE" );
                                if (sourceStatRetry == 3 || retry == false)
                                    {
                                        logger.INFO() << "No more retries for stat the source" << std::endl;
                                        g_clear_error(&tmp_err);
                                        goto stop;
                                    }
                                g_clear_error(&tmp_err);
                            }
                        else
                            {
                                if (statbufsrc.st_size <= 0)
                                    {
                                        errorMessage = "Source file size is 0";
                                        logger.ERROR() << errorMessage << std::endl;
                                        errorScope = SOURCE;
                                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                                        errorPhase = TRANSFER_PREPARATION;
                                        if (sourceStatRetry == 3)
                                            {
                                                logger.INFO() << "No more retries for stat the source" << std::endl;
                                                retry = true;
                                                goto stop;
                                            }
                                    }
                                else if (strArray[4]!= "x" && boost::lexical_cast<double>(strArray[4]) != 0 && boost::lexical_cast<double>(strArray[4]) != statbufsrc.st_size)
                                    {
                                        std::stringstream error_;
                                        error_ << "User specified source file size is " << strArray[4] << " but stat returned " << statbufsrc.st_size;
                                        errorMessage = error_.str();
                                        logger.ERROR() << errorMessage << std::endl;
                                        errorScope = SOURCE;
                                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                                        errorPhase = TRANSFER_PREPARATION;
                                        if (sourceStatRetry == 3)
                                            {
                                                logger.INFO() << "No more retries for stat the source" << std::endl;
                                                retry = true;
                                                goto stop;
                                            }
                                    }
                                else
                                    {
                                        logger.INFO() << "Source file size: " << statbufsrc.st_size << std::endl;
                                        if (statbufsrc.st_size > 0)
                                            source_size = (double) statbufsrc.st_size;
                                        //conver longlong to string
                                        std::string size_to_string = to_string<double > (source_size, std::dec);
                                        //set the value of file size to the message
                                        msg_ifce::getInstance()->set_file_size(&tr_completed, size_to_string.c_str());
                                        break;
                                    }
                            }
                        logger.INFO() <<"Stat the source file will be retried" << std::endl;
                        sleep(3); //give it some time to breath
                    }

                //overwrite dest file if exists
                if (opts.overwrite)
                    {
                        logger.INFO() << "Overwrite is enabled" << std::endl;
                        gfalt_set_replace_existing_file(params, TRUE, NULL);
                    }
                else
                    {
                        //if overwrite is not enabled, check if  exists and stop the transfer if it does
                        logger.INFO() << "Stat the dest surl to check if file already exists" << std::endl;
                        errorMessage = ""; //reset
                        if (gfal2_stat(handle, (strArray[2]).c_str(), &statbufdestOver, &tmp_err) == 0)
                            {
                                double dest_sizeOver = (double) statbufdestOver.st_size;
                                if(dest_sizeOver > 0)
                                    {
                                        errorMessage = "Destination file already exists and overwrite is not enabled";
                                        logger.ERROR() << errorMessage << std::endl;
                                        errorScope = DESTINATION;
                                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                                        errorPhase = TRANSFER_PREPARATION;
                                        retry = false;
                                        goto stop;
                                    }
                            }
                        else
                            {
                                g_clear_error(&tmp_err); //don't do anything
                            }
                    }

                unsigned int experimentalTimeout = adjustTimeoutBasedOnSize(statbufsrc.st_size, opts.timeout);
                if(!opts.manualConfig || opts.autoTunned || opts.timeout==0)
                    opts.timeout = experimentalTimeout;
                gfalt_set_timeout(params, opts.timeout, NULL);
                timeout_to_string = to_string<unsigned int>(opts.timeout, std::dec);
                msg_ifce::getInstance()->set_transfer_timeout(&tr_completed, timeout_to_string.c_str());
                logger.INFO() << "Timeout:" << opts.timeout << std::endl;
                globalTimeout = experimentalTimeout + 500;
                logger.INFO() << "Resetting global timeout thread to " << globalTimeout << " seconds" << std::endl;

                unsigned int experimentalNstreams = adjustStreamsBasedOnSize(statbufsrc.st_size, opts.nStreams);
                if(!opts.manualConfig || opts.autoTunned || opts.nStreams==0)
                    opts.nStreams = experimentalNstreams;
                gfalt_set_nbstreams(params, opts.nStreams, NULL);
                nstream_to_string = to_string<unsigned int>(opts.nStreams, std::dec);
                msg_ifce::getInstance()->set_number_of_streams(&tr_completed, nstream_to_string.c_str());
                logger.INFO() << "nbstreams:" << opts.nStreams << std::endl;

                //update protocol stuff
                logger.INFO() << "Update protocol stuff, report back to the server" << std::endl;
                reporter.timeout = opts.timeout;
                reporter.nostreams = opts.nStreams;
                reporter.buffersize = opts.tcpBuffersize;
                reporter.sendMessage(throughput, false, opts.jobId, strArray[0], "UPDATE", "", diff, source_size);

                gfalt_set_tcp_buffer_size(params, opts.tcpBuffersize, NULL);
                gfalt_set_monitor_callback(params, &call_perf, NULL);

                //check all params before passed to gfal2
                if ((strArray[1]).c_str() == NULL || (strArray[2]).c_str() == NULL)
                    {
                        errorMessage = "Failed to get source or dest surl";
                        logger.ERROR() << errorMessage << std::endl;
                        errorScope = TRANSFER;
                        reasonClass = GENERAL_FAILURE;
                        errorPhase = TRANSFER;
                        goto stop;
                    }


                logger.INFO() << "Transfer Starting" << std::endl;
                if ((ret = gfalt_copy_file(handle, params, (strArray[1]).c_str(), (strArray[2]).c_str(), &tmp_err)) != 0)
                    {
                        if (tmp_err != NULL && tmp_err->message != NULL)
                            {
                                logger.ERROR() << "Transfer failed - errno: " << tmp_err->code
                                               << " Error message:" << tmp_err->message
                                               << std::endl;
                                if (tmp_err->code == 110)
                                    {
                                        errorMessage = std::string(tmp_err->message);
                                        errorMessage += ", operation timeout";
                                    }
                                else
                                    {
                                        errorMessage = std::string(tmp_err->message);
                                    }
                                errorScope = TRANSFER;
                                reasonClass = mapErrnoToString(tmp_err->code);
                                errorPhase = TRANSFER;
                            }
                        else
                            {
                                logger.ERROR() << "Transfer failed - Error message: Unresolved error" << std::endl;
                                errorMessage = std::string("Unresolved error");
                                errorScope = TRANSFER;
                                reasonClass = GENERAL_FAILURE;
                                errorPhase = TRANSFER;
                            }
                        if(tmp_err)
                            {
                                retry = retryTransfer(tmp_err->code, "TRANSFER" );
                                g_clear_error(&tmp_err);
                            }
                        g_clear_error(&tmp_err);
                        goto stop;
                    }
                else
                    {
                        logger.INFO() << "Transfer completed successfully" << std::endl;
                    }


                transferred_bytes = (double) source_size;
                bytes_to_string = to_string<double>(transferred_bytes, std::dec);
                msg_ifce::getInstance()->set_total_bytes_transfered(&tr_completed, bytes_to_string.c_str());

                logger.INFO() << "Stat the dest surl start" << std::endl;
                for (int destStatRetry = 0; destStatRetry < 4; destStatRetry++)
                    {
                        errorMessage = ""; //reset
                        if (gfal2_stat(handle, (strArray[2]).c_str(), &statbufdest, &tmp_err) < 0)
                            {
                                std::string tempError(tmp_err->message);
                                logger.ERROR() << "Failed to get dest file size, errno:" << tmp_err->code << ", "
                                               << tempError
                                               << std::endl;
                                errorMessage = "Failed to get dest file size: " + tempError;
                                errorScope = DESTINATION;
                                reasonClass = mapErrnoToString(tmp_err->code);
                                errorPhase = TRANSFER_FINALIZATION;
                                retry = retryTransfer(tmp_err->code, "DESTINATION" );
                                if (destStatRetry == 3 || false == retry)
                                    {
                                        logger.INFO() << "No more retry stating the destination" << std::endl;
                                        g_clear_error(&tmp_err);
                                        goto stop;
                                    }
                                g_clear_error(&tmp_err);
                            }
                        else
                            {
                                if (statbufdest.st_size <= 0)
                                    {
                                        errorMessage = "Destination file size is 0";
                                        logger.ERROR() << errorMessage << std::endl;
                                        errorScope = DESTINATION;
                                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                                        errorPhase = TRANSFER_FINALIZATION;
                                        if (destStatRetry == 3)
                                            {
                                                logger.INFO() << "No more retry stating the destination" << std::endl;
                                                retry = true;
                                                goto stop;
                                            }
                                    }
                                else if (strArray[4]!= "x" && boost::lexical_cast<double>(strArray[4]) != 0 && boost::lexical_cast<double>(strArray[4]) != statbufdest.st_size)
                                    {
                                        std::stringstream error_;
                                        error_ << "User specified destination file size is " << strArray[4] << " but stat returned " << statbufdest.st_size;
                                        errorMessage = error_.str();
                                        logger.ERROR() << errorMessage << std::endl;
                                        errorScope = DESTINATION;
                                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                                        errorPhase = TRANSFER_FINALIZATION;
                                        if (destStatRetry == 3)
                                            {
                                                retry = true;
                                                logger.INFO() << "No more retry stating the destination" << std::endl;
                                                goto stop;
                                            }
                                    }
                                else
                                    {
                                        logger.INFO() << "Destination file size: " << statbufdest.st_size << std::endl;
                                        dest_size = (double) statbufdest.st_size;
                                        break;
                                    }
                            }
                        logger.WARNING() << "Stat the destination will be retried" << std::endl;
                        sleep(3); //give it some time to breath
                    }

                //check source and dest file sizes
                if (source_size == dest_size)
                    {
                        logger.INFO() << "Source and destination file size matching" << std::endl;
                    }
                else
                    {
                        logger.ERROR() << "Source and destination file size are different" << std::endl;
                        errorMessage = "Source and destination file size mismatch";
                        errorScope = DESTINATION;
                        reasonClass = mapErrnoToString(gfal_posix_code_error());
                        errorPhase = TRANSFER_FINALIZATION;
                        goto stop;
                    }

                gfalt_set_user_data(params, NULL, NULL);
            }//logStream
stop:
            diff = transferDuration(transfer_start, transfer_complete);
            msg_ifce::getInstance()->set_transfer_error_scope(&tr_completed, errorScope);
            msg_ifce::getInstance()->set_transfer_error_category(&tr_completed, reasonClass);
            msg_ifce::getInstance()->set_failure_phase(&tr_completed, errorPhase);
            msg_ifce::getInstance()->set_transfer_error_message(&tr_completed, errorMessage);
            if (errorMessage.length() > 0)
                {
                    msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "Error");
                    reporter.timeout = opts.timeout;
                    reporter.nostreams = opts.nStreams;
                    reporter.buffersize = opts.tcpBuffersize;
                    if (!terminalState)
                        {
                            logger.INFO() << "Report FAILED back to the server" << std::endl;
                            reporter.sendTerminal(throughput, retry, opts.jobId, strArray[0], "FAILED", errorMessage, diff, source_size);
                        }
                }
            else
                {
                    msg_ifce::getInstance()->set_final_transfer_state(&tr_completed, "Ok");
                    reporter.timeout = opts.timeout;
                    reporter.nostreams = opts.nStreams;
                    reporter.buffersize = opts.tcpBuffersize;
                    logger.INFO() << "Report FINISHED back to the server" << std::endl;
                    reporter.sendTerminal(throughput, false, opts.jobId, strArray[0], "FINISHED", errorMessage, diff, source_size);
                    /*unpin the file here and report the result in the log file...*/
                    g_clear_error(&tmp_err);

                    if (opts.bringOnline > 0)
                        {
                            logger.INFO() << "Token will be unpinned: " << strArray[6] << std::endl;
                            if(gfal2_release_file(handle, (strArray[1]).c_str(), (strArray[6]).c_str(), &tmp_err) < 0)
                                {
                                    if (tmp_err && tmp_err->message)
                                        {
                                            logger.WARNING() << "Failed unpinning the file: " << std::string(tmp_err->message) << std::endl;
                                        }
                                }
                            else
                                {
                                    logger.INFO() << "Token unpinned: " << strArray[6] << std::endl;
                                }
                        }
                }
            logger.INFO() << "Send monitoring complete message" << std::endl;
            msg_ifce::getInstance()->set_tr_timestamp_complete(&tr_completed, msg_ifce::getInstance()->getTimestamp());

            if(opts.monitoringMessages)
                msg_ifce::getInstance()->SendTransferFinishMessage(&tr_completed);

            std::string archiveErr = fileManagement->archive();
            if (!archiveErr.empty())
                logger.ERROR() << "Could not archive: " << archiveErr << std::endl;
            reporter.sendLog(opts.jobId, strArray[0], fileManagement->_getLogArchivedFileFullPath(),
                             opts.debug);
        }//end for reuse loop

    if (params)
        {
            gfalt_params_handle_delete(params, NULL);
            params = NULL;
        }
    if (handle)
        {
            gfal_context_free(handle);
            handle = NULL;
        }

    if (cert)
        {
            delete cert;
            cert = NULL;
        }

    if (opts.reuseFile && readFile.length() > 0)
        unlink(readFile.c_str());


    if (fileManagement)
        delete fileManagement;


    StaticSslLocking::kill_locks();

    return EXIT_SUCCESS;
}
