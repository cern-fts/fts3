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

#include "DynamicLibraryManager.h"
#include "error.h"
#include <dlfcn.h>
#include <unistd.h>
#include <iostream>
#include <memory>


using namespace fts3::common; 

DynamicLibraryManager::DynamicLibraryManager(const std::string &libraryFileName)
    : m_libraryHandle(NULL)
    , m_libraryName(libraryFileName)
{
    loadLibrary(libraryFileName);
}

DynamicLibraryManager::~DynamicLibraryManager()
{
    releaseLibrary();
}

DynamicLibraryManager::Symbol
DynamicLibraryManager::findSymbol(const std::string &symbol)
{
    try
        {
            Symbol symbolPointer = doFindSymbol(symbol);
            if (symbolPointer != NULL)
                return symbolPointer;
        }
    catch (...)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Symbol cannot be found in the database plugin library"));
        }

    return NULL; // keep compiler happy
}

std::string
DynamicLibraryManager::getLastError(void)
{
    return ::dlerror();
}

void
DynamicLibraryManager::loadLibrary(const std::string &libraryName)
{
    try
        {
            releaseLibrary();
            m_libraryHandle = doLoadLibrary(libraryName);
            if (m_libraryHandle != NULL)
                return;
        }
    catch (...)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Failed to load database plugin library"));
        }
}

void
DynamicLibraryManager::releaseLibrary()
{
    if (m_libraryHandle != NULL)
        {
            doReleaseLibrary();
            m_libraryHandle = NULL;
        }
}

DynamicLibraryManager::LibraryHandle
DynamicLibraryManager::doLoadLibrary(const std::string &libraryName)
{
    return ::dlopen(libraryName.c_str(), RTLD_LAZY);
}

void DynamicLibraryManager::doReleaseLibrary()
{
    ::dlclose(m_libraryHandle);
}

DynamicLibraryManager::Symbol DynamicLibraryManager::doFindSymbol(const std::string &symbol)
{
    return ::dlsym(m_libraryHandle, symbol.c_str());
}


