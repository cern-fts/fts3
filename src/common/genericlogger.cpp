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

/** \file genericlogger.cpp Implementation/tests of GenericLogger class. */

#include "dev.h"
#include "genericlogger.h"

#include <boost/scoped_ptr.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS


FTS3_COMMON_NAMESPACE_START

/* ========================================================================== */

LoggerBase::LoggerBase()
	: _isLogOn( true )
{
    // EMPTY
}

/* -------------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

BOOST_FIXTURE_TEST_CASE (Common__LoggerBase__Constructor, LoggerBase)
{
    BOOST_CHECK_EQUAL (_isLogOn, true );
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ========================================================================== */

LoggerBase::~LoggerBase()
{
    // EMPTY
}

/* ========================================================================== */

const std::string& LoggerBase::_separator()
{
	static std::string s(";");
	return s;
}

/* ========================================================================== */
// Test GenericLogger

#ifdef FTS3_COMPILE_WITH_UNITTEST

/// Injected dependencies: does not call the production functions, only sets flags
/// to record the fact of calling those functions.
struct Common__GenericLogger_TestTraits
{
    /// Test log levels
    enum {DEBUG, ERR, INFO};

    /* --------------------------------------------------------------------- */
    
    static void openLog()
    {
        openCalled = true;
    }
    
    /* ---------------------------------------------------------------------- */

    static void closeLog()
    {
        closeCalled = true;
    }

    /* ---------------------------------------------------------------------- */
    
    static void sysLog(const int aLogLevel, const char* aMessage)
    {
        syslogCalled = true;
        loglevel = aLogLevel;
        message = aMessage;
    }
    /* ---------------------------------------------------------------------- */
    
    static const char* strerror(const int aErrno)
    {
        strerrorCalled = true;
        static std::stringstream str;
        str.str("Error:");
        str << aErrno;
        return str.str().c_str();
    }
    
    /* ---------------------------------------------------------------------- */
    
    static void reset()
    {
        openCalled = false;
        closeCalled = false;
        syslogCalled = false;
        strerrorCalled = false;
        message = NULL;
        loglevel = -1;
    }

    /* ---------------------------------------------------------------------- */
    
    static const std::string initialLogLine()
    {
        return "randomstroLHFUDS";
    }

    /* ---------------------------------------------------------------------- */

    static bool openCalled;
    static bool closeCalled;
    static bool syslogCalled;
    static bool strerrorCalled;
    static const char* message;
    static int loglevel;
};

bool Common__GenericLogger_TestTraits::openCalled = false;
bool Common__GenericLogger_TestTraits::closeCalled = false;
bool Common__GenericLogger_TestTraits::syslogCalled = false;
bool Common__GenericLogger_TestTraits::strerrorCalled = false;
const char* Common__GenericLogger_TestTraits::message = NULL;
int Common__GenericLogger_TestTraits::loglevel = -1;

/* -------------------------------------------------------------------------- */

/// The test class: using Common__GenericLogger_TestTraits as mock.
typedef GenericLogger <Common__GenericLogger_TestTraits>
    GenericLogger_TestClass;

/* -------------------------------------------------------------------------- */

/// Constructor/destructor test scenario.
struct GenericLogger_Constructor_Test
{
    /// Checks if GenericLogger constructor calls openLog
    GenericLogger_Constructor_Test()
        : testObject (new GenericLogger_TestClass)
    {
        BOOST_CHECK (Common__GenericLogger_TestTraits::openCalled);
    }
    
    /// Checks if GenericLogger destructor calls closeLog
    ~GenericLogger_Constructor_Test()
    {
        testObject.reset(NULL);
        BOOST_CHECK (Common__GenericLogger_TestTraits::closeCalled);
        Common__GenericLogger_TestTraits::reset();
    }

