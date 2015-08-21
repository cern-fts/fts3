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

#include "DynamicLibraryManagerException.h"
#include "logger.h"


using namespace FTS3_COMMON_NAMESPACE;

DynamicLibraryManagerException::DynamicLibraryManagerException(
    const std::string &libraryName,
    const std::string &errorDetail,
    Cause cause)
    : m_cause(cause)
{
    if (cause == loadingFailed)
        {
            m_message = "Failed to load dynamic library: " + libraryName + "\n" +
                        errorDetail;
            FTS3_COMMON_LOGGER_LOG(ERR,m_message );
        }
    else
        {
            m_message = "Symbol [" + errorDetail + "] not found in dynamic libary:" +
                        libraryName;
            FTS3_COMMON_LOGGER_LOG(ERR,m_message );
        }
}

DynamicLibraryManagerException::Cause
DynamicLibraryManagerException::getCause() const
{
    return m_cause;
}

const char *
DynamicLibraryManagerException::what() const throw ()
{
    return m_message.c_str();
}



