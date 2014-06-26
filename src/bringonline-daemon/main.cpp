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

#include "server_dev.h"
#include <list>
#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "common/error.h"
#include "common/logger.h"
#include "config/serverconfig.h"
#include "db/generic/SingleDbInstance.h"
#include "ws/delegation/GSoapDelegationHandler.h"
#include <fstream>
#include "server.h"
#include "daemonize.h"
#include "signal_logger.h"
#include "StaticSslLocking.h"
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include "queue_updater.h"
#include <boost/filesystem.hpp>
#include "name_to_uid.h"
#include <sys/resource.h>
#include "queue_bringonline.h"
#include "UserProxyEnv.h"
#include "DelegCred.h"
#include "CredService.h"
#include <gfal_api.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "name_to_uid.h"
#include "cred-utility.h"
#include "name_to_uid.h"
#include "DrainMode.h"

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

extern std::string stackTrace;
bool stopThreads = false;
const char *hostcert = "/etc/grid-security/fts3hostcert.pem";
const char *hostkey = "/etc/grid-security/fts3hostkey.pem";
const char *configfile = "/etc/fts3/fts3config";

// exp backoff for bringonline ops
static time_t getPollInterval(int nPolls)
{
    if (nPolls > 5)
        return 180;
    else
        return (2 << nPolls);
}

bool retryTransfer(int errorNo, const std::string& category, const std::string& message)
{
    bool retry = true;

    // ETIMEDOUT shortcuts the following
    if (errorNo == ETIMEDOUT)
        return true;

    //search for error patterns not reported by SRM or GSIFTP
    std::size_t found = message.find("performance marker");
    if (found!=std::string::npos)
        return true;
    found = message.find("Connection timed out");
    if (found!=std::string::npos)
        return true;
    found = message.find("end-of-file was reached");
    if (found!=std::string::npos)
        return true;



    if (category == "SOURCE")
        {
            switch (errorNo)
                {
                case ENOENT:          // No such file or directory
                case EPERM:           // Operation not permitted
                case EACCES:          // Permission denied
                case EISDIR:          // Is a directory
                case ENAMETOOLONG:    // File name too long
                case E2BIG:           // Argument list too long
                case ENOTDIR:         // Part of the path is not a directory
                case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
                case EINVAL:          // Invalid argument. i.e: invalid request token
                    retry = false;
                    break;
                default:
                    retry = true;
                    break;
                }
        }

    found = message.find("proxy expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("File exists and overwrite");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("No such file or directory");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_INVALID_PATH");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("The certificate has expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("The available CRL has expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM Authentication failed");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_DUPLICATION_ERROR");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_AUTHENTICATION_FAILURE");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_NO_FREE_SPACE");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("digest too big for rsa key");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("Can not determine address of local host");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("Permission denied");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("System error in write into HDFS");
    if (found!=std::string::npos)
        retry = false;

    return retry;
}



