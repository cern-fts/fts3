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

#ifndef CONFIGURATIONPARSER_H_
#define CONFIGURATIONPARSER_H_

#include "common/error.h"

#include <set>
#include <map>
#include <vector>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>

namespace fts3
{
namespace common
{

using namespace boost;
using namespace boost::property_tree;


/**
 * CfgParser is a wrapper class for boost ptree.
 *
 * It allows to parse a configuration std::string in JSON format.
 * After parsing a given value can be accessed using a specific path
 * e.g.: 'share.in' will provide access to the value inbound
 * value specified in share JSON object
 * (see https://svnweb.cern.ch/trac/fts3/wiki/UserGuide for the JSON schema)
 *
 * Access to JSON array is supported.
 *
 */
class CfgParser
{

public:

    enum CfgType
    {
        NOT_A_CFG,
        STANDALONE_SE_CFG,
        STANDALONE_GR_CFG,
        SE_PAIR_CFG,
        GR_PAIR_CFG,
        SHARE_ONLY_CFG,
        ACTIVITY_SHARE_CFG
    };

    /**
     * Constructor.
     * Parses the given JSON configuration.
     *
     * @param configuration - the configuration in JSON format
     */
    CfgParser(std::string configuration);

    /**
     * Destructor.
     */
    virtual ~CfgParser();

    /**
     * Gets the specific value from the JSON object
     *
     * Please not that in case of a std::map<std::string, int> negative values are not allowed, they will result
     * in throwing an exception. However, a 'auto' value is allowed that will be indicated by -1 value!
     *
     * @param path - path that specifies the value, e.g. 'share.in'
     * @param T - the expected type of the value (int, std::string, bool, vector and std::map are supported)
     *
     * @return the value
     */
    template <typename T>
    T get(std::string path);

    /**
     * Gets the specific value for an optional object in JSON configuration
     *
     * @param path - path that specifies the value, e.g. 'share.in'
     *
     * @return an instance of optional which holds the value
     */
    optional<std::string> get_opt(std::string path);

    /**
     * Checks if the given property was std::set to 'auto'
     *
     * @return true if the property of interest was std::set to 'auto', false otherwise
     */
    bool isAuto(std::string path);

    /**
     *
     */
    CfgType getCfgType()
    {
        return type;
    }

    /// the auto value of a share
    static const std::string auto_value;

private:

    CfgType type;

    /**
     * Validates the ptree object. Checks if the configuration format is OK.
     *
     * @param pt - the ptree that has to be validated
     * @param allowed - a collection of fileds name in the cfg JASON
     * 					characteristic for a given type of configuration
     * @param path - the path in main ptree (by default root)
     *
     * @return true if it's a configuration of a given type, false otherwise
     */
    bool validate(ptree pt, std::map< std::string, std::set <std::string> > allowed, std::string path = std::string());

    /// The object that contains the parsed configuration
    ptree pt;

    /// the tokens used in standalone SE configuration
    static const std::map<std::string, std::set <std::string> > standaloneSeCfgTokens;
    /// the tokens used in standalone SE groupconfiguration
    static const std::map<std::string, std::set <std::string> > standaloneGrCfgTokens;
    /// the tokens used in se std::pair configuration
    static const std::map<std::string, std::set <std::string> > sePairCfgTokens;
    /// the tokens used in se group std::pair configuration
    static const std::map<std::string, std::set <std::string> > grPairCfgTokens;
    /// the tokens used  in a share-only configuration
    static const std::map<std::string, std::set <std::string> > shareOnlyCfgTokens;
    /// the tokens used  in a share-only configuration
    static const std::map<std::string, std::set <std::string> > activityShareCfgTokens;
    /// all the allowed tokens
    static const std::set<std::string> allTokens;

