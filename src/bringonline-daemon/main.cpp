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
#include "config/FileMonitor.h"
#include <boost/filesystem.hpp>
#include "name_to_uid.h"
#include <sys/resource.h>
#include "queue_bringonline.h"
#include "UserProxyEnv.h"
#include "DelegCred.h"
#include <gfal_api.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "name_to_uid.h"

using namespace FTS3_SERVER_NAMESPACE;
using namespace FTS3_COMMON_NAMESPACE;

extern std::string stackTrace;
bool stopThreads = false;
const char * const PROXY_NAME_PREFIX = "x509up_h";
const std::string repository = "/tmp/";

static bool isSrmUrl(const std::string & url) {
    if (url.compare(0, 6, "srm://") == 0)
        return true;

    return false;
}

void fts3_teardown_db_backend() {
    try {
        db::DBSingleton::tearDown();
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Unexpected exception when forcing the database teardown" << commit;
        exit(1);
    }
}

void _handle_sigint(int) {
    if (stackTrace.length() > 0)
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << stackTrace << commit;
    stopThreads = true;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon stopping" << commit;
    sleep(2);
    fts3_teardown_db_backend();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon stopped" << commit;
    exit(0);
}

/* -------------------------------------------------------------------------- */

void fts3_initialize_db_backend() {
    std::string dbUserName = theServerConfig().get<std::string > ("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string > ("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string > ("DbConnectString");

    try {
        //use 1 hardcoded connection 
        db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, 4);
    } catch (Err& e) {
        throw;
    } catch (std::exception& ex) {
        throw;
    } catch (...) {
        throw;
    }
}

