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

#include "utility.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <ctype.h>
#include <libgen.h>
#include "common/logger.h"
#include "common/error.h"

using namespace FTS3_COMMON_NAMESPACE;


namespace
{
const unsigned long KILOBYTE_SIZE  = 1024;
const unsigned long MEGABYTE_SIZE  = 1024 * 1024;
const unsigned long GIGABYTE_SIZE  = 1024 * 1024 * 1024;
}



/*
 * str_to_size
 *
 * Convert a string representing a size (in bytes) to an integer. Examples of the allowed formats are:
 *   122
 *   100 KB
 *   1.2 MB (or without the space 1.2MB)
 *   .5 GB
 */
unsigned long str_to_size(std::string str) /*throw (LogicError)*/
{

    unsigned long size      = 0;
    unsigned long unit_size = 1;

    // trim beginning  white spaces
    boost::trim(str);
    if(true == str.empty())
        {
            // Empty string. Retrun 0
            return size;
        }

    try
        {
            if(str.at(0) == '.')
                {
                    // String starts with a ".". Append a 0
                    str.insert(str.begin(),'0');
                }
            // check if the string starts with a number
            if(isdigit(str.at(0)))
                {
                    // Check if the string has only digit
                    if(boost::all(str,boost::is_digit()))
                        {
                            size = boost::lexical_cast<unsigned long>(str);
                        }
                    else
                        {
                            // Check if the string has a dot
                            std::string::size_type dot_pos = str.find('.');

                            // find the last element that is not a digit
                            std::string::iterator num_end = (dot_pos != std::string::npos)? str.begin() + dot_pos + 1: str.begin();
                            for(; num_end != str.end(); ++num_end)
                                {
                                    if(0 == isdigit(*num_end))
                                        {
                                            break;
                                        }
                                }
                            // Check if the string has the unit
                            std::string unit;
                            if(num_end != str.end())
                                {
                                    unit = str.substr(std::distance(str.begin(),num_end));
                                    boost::trim(unit);
                                }
                            if(unit.length() >= 2)
                                {
                                    if(unit == "KB")
                                        {
                                            unit_size = KILOBYTE_SIZE;
                                        }
                                    else if(unit == "MB")
                                        {
                                            unit_size = MEGABYTE_SIZE;
                                        }
                                    else if(unit == "GB")
                                        {
                                            unit_size = GIGABYTE_SIZE;
                                        }
                                    else
                                        {
                                            //throw LogicError("Invalid string size representation: unknown unit");
                                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid string size representation: unknown unit" << commit;
                                        }
                                }
                            else
                                {
                                    if(dot_pos != std::string::npos)
                                        {
                                            //throw LogicError("Decimal number used to express a size");
                                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Decimal number used to express a size" << commit;
                                        }
                                    else
                                        {
                                            //throw LogicError("Invalid string size representation: string contains invalid characters");
                                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid string size representation: string contains invalid characters" << commit;
                                        }
                                }

                            std::string num_units_str = str.substr(0, std::distance(str.begin(),num_end));
                            double num_units = boost::lexical_cast<double>(num_units_str);
                            size = static_cast<unsigned long>(num_units * static_cast<double>(unit_size));
                        }
                }
            else
                {
                    // Invalid String
                    //throw LogicError("Invalid string size representation: string doesn't start with a digit or .");
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid string size representation: string doesn't start with a digit or ." << commit;
                }
        }
    catch(const boost::bad_lexical_cast&)
        {
            //throw LogicError("Invalid string to number conversion");
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Invalid string to number conversion" << commit;
        }

    return size;
}


/*
 * hash_string
 *
 * hash a string (for the moment use SDBM algorithm, when boost 1.33 will be
 * used, switch to boost::hash)
 */
unsigned long hash_string(const std::string& str) throw()
{
    unsigned long hash = 0;
    int c;
    std::string::const_iterator it;
    for(it = str.begin(); it != str.end(); ++it)
        {
            c = *it;
            hash = static_cast<unsigned long>(c) + (hash << 6) + (hash << 16) - hash;
        }
    return hash;
}

/*
 * get_dirname
 *
 * Get the parent Directory name
 */
std::string get_dirname(const std::string& dir_path) throw ()
{
    char * parent_name_str = strdup(dir_path.c_str());
    ::dirname(parent_name_str);
    std::string parent_name = parent_name_str;
    free(parent_name_str);
    return parent_name;
}
