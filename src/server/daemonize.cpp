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

#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <syslog.h>

#include "common/error.h"
#include "common/logger.h"
#include "daemonize.h"

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

/** Handler of SIGCHLD */
static void _handle_sigchld(int)
{
    int state;
    wait(&state);
}

/* -------------------------------------------------------------------------- */

/** The usual Linux daemon start-up sequence */
void daemonize()
{
    struct sigaction reap_action;
    reap_action.sa_handler = _handle_sigchld;
    sigemptyset(&reap_action.sa_mask);
    reap_action.sa_flags = SA_RESTART; 

    if (sigaction(SIGCHLD, &reap_action, NULL) == -1) {
        FTS3_COMMON_EXCEPTION_THROW(Err_System());
    }
    
    int status = fork();   

    if (status < 0) {
        FTS3_COMMON_EXCEPTION_THROW(Err_System());
    }
    else if (status > 0) {
        exit(EXIT_SUCCESS);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    if (setsid() < 0) {
        FTS3_COMMON_EXCEPTION_THROW(Err_System());
    }

    status = fork();

    if (status < 0) {
        FTS3_COMMON_EXCEPTION_THROW(Err_System());
    }
    else if (status > 0) {
        exit(EXIT_SUCCESS);
    }

    chdir("/");
    umask(0);
    int fileDesc = open("/dev/null", O_RDWR);/* stdin */
    (void) dup(fileDesc);  /* stdout */
    (void) dup(fileDesc);  /* stderr */
    
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Daemon is initialized" << commit;
}

FTS3_SERVER_NAMESPACE_END

