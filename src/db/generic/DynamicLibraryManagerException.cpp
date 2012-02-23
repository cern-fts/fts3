#include "DynamicLibraryManagerException.h"
#include "logger.h"


using namespace FTS3_COMMON_NAMESPACE;

DynamicLibraryManagerException::DynamicLibraryManagerException(
        const std::string &libraryName,
        const std::string &errorDetail,
        Cause cause)
: m_cause(cause) {
    if (cause == loadingFailed) {
        m_message = "Failed to load dynamic library: " + libraryName + "\n" +
                errorDetail;
	FTS3_COMMON_LOGGER_LOG(ERR,m_message );
    } else {
        m_message = "Symbol [" + errorDetail + "] not found in dynamic libary:" +
                libraryName;
	FTS3_COMMON_LOGGER_LOG(ERR,m_message );
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



