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

using namespace FTS3_COMMON_NAMESPACE;
using namespace std;
using namespace StringHelper;

static int fexists(const char *filename) {
    struct stat buffer;
    if (stat(filename, &buffer) == 0) return 0;
    return -1;
}

ExecuteProcess::ExecuteProcess(const string& app, const string& arguments, int fdlog)
: m_app(app), m_arguments(arguments), m_fdlog(fdlog) {
}

int ExecuteProcess::executeProcess() {
    list<string> args;
    split(m_arguments, ' ', args, 0, false);

    int argc = 1 + args.size() + 1;

    char** argv = new char*[argc];
    list<string>::iterator it = args.begin();

    int i = 0;
    argv[i] = const_cast<char*> (m_app.c_str());
    for (; it != args.end(); ++it) {
        ++i;
        argv[i] = const_cast<char*> (it->c_str());
    }

    ++i;
    assert(i + 1 == argc);
    argv[i] = NULL;
    int status = 0;

    if (m_fdlog > 0) {
        status = execProcessLog(argc, argv);
    } else {
        status = execProcess(argc, argv);
    }
    delete [] argv;

    return status;
}

std::string ExecuteProcess::generate_request_id(const std::string& prefix) {

    std::string new_name = std::string("");

    // Get current time
    time_t current;
    time(&current);
    struct tm * date = gmtime(&current);

    // Create template
    std::stringstream ss;
    if (false == prefix.empty()) {
        ss << prefix;
    }
    ss << std::setfill('0');
    ss << "__" << std::setw(4) << (date->tm_year + 1900)
            << "-" << std::setw(2) << (date->tm_mon + 1)
            << "-" << std::setw(2) << (date->tm_mday)
            << "-" << std::setw(2) << (date->tm_hour)
            << std::setw(2) << (date->tm_min)
            << "_XXXXXX";

    // Generate File name
    new_name = "/var/log/fts3";
    new_name += ss.str();
    char* ret = mktemp(&(*(new_name.begin())));

    if (ret == NULL)
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Job id " + prefix + " log file cannot be generated"));

    return new_name;
}

int ExecuteProcess::execProcessLog(int argc, char** argv) {
    int status = 0;
    int fdpipe[2];
    ssize_t  write_size;
    argc = 0;
    int value = 0;
    value = pipe(fdpipe);
    if (value != 0)
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << " Pipe system call failed, errno: " << errno << commit;

    pid_t pid = fork();
    if (pid == 0) {
        // child process
        close(fdpipe[0]);
        dup2(fdpipe[1], 1);
        dup2(fdpipe[1], 2);

        execv(m_app.c_str(), argv);
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // fork failed
        status = -1;
    } else {
        // parent process
        close(fdpipe[1]);

        char readbuf[1024] = {0};
        ssize_t  bytes;
	pid_t wpval;

        while ((wpval = waitpid(pid, &status, WNOHANG)) == 0) {
            while ((bytes = read(fdpipe[0], readbuf, sizeof (readbuf) - 1)) > 0) {
                readbuf[bytes] = 0;
                fflush(stdout);
                fflush(stderr);
                write_size = write(m_fdlog, readbuf, bytes);
            }
        }

        if (wpval != pid) {
            status = -1;
        }
    }

    return status;
}

int ExecuteProcess::execProcess(int argc, char** argv) {
    int status = 0;
    argc = 0;
    pid_t pid = fork();
    if (pid == 0) {
        // child process
        execv(m_app.c_str(), argv);
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // fork failed
        status = -1;
    } else {
        // parent process
        if (waitpid(pid, &status, 0) != pid) {
            status = -1;
        }
    }
    return status;
}

int ExecuteProcess::executeProcessShell() {
    static const char SHELL[] = "/bin/sh";
    int status = 0;
    if (m_fdlog > 0) {
        status = execProcessShellLog(SHELL);
    } else {
        status = execProcessShell();
    }

    return status;
}

int ExecuteProcess::execProcessShellLog(const char* SHELL) {
    int status = 0;
    ssize_t write_size;
    int fdpipe[2];
    int value = 0;
    value = pipe(fdpipe);
    if (value != 0)
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << " Pipe system call failed, errno: " << errno << commit;


    pid_t pid = fork();
    if (pid == 0) {
        // child process
        close(fdpipe[0]);
        dup2(fdpipe[1], 1);
        dup2(fdpipe[1], 2);

        execl(SHELL, SHELL, "-c", (m_app + " " + m_arguments).c_str(), NULL);
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // fork failed
        status = -1;
    } else {
        // parent process
        close(fdpipe[1]);

        char readbuf[1024];
        ssize_t bytes;
	pid_t wpval;

        while ((wpval = waitpid(pid, &status, WNOHANG)) == 0) {
            while ((bytes = read(fdpipe[0], readbuf, sizeof (readbuf) - 1)) > 0) {
                readbuf[bytes] = 0;
                fflush(stdout);
                fflush(stderr);
                write_size = write(m_fdlog, readbuf, bytes);
            }
        }

        if (wpval != pid) {
            status = -1;
        }
    }

    return status;
}

