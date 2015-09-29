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

#include <boost/filesystem.hpp>

#include "common/Exceptions.h"
#include "common/Logger.h"
#include "common/name_to_uid.h"
#include "common/panic.h"
#include "config/serverconfig.h"
#include "common/ThreadPool.h"


#include "task/Gfal2Task.h"
#include "fetch/FetchStaging.h"
#include "fetch/FetchCancelStaging.h"
#include "fetch/FetchDeletion.h"
#include "state/StagingStateUpdater.h"
#include "state/DeletionStateUpdater.h"
#include "server/DrainMode.h"


using namespace fts3::common;
using namespace fts3::config;
namespace fs = boost::filesystem;


extern std::string stackTrace;
bool stopThreads = false;
const char *hostcert = "/etc/grid-security/fts3hostcert.pem";
const char *hostkey = "/etc/grid-security/fts3hostkey.pem";
const char *configfile = "/etc/fts3/fts3config";


int fts3_teardown_db_backend()
{
    try
        {
            db::DBSingleton::destroy();
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Unexpected exception when forcing the database teardown" << commit;
            return -1;
        }
    return 0;
}

void shutdown_callback(int signum, void*)
{
    StagingStateUpdater::instance().recover();
    DeletionStateUpdater::instance().recover();


    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Caught signal " << signum
                                    << " (" << strsignal(signum) << ")" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Future signals will be ignored!" << commit;

    stopThreads = true;

    // Some require traceback
    switch (signum)
        {
        case SIGABRT:
        case SIGSEGV:
        case SIGTERM:
        case SIGILL:
        case SIGFPE:
        case SIGBUS:
        case SIGTRAP:
        case SIGSYS:
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Stack trace: \n" << panic::stack_dump(panic::stack_backtrace, panic::stack_backtrace_size) << commit;
            break;
        default:
            break;
        }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon stopping" << commit;
    sleep(5);
    int db_status = fts3_teardown_db_backend();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon stopped" << commit;
    // Handle termination for signals that do not imply errors
    // Signals that do imply an error (i.e. SIGSEGV) will trigger a coredump in panic.c
    switch (signum)
    {
        case SIGINT: case SIGTERM: case SIGUSR1:
            _exit(-signum);
    }
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
            db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, 8);
        }
    catch (BaseException& e)
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
                    if (fts3::server::DrainMode::instance())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                            sleep(15);
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
                    sleep(2);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                }
            catch (...)
                {
                    sleep(2);
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
            setenv("GLOBUS_THREAD_MODEL", "pthread", 1);

            panic::setup_signal_handlers(shutdown_callback, NULL);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE signal handlers installed" << commit;

            //re-read here
            theServerConfig().read(argc, argv, true);
            bool isDaemon = !theServerConfig().get<bool> ("no-daemon");

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
                    if (foundHelp != std::string::npos)
                        {
                            return -1;
                        }
                }

            std::string logDir = theServerConfig().get<std::string > ("ServerLogDirectory");
            if (isDaemon && logDir.length() > 0)
                {
                    logDir += "/fts3bringonline.log";
                    if (theLogger().redirect(logDir, logDir) != 0)
                        {
                            std::cerr << "BRINGONLINE  daemon failed to open log file, errno is:" << strerror(errno) << std::endl;
                            return -1;
                        }
                }

            /*set infosys to gfal2*/
            infosys = theServerConfig().get<std::string > ("Infosys");

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE starting daemon..." << commit;

            try
                {
                    fts3_initialize_db_backend();
                }
            catch (BaseException& e)
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
            fts3::ProfilingSubsystem::instance().start();

            boost::thread hbThread(heartbeat);

            Gfal2Task::createPrototype(infosys);

            fts3::common::ThreadPool<Gfal2Task> threadpool(10);
            FetchStaging fs(threadpool);
            FetchCancelStaging fcs(threadpool);
            FetchDeletion fd(threadpool);

            boost::thread_group gr;
            gr.create_thread(boost::bind(&FetchStaging::fetch, fs));
            gr.create_thread(boost::bind(&FetchCancelStaging::fetch, fcs));
            gr.create_thread(boost::bind(&FetchDeletion::fetch, fd));
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon started..." << commit;
            gr.join_all();
        }
    catch (BaseException& e)
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
    if (!fs::exists(hostcert))
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE ERROR check if hostcert/key are installed" << commit;
            return EXIT_FAILURE;
        }


    if (!fs::exists(configfile))
        {
            std::cerr << "BRINGONLINE ERROR config file " << configfile << " doesn't exist" << std::endl;
            return EXIT_FAILURE;
        }


    //very first check before it goes to deamon mode
    try
        {
            theServerConfig().read(argc, argv, true);

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
                    if (found != std::string::npos)
                        {
                            {
                                DoServer(argc, argv);
                            }
                            return EXIT_SUCCESS;
                        }
                    else if (foundHelp != std::string::npos)
                        {
                            {
                                DoServer(argc, argv);
                            }
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
    catch (BaseException& e)
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

