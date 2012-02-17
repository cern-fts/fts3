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

#ifndef FTS3_CONFIG_INTERFACE_SERVERCONFIG_H
#define FTS3_CONFIG_INTERFACE_SERVERCONFIG_H

/** \file iserverconfig.h Interface for accessing server configuration data. */

#include "config_dev.h"

#include <boost/lexical_cast.hpp>

FTS3_CONFIG_NAMESPACE_START

/* ========================================================================== */

/** Class representing the server configuration data. Provides read-only access
 * for the other system components. Should be used as Singleton. */
class  IServerConfig 
{
public:
    /** Destructor. */
    virtual ~IServerConfig() {};

    /** Retrieve configuration data, in a required representation (type). 
     * If the conversion does not work, it results a compile-time error. 
     * If the variable cannot be found, it throws exception. */
    template <class RET> RET get
    (
        const std::string& aVariable /**< Config variable name. */
    ) 
    {
    	const std::string& str = _get_str(aVariable);
    	return boost::lexical_cast<RET>(str);
    } 

protected:   
    /** Implementation of variable retrieval logic. It returns config value 
     * as a string. "get" method will convert it. */
    virtual const std::string& _get_str
    (   
        const std::string& aVariable /**< Config variable name. */
    ) = 0;
};

/* -------------------------------------------------------------------------- */

/** Singleton access to the server config data. Returns a reference to the actual 
 * config retrieval implementation. */
IServerConfig& theServerConfig();

/* ========================================================================== */

#ifdef FTS3_COMPILE_WITH_UNITTESTS

#define TEST_NAMES (test)

class IServerConfigTest : 
    public CPPUNIT_NS::TestFixture, 
    public IServerConfig
{
    CPPUNIT_TEST_SUITE (IServerConfigTest);
    BOOST_PP_SEQ_FOR_EACH(TESTBASE_TESTDEF, ~, TEST_NAMES);
    CPPUNIT_TEST_SUITE_END ();

protected:
    BOOST_PP_SEQ_FOR_EACH(TESTBASE_METHODEF, ~, TEST_NAMES)
};

#undef TEST_NAMES

#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_CONFIG_NAMESPACE_END

#endif // FTS3_CONFIG_INTERFACE_SERVERCONFIG_H

