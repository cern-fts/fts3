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

#ifndef BAD_OPTION_H_
#define BAD_OPTION_H_


#include <exception>
#include <string>

#include "cli_exception.h"

namespace fts3
{
namespace cli
{

/**
 * A Exception class used man a program option was wrongly used
 */
class bad_option : public cli_exception
{

public:
    /**
     * Constructor
     */
    bad_option(std::string const & opt, std::string const & msg) : cli_exception(msg), opt(opt), full_msg(opt + ": " + msg) {}

    /**
     * returns the error message
     */
    virtual char const * what() const
#if __cplusplus >= 199711L
    noexcept (true)
#endif
    {
        return full_msg.c_str();
    }

    /**
     * returns the error message that should be included in the JSON output
     */
    virtual pt::ptree const json_obj() const
    {
        pt::ptree obj;
        obj.put(opt, msg);

        return obj;
    }

private:
    /// program option name
    std::string opt;
    /// combined error message
    std::string full_msg;
};

}
}

#endif /* BAD_OPTION_H_ */
