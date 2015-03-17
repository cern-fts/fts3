#include "logger.h"
#include <ctime>
#include <sys/stat.h>


Logger Logger::instance;



static std::string getTimestamp()
{
    char timebuf[128] = "";
    time_t current = time(NULL);
    struct tm local_tm;
    localtime_r(&current, &local_tm);
    std::string timestampStr = asctime_r(&local_tm, timebuf);
    // asctime_r ends with '\n'!
    timestampStr.erase(timestampStr.end() - 1);
    return timestampStr;
}



Logger::Logger(): log(&std::cerr)
{
}



Logger::~Logger()
{
    INFO() << "Closing the log stream" << std::endl;
    logHandle.close();
}



Logger& Logger::getInstance()
{
    return Logger::instance;
}



std::ostream& Logger::INFO()
{
    return (*log << std::fixed << getTimestamp() << " INFO     ");
}



std::ostream& Logger::WARNING()
{
    return (*log << std::fixed << getTimestamp() << " WARNING  ");
}



std::ostream& Logger::ERROR()
{
    return (*log << std::fixed << getTimestamp() << " ERROR    ");
}



std::ostream& Logger::DEBUG()
{
    return (*log << std::fixed << getTimestamp() << " DEBUG    ");
}



int Logger::redirectTo(const std::string& path, bool debug)
{
    // Regular output
    logHandle.close();
    logHandle.open(path.c_str(), std::ios::app);
    if (logHandle.fail())
        return errno;
    if (chmod(path.c_str(), 0644))
        this->ERROR() << "Failed to change the mode of the log file" << std::endl;
    log = &logHandle;

    // Debug output
    if (debug)
        {
            std::string debugPath = path + ".debug";
            FILE* freopenExec = freopen(debugPath.c_str(), "w", stderr);
            if(!freopenExec)  //try again
                {
                    freopenExec = freopen(debugPath.c_str(), "w", stderr);
                    chmod(debugPath.c_str(), 0644);
                }
            chmod(debugPath.c_str(), 0644);
        }
    return 0;
}
