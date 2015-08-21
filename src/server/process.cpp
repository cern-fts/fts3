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

#include <list>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "common/logger.h"
#include "common/error.h"
#include <stdio.h>
#include <signal.h>
#include <paths.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include "process.h"
#include "stringhelper.h"
#include "common/logger.h"
#include "common/error.h"
#include <iostream>
#include <fstream>
#include "SingleDbInstance.h"
#include <dirent.h>
#include <sys/socket.h>

using namespace FTS3_COMMON_NAMESPACE;
using namespace std;
using namespace StringHelper;
using namespace db;


ExecuteProcess::ExecuteProcess(const string& app, const string& arguments)
    : pid(0), m_app(app), m_arguments(arguments)
{
}

int ExecuteProcess::executeProcessShell(std::string& forkMessage)
{
    return execProcessShell(forkMessage);
}

// argsHolder is used to keep the argument pointers alive
// for as long as needed
void ExecuteProcess::getArgv(list<string>& argsHolder, size_t* argc, char*** argv)
{
    split(m_arguments, ' ', argsHolder, 0, false);

    *argc = argsHolder.size() + 2; // Need place for the binary and the NULL
    *argv = new char*[*argc];

    list<string>::iterator it;
    int i = 0;
    (*argv)[i] = const_cast<char*> (m_app.c_str());
    for (it = argsHolder.begin(); it != argsHolder.end(); ++it)
        {
            ++i;
            (*argv)[i] = const_cast<char*> (it->c_str());
        }

    ++i;
    assert(i + 1 == argc);
    (*argv)[i] = NULL;
}

static void closeAllFilesExcept(int exception)
{
    long maxfd = sysconf(_SC_OPEN_MAX);

    register int fdAll;
    for(fdAll = 3; fdAll < maxfd; fdAll++)
        {
            if(fdAll != exception)
                close(fdAll);
        }
}

int ExecuteProcess::execProcessShell(std::string& forkMessage)
{
    // Open pipe
    int pipefds[2] = {0, 0};
    if (pipe(pipefds))
        {
            forkMessage = "Failed to create pipe between parent/child processes";
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage  << commit;
            return -1;
        }
    if (fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC))
        {
            close(pipefds[0]);
            close(pipefds[1]);

            forkMessage = "Failed to set fd FD_CLOEXEC";
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage  << commit;
            return -1;
        }

    // Ignore SIGCLD: Don't wait for the child to complete
    signal(SIGCLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    pid_t child = fork();
    // Error
    if (child == -1 )
        {
            close(pipefds[0]);
            close(pipefds[1]);

            forkMessage = "Failed to fork " + std::string(strerror(errno));
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage  << commit;
            return -1;
        }
    // Child
    else if (child == 0)
        {
            // Detach from parent
            setsid();

            // Set working directory
            if(chdir(_PATH_TMP) != 0)
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to chdir"  << commit;

            // Close all open file descriptors _except_ the write-end of the pipe
            // (and stdin, stdout and stderr)
            closeAllFilesExcept(pipefds[1]);

            // Redirect stderr (points to the log)
            freopen("/dev/null", "a", stderr);

            // Get parameter array
            list<string> argsHolder;
            size_t       argc;
            char       **argv;
            getArgv(argsHolder, &argc, &argv);

            // Execute the new binary
            execvp(m_app.c_str(), argv);

            // If we are here, execvp failed, so write the errno to the pipe
            write(pipefds[1], &errno, sizeof(int));
            _exit(EXIT_FAILURE);
        }

    // Parent process
    // Close writting end of the pipe, and wait and see if we got an error from
    // the child
    pid = child;
    close(pipefds[1]);

    ssize_t count = 0;
    int     err = 0;
    while ((count = read(pipefds[0], &err, sizeof(errno))) == -1)
        {
            if (errno != EAGAIN && errno != EINTR) break;
        }
    if (count)
        {
            forkMessage = "Child process failure " + std::string(strerror(errno));
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage  << commit;
            return -1;
        }

    // Close reading end
    close(pipefds[0]);

    // Sleep for awhile but do not block waiting for child
    usleep(50000);
    if(waitpid(pid, NULL, WNOHANG) != 0)
        {
            forkMessage = "Waiting for child process failure " + std::string(strerror(errno));
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage  << commit;
            return -1;
        }

    return 0;
}