int ExecuteProcess::execProcessShell() {   
    std::vector<std::string> pathV;
    std::vector<std::string>::iterator iter;
    std::string p;

    list<string> args;
    split(m_arguments, ' ', args, 0, false);

    int argc = 1 + args.size() + 1;

    char** argv = new char*[argc];
    list<string>::iterator it = args.begin();

    int i = 0;
    argv[i] = const_cast<char*> (m_app.c_str());
    for (; it != args.end(); ++it) {
        ++i;
        argv[i] = const_cast<char*> (it->c_str());
    }

    ++i;
    assert(i + 1 == argc);
    argv[i] = NULL;
    
    // Fork a child process
    pid_t pid_1 = fork();

    // Ignore SIGCLD: Don't wait for the child to complete
    signal(SIGCLD, SIG_IGN);

    if (pid_1 == 0) {
        // Detach from parent
        setsid();
        // Set working directory
        int ret = chdir(_PATH_TMP);

        if (ret == -1)
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << " chdir " << errno << commit;

        // Close stdin and out
        int fd;
        if ((fd = ::open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
            ::dup2(fd, STDIN_FILENO);
            ::dup2(fd, STDOUT_FILENO);
            ::dup2(fd, STDERR_FILENO);
            if (fd > 2) {
                close(fd);
            }
        }

        char *token;
        const char *path = getenv("PATH");
        char *copy = (char *) malloc(strlen(path) + 1);     
        strcpy(copy, path);
        token = strtok(copy, ":");

        while ( (token = strtok(0, ":")) != NULL) {
            pathV.push_back(std::string(token));
        }

        for (iter = pathV.begin(); iter < pathV.end(); iter++) {
            p = *iter + "/" + std::string(argv[0]);
            if (fexists(p.c_str()) == 0)
                break;
        }

        free(copy);
        copy = NULL;
        pathV.clear();
	
        int r = execvp(p.c_str(), argv);
        if (-1 == r) {
            // Process Execution failed: exit the forked process
            // The status will detect if the request has not been
            // assigned
	    //freopen ("/var/log/fts3/fts3server.log","a",stderr);
	    int r2 = execvp(p.c_str(), argv);
	    if (-1 == r2) {
            	FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process Execution failed: exit the forked process" << commit;
            	_exit(1);
	    }
        }
    }

    //nothing to do in the parent, already detached, no reason to wait
    delete [] argv;
    return 0;
}


/*
int ExecuteProcess::execProcessShell(const char* SHELL)
{	

    const char* pPath = "/home/user/workspace/fts3svn/build/src/url-copy/fts3_url_copy";
    list<string> args;
    split( m_arguments, ' ', args, 0, false );

    int argc = 1 + args.size() + 1; 

    char** argv = new char*[argc];
    list<string>::iterator it = args.begin();

    int i = 0;
    argv[i] = const_cast<char*>( m_app.c_str() );
    for ( ; it != args.end(); ++it ) {
        ++i;
        argv[i] = const_cast<char*>( it->c_str() );
    }

    ++i;
    assert( i+1 == argc );
    argv[i] = NULL;
   
	
       // Fork a child process
        pid_t pid_1 = fork();
        if(-1 == pid_1){
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << " fork, errno: " << errno << commit;
        } else if(0 != pid_1){
            // Parent process: wait for the first child
            int status;
            ::wait(&status);
            if(0 == WEXITSTATUS(status)){
                // OK
            } else if(1 == WEXITSTATUS(status)){
                 FTS3_COMMON_LOGGER_NEWLOG (ERR) << " second fork failed " << commit;
            } 
        } else {
            // Child process
            // Ignore SIGCLD: Don't wait for the child to complete
            signal(SIGCLD, SIG_IGN);
            pid_t pid_2 = fork();
            if(-1 == pid_2){
                // Failed to do the second fork
                FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Failed to do the second fork " << commit;
                exit(1);
            } else if(0 != pid_2){
                // Parent process exit 0
                                FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Parent process exit 0" << commit;
                exit(0);
            } else {
                // Detach from parent
                setsid();
                // Set working directory
                int ret = chdir(_PATH_TMP);

                if (ret == -1)
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << " chdir " << errno << commit;

                // Close stdin and out
                int fd;
                if ((fd = ::open(_PATH_DEVNULL, O_RDWR, 0)) != -1) {
                    ::dup2(fd, STDIN_FILENO);
                    ::dup2(fd, STDOUT_FILENO);
                    ::dup2(fd, STDERR_FILENO);
                    if (fd > 2){
                       close(fd);
                    }
                }		
						
                int r = execvp(pPath, argv);
                if(-1 == r){
                    // Process Execution failed: exit the forked process
                    // The status will detect if the request has not been
                    // assigned
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Process Execution failed: exit the forked process" << commit;		    
                    exit(1);
                }
            }
        }    
        delete [] argv;
        return 0;
}
 */