static int fexists(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

static bool isSrmUrl(const std::string & url)
{
    if (url.compare(0, 6, "srm://") == 0)
        return true;

    return false;
}

int fts3_teardown_db_backend()
{
    try
        {
            db::DBSingleton::tearDown();
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Unexpected exception when forcing the database teardown" << commit;
            return -1;
        }
    return 0;
}

void _handle_sigint(int)
{
    if (stackTrace.length() > 0)
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << stackTrace << commit;
    stopThreads = true;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon stopping" << commit;
    sleep(5);
    int db_status = fts3_teardown_db_backend();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon stopped" << commit;
    _exit(db_status);
}

/* -------------------------------------------------------------------------- */

void fts3_initialize_db_backend()
{
    std::string dbUserName = theServerConfig().get<std::string > ("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string > ("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string > ("DbConnectString");

    try
        {
            //use 4 hardcoded connection
            db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, 4);
        }
    catch (Err& e)
        {
            throw;
        }
    catch (std::exception& ex)
        {
            throw;
        }
    catch (...)
        {
            throw;
        }
}

static std::string generateProxy(const std::string& dn, const std::string& dlg_id)
{
    boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
    return delegCredPtr->getFileName(dn, dlg_id);
}


static bool checkValidProxy(const std::string& filename, std::string& message)
{
    boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
    return delegCredPtr->isValidProxy(filename, message);
}

void issueBringOnLineStatus(gfal2_context_t handle, std::string infosys)
{
    char token[512] = {0};
    int statusA = 0;
    int statusB = 0;
    GError *error = NULL;
    const char *protocols[] = {"rfio", "gsidcap", "dcap", "gsiftp"};
    UserProxyEnv* cert = NULL;
    long int pinlifetime = 28800;
    long int bringonlineTimeout = 28800;

    gfal2_set_opt_string_list(handle, "SRM PLUGIN", "TURL_PROTOCOLS", protocols, 4, &error);
    if (error)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Could not set the protocol list " << error->code << " " << error->message << commit;
            return;
        }

    if (infosys.compare("false") == 0)
        {
            gfal2_set_opt_boolean(handle, "BDII", "ENABLED", false, NULL);
        }
    else
        {
            gfal2_set_opt_string(handle, "BDII", "LCG_GFAL_INFOSYS", (char *) infosys.c_str(), NULL);
        }

    while (!stopThreads)
        {
            std::list<struct message_bringonline>::iterator i = ThreadSafeBringOnlineList::get_instance().m_list.begin();
            if (!ThreadSafeBringOnlineList::get_instance().m_list.empty())
                {
                    while (i != ThreadSafeBringOnlineList::get_instance().m_list.end() && !stopThreads)
                        {
                            cert = new UserProxyEnv((*i).proxy);
                            bool deleteIt = false;
                            time_t now = time(NULL);


                            //before any operation, check if the proxy is valid
                            std::string message;
                            bool isValid = checkValidProxy((*i).proxy, message);
                            if(!isValid)
                                {
                                    db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FAILED", message, (*i));
                                    ThreadSafeBringOnlineList::get_instance().m_list.erase(i++);
                                    continue;
                                }

                            if ((*i).started == false)   //issue bringonline
                                {
                                    if((*i).pinlifetime > pinlifetime)
                                        {
                                            pinlifetime = (*i).pinlifetime;
                                        }

                                    if((*i).bringonlineTimeout > bringonlineTimeout)
                                        {
                                            bringonlineTimeout = (*i).bringonlineTimeout;
                                        }

                                    statusA = gfal2_bring_online(handle, ((*i).url).c_str(), pinlifetime, bringonlineTimeout, token, sizeof (token), 1, &error);

                                    if (statusA < 0)
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE failed " << error->code << " " << error->message << commit;
                                            if(true == retryTransfer(error->code, "SOURCE", std::string(error->message)) && (*i).retries < 3 )
                                                {
                                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE will be retried" << commit;
                                                    (*i).retries +=1;
                                                }
                                            else
                                                {
                                                    db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FAILED", std::string(error->message), (*i));
                                                    (*i).started = true;
                                                    deleteIt = true;
                                                }
                                            g_clear_error(&error);
                                        }
                                    else if (statusA == 0)
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE queued, got token " << token << commit;
                                            (*i).token = std::string(token);
                                            db::DBSingleton::instance().getDBObjectInstance()->addToken((*i).job_id, (*i).file_id, std::string(token));
                                            (*i).started = true;
                                            (*i).retries = 0;
                                        }
                                    else
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE succeeded, got token " << token << commit;
                                            db::DBSingleton::instance().getDBObjectInstance()->addToken((*i).job_id, (*i).file_id, std::string(token));
                                            (*i).started = true;
                                            (*i).retries = 0;
                                            // No need to poll in this case!
                                            db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FINISHED", "", (*i));
                                            deleteIt = true;
                                        }
                                }
                            else if ((*i).nextPoll <= now)     //poll
                                {
                                    statusB = gfal2_bring_online_poll(handle, ((*i).url).c_str(), ((*i).token).c_str(), &error);

                                    if (statusB < 0)
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE polling failed, token " << (*i).token << ", "  << error->code << " " << error->message << commit;
                                            if(true == retryTransfer(error->code, "SOURCE", std::string(error->message)) && (*i).retries < 3 )
                                                {
                                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE will be retried" << commit;
                                                    (*i).retries +=1;
                                                    (*i).started = false;
                                                }
                                            else
                                                {
                                                    db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FAILED", std::string(error->message), (*i));
                                                    (*i).started = true;
                                                    deleteIt = true;
                                                }
                                            g_clear_error(&error);
                                        }
                                    else if(statusB == 0)
                                        {
                                            time_t interval = getPollInterval(++(*i).nPolls);
                                            (*i).nextPoll = now + interval;

                                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE polling token " << (*i).token << commit;
                                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE next attempt in "
                                                                            << interval << " seconds" << commit;
                                            (*i).started = true;
                                        }
                                    else
                                        {
                                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE finished token " << (*i).token << commit;
                                            (*i).started = true;
                                            deleteIt = true;
                                            db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FINISHED", "", (*i));
                                        }
                                }

                            if (deleteIt)
                                {
                                    ThreadSafeBringOnlineList::get_instance().m_list.erase(i++);
                                }
                            else
                                {
                                    ++i;
                                }

                            if (cert)
                                {
                                    delete cert;
                                    cert = NULL;
                                }
                        }
                }
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }
}



