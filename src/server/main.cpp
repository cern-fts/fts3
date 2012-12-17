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

/** \file main.cpp FTS3 server entry point. */

#include "server_dev.h"

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

#include "config/FileMonitor.h"

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

extern std::string stackTrace;
bool stopThreads = false;

/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
static int fexists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

void fts3_teardown_db_backend() {
    try {
        db::DBSingleton::tearDown();
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception when forcing the database teardown" << commit;
        exit(1);
    }
}

void _handle_sigint(int) {    
    if (stackTrace.length() > 0)
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << stackTrace << commit;
    stopThreads = true;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopping" << commit;
    sleep(4);
    theServer().stop();
    sleep(4);
    fts3_teardown_db_backend();
    StaticSslLocking::kill_locks();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopped" << commit;
    exit(0);
}

/* -------------------------------------------------------------------------- */

void fts3_initialize_db_backend() {
    std::string dbUserName = theServerConfig().get<std::string > ("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string > ("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string > ("DbConnectString");

    try {
        db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString);
    } catch (Err& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
        exit(1);
    } catch (std::exception& ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
        exit(1);
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something is going on with the database, check username/password/connstring" << commit;
        exit(1);
    }


}

static bool checkUrlCopy() {
    std::string p("");
    std::vector<std::string> pathV;
    std::vector<std::string>::iterator iter;
    char *token;
    const char *path = getenv("PATH");
    char *copy = (char *) malloc(strlen(path) + 1);
    strcpy(copy, path);
    token = strtok(copy, ":");

    while ((token = strtok(0, ":")) != NULL) {
        pathV.push_back(std::string(token));
    }

    for (iter = pathV.begin(); iter < pathV.end(); iter++) {
        p = *iter + "/fts_url_copy";
        if (fexists(p.c_str()) == 0) {
            free(copy);
            copy = NULL;
            pathV.clear();
            return true;
        }
    }

    free(copy);
    return false;
}

void myterminate() {
    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "myterminate() was called" << commit;
}

void myunexpected() {
    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "myunexpected() was called" << commit;
}

int DoServer(int argc, char** argv) {
    const char *hostcert = "/etc/grid-security/hostcert.pem";
    const char *configfile = "/etc/fts3/fts3config";
    int res = 0;

    try {
        REGISTER_SIGNAL(SIGABRT);
        REGISTER_SIGNAL(SIGSEGV);
        REGISTER_SIGNAL(SIGTERM);
        REGISTER_SIGNAL(SIGILL);
        REGISTER_SIGNAL(SIGFPE);
        REGISTER_SIGNAL(SIGBUS);
        REGISTER_SIGNAL(SIGTRAP);
        REGISTER_SIGNAL(SIGSYS);

        if (fexists(configfile) != 0) {
            std::cerr << "fts3 server config file doesn't exist" << std::endl;
            exit(1);
        }

        FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv, true);
        std::string arguments("");
        size_t foundHelp;
        if (argc > 1) {
            int i;
            for (i = 1; i < argc; i++) {
                arguments += argv[i];
            }
            foundHelp = arguments.find("-h");
            if (foundHelp != string::npos) {
                exit(0);
            }
        }

        std::string logDir = theServerConfig().get<std::string > ("TransferLogDirectory");
        if (logDir.length() > 0) {
            logDir += "/fts3server.log";
            int filedesc = open(logDir.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0777);
            if (filedesc != -1) { //if ok
                close(filedesc);
                FILE* freopenLogFile = freopen(logDir.c_str(), "a", stderr);
                if (freopenLogFile == NULL) {
                    std::cerr << "fts3 server failed to open log file, errno is:" << strerror(errno) << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "fts3 server failed to open log file, errno is:" << strerror(errno) << std::endl;
                exit(1);
            }
        }

        if (false == checkUrlCopy()) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if fts_url_copy process is set in the PATH env variable" << commit;
            exit(1);
        }

        if (fexists(hostcert) != 0) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Check if hostcert/key are installed" << commit;
            exit(1);
        }

        //set_terminate(myterminate);
        //set_unexpected(myunexpected);

        if (res == -1) {
            FTS3_COMMON_EXCEPTION_THROW(Err_System());
        }

        bool isDaemon = !FTS3_CONFIG_NAMESPACE::theServerConfig().get<bool> ("no-daemon");

        if (isDaemon) {
            //daemonize();            
            FILE* openlog = freopen(logDir.c_str(), "a", stderr);
            if (openlog == NULL)
                std::cerr << "Can't open log file" << std::endl;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting server..." << commit;
        fts3::ws::GSoapDelegationHandler::init();
        StaticSslLocking::init_locks();
        fts3_initialize_db_backend();
        struct sigaction action;
        action.sa_handler = _handle_sigint;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        res = sigaction(SIGINT, &action, NULL);

        //initialize queue updater here to avoid race conditions  
        ThreadSafeList::get_instance();

        std::string infosys = theServerConfig().get<std::string > ("Infosys");
        if (infosys.length() > 0) {
            /*only bdii to be used, not the cache file*/
            setenv("GLITE_SD_PLUGIN", "bdii", 1);
            setenv("LCG_GFAL_INFOSYS", infosys.c_str(), 1);
        }

        theServer().start();
    } catch (Err& e) {
        std::string msg = "Fatal error, exiting...";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        return EXIT_FAILURE;
    } catch (...) {
        std::string msg = "Fatal error (unknown origin), exiting...";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << msg << commit;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {

    std::string arguments("");
    size_t found;
    size_t foundHelp;
    int d = 0;
    if (argc > 1) {
        int i;
        for (i = 1; i < argc; i++) {
            arguments += argv[i];
        }
        found = arguments.find("-n");
        foundHelp = arguments.find("-h");
        if (found != string::npos) {
            {
                DoServer(argc, argv);
            }
            pthread_exit(0);
            return EXIT_SUCCESS;
        } else if (foundHelp != string::npos) {
            {
                DoServer(argc, argv);
            }
            pthread_exit(0);
            return EXIT_SUCCESS;
        }
        else {
            d = daemon(0, 0);
            if (d < 0)
                std::cerr << "Can't set daemon, will continue attached to tty" << std::endl;
        }
    } else {
        d = daemon(0, 0);
        if (d < 0)
            std::cerr << "Can't set daemon, will continue attached to tty" << std::endl;
    }

    int result = fork();

    if (result == 0) {
        DoServer(argc, argv);
    }

    if (result < 0) {
        std::cerr << "Can't start the server" << std::endl;
        exit(1);
    }

    for (;;) {
        int status = 0;
        waitpid(-1, &status, 0);
        if (!WIFSTOPPED(status)) {
            result = fork();
            if (result == 0) {
                DoServer(argc, argv);
            }
            if (result < 0) {
                exit(1);
            }
        }
        sleep(15);
    }

    return 0;
}

