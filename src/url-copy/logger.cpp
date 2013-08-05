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



Logger::Logger(): os(&std::cerr)
{
}

Logger::~Logger()
{
   INFO() << "Closing the log stream" << std::endl;
   fhandle.close();
}

Logger& Logger::getInstance()
{
    return Logger::instance;
}



std::ostream& Logger::INFO()
{
   return (*os << getTimestamp() << " INFO    ");
}



std::ostream& Logger::WARNING()
{
   return (*os << getTimestamp() << " WARNING  ");
}



std::ostream& Logger::ERROR()
{
   return (*os << getTimestamp() << " ERROR    ");
}


std::ostream& Logger::DEBUG()
{
   return (*os << getTimestamp() << " DEBUG    ");
}



int Logger::redirectTo(const std::string& path)
{
    fhandle.close();
    fhandle.open(path.c_str(), std::ios::app);
    if (fhandle.fail())
       return errno;
    chmod(path.c_str(), 0644);
    os = &fhandle;
    return 0;
}
