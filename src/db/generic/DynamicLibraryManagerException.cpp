#include "DynamicLibraryManagerException.h"
#include "Logger.h"

DynamicLibraryManagerException::DynamicLibraryManagerException(
        const std::string &libraryName,
        const std::string &errorDetail,
        Cause cause)
: m_cause(cause) {
    if (cause == loadingFailed) {
        m_message = "Failed to load dynamic library: " + libraryName + "\n" +
                errorDetail;
        Logger::instance().error(m_message);
    } else {
        m_message = "Symbol [" + errorDetail + "] not found in dynamic libary:" +
                libraryName;
        Logger::instance().error(m_message);
    }
}

DynamicLibraryManagerException::Cause
DynamicLibraryManagerException::getCause() const {
    return m_cause;
}

const char *
DynamicLibraryManagerException::what() const throw () {
    return m_message.c_str();
}



