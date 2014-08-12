/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * BulkSubmissionParser.h
 *
 *  Created on: Feb 11, 2013
 *      Author: Michał Simon
 */

#ifndef BULKSUBMISSIONPARSER_H_
#define BULKSUBMISSIONPARSER_H_

#include "File.h"

#include "exception/cli_exception.h"

#include "common/error.h"

#include <vector>
#include <string>
#include <set>
#include <fstream>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>


namespace fts3
{
namespace cli
{

using namespace fts3::common;

using namespace std;
using namespace boost;
using namespace boost::property_tree;

class BulkSubmissionParser
{

public:

    BulkSubmissionParser(istream& ifs);
    virtual ~BulkSubmissionParser();

    /**
     * Gets the files from the bulk submission
     *
     * @return vector of files that are being a part of a transfer job
     */
    vector<File> getFiles();

private:

    /**
     * checks if the given node is an array or not
     *
     * @return true if the node under the given path is an array, false otherwise
     */
    bool isArray(ptree& item, string path);

    /**
     * Gets the value under the given path
     *
     * @param T - the type of the return value
     *
     * @return a value of the node if it exists, an uninitialized optional otherwise
     */
    template<typename T>
    optional<T> get(ptree& item, string path);

    /**
     * Gets the metadata
     *
     * @return a string representing the metadata, or an uninitialized optional if the metada have not been specified
     */
    optional<string> getMetadata(ptree& item);

    /**
     * Parses the bulk submission file. If the file is not valid an exception is thrown.
     */
    void parse();

    /**
     * Parses a single item in the 'Files' array and puts the parsed item into the files vector
     *
     * @param item - the item from 'Files' array
     */
    void parse_item(ptree& item);

    /**
     * Validates a single item in the bulk submission file. If the file is not valid an exception is thrown.
     */
    void validate(ptree& item);

    /// the root of the JSON document
    ptree pt;

    /// the files that were described in the bulk submission file
    vector<File> files;

    /// token that are allowed for a file description
    static const set<string> file_tokens;
};

template <typename T>
optional<T> BulkSubmissionParser::get(ptree& item, string path)
{
    try
        {
            // return the value under the given path
            return item.get_optional<T>(path);
            // in case of exception handle them: ...
        }
    catch (ptree_bad_path& ex)
        {
            // if the value does not exist throw an exception
            throw cli_exception("The " + path + " has to be specified!");
        }
    catch (ptree_bad_data& ex)
        {
            // if the type of the value is wrong throw an exception
            throw cli_exception("Wrong value type of " + path);
        }
}

template <>
inline optional< vector<string> > BulkSubmissionParser::get< vector<string> >(ptree& item, string path)
{
    // check if the value was specified
    optional<ptree&> value = item.get_child_optional(path);
    if (!value.is_initialized())
        {
            // the vector member was not specified in the configuration
            return optional< vector<string> >();
        }
    ptree& array = value.get();
    // check if the node has a value,
    // accordingly to boost it should be empty if array syntax was used in JSON
    string wrong = array.get_value<string>();
    if (!wrong.empty())
        {
            throw cli_exception("Wrong value: '" + wrong + "'");
        }
    // iterate over the nodes
    vector<string> ret;
    ptree::iterator it;
    for (it = array.begin(); it != array.end(); it++)
        {
            pair<string, ptree> v = *it;
            // check if the node has a name,
            // accordingly to boost it should be empty if object weren't
            // members of the array (our case)
            if (!v.first.empty())
                {
                    throw cli_exception("An array was expected, instead an object was found (at '" + path + "', name: '" + v.first + "')");
                }
            // check if the node has children, it should only have a value!
            if (!v.second.empty())
                {
                    throw cli_exception("Unexpected object in array '" + path + "' (only a list of values was expected)");
                }
            // put the value into the vector
            ret.push_back(v.second.get_value<string>());
        }

    return ret;
}

} /* namespace cli */
} /* namespace fts3 */
#endif /* BULKSUBMISSIONPARSER_H_ */