void heartbeat(void)
{
    unsigned myIndex=0, count=0;
    unsigned hashStart=0, hashEnd=0;
    std::string service_name = "fts_bringonline";

    while (!stopThreads)
        {
            try
                {
                    //check if draining is on
                    if (DrainMode::getInstance())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                            sleep(5);
                            continue;
                        }

                    db::DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(
                        &myIndex, &count, &hashStart, &hashEnd, service_name);

                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Systole: host " << myIndex << " out of " << count
                                                    << " [" << std::hex << hashStart << ':' << std::hex << hashEnd << ']'
                                                    << std::dec
                                                    << commit;

                    boost::this_thread::sleep(boost::posix_time::seconds(60));
                }
            catch (std::exception& ex)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                }
            catch (...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unhandled exception" << commit;
                }
        }
}



int DoServer(int argc, char** argv)
{
    std::string proxy_file("");
    std::string infosys("");
    gfal2_context_t handle;

    try
        {
            REGISTER_SIGNAL(SIGABRT);
            REGISTER_SIGNAL(SIGSEGV);
            REGISTER_SIGNAL(SIGTERM);
            REGISTER_SIGNAL(SIGILL);
            REGISTER_SIGNAL(SIGFPE);
            REGISTER_SIGNAL(SIGBUS);
            REGISTER_SIGNAL(SIGTRAP);
            REGISTER_SIGNAL(SIGSYS);

            // Set X509_ environment variables properly
            setenv("X509_USER_CERT", hostcert, 1);
            setenv("X509_USER_KEY", hostkey, 1);

            //re-read here
            FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv, true);

            std::string arguments("");
            if (argc > 1)
                {
                    int i;
                    for (i = 1; i < argc; i++)
                        {
                            arguments += argv[i];
                        }
                    // Should never happen, actually
                    size_t foundHelp = arguments.find("-h");
                    if (foundHelp != string::npos)
                        {
                            return -1;
                        }
                }

            std::string logDir = theServerConfig().get<std::string > ("TransferLogDirectory");
            if (logDir.length() > 0)
                {
                    logDir += "/fts3bringonline.log";
                    int filedesc = open(logDir.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
                    if (filedesc != -1)   //if ok
                        {
                            close(filedesc);
                            FILE* freopenLogFile = freopen(logDir.c_str(), "a", stderr);
                            if (freopenLogFile == NULL)
                                {
                                    std::cerr << "BRINGONLINE  daemon failed to open log file, errno is:" << strerror(errno) << std::endl;
                                    return -1;
                                }
                        }
                    else
                        {
                            std::cerr << "BRINGONLINE  daemon failed to open log file, errno is:" << strerror(errno) << std::endl;
                            return -1;
                        }
                }

            bool isDaemon = !FTS3_CONFIG_NAMESPACE::theServerConfig().get<bool> ("no-daemon");

            if (isDaemon)
                {
                    FILE* openlog = freopen(logDir.c_str(), "a", stderr);
                    if (openlog == NULL)
                        std::cerr << "BRINGONLINE  Can't open log file" << std::endl;
                }

            /*set infosys to gfal2*/
            infosys = theServerConfig().get<std::string > ("Infosys");

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE starting daemon..." << commit;
            struct sigaction action;
            action.sa_handler = _handle_sigint;
            sigemptyset(&action.sa_mask);
            action.sa_flags = SA_RESTART;
            sigaction(SIGINT, &action, NULL);

            try
                {
                    // Set up handle
                    GError *error = NULL;
                    handle = gfal2_context_new(&error);
                    if (!handle)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE bad initialization " << error->code << " " << error->message << commit;
                            return -1;
                        }
                    boost::thread bt2(issueBringOnLineStatus, handle, infosys);
                }
            catch (std::exception& e)
                {
                    throw;
                }
            catch (...)
                {
                    throw;
                }

            try
                {
                    fts3_initialize_db_backend();
                }
            catch (Err& e)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
                    return -1;
                }
            catch (...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin), exiting..." << commit;
                    return -1;
                }

            // Start profiling
            ProfilingSubsystem::getInstance().start();

            boost::thread hbThread(heartbeat);

            vector< boost::tuple<string, string, int> >::iterator it;
            std::vector< boost::tuple<std::string, std::string, int> > voHostnameConfig;
            std::vector<struct message_bringonline> urls;
            vector<struct message_bringonline>::iterator itUrls;

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon started..." << commit;
            while (!stopThreads)
                {
                    //if we drain a host, no need to check if url_copy are reporting being alive
                    if (DrainMode::getInstance())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                            sleep(5);
                            continue;
                        }


                    //select from the database the config for bringonline for each VO / hostname
                    voHostnameConfig = db::DBSingleton::instance().getDBObjectInstance()->getVOBringonlineMax();

                    if (!voHostnameConfig.empty())
                        {
                            for (it = voHostnameConfig.begin(); it != voHostnameConfig.end(); ++it)
                                {
                                    string voName = get < 0 > (*it);
                                    string hostName = get < 1 > (*it);
                                    int maxValue = get < 2 > (*it);
                                    //get the files to be brought online for this VO/host/max
                                    urls = db::DBSingleton::instance().getDBObjectInstance()->getBringOnlineFiles(voName, hostName, maxValue);
                                }
                        }
                    else    //no config for any vo and/or se, hardcode 100 files at any given time for each hostname
                        {
                            urls = db::DBSingleton::instance().getDBObjectInstance()->getBringOnlineFiles("", "", 500);
                        }

                    if (!urls.empty())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE " << urls.size() << " are ready for bringonline"  << commit;
                            for (itUrls = urls.begin(); itUrls != urls.end(); ++itUrls)
                                {
                                    if (true == isSrmUrl((*itUrls).url))
                                        {
                                            std::string dn;
                                            std::string dlg_id;
                                            std::string vo_name;
                                            db::DBSingleton::instance().getDBObjectInstance()->getCredentials(vo_name, (*itUrls).job_id, (*itUrls).file_id, dn, dlg_id);

                                            //get the proxy
                                            proxy_file = generateProxy(dn, dlg_id);
                                            (*itUrls).proxy = proxy_file;

                                            std::string message;
                                            if(false == checkValidProxy(proxy_file, message))
                                                {
                                                    proxy_file = get_proxy_cert(
                                                                     dn, // user_dn
                                                                     dlg_id, // user_cred
                                                                     vo_name, // vo_name
                                                                     "",
                                                                     "", // assoc_service
                                                                     "", // assoc_service_type
                                                                     false,
                                                                     ""
                                                                 );
                                                }

                                            ThreadSafeBringOnlineList::get_instance().push_back(*itUrls);
                                        }
                                }
                        }

                    urls.clear();
                    voHostnameConfig.clear();
                    sleep(1);
                }
            gfal2_context_free(handle);
        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
            return -1;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin), exiting..." << commit;
            return -1;
        }
    stopThreads = true;
    return EXIT_SUCCESS;
}


