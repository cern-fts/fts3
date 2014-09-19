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
#include "common/error.h"
#include "common/logger.h"
#include "common/ThreadPool.h"
#include "common/panic.h"
#include "config/serverconfig.h"
#include "server.h"

#include "task/Gfal2Task.h"
#include "fetch/FetchStaging.h"
#include "fetch/FetchCancelStaging.h"
#include "fetch/FetchDeletion.h"
#include "state/StagingStateUpdater.h"
#include "state/DeletionStateUpdater.h"

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

void shutdown_callback(int signal, void*)
{
    StagingStateUpdater::instance().recover();
    DeletionStateUpdater::instance().recover();


    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Caught signal " << signal
                                    << " (" << strsignal(signal) << ")" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Future signals will be ignored!" << commit;

    stopThreads = true;

    // Some require traceback
    switch (signal)
        {
        case SIGABRT:
        case SIGSEGV:
        case SIGTERM:
        case SIGILL:
        case SIGFPE:
        case SIGBUS:
        case SIGTRAP:
        case SIGSYS:
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Stack trace: \n" << Panic::stack_dump(Panic::stack_backtrace, Panic::stack_backtrace_size) << commit;
            break;
        default:
            break;
        }

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
            db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, 8);
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
            setenv("GLOBUS_THREAD_MODEL", "pthread", 1);

            Panic::setup_signal_handlers(shutdown_callback, NULL);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE signal handlers installed" << commit;

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
                    if (FTS3_COMMON_NAMESPACE::theLogger().open(logDir) != 0)
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