    /// Internal test object
    boost::scoped_ptr <GenericLogger_TestClass> testObject;
};

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_Constructor, GenericLogger_TestClass)
{
    // EMPTY 
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_logOnOff, GenericLogger_TestClass)
{
    int logLevel = _actLogLevel;

    // Test log OFF
    setLogOff();
    BOOST_CHECK (Common__GenericLogger_TestTraits::closeCalled);
    BOOST_CHECK ( !_isLogOn );
    Common__GenericLogger_TestTraits::reset();

    // Test log ON
    BOOST_CHECK ( !Common__GenericLogger_TestTraits::openCalled );
    setLogOn();
    BOOST_CHECK (Common__GenericLogger_TestTraits::openCalled);
    BOOST_CHECK ( _isLogOn );

    // Check if log level has not been changed
    BOOST_CHECK_EQUAL (_actLogLevel, logLevel);
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_do_commit_if_logon, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    BOOST_CHECK ( ! Common__GenericLogger_TestTraits::syslogCalled );
    setLogOn();
    _commit();
    BOOST_CHECK ( Common__GenericLogger_TestTraits::syslogCalled );
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_no_commit_if_logoff, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    BOOST_CHECK ( ! Common__GenericLogger_TestTraits::syslogCalled );
    setLogOff();
    _commit();
    BOOST_CHECK ( ! Common__GenericLogger_TestTraits::syslogCalled );
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_no_commit_if_empty, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    _logLine.str ("");
    BOOST_CHECK ( ! Common__GenericLogger_TestTraits::syslogCalled );
    setLogOn();
    _commit();
    BOOST_CHECK ( ! Common__GenericLogger_TestTraits::syslogCalled );
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_logline_cleared_after_commit, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    setLogOn();
    _commit();
    BOOST_CHECK ( _logLine.str().empty() );
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_commit_modifier, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    GenericLogger_TestClass& returnedClass = commit(*this);
    // Check if the returned class contains the same data than this one
    BOOST_CHECK_EQUAL (this, &returnedClass);
    // Check if syslog was really called
    BOOST_CHECK ( Common__GenericLogger_TestTraits::syslogCalled );
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_newLog_general, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    // Anything but debug must be here, otherwise, we have troubles with the NULL 
    // pointers...
    std::stringstream f_str;
    f_str << "thread:" << ThreadTraits::get_id() << _separator();
    // Check test initial conditions. Actual log level cannot be debug level
    // (we test the non-debug case, and we must ensure that it really changed
    // to a new value after newlog.
    _actLogLevel = Common__GenericLogger_TestTraits::INFO;
    BOOST_CHECK_NE (_logLine, f_str);
    // Start real test...
    GenericLogger_TestClass& returnedClass = 
        newLog<Common__GenericLogger_TestTraits::ERR> (NULL, NULL, 0);
    // Check if syslog was written with old content
    BOOST_CHECK ( Common__GenericLogger_TestTraits::syslogCalled );
    // New log level stored?
    BOOST_CHECK_EQUAL (_actLogLevel, static_cast<int>(Common__GenericLogger_TestTraits::ERR));
    // Initial log line is the required one?
    BOOST_CHECK_EQUAL (_logLine.str(), f_str.str());
    // Check if the returned class contains the same data than this one
    BOOST_CHECK_EQUAL (this, &returnedClass);
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_newLog_debug, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    const char* f_file = "file";
    const char* f_function = "function";
    const int f_lineno = 12;

    std::stringstream f_str;

    f_str << "thread:" << ThreadTraits::get_id() << _separator() << f_file << _separator() 
        << f_function << _separator() << std::dec << f_lineno << _separator();

    newLog<Common__GenericLogger_TestTraits::DEBUG> (f_file, f_function, f_lineno);

    // Initial log line is the required one?
    BOOST_CHECK_EQUAL (_logLine.str(), f_str.str());
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_modifier, GenericLogger_TestClass)
{
    Common__GenericLogger_TestTraits::reset();
    // Execute "commit" in modifoer style, and check if syslog was written. It 
    // proves that the modofier was executed.
    *this << commit;
    BOOST_CHECK ( Common__GenericLogger_TestTraits::syslogCalled );
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_stream_write, GenericLogger_TestClass)
{
    // Empty everything...
    _commit();
    setLogOn();
    Common__GenericLogger_TestTraits::reset();
    const std::string f_str ("randomdsfsdoj");
    *this << f_str;
    BOOST_CHECK_EQUAL (_logLine.str(), f_str);

    // No logging when log is off
    setLogOff();
    _logLine.str("");
    *this << f_str;
    BOOST_CHECK (_logLine.str().empty()); 
}

/* -------------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE (Common__GenericLogger_addErr, GenericLogger_TestClass)
{
    // Test setup
    _logLine.str("");
    Common__GenericLogger_TestTraits::reset();
    const int f_errn = 3;
    const std::string f_str = Common__GenericLogger_TestTraits::strerror(f_errn);
    errno = f_errn;
    // Do the real work...
    *this << addErr;
    // The checks
    BOOST_CHECK ( Common__GenericLogger_TestTraits::strerrorCalled );
    BOOST_CHECK_EQUAL (_logLine.str(), f_str);
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_COMMON_NAMESPACE_END