__attribute__((constructor)) void begin(void)
{
    //switch to non-priviledged user to avoid reading the hostcert
    uid_t pw_uid;
    pw_uid = name_to_uid();
    setuid(pw_uid);
    seteuid(pw_uid);
}
int main(int argc, char** argv)
{
    if (fexists(hostcert) != 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE ERROR check if hostcert/key are installed" << commit;
            return EXIT_FAILURE;
        }


    if (fexists(configfile) != 0)
        {
            std::cerr << "BRINGONLINE ERROR config file " << configfile << " doesn't exist" << std::endl;
            return EXIT_FAILURE;
        }


    //very first check before it goes to deamon mode
    try
        {
            FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv, true);

            std::string arguments("");
            int d = 0;
            if (argc > 1)
                {
                    int i;
                    for (i = 1; i < argc; i++)
                        {
                            arguments += argv[i];
                        }
                    size_t found = arguments.find("-n");
                    size_t foundHelp = arguments.find("-h");
                    if (found != string::npos)
                        {
                            {
                                DoServer(argc, argv);
                            }
                            pthread_exit(0);
                            return EXIT_SUCCESS;
                        }
                    else if (foundHelp != string::npos)
                        {
                            {
                                DoServer(argc, argv);
                            }
                            pthread_exit(0);
                            return EXIT_SUCCESS;
                        }
                    else
                        {
                            d = daemon(0, 0);
                            if (d < 0)
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Can't set daemon, will continue attached to tty" << commit;
                        }
                }
            else
                {
                    d = daemon(0, 0);
                    if (d < 0)
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Can't set daemon, will continue attached to tty" << commit;
                }
            DoServer(argc, argv);
        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
            return EXIT_FAILURE;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin), exiting..." << commit;
            return EXIT_FAILURE;
        }

    return 0;
}