void issueBringOnLineStatus(gfal2_context_t handle, std::string infosys) {
    char token[512] = {0};
    int statusA = 0;
    int statusB = 0;
    GError *error = NULL;
    const char *protocols[10];
    protocols[0] = "rfio";
    UserProxyEnv* cert = NULL;

    gfal2_set_opt_string_list(handle, "SRM PLUGIN", "TURL_PROTOCOLS", protocols, 1, &error);
    if (error) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Could not set the protocol list " << error->code << " " << error->message << commit;
        return;
    }

    if (infosys.compare("false") == 0) {
        gfal2_set_opt_boolean(handle, "BDII", "ENABLED", false, NULL);
    } else {
        gfal2_set_opt_string(handle, "BDII", "LCG_GFAL_INFOSYS", (char *) infosys.c_str(), NULL);
    }    
    
    while (!stopThreads) {        
        std::list<struct message_bringonline>::iterator i = ThreadSafeBringOnlineList::get_instance().m_list.begin();	
        if (!ThreadSafeBringOnlineList::get_instance().m_list.empty()) {
            while (i != ThreadSafeBringOnlineList::get_instance().m_list.end()) {
                cert = new UserProxyEnv((*i).proxy);		
                bool deleteIt = false;					
                if ((*i).started == false) { //issue bringonline
                    db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("STARTED", "", (*i));

                    statusA = gfal2_bring_online(handle, ((*i).url).c_str(), 28800, 28800, token, sizeof (token), 1, &error);
		    
                    if (statusA < 0) {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE online failed " << error->code << " " << error->message << commit;
                        db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FAILED", std::string(error->message), (*i));
                        (*i).started = true;
                        deleteIt = true;
                        g_clear_error(&error);
                    } else if (statusA == 0) {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE online queued, got token " << token << commit;
                        (*i).token = std::string(token);
			db::DBSingleton::instance().getDBObjectInstance()->addToken((*i).job_id, (*i).file_id, std::string(token));
                        (*i).started = true;			
                    } else {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE online succeeded, got token " << token << commit;
                        db::DBSingleton::instance().getDBObjectInstance()->addToken((*i).job_id, (*i).file_id, std::string(token));
                        (*i).started = true;
                    }
                } else { //poll
                    statusB = gfal2_bring_online_poll(handle, ((*i).url).c_str(), ((*i).token).c_str(), &error);

                    if (statusB < 0) {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE polling failed, token " << (*i).token << ", "  << error->code << " " << error->message << commit;
                        db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FAILED", std::string(error->message), (*i));
                        deleteIt = true;
                        (*i).started = true;
                        g_clear_error(&error);
                    } else if(statusB == 0) {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE polling token " << (*i).token << commit;
                        (*i).started = true;
                    } else{
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE online finished token " << (*i).token << commit;
                        (*i).started = true;
			deleteIt = true;
                        db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FINISHED", "", (*i));
		    }
                }

                if (deleteIt) {
                    ThreadSafeBringOnlineList::get_instance().m_list.erase(i++);                    
                } else {
                    ++i;
                }

                if (cert) {
                    delete cert;
                    cert = NULL;
                }
            }
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
}

static std::string generateProxy(const std::string& dn, const std::string& dlg_id) {
    boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
    return delegCredPtr->getFileName(dn, dlg_id);
}

int DoServer(int argc, char** argv) {

    int res = 0;
    std::string proxy_file("");
    std::string infosys("");
    gfal2_context_t handle;

    try {
        REGISTER_SIGNAL(SIGABRT);
        REGISTER_SIGNAL(SIGSEGV);
        REGISTER_SIGNAL(SIGTERM);
        REGISTER_SIGNAL(SIGILL);
        REGISTER_SIGNAL(SIGFPE);
        REGISTER_SIGNAL(SIGBUS);
        REGISTER_SIGNAL(SIGTRAP);
        REGISTER_SIGNAL(SIGSYS);

        //re-read here
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
            logDir += "/fts3bringonline.log";
            int filedesc = open(logDir.c_str(), O_CREAT | O_WRONLY | O_APPEND, 0644);
            if (filedesc != -1) { //if ok
                close(filedesc);
                FILE* freopenLogFile = freopen(logDir.c_str(), "a", stderr);
                if (freopenLogFile == NULL) {
                    std::cerr << "BRINGONLINE  daemon failed to open log file, errno is:" << strerror(errno) << std::endl;
                    exit(1);
                }
            } else {
                std::cerr << "BRINGONLINE  daemon failed to open log file, errno is:" << strerror(errno) << std::endl;
                exit(1);
            }
        }

        bool isDaemon = !FTS3_CONFIG_NAMESPACE::theServerConfig().get<bool> ("no-daemon");

        if (isDaemon) {
            FILE* openlog = freopen(logDir.c_str(), "a", stderr);
            if (openlog == NULL)
                std::cerr << "BRINGONLINE  Can't open log file" << std::endl;
        }

        /*set infosys to gfal2*/
        infosys = theServerConfig().get<std::string > ("Infosys");

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE starting daemon..." << commit;
        fts3_initialize_db_backend();
        struct sigaction action;
        action.sa_handler = _handle_sigint;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        res = sigaction(SIGINT, &action, NULL);

        //initialize here to avoid race conditions  
        ThreadSafeList::get_instance();

        try {
            // Set up handle
            GError *error = NULL;
            handle = gfal2_context_new(&error);
            if (!handle) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE bad initialization " << error->code << " " << error->message << commit;
                return -1;
            }
            boost::thread bt2(issueBringOnLineStatus, handle, infosys);
        } catch (std::exception& e) {
            throw;
        } catch (...) {
            throw;
        }

        vector< tuple<string, string, int> >::iterator it;
        std::vector< boost::tuple<std::string, std::string, int> > voHostnameConfig;
        std::vector<struct message_bringonline> urls;
        vector<struct message_bringonline>::iterator itUrls;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE daemon started..." << commit;
        while (!stopThreads) {

            //select from the database the config for bringonline for each VO / hostname
            voHostnameConfig = db::DBSingleton::instance().getDBObjectInstance()->getVOBringonlimeMax();

            if (!voHostnameConfig.empty()) {
                for (it = voHostnameConfig.begin(); it != voHostnameConfig.end(); it++) {
                    string voName = get < 0 > (*it);
                    string hostName = get < 1 > (*it);
                    int maxValue = get < 2 > (*it);
                    //get the files to be brought online for this VO/host/max
                    urls = db::DBSingleton::instance().getDBObjectInstance()->getBringOnlineFiles(voName, hostName, maxValue);
                }
            } else {//no config for any vo and/or se, hardcode 100 files at any given time for each hostname
                urls = db::DBSingleton::instance().getDBObjectInstance()->getBringOnlineFiles("", "", 500);
            }

            if (!urls.empty()) {
	        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << urls.size() << " are ready for bringonline"  << commit;
                for (itUrls = urls.begin(); itUrls != urls.end(); itUrls++) {
                    if (true == isSrmUrl((*itUrls).url)) {
                        std::string dn;
                        std::string dlg_id;
                        db::DBSingleton::instance().getDBObjectInstance()->getCredentials((*itUrls).job_id, (*itUrls).file_id, dn, dlg_id);

                        //get the proxy
                        proxy_file = generateProxy(dn, dlg_id);
                        (*itUrls).proxy = proxy_file;
                        ThreadSafeBringOnlineList::get_instance().push_back(*itUrls);
                    }
                }
            }

            urls.clear();
            voHostnameConfig.clear();
            sleep(1);
        }
        gfal2_context_free(handle);
    } catch (Err& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
        return -1;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin), exiting..." << commit;
        return -1;
    }
    return EXIT_SUCCESS;
}


__attribute__((constructor)) void begin(void) {
    //switch to non-priviledged user to avoid reading the hostcert
    uid_t pw_uid;
    pw_uid = name_to_uid();
    setuid(pw_uid);
    seteuid(pw_uid);    
}

int main(int argc, char** argv) {

    pid_t child;
    //very first check before it goes to deamon mode
    try {
        FTS3_CONFIG_NAMESPACE::theServerConfig().read(argc, argv, true);
    } catch (Err& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
        return EXIT_FAILURE;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin), exiting..." << commit;
        return EXIT_FAILURE;
    }

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
        } else {
            d = daemon(0, 0);
            if (d < 0)
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Can't set daemon, will continue attached to tty" << commit;
        }
    } else {
        d = daemon(0, 0);
        if (d < 0)
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Can't set daemon, will continue attached to tty" << commit;
    }

    int result = fork();

    if (result == 0) { //child
        int resultExec = DoServer(argc, argv);
        if (resultExec < 0) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Can't start bringonline daemon" << commit;
            exit(1);
        }
    } else { //parent
        child = result;
        sleep(2);
        int err = waitpid(child, NULL, WNOHANG);
        if (err != 0) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE waitpid error: " << strerror(errno) << commit;
            return -1;
        }
    }

    for (;;) {
        int status = 0;
        waitpid(-1, &status, 0);
        if (!WIFSTOPPED(status)) {
            result = fork();
            if (result == 0) {
                result = DoServer(argc, argv);
            }
            if (result < 0) {
                exit(1);
            }
        }
        sleep(15);
    }

    return 0;
}

