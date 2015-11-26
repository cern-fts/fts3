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

#ifndef CLI_EXCEPTION_H_
#define CLI_EXCEPTION_H_

#include <exception>
#include <string>

#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace fts3
{

namespace cli
{

/**
 * Generic CLI exception
 */
class cli_exception
{

public:
    /**
     * Constructor.
     *
     * @param msg - exception message
     */
    cli_exception(std::string const & msg) : msg(msg) {}

    /**
     * Destructor
     */
    virtual ~cli_exception() {};

    /**
     * returns the error message
     */
    virtual char const * what() const
#if __cplusplus >= 199711L
    noexcept (true)
#endif
    {
        return msg.c_str();
    }

    /**
     * returns the error message that should be included in the JSON output
     */
    virtual pt::ptree const json_obj() const
    {
        pt::ptree obj;
        obj.put("message", msg);

        return obj;
    }

    /**
     * returns the node name of the JSON output where the respective error message should be included
     */
    virtual std::string const json_node() const
    {
        return "error";
    }

   /**
    * should a cli try falling back to another protocol after getting this exception
    */
    virtual bool tryFallback() const
    {
        return false;
    }

protected:
    /// the exception message
    std::string msg;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* CLI_EXCEPTION_H_ */
