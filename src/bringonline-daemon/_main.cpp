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
#include "common/ThreadPool.h"
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

#include "StagingTask.h"
#include "BringOnlineTask.h"
#include "WaitingRoom.h"

#include <vector>
#include <string>

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

extern std::string stackTrace;
bool stopThreads = false;
const char *hostcert = "/etc/grid-security/fts3hostcert.pem";
const char *hostkey = "/etc/grid-security/fts3hostkey.pem";
const char *configfile = "/etc/fts3/fts3config";

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

            StagingTask::createPrototype(infosys);
            fts3::common::ThreadPool<StagingTask> threadpool(10);
            WaitingRoom::instance().attach(threadpool);

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

                    std::vector<StagingTask::context_type> files;
                    db::DBSingleton::instance().getDBObjectInstance()->getFilesForStaging(files);

                    std::vector<StagingTask::context_type>::iterator it;
                    for (it = files.begin(); it != files.end(); ++it)
                    {
                    	// make sure it is a srm SE
                    	std::string & url = boost::get<BringOnlineTask::url>(*it);
                    	if (!isSrmUrl(url)) continue;

                    	//get the proxy
                    	std::string & dn = boost::get<BringOnlineTask::dn>(*it);
                    	std::string & dlg_id = boost::get<BringOnlineTask::dlg_id>(*it);
                    	std::string & vo = boost::get<BringOnlineTask::vo>(*it);
    					proxy_file = generateProxy(dn, dlg_id);

    					std::string message;
                        if(!BringOnlineTask::checkValidProxy(proxy_file, message))
						{
							proxy_file = get_proxy_cert(
											 dn, // user_dn
											 dlg_id, // user_cred
											 vo, // vo_name
											 "",
											 "", // assoc_service
											 "", // assoc_service_type
											 false,
											 ""
										 );
						}

    					try
						{
							threadpool.start(new BringOnlineTask(*it, proxy_file));
						}
    					catch(Err_Custom const & ex)
    					{
    						FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
    					}
                    }

                    sleep(1);
                }
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

