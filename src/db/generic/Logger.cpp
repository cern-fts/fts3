#include "Logger.h"
#include <ctime>
#include <algorithm>
#include <string>


boost::scoped_ptr<Logger> Logger::i;
Mutex Logger::m;

Logger::Logger() : appender(NULL), layout(NULL) {
    appender = new log4cpp::FileAppender("FileAppender", LOGFILE);
    layout = new log4cpp::SimpleLayout();
    appender->setLayout(layout);
    category().setAppender(appender);
    category().setPriority(log4cpp::Priority::INFO);
}

Logger::~Logger() {

}

std::string Logger::getTimeString(std::string message) {
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    std::string timeInf0 = asctime(timeinfo);
    timeInf0 += " " + message;
    timeInf0.erase(std::remove(timeInf0.begin(), timeInf0.end(), '\n'), timeInf0.end());

    return timeInf0;
}

void Logger::info(std::string message) {
    category().info(getTimeString(message));
}

void Logger::warn(std::string message) {
    category().warn(getTimeString(message));
}

void Logger::error(std::string message) {
    category().error(getTimeString(message));
}
