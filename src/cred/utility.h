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

#ifndef UTILITY_H_
#define UTILITY_H_

#include <string>
#include <algorithm>
#include <ctype.h>



/*
 * eq_nocase
 *
 * Character Comparison that ignore the case
 */
inline bool eq_nocase(char c1, char c2)
{
    return tolower(c1) == tolower(c2);
}

/**
 * Compare the first element of a std::pair
 */
struct compare_first: std::unary_function<const std::pair<std::string,std::string>&, bool>
{
    /**
     * Constructor
     */
    compare_first(const std::string& s):m_str(s) {}
    /**
     * Call Operator
     */
    bool operator()(const std::pair<std::string,std::string>& p) const
    {
        return ( p.first == m_str )?true:false;
    }
private:
    const std::string& m_str;
};


/**
 * Convert a string representing a size (in bytes) to an integer. Examples of the allowed formats are:
 *   122
 *   100 KB
 *   1.2 MB (or without the space 1.2MB)
 *   .5 GB
 */
unsigned long str_to_size(std::string str) /*throw (LogicError)*/;

/**
 * Return the hash of a string
 */
unsigned long hash_string(const std::string& str) throw ();

/**
 * Get the parent Directory name
 */
std::string get_dirname(const std::string& dir_path) throw ();


#endif //UTILITY_H_
