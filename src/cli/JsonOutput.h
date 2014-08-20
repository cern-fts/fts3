/*
 * JsonOutput.h
 *
 *  Created on: 8 May 2014
 *      Author: simonm
 */

#ifndef JSONOUTPUT_H_
#define JSONOUTPUT_H_

#include <ostream>
#include <string>
#include <map>

#include <boost/scoped_ptr.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "exception/cli_exception.h"
#include "exception/gsoap_error.h"
#include "exception/bad_option.h"

namespace pt = boost::property_tree;

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
    void print(std::string const & path, std::string const & msg);

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
    void printArray(std::string const & path, pt::ptree const & obj);

private:

    /// converts all the pairs from a given container to ptree
    template <typename CONTAINER>
    static pt::ptree to_ptree(CONTAINER const & values)
    {
        pt::ptree pt;

        typename CONTAINER::const_iterator it;
        for (it = values.begin(); it != values.end(); ++it)
            {
                pt.put(it->first, it->second);
            }

        return pt;
    }


    /// the ptree used to store the JSON object
    pt::ptree json_out;
    /// the output stream
    std::ostream * out;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* JSONOUTPUT_H_ */
