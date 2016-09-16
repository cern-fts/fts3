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

#ifndef JSONOUTPUT_H_
#define JSONOUTPUT_H_

#include <ostream>
#include <string>
#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "exception/cli_exception.h"
#include "exception/bad_option.h"

namespace fts3
{

namespace cli
{

/**
 * A class responsible for printing messages in JSON format
 */
class JsonOutput
{

public:

    /// Constructor
    JsonOutput(std::ostream& out) : out (&out) {};

    /**
     * Prints the JSON message to the given output stream
     */
    virtual ~JsonOutput();

    /**
     * Sets the output stream
     *
     * @param ostr : output stream
     */
    void setOutputStream(std::ostream & ostr)
    {
        out = &ostr;
    }

    /**
     * Puts given message in the given node of JSON output
     */
    template <typename T>
    void print(std::string const & path, T const & msg)
    {
        json_out.put(path, msg);
    }

    /**
     * Puts the CLI exception into respective node of JSON output
     */
    void print(cli_exception const & ex);

    /**
     * Puts the standard exception into respective node of JSON output
     */
    void print(std::exception const & ex);

    /**
     * Puts the container object into given node as a array of key-value pairs
     */
    template <typename CONTAINER>
    void printArray(std::string const & path, CONTAINER const & object)
    {
        printArray(path, to_ptree(object));
    }

    /**
     * Puts the value into given node as a array member
     */
    void printArray(std::string const & path, std::string const & value);

    /**
     * Puts the value into given node as a array member
     */
    void printArray(std::string const & path, boost::property_tree::ptree const & obj);

private:

    /// converts all the pairs from a given container to ptree
    template <typename CONTAINER>
    static boost::property_tree::ptree to_ptree(CONTAINER const & values)
    {
        boost::property_tree::ptree pt;

        typename CONTAINER::const_iterator it;
        for (it = values.begin(); it != values.end(); ++it)
            {
                pt.put(it->first, it->second);
            }

        return pt;
    }


    /// the ptree used to store the JSON object
    boost::property_tree::ptree json_out;
    /// the output stream
    std::ostream * out;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* JSONOUTPUT_H_ */
