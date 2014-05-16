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

    /**
     * Creates a single instance of JsonOutput
     */
    static void create(std::ostream& out = std::cout);

    /**
     * Puts given message in the given node of JSON output
     */
    static void print(std::string const path, std::string const msg);

    /**
     * Puts the CLI exception into respective node of JSON output
     */
    static void print(cli_exception const & ex);

    /**
     * Puts the map object into given node as a array of key-value pairs
     */
    static void printArray(std::string const path, std::map<std::string, std::string> const & object);

    /**
     * Puts the value into given node as a array member
     */
    static void printArray(std::string const path, std::string const value);

    /**
     * Puts the value into given node as a array member
     */
    static void printArray(std::string const path, pt::ptree const & obj);

    /**
     * Prints the JSON message to the given output stream
     */
    virtual ~JsonOutput();

private:

    /// Private constructor
    JsonOutput(std::ostream& out) : out (out) {};
    /// Copy constructor (not implemented)
    JsonOutput(JsonOutput const&);
    /// Assignment operator (not implemented)
    void operator=(JsonOutput const&);

    /// converts all the pairs from map to ptree
    static pt::ptree to_ptree(std::map<std::string, std::string> const & values);

    /// the singleton instance
    static boost::scoped_ptr<JsonOutput> instance;

    /// the ptree used to store the JSON object
    pt::ptree json_out;
    /// the output stream
    std::ostream& out;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* JSONOUTPUT_H_ */
