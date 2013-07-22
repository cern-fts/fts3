/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
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

static int fexists(const char *filename)
{
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}


ExecuteProcess::ExecuteProcess(const string& app, const string& arguments, int fdlog)
    : pid(0), _jobId(""), _fileId(""), m_app(app), m_arguments(arguments),
      m_fdlog(fdlog)
{
}

int ExecuteProcess::executeProcessShell()
{
    static const char SHELL[] = "/bin/sh";
    int status = 0;
    if (m_fdlog > 0)
        {
            status = execProcessShellLog(SHELL);
        }
    else
        {
            status = execProcessShell();
        }

    return status;
}


void ExecuteProcess::setPid(const string& jobId, const string& fileId)
{
    _jobId = jobId;
    _fileId = fileId;
}

void ExecuteProcess::setPidV(std::map<int,std::string>& pids)
{
    _fileIds.insert(pids.begin(), pids.end());
}

int ExecuteProcess::execProcessShellLog(const char* SHELL)
{
    int status = 0;
    ssize_t write_size;
    int fdpipe[2];
    int value = 0;
    value = pipe(fdpipe);
    if (value != 0) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Pipe system call failed, errno: " << errno << commit;
        return -1;
    }


    pid_t pid = fork();
    if (pid == 0)
        {
            // child process
            close(fdpipe[0]);
            dup2(fdpipe[1], 1);
            dup2(fdpipe[1], 2);

            execl(SHELL, SHELL, "-c", (m_app + " " + m_arguments).c_str(), NULL);
            _exit(EXIT_FAILURE);
        }
    else if (pid < 0)
        {
            // fork failed
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to fork"  << commit;
            close(fdpipe[0]);
            close(fdpipe[1]);
            status = -1;
        }
    else
        {
            // parent process
            close(fdpipe[1]);

            char readbuf[1024];
            ssize_t bytes;
            pid_t wpval;

            while ((wpval = waitpid(pid, &status, WNOHANG)) == 0)
                {
                    while ((bytes = read(fdpipe[0], readbuf, sizeof (readbuf) - 1)) > 0)
                        {
                            readbuf[bytes] = 0;
                            fflush(stdout);
                            fflush(stderr);
                            write_size = write(m_fdlog, readbuf, static_cast<size_t>(bytes));
                        }
                }

            if (wpval != pid)
                {
                    status = -1;
                }
        }

    return status;
}

int ExecuteProcess::execProcessShell()
{
    std::vector<std::string> pathV;
    std::vector<std::string>::iterator iter;
    std::string p;
    int pipefds[2];
    ssize_t count=0;
    int err=0;
    pid_t child;
    const char *path=NULL;
    char *copy=NULL;
    long int maxfd;
    ssize_t checkWriteSize;
    int checkDir = 0;

    list<string> args;
    split(m_arguments, ' ', args, 0, false);

    size_t argc = 1 + args.size() + 1;

    char** argv = new char*[argc];
    list<string>::iterator it = args.begin();

    int i = 0;
    argv[i] = const_cast<char*> (m_app.c_str());
    for (; it != args.end(); ++it)
        {
            ++i;
            argv[i] = const_cast<char*> (it->c_str());
        }

    ++i;
    assert(i + 1 == argc);
    argv[i] = NULL;

    if (pipe(pipefds))
        {
            if(argv)
                delete [] argv;
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to create pipe between parent/child processes"  << commit;
            return -1;
        }
    if (fcntl(pipefds[1], F_SETFD, fcntl(pipefds[1], F_GETFD) | FD_CLOEXEC))
        {
            if(argv)
                delete [] argv;
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to set fd FD_CLOEXEC"  << commit;
            return -1;
        }

    // Ignore SIGCLD: Don't wait for the child to complete
    signal(SIGCLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    switch (child = fork())
        {
        case -1:
            if(argv)
                delete [] argv;
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to fork"  << commit;
            close(pipefds[0]);
            close(pipefds[1]);
            return -1;
        case 0:
            // Detach from parent
            setsid();
            // Set working directory
            checkDir = chdir(_PATH_TMP);
            if(-1 == checkDir)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to chdir"  << commit;
                }
            maxfd=sysconf(_SC_OPEN_MAX);
            register int fdAll;
            for(fdAll=3; fdAll<maxfd; fdAll++)
                {
                    if(fdAll == pipefds[0])
                        continue;
                    if(fdAll == pipefds[1])
                        continue;
                    close(fdAll);
                }

            close(pipefds[0]);
            char *token;
            path = getenv("PATH");
            if (path == NULL || path[0] == '\0')
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to getenv PATH" << commit;
                    if(argv)
                        delete [] argv;
                    return -1;
                }

            copy = (char *) malloc(strlen(path) + 1);
            strcpy(copy, path);
            token = strtok(copy, ":");
            if(token)
                {
                    pathV.push_back(std::string(token));
                }
            while ( (token = strtok(0, ":")) != NULL)
                {
                    pathV.push_back(std::string(token));
                }
            for (iter = pathV.begin(); iter < pathV.end(); ++iter)
                {
                    p = *iter + "/" + std::string(argv[0]);
                    if (fexists(p.c_str()) == 0)
                        break;
                }
            if(copy)
                {
                    free(copy);
                    copy = NULL;
                }
            pathV.clear();
            execvp(p.c_str(), argv);
            checkWriteSize = write(pipefds[1], &errno, sizeof(int));
            _exit(EXIT_FAILURE);
        default:
            pid = (int) child;
            if(argv)
                delete [] argv;
            close(pipefds[1]);
            while ((count = read(pipefds[0], &err, sizeof(errno))) == -1)
                {
                    if (errno != EAGAIN && errno != EINTR) break;
                }
            if (count)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Child's execvp error: " << strerror(err)  << commit;
                    return -1;
                }
            close(pipefds[0]);
        }

    /*sleep for awhile but do not block waiting for child*/
    usleep(10000);
    err  = waitpid(pid, NULL, WNOHANG);
    if(err != 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "waitpid error: " << strerror(errno)  << commit;
        }
    return err;
}



int ExecuteProcess::check_pid(int pid)
{
    int result = -1;
    const char * PROC_DIR = "/proc";

    char pidstr[256];
    sprintf(pidstr,"%d",pid);
    std::string dir_name = (std::string)PROC_DIR + "/"+ pidstr;
    // Search the process in the /proc dir
    DIR * dir = opendir(dir_name.c_str());
    if(0 != dir)
        {
            closedir(dir);
            // Try to open the cmdline file
            std::string fname = (std::string)PROC_DIR + "/"+ pidstr + "/cmdline";
            std::ifstream cmdline_file(fname.c_str());
            if(cmdline_file.is_open())
                {
                    //try to read a char
                    char c;
                    cmdline_file.read(&c,1);
                    if(cmdline_file.good())
                        {
                            result = 0;
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Checking cmdline_file.good(): " << pid << " " << strerror(errno) << commit;
                            result = -1;
                        }
                }
            else
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Checking cmdline_file.is_open() : " << pid << " " << strerror(errno) << commit;
                    result = -1;
                }
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Checking 0 != dir: " << pid << " " << strerror(errno) << commit;
            result = -1;
        }
    return result;
}

