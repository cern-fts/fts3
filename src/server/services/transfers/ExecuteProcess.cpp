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
#include <stdio.h>
#include <signal.h>
#include <paths.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <dirent.h>
#include <sys/socket.h>
#include <linux/close_range.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "db/generic/SingleDbInstance.h"
#include "common/Logger.h"
#include "ExecuteProcess.h"
#include "common/Exceptions.h"


using namespace fts3::common;
using namespace db;


ExecuteProcess::ExecuteProcess(const std::string &app, const std::string &arguments)
    : pid(0), m_app(app), m_arguments(arguments)
{
}

int ExecuteProcess::executeProcessShell(std::string &forkMessage)
{
    return execProcessShell(forkMessage);
}

// argsHolder is used to keep the argument pointers alive
// for as long as needed
void ExecuteProcess::getArgv(std::list<std::string> &argsHolder, size_t *argc, char ***argv)
{
    boost::split(argsHolder, m_arguments, boost::is_any_of(" "));

    *argc = argsHolder.size() + 2; // Need place for the binary and the NULL
    *argv = new char *[*argc];

    std::list<std::string>::iterator it;
    int i = 0;
    (*argv)[i] = const_cast<char *> (m_app.c_str());
    for (it = argsHolder.begin(); it != argsHolder.end(); ++it) {
        ++i;
        (*argv)[i] = const_cast<char *> (it->c_str());
    }

    ++i;
    (*argv)[i] = NULL;
}

static void closeAllFilesExcept(int exception)
{
    long maxfd = sysconf(_SC_OPEN_MAX);

    if (3 < exception && exception < maxfd) {
        close_range(3, exception-1, CLOSE_RANGE_UNSHARE);
        close_range(exception+1, maxfd, CLOSE_RANGE_UNSHARE);
    }
    else {
        close_range(3, maxfd, CLOSE_RANGE_UNSHARE);
    }
}

int ExecuteProcess::execProcessShell(std::string &forkMessage)
{
    // Open pipe
    int pipefds[2] = {0, 0};
    if (pipe(pipefds)) {
        forkMessage = "Failed to create pipe between parent/child processes";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage << commit;
        return -1;
    }
    if (fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC)) {
        close(pipefds[0]);
        close(pipefds[1]);

        forkMessage = "Failed to set fd FD_CLOEXEC";
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage << commit;
        return -1;
    }

    signal(SIGCLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    pid = fork();
    // Error
    if (pid == -1) {
        close(pipefds[0]);
        close(pipefds[1]);

        forkMessage = "Failed to fork " + std::string(strerror(errno));
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage << commit;
        return -1;
    }
        // Child
    else if (pid == 0) {
        // Detach from parent
        setsid();

        // Set working directory
        if (chdir(_PATH_TMP) != 0)
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to chdir" << commit;

        // Close all open file descriptors _except_ the write-end of the pipe
        // (and stdin, stdout and stderr)
        closeAllFilesExcept(pipefds[1]);

        // Redirect stderr (points to the log)
        //stderr = freopen("/dev/null", "a", stderr);

        // Get parameter array
        std::list<std::string> argsHolder;
        size_t argc;
        char **argv;
        getArgv(argsHolder, &argc, &argv);

        // Execute the new binary
        execvp(m_app.c_str(), argv);

        // If we are here, execvp failed, so write the errno to the pipe
        if (write(pipefds[1], &errno, sizeof(int)) < 0) {
            fprintf(stderr, "Failed to write to the pipe!");
        }
        _exit(EXIT_FAILURE);
    }

    // Parent process
    // Close writing end of the pipe, and wait and see if we got an error from
    // the child
    close(pipefds[1]);

    ssize_t count = 0;
    int err = 0;
    while ((count = read(pipefds[0], &err, sizeof(err))) == -1) {
        if (errno != EAGAIN && errno != EINTR) break;
    }
    if (count) {
        forkMessage = "Child process failed to execute: " + std::string(strerror(errno));
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << forkMessage << commit;
        return -1;
    }

    // Close reading end
    close(pipefds[0]);

    return 0;
}
