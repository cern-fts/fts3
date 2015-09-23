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

/** \file main.cpp FTS3 server entry point. */

#include <cstdio>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "common/error.h"
#include "config/serverconfig.h"
#include "db/generic/SingleDbInstance.h"
#include "ws/delegation/GSoapDelegationHandler.h"
#include <fstream>
#include "Server.h"
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include <boost/filesystem.hpp>
#include "common/name_to_uid.h"
#include <sys/resource.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include "profiler/Profiler.h"
#include <fstream>
#include "common/panic.h"
#include <execinfo.h>
#include "common/Logger.h"
#include "common/ThreadSafeList.h"


namespace fs = boost::filesystem;
using boost::thread;
using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::server; 

extern std::string stackTrace;
extern bool stopThreads;
const char *hostcert = "/etc/grid-security/fts3hostcert.pem";
const char *hostkey = "/etc/grid-security/fts3hostkey.pem";
const char *configfile = "/etc/fts3/fts3config";

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */


int proc_find()
{
    DIR* dir=NULL;
    struct dirent* ent=NULL;
    char* endptr=NULL;
    char buf[512]= {0};
    unsigned count = 0;
    const char* name = "fts_server";

    if (!(dir = opendir("/proc")))
        {
            return -1;
        }


    while((ent = readdir(dir)) != NULL)
        {
            /* if endptr is not a null character, the directory is not
             * entirely numeric, so ignore it */
            long lpid = strtol(ent->d_name, &endptr, 10);
            if (*endptr != '\0')
                {
                    continue;
                }

            /* try to open the cmdline file */
            snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
            FILE* fp = fopen(buf, "r");

            if (fp)
                {
                    if (fgets(buf, sizeof(buf), fp) != NULL)
                        {
                            /* check the first token in the file, the program name */
                            char* first = NULL;
                            first = strtok(buf, " ");

                            if (first && strstr(first, name))
                                {
                                    fclose(fp);
                                    fp = NULL;
                                    ++count;
                                    continue;
                                }
                        }
                    if(fp)
                        fclose(fp);
                }

        }
    closedir(dir);
    return count;
}


