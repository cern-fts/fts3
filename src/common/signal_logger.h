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

