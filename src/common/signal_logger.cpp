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


#include "signal_logger.h"
#include <unistd.h>
#include <execinfo.h>
#include <stdlib.h>
#include <iostream>


std::string stackTrace("");

/**
 * log_stack
 * log program stack as warnings
 */
void SignalLogger::log_stack(int sig)
{
    if(sig ==15)  //SIGTERM
        {
            raise(SIGINT);
        }
    if(sig ==10)  //SIGUSR1
        {
            raise(SIGUSR1);
        }
    else
        {
            signal(sig, SIG_DFL);
            const int stack_size = 25;
            void * array[stack_size]= {0};
            int nSize = backtrace(array, stack_size);
            char ** symbols = backtrace_symbols(array, nSize);
            for (register int i = 0; i < nSize; ++i)
                {
                    if(symbols && symbols[i])
                        {
                            stackTrace+=std::string(symbols[i]) + '\n';
                        }
                }
            if(symbols)
                {
                    free(symbols);
                }
            kill(getpid(), SIGINT); //SIGINT
        }
}

//------------------------------------------------------------------------------
// SignalLogger
//------------------------------------------------------------------------------

/**
 * registerSignal
 * register a handler to log given signal
 * @param signum [IN] signal number
 * @param signame [IN] signal name
 */
void SignalLogger::registerSignal(const int signum,const std::string& signame)
{
    SignalInfoMap::const_iterator it = m_map.find(signum);
    if (m_map.end() == it)
        {
            m_map.insert(std::make_pair<int,SignalInfo *>(signum,new SignalInfo(signum,signame)));
        }
}

/**
 * destructor
 */
SignalLogger::~SignalLogger()
{
    for (SignalInfoMap::iterator it = m_map.begin(); it != m_map.end(); ++it)
        {
            SignalInfo * info = it->second;
            delete info;
        }
    m_map.clear();
}

/**
 * constructor
 * @param signum [IN] signal number
 * @param signame [IN] signal name
 */
SignalLogger::SignalInfo::SignalInfo(int signum,const std::string& signame) :
    m_signum(signum),
    m_signame(signame),
    m_set(true)
{
    struct sigaction sa;
    sa.sa_handler = SignalLogger::handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(m_signum, &sa, &m_old);
}

/**
 * deregister
 * deregister handler
 */
void SignalLogger::SignalInfo::deregister()
{
    if (m_set)
        {
            m_set = false;
            sigaction(m_signum, &m_old, 0);
        }
}

/**
 * destructor
 */
SignalLogger::SignalInfo::~SignalInfo()
{
    deregister();
}

/**
 * handleSignal
 * actual signal handler function (registered by SignalInfo objects)
 */
void SignalLogger::handleSignal(int signum)
{
    instance().logSignal(signum);
}

/**
 * logSignal
 * actual signal logging
 */
void SignalLogger::logSignal(int signum)
{

    SignalInfoMap::iterator it = m_map.find(signum);
    if (m_map.end() != it)
        {
            SignalInfo * info = it->second;
            info->deregister();
            log_stack(signum);
        }
}