static int fexists(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

/* -------------------------------------------------------------------------- */

void fts3_initialize_db_backend(bool test)
{
    std::string dbUserName = theServerConfig().get<std::string > ("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string > ("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string > ("DbConnectString");
    int pooledConn = theServerConfig().get<int> ("DbThreadsNum");
    if(test)
        pooledConn = 2;

    try
        {
            db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, pooledConn);
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

static bool checkUrlCopy()
{
    std::string p("");
    std::vector<std::string> pathV;
    std::vector<std::string>::iterator iter;
    char *token=NULL;
    const char *path = getenv("PATH");
    if(path)
        {
            char *copy = (char *) malloc(strlen(path) + 1);
            strcpy(copy, path);
            token = strtok(copy, ":");
            if(token)
                pathV.push_back(std::string(token));
            while ((token = strtok(0, ":")) != NULL)
                {
                    pathV.push_back(std::string(token));
                }

            for (iter = pathV.begin(); iter < pathV.end(); ++iter)
                {
                    p = *iter + "/fts_url_copy";
                    if (fexists(p.c_str()) == 0)
                        {
                            free(copy);
                            copy = NULL;
                            pathV.clear();
                            return true;
                        }
                }

            free(copy);
        }
    return false;
}

static std::string requiredToString(int mode)
{
    char strMode[] = "---";

    if (mode & R_OK)
        strMode[0] = 'r';
    if (mode & W_OK)
        strMode[1] = 'w';
    if (mode & X_OK)
        strMode[2] = 'x';

    return strMode;
}

static void isPathSane(const std::string& path,
                       bool isDir = true,
                       int requiredMode = R_OK | W_OK,
                       bool changeOwner = true)
{
    std::ostringstream msg;

    // If it does not exist, create
    if (!fs::exists(path))
        {
            if (isDir)
                {
                    if (fs::create_directory(path))
                        {
                            if (changeOwner)
                                {
                                    uid_t pw_uid = name_to_uid();
                                    int checkChown = chown(path.c_str(), pw_uid, getgid());
                                    if (checkChown != 0)
                                        {
                                            msg << "Failed to chown for " << path;
                                            throw Err_System(msg.str());
                                        }
                                    int checkmode = chmod (path.c_str(), 0755);
                                    if (checkmode != 0)
                                        {
                                            msg << "Failed to chmod for " << path;
                                            throw Err_System(msg.str());
                                        }
                                }
                        }
                    else
                        {
                            msg << "Directory " << path
                                << " does not exist and could not be created";
                            throw Err_System(msg.str());
                        }
                }
            else
                {
                    msg << "File " << path
                        << " does not exist";
                    throw Err_System(msg.str());
                }
        }
    // It does exist, but it is not the kind of file we want
    else if (isDir && !fs::is_directory(path))
        {
            msg << path
                << " exists but it is not a directory";
            throw Err_System(msg.str());
        }
    else if (!isDir && fs::is_directory(path))
        {
            msg << path
                << " exists but it is a directory";
            throw Err_System(msg.str());
        }

    // It exists, so check we have the right permissions
    if (access(path.c_str(), requiredMode) != 0)
        {
            msg << "Not enough permissions on " << path
                << " (Required " << requiredToString(requiredMode) << ")";
            throw Err_System(msg.str());
        }
}

void checkDbSchema()
{
    try
        {
            fts3_initialize_db_backend(true);

            db::DBSingleton::instance().getDBObjectInstance()->checkSchemaLoaded();

            db::DBSingleton::tearDown();

        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
            throw;
        }
    catch (std::exception& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
            throw;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something is going on the db schema, check if installed" << commit;
            throw;
        }
}



void checkInitDirs(std::string logsDir)
{
    try
        {
            isPathSane("/etc/fts3", true, R_OK, false);
            isPathSane(hostcert, false, R_OK);
            isPathSane(hostkey, false, R_OK);
            isPathSane(configfile, false, R_OK, true);
            isPathSane(logsDir);
            isPathSane("/var/lib/fts3");
            isPathSane("/var/lib/fts3/monitoring");
            isPathSane("/var/lib/fts3/status");
            isPathSane("/var/lib/fts3/stalled");
            isPathSane("/var/lib/fts3/logs");
        }
    catch (const fs::filesystem_error& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
            throw;
        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
            throw;
        }
    catch (std::exception& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
            throw;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something is going on with the filesystem required directories" << commit;
            throw;
        }
}

static void shutdown_callback(int signum, void*)
{
    int exit_status = 0;

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Caught signal " << signum
                                    << " (" << strsignal(signum) << ")" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Future signals will be ignored!" << commit;

    stopThreads = true;

    // Some require traceback
    switch (signum)
        {
        case SIGABRT:
        case SIGSEGV:
        case SIGILL:
        case SIGFPE:
        case SIGBUS:
        case SIGTRAP:
        case SIGSYS:
            exit_status = -signum;
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Stack trace: \n" << panic::stack_dump(panic::stack_backtrace, panic::stack_backtrace_size) << commit;
            break;
        default:
            break;
        }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopping" << commit;

    if (!theServerConfig().get<bool> ("rush"))
        sleep(15);

    try
        {
            Server::getInstance().stop();
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS db connections closing" << commit;
            db::DBSingleton::tearDown();
            if (!theServerConfig().get<bool> ("rush"))
                sleep(10);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS db connections closed" << commit;
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception when forcing the database teardown" << commit;
        }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopped" << commit;
    // Let the default handler do the rest (coredump, exit, whatever is neccessary)
}


int DoServer(int argc, char** argv)
{
    setenv("GLOBUS_THREAD_MODEL","pthread",1); //reset it
    // Register signal handlers
    panic::setup_signal_handlers(shutdown_callback, NULL);
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Signal handlers installed" << commit;

    try
        {
            std::string arguments("");
            size_t foundHelp;
            if (argc > 1)
                {
                    int i;
                    for (i = 1; i < argc; i++)
                        {
                            arguments += argv[i];
                        }
                    foundHelp = arguments.find("-h");
                    if (foundHelp != std::string::npos)
                        {
                            exit(0);
                        }
                }

            //re-read here
            theServerConfig().read(argc, argv, true);
            bool isDaemon = !theServerConfig().get<bool> ("no-daemon");

            // Set X509_ environment variables properly - reset here is case the child crashes
            setenv("X509_USER_CERT", hostcert, 1);
            setenv("X509_USER_KEY", hostkey, 1);

            std::string logDir = theServerConfig().get<std::string > ("ServerLogDirectory");
            if (isDaemon && logDir.length() > 0)
                {
                    logDir += "/fts3server.log";
                    if (theLogger().redirect(logDir, logDir) != 0)
                        {
                            std::cerr << "fts3 server failed to open log file " << logDir << " error is:" << strerror(errno) << std::endl;
                            return -1;
                        }
                }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting server..." << commit;

            fts3_initialize_db_backend(false);

            //initialize queue updater here to avoid race conditions
            ThreadSafeList::get_instance();

            // Start profiling
            ProfilingSubsystem::getInstance().start();

            // Start server
            Server::getInstance().start();

        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
            exit(1);
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Fatal error (unknown origin), exiting..." << commit;
            exit(1);
        }
    return EXIT_SUCCESS;
}

/// Spawn the process that runs the server
/// Returns the child PID on success, -1 on failure
/// Does NOT return on the child process
void SpawnServer(int argc, char** argv)
{
    int resultExec = DoServer(argc, argv);
    if (resultExec < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't start the server" << commit;
            exit(1);
        }
}

int main(int argc, char** argv)
{
    // Do not even try if already running
    int n_running = proc_find();
    if (n_running < 0)
        {
            std::cerr << "Could not check if FTS3 is already running" << std::endl;
            return EXIT_FAILURE;
        }
    else if (n_running > 1)
        {
            std::cerr << "Only one instance of FTS3 can run at the time" << std::endl;
            return EXIT_FAILURE;
        }

    //switch to non-priviledged user to avoid reading the hostcert
    uid_t pw_uid = name_to_uid();
    setuid(pw_uid);
    seteuid(pw_uid);

    //check if user is set to fts3
    char *buf = {0};
    buf=(char *)malloc(10*sizeof(char));
    cuserid(buf);
    std::string user(buf);
    if(user.empty() || user != "fts3")
        {
            std::cerr << "user fts3 does not exist, create first" << std::endl;
            return EXIT_FAILURE;
        }

    //very first check before it goes to daemon mode
    try
        {
            if (fexists(configfile) != 0)
                {
                    std::cerr << "fts3 server config file " << configfile << " doesn't exist" << std::endl;
                    return EXIT_FAILURE;
                }

            if (false == checkUrlCopy())
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if fts_url_copy process is set in the PATH env variable" << commit;
                    return EXIT_FAILURE;
                }

            if (fexists(hostcert) != 0)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if hostcert/key are installed" << commit;
                    return EXIT_FAILURE;
                }

            theServerConfig().read(argc, argv, true);

            //check file/dir persmissions
	    std::string logDir = theServerConfig().get<std::string > ("ServerLogDirectory");
            checkInitDirs(logDir);

            //check if db schema is installed
            checkDbSchema();

            // Set X509_ environment variables properly
            setenv("X509_USER_CERT", hostcert, 1);
            setenv("X509_USER_KEY", hostkey, 1);
        }
    catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
            return EXIT_FAILURE;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Fatal error (unknown origin), exiting..." << commit;
            return EXIT_FAILURE;
        }

    std::string arguments("");
    size_t found;
    size_t foundHelp;
    int d = 0;
    if (argc > 1)
        {
            int i;
            for (i = 1; i < argc; i++)
                {
                    arguments += argv[i];
                }
            found = arguments.find("-n");
            foundHelp = arguments.find("-h");
            if (found != std::string::npos)
                {
                    {
                        DoServer(argc, argv);
                    }
                    pthread_exit(0);
                    return EXIT_SUCCESS;
                }
            else if (foundHelp != std::string::npos)
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
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't set daemon, will continue attached to tty"  << commit;
                }
        }
    else
        {
            d = daemon(0, 0);
            if (d < 0)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Can't set daemon"  << commit;
                    exit(1);
                }
        }

    SpawnServer(argc, argv);

    return 0;
}