    /// initializes allowed JSON members for se config
    static const std::map< std::string, std::set <std::string> > initStandaloneSeCfgTokens();
    /// initializes allowed JSON members for se group config
    static const std::map< std::string, std::set <std::string> > initStandaloneGrCfgTokens();
    /// initializes allowed JSON members for se std::pair
    static const std::map< std::string, std::set <std::string> > initSePairCfgTokens();
    /// initializes allowed JSON members for se group std::pair
    static const std::map< std::string, std::set <std::string> > initGrPairCfgTokens();
    /// initializes allowed JSON members for share-only configuration
    static const std::map<std::string, std::set <std::string> > initShareOnlyCfgTokens();
    /// initializes allowed JSON members for activity share configuration
    static const std::map<std::string, std::set <std::string> > initActivityShareCfgTokens();
};

template <typename T>
T CfgParser::get(std::string path)
{

    T v;
    try
        {

            v = pt.get<T>(path);

        }
    catch (ptree_bad_path& ex)
        {
            throw Err_Custom("The " + path + " has to be specified!");

        }
    catch (ptree_bad_data& ex)
        {
            // if the type of the value is wrong throw an exception
            throw Err_Custom("Wrong value type of " + path);
        }

    return v;
}

template <>
inline std::vector<std::string> CfgParser::get< std::vector<std::string> >(std::string path)
{

    std::vector<std::string> ret;

    optional<ptree&> value = pt.get_child_optional(path);
    if (!value.is_initialized())
        {
            // the vector member was not specified in the configuration
            throw Err_Custom("The " + path + " has to be specified!");
        }
    ptree& array = value.get();

    // check if the node has a value,
    // accordingly to boost it should be empty if array syntax was used in JSON
    std::string wrong = array.get_value<std::string>();
    if (!wrong.empty())
        {
            throw Err_Custom("Wrong value: '" + wrong + "'");
        }

    ptree::iterator it;
    for (it = array.begin(); it != array.end(); ++it)
        {
            std::pair<std::string, ptree> v = *it;

            // check if the node has a name,
            // accordingly to boost it should be empty if object weren't
            // members of the array (our case)
            if (!v.first.empty())
                {
                    throw Err_Custom("An array was expected, instead an object was found (at '" + path + "', name: '" + v.first + "')");
                }

            // check if the node has children, it should only have a value!
            if (!v.second.empty())
                {
                    throw Err_Custom("Unexpected object in array '" + path + "' (only a list of values was expected)");
                }

            ret.push_back(v.second.get_value<std::string>());
        }

    return ret;
}

template <>
inline std::map <std::string, int> CfgParser::get< std::map<std::string, int> >(std::string path)
{

    std::map<std::string, int> ret;

    optional<ptree&> value = pt.get_child_optional(path);
    if (!value.is_initialized()) throw Err_Custom("The " + path + " has to be specified!");
    ptree& array = value.get();

    // check if the node has a value,
    // accordingly to boost it should be empty if array syntax was used in JSON
    std::string wrong = array.get_value<std::string>();
    if (!wrong.empty())
        {
            throw Err_Custom("Wrong value: '" + wrong + "'");
        }

    ptree::iterator it;
    for (it = array.begin(); it != array.end(); ++it)
        {
            std::pair<std::string, ptree> v = *it;

            // check if the node has a name,
            // accordingly to boost it should be empty if object weren't
            // members of the array (our case)
            if (!v.first.empty())
                {
                    throw Err_Custom("An array was expected, instead an object was found (at '" + path + "', name: '" + v.first + "')");
                }

            // check if there is a value,
            // the value should be empty because only a 'key:value' object should be specified
            if (!v.second.get_value<std::string>().empty())
                {
                    throw Err_Custom("'{key:value}' object was expected, not just the value");
                }

            // there should be only one child the 'key:value' object
            if (v.second.size() != 1)
                {
                    throw Err_Custom("In array '" + path + "' only ('{key:value}' objects were expected)");
                }

            std::pair<std::string, ptree> kv = v.second.front();
            try
                {
                    // get the std::string value
                    std::string str_value = kv.second.get_value<std::string>();
                    // check if it's auto-value
                    if (str_value == auto_value)
                        {
                            ret[kv.first] = -1;
                        }
                    else
                        {
                            // get the integer value
                            int value = kv.second.get_value<int>();
                            // make sure it's not negative
                            if (value < 0) throw Err_Custom("The value of " + kv.first + " cannot be negative!");
                            // std::set the value
                            ret[kv.first] = value;
                        }

                }
            catch(ptree_bad_data& ex)
                {
                    throw Err_Custom("Wrong value type of " + kv.first);
                }
        }

    return ret;
}

template <>
inline std::map <std::string, double> CfgParser::get< std::map<std::string, double> >(std::string path)
{

    std::map<std::string, double> ret;

    optional<ptree&> value = pt.get_child_optional(path);
    if (!value.is_initialized()) throw Err_Custom("The " + path + " has to be specified!");
    ptree& array = value.get();

    // check if the node has a value,
    // accordingly to boost it should be empty if array syntax was used in JSON
    std::string wrong = array.get_value<std::string>();
    if (!wrong.empty())
        {
            throw Err_Custom("Wrong value: '" + wrong + "'");
        }

    ptree::iterator it;
    for (it = array.begin(); it != array.end(); ++it)
        {
            std::pair<std::string, ptree> v = *it;

            // check if the node has a name,
            // accordingly to boost it should be empty if object weren't
            // members of the array (our case)
            if (!v.first.empty())
                {
                    throw Err_Custom("An array was expected, instead an object was found (at '" + path + "', name: '" + v.first + "')");
                }

            // check if there is a value,
            // the value should be empty because only a 'key:value' object should be specified
            if (!v.second.get_value<std::string>().empty())
                {
                    throw Err_Custom("'{key:value}' object was expected, not just the value");
                }

            // there should be only one child the 'key:value' object
            if (v.second.size() != 1)
                {
                    throw Err_Custom("In array '" + path + "' only ('{key:value}' objects were expected)");
                }

            std::pair<std::string, ptree> kv = v.second.front();
            try
                {
                    // get the std::string value
                    std::string str_value = kv.second.get_value<std::string>();
                    // check if it's auto-value
                    if (str_value == auto_value)
                        {
                            ret[kv.first] = -1;
                        }
                    else
                        {
                            // get the integer value
                            double value = kv.second.get_value<double>();
                            // make sure it's not negative
                            if (value < 0) throw Err_Custom("The value of " + kv.first + " cannot be negative!");
                            // Set the value
                            ret[kv.first] = value;
                        }

                }
            catch(ptree_bad_data& ex)
                {
                    throw Err_Custom("Wrong value type of " + kv.first);
                }
        }

    return ret;
}

}
}

#endif /* CONFIGURATIONPARSER_H_ */
