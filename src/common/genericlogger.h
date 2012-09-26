/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

/** \file logger.h Interface of FTS3 logger component. */

#pragma once

#include "common_dev.h"
#include "monitorobject.h"
#include <iostream>
#include <stdio.h>


FTS3_COMMON_NAMESPACE_START

/* ========================================================================== */

/** \brief Base of each Logger-s for FTS3 (common logic). */
class LoggerBase : public MonitorObject
{
protected:
    /// Constructor.
	LoggerBase();

    /* ---------------------------------------------------------------------- */

    /// Destructor.
	virtual ~LoggerBase();

    /* ---------------------------------------------------------------------- */

    /// Separator for message parts. In case you want to filter by column.
	static const std::string& _separator();

    /* ====================================================================== */

    /// true: log lines are written, false: no log is written.
    bool _isLogOn;
};

/* ========================================================================== */

/** \file Generic Logger class. Supports dependency injection, etc.
 * You can redefine the behaviour of Logger by using different Traits class.
 * Good for testing as well, to inject dependencies. */
template <typename Traits>
class GenericLogger : public LoggerBase
{
public:
    /// The logger tratis type
    typedef Traits type_traits;


    /* ---------------------------------------------------------------------- */

    /// Constructor
    GenericLogger () :
        LoggerBase(),
        _logLine (Traits::initialLogLine()),
	    _actLogLevel (static_cast<int>(Traits::INFO))
    {
        
    }
    
    std::string timestamp() {
    		std::string timestapStr("");
    		time_t ltime; /* calendar time */
    		ltime = time(NULL); /* get current cal time */
    		timestapStr = asctime(localtime(&ltime));
    		timestapStr.erase(timestapStr.end() - 1);
    		return timestapStr + " ";
	}

    std::string logLevelStringRepresentation(int loglevel) {
		switch (loglevel) {
  			case 0:
				return std::string("EMERG   ");
  			case 1:
				return std::string("DEBUG   ");			
  			case 2:
				return std::string("WARNING ");						
  			case 3:
				return std::string("INFO    ");						
  			case 4:
				return std::string("ALERT   ");						
  			case 5:
				return std::string("CRIT    ");						
  			case 6:							
				return std::string("ERR     ");								   				 
  			case 7:	 			
				return std::string("NOTICE  ");						
  			default:
				return std::string("INFO    ");						
    				
  		}
	}


    /* ---------------------------------------------------------------------- */

    /// Destructor
    virtual ~GenericLogger ()
    {

    }

    /* ---------------------------------------------------------------------- */

    /// Switch logging on. Log messages will be displayed.
	GenericLogger& setLogOn()
    {
    FTS3_COMMON_MONITOR_START_CRITICAL
	    _isLogOn = true;
        //Traits::openLog();
    FTS3_COMMON_MONITOR_END_CRITICAL
	    return *this;
    }

    /* ---------------------------------------------------------------------- */

    /// Switch log messages off. No log messages are displayed.
    GenericLogger& setLogOff()
    {
    FTS3_COMMON_MONITOR_START_CRITICAL
	    _isLogOn = false;
        //Traits::closeLog();
    FTS3_COMMON_MONITOR_END_CRITICAL
	    return *this;
    }

    /* ---------------------------------------------------------------------- */

    /// Commits (writes) the actual log line.
	void _commit()
    {
    FTS3_COMMON_MONITOR_START_CRITICAL
        if ( _isLogOn &&
             ! _logLine.str().empty())
        {
	   fprintf(stderr, "%s\n",_logLine.str().c_str());
	   fflush(stderr);
	   fprintf(stdout, "%s\n",_logLine.str().c_str());
	   fflush(stdout);
        }

        _logLine.str("");
    FTS3_COMMON_MONITOR_END_CRITICAL
    }

    /* ---------------------------------------------------------------------- */

    /// Start a new log message. But this is not the recommended way,
    /// use FTS3_COMMON_LOGGER_NEWLOG. It calls this method, but adds
    /// proper debug information. Th einteher LOGLEVEL template parameter
    /// is understood by the logger traits.
	template <int LOGLEVEL>
    GenericLogger& newLog
    (
        const char* aFile, /**< Actual source file */
        const char* aFunc, /**< Actual function that is logging */
        const int aLineNo /**< Line number where the log file was written. */
    )
    {
	    commit(*this);
    FTS3_COMMON_MONITOR_START_CRITICAL
	    _actLogLevel = LOGLEVEL;
        _logLine << logLevelStringRepresentation(_actLogLevel) << timestamp() << _separator();
        bool isDebug = (Traits::ERR == _actLogLevel);

	    if (isDebug)
        {
	        _logLine << aFile << _separator() << aFunc << _separator() << std::dec << aLineNo << _separator();
	    }
    FTS3_COMMON_MONITOR_END_CRITICAL

        return *this;
    }

    /* ---------------------------------------------------------------------- */

    /// Handles stream modifiers as well. So, you can do things like
    ///
    /// \code
    /// ... << message << commit()
    /// \endcode
	GenericLogger& operator <<
    (
        GenericLogger& (*aFunc)(GenericLogger &aLogger) /**< The functor name */
    )
    {
    	return (aFunc)(*this);
    }

    /* ---------------------------------------------------------------------- */
    template <typename T>
    GenericLogger& operator << (const T& aSrc)
    {
    FTS3_COMMON_MONITOR_START_CRITICAL
	    if (_isLogOn)
        {
		    _logLine << aSrc;
        }
    FTS3_COMMON_MONITOR_END_CRITICAL

        return *this;
    }

    /* ====================================================================== */

    /// The source code line where the logging started (i.e. where the
    /// FTS3_COMMON_LOGGER_NEWLOG macro was called).
    std::stringstream  _logLine;

    /* ---------------------------------------------------------------------- */

    /// Actual log level (see syslog).
    int _actLogLevel;
};

/// Stream modifier: writes actual system error into the log stream.
template <typename Traits>
GenericLogger<Traits> & commit
(
    GenericLogger<Traits>& aLogger /**< The logger object to be modified. */
)
{
	aLogger._commit();
	return aLogger;
}

/* -------------------------------------------------------------------------- */

/// Stream modifier: writes actual system error into the log stream.
template <typename Traits>
GenericLogger<Traits> & addErr
(
    GenericLogger<Traits>& aLogger /**< The logger object to be modified. */
)
{
	aLogger << Traits::strerror(errno);
	return aLogger;
}

FTS3_COMMON_NAMESPACE_END

