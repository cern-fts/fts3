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

using namespace FTS3_COMMON_NAMESPACE;
using namespace std;
using namespace StringHelper;


ExecuteProcess::ExecuteProcess( const string& app, const string& arguments, int fdlog )
  : m_app( app ), m_arguments( arguments ), m_fdlog( fdlog )
{
}

int ExecuteProcess::executeProcess()
{
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
    int status = 0;

    if ( m_fdlog > 0 ) {
        status = execProcessLog(argc, argv);
    } else {
        status = execProcess(argc, argv);
    }
    delete [] argv;

    return status;
}

std::string ExecuteProcess::generate_request_id(const std::string& prefix){

    std::string new_name = std::string("");

    // Get current time
    time_t current;
    time(&current);
    struct tm * date = gmtime(&current);

    // Create template
    std::stringstream ss;
    if(false == prefix.empty()){
        ss << prefix;
    }
    ss << std::setfill('0');
    ss << "__" << std::setw(4) << (date->tm_year+1900)
       <<  "-" << std::setw(2) << (date->tm_mon+1)
       <<  "-" << std::setw(2) << (date->tm_mday)
       <<  "-" << std::setw(2) << (date->tm_hour)
               << std::setw(2) << (date->tm_min)
       << "_XXXXXX";

    // Generate File name
    new_name = "/var/log/fts3";
    new_name += ss.str();
    char* ret = mktemp(&(*(new_name.begin())));

    if (ret == NULL)
       FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Job id " +prefix+ " log file cannot be generated"));

    return new_name;
}


int ExecuteProcess::execProcessLog(int argc, char** argv)
{
    int status = 0;
    int fdpipe[2];
    size_t write_size;
    argc = 0;    
    int value = 0;
    value = pipe( fdpipe );
    if(value != 0)
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << " Pipe system call failed, errno: " << errno << commit;

    pid_t pid = fork();
    if ( pid == 0 ) {
        // child process
        close( fdpipe[0] );
	dup2( fdpipe[1], 1 );
	dup2( fdpipe[1], 2 );

        execv( m_app.c_str(), argv );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        close( fdpipe[1] );

	char readbuf[1024]={0};
	int bytes, wpval;

	while ( (wpval = waitpid ( pid, &status, WNOHANG )) == 0 ) {
	    while ( (bytes=read(fdpipe[0], readbuf, sizeof(readbuf)-1)) > 0 ) {
	        readbuf[bytes] = 0;
                fflush(stdout);
                fflush(stderr);
                write_size = write( m_fdlog, readbuf, bytes );
	    }
	}

        if ( wpval != pid ) {
            status = -1;
        }
    }

    return status;
}


int ExecuteProcess::execProcess(int argc, char** argv)
{
    int status = 0;
    argc = 0;
    pid_t pid = fork();
    if ( pid == 0 ) {
        // child process
        execv( m_app.c_str(), argv );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        if ( waitpid ( pid, &status, 0 ) != pid ) {
            status = -1;
        }
    }
    return status;
}


int ExecuteProcess::executeProcessShell()
{
    static const char SHELL[] = "/bin/sh";
    int status = 0;
    if ( m_fdlog > 0 ) {
        status = execProcessShellLog(SHELL);
    } else {
        status = execProcessShell(SHELL);
    }

    return status;
}


int ExecuteProcess::execProcessShellLog(const char* SHELL)
{
    int status = 0;
    size_t write_size;
    int fdpipe[2];
    int value = 0;
    value = pipe( fdpipe );
    if(value != 0)
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << " Pipe system call failed, errno: " << errno << commit;
    

    pid_t pid = fork();
    if ( pid == 0 ) {
        // child process
        close( fdpipe[0] );
	dup2( fdpipe[1], 1 );
	dup2( fdpipe[1], 2 );

        execl( SHELL, SHELL, "-c", (m_app + " " + m_arguments).c_str(), NULL );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        close( fdpipe[1] );

	char readbuf[1024];
	int bytes, wpval;

	while ( (wpval = waitpid ( pid, &status, WNOHANG )) == 0 ) {
	    while ( (bytes=read(fdpipe[0], readbuf, sizeof(readbuf)-1)) > 0 ) {
	        readbuf[bytes] = 0;
                fflush(stdout);
                fflush(stderr);
                write_size = write( m_fdlog, readbuf, bytes );
	    }
	}

        if ( wpval != pid ) {
            status = -1;
        }
    }

    return status;
}

int ExecuteProcess::execProcessShell(const char* SHELL)
{
    int status = 0;

    pid_t pid = fork();
    if ( pid == 0 ) {
        execl( SHELL, SHELL, "-c", (m_app + " " + m_arguments).c_str(), NULL );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        if ( waitpid ( pid, &status, 0 ) != pid ) {
            status = -1;
        }
    }

    return status;
}
