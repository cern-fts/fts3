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

/** \file serverconfig.h Define FTS3 server configuration. */

#pragma once

#include "config_dev.h"
#include <map>
#include <boost/lexical_cast.hpp>

FTS3_CONFIG_NAMESPACE_START

/* ========================================================================== */

/** \brief Class representing server configuration. Server configuration read once,
 * when the server starts. It provides read-only singleton access. */
class ServerConfig 
{
public:
    /// Constructor
    ServerConfig();

    /* ---------------------------------------------------------------------- */

    /// Destructor
    virtual ~ServerConfig();

    /* ---------------------------------------------------------------------- */
    
    /** Read the configurations */
    void read
    (
        int argc, /**< The command line arguments (from main) */
        char** argv /**< The command line arguments (from main) */
    );

    /* ---------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST 
    /** Read the configurations from config file only */
    void read
    (
        const std::string& aFileName /**< Config file name */
    );
#endif // FTS3_COMPILE_WITH_UNITTEST

    /* ---------------------------------------------------------------------- */
    
    /** General getter of an option. It returns the option, converted to the 
     * desired type. Throws exception if the option is not found. */
    template <class RET> RET get
    (
        const std::string& aVariable /**< A config variable name. */
    )
    {
    	const std::string& str = _get_str(aVariable);
    	return boost::lexical_cast<RET>(str);
    }

protected:

    /** Type of the internal store of config variables. */
    typedef std::map<std::string, std::string> _t_vars;

    /* ---------------------------------------------------------------------- */
    
    /** Return the variable value as a string. */
    const std::string& _get_str
    (
        const std::string& aVariable  /**< A config variable name. */
    );

    /* ---------------------------------------------------------------------- */
    
    /** Read the configurations - using injected reader type. */
    template <typename READER_TYPE> 
    void _read
    (
        int argc, /**< The command line arguments (from main) */
        char** argv /**< The command line arguments (from main) */
    )
    {
         READER_TYPE reader;
        _vars = reader(argc, argv);
    }

    /* ---------------------------------------------------------------------- */
    
    /** Read the configurations from config file only - using injected reader.*/
    template <typename READER_TYPE> 
    void _read
    (
        const std::string& aFileName /**< Config file name */
    )
    {
         READER_TYPE reader;
        _vars = reader(aFileName);
    }

    /* ---------------------------------------------------------------------- */
    
    /** The internal store of config variables. */
    _t_vars _vars;
};

/* ========================================================================== */

/** Singleton access to teh server config. */
inline ServerConfig& theServerConfig()
{
    static ServerConfig e;
    return e;
}

FTS3_CONFIG_NAMESPACE_END

