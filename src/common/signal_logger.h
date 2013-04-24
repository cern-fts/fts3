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


#pragma once

#include <string>
#include <map>
#include <sys/types.h>
#include <syslog.h>
#include <signal.h>
#include <fstream>


// -----------------------------------------------------------------------------
// Signal logging
// -----------------------------------------------------------------------------
#define REGISTER_SIGNAL(signum) SignalLogger::instance().registerSignal(signum,#signum)

/**
 * SignalLogger class
 */
class SignalLogger {
public:

    /**
     * instance
     * return singleton instance
     */
    static SignalLogger& instance(){
        static SignalLogger s_instance;
        return s_instance;
    }

    /**
     * registerSignal
     * register a handler to log given signal
     * @param signum [IN] signal number
     * @param signame [IN] signal name
     */
    void registerSignal(const int signum,const std::string& signame);
    
    /*void setLogStream(std::ostream& logStream){
    	logStream_ = &logStream;
    }
	*/
	
    static void log_stack(int sig);

    /**
     * destructor
     */
    ~SignalLogger();

private:

    /**
     * SignalInfo
     * helper class to register and deregister signal handlers
     */
    class SignalInfo {
    public:
        /**
         * constructor
         * @param signum [IN] signal number
         * @param signame [IN] signal name
         */
        SignalInfo(int signum,const std::string& signame);
        /**
         * deregister
         * deregister handler
         */
        void deregister();
        /**
         * signame
         * @return signal name
         */
        const std::string& signame() const{
            return m_signame;
        }
        /**
         * destructor
         */
        ~SignalInfo();
    private:
        // signal number
        int m_signum;
        // signal name
        std::string m_signame;
        // flag indicating if signal handler is registered
        bool m_set;
        // old sigaction struct
        struct sigaction m_old;
    };

    /**
     * handleSignal
     * actual signal handler function (registered by SignalInfo objects)
     */
    static void handleSignal(int signum);

    /**
     * logSignal
     * actual signal logging
     */
    void logSignal(int signum);

    /**
     * constructor
     */
    SignalLogger(){
    }

    // map of SignalInfo objects
    typedef std::map<int,SignalInfo *> SignalInfoMap;
    SignalInfoMap m_map;
    //std::ostream* logStream_;
};

