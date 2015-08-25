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

#include "stringhelper.h"
#include <cctype>

using namespace std;

namespace StringHelper
{

std::string getValue( const std::string& s, char del )
{
    std::string::size_type pos = s.find( del );
    if ( pos != std::string::npos && pos+1 < s.length() )
        {
            return s.substr( pos + 1 );
        }
    return "";
}


std::string getValueBefore( const std::string& s, char del )
{
    std::string::size_type pos = s.find( del );
    if ( pos != std::string::npos )
        {
            return s.substr( 0, pos );
        }
    return s;
}


std::string stripWhiteSpace( const std::string& s )
{
    if ( s.empty() )
        {
            return s;
        }

    int pos = 0;
    std::string line = s;
    int len = (int) line.length();
    while ( pos < len && isspace( line[static_cast<size_t>(pos)] ) )
        {
            ++pos;
        }
    line.erase( 0, static_cast<size_t>(pos) );
    pos = static_cast<int>(line.length()) - 1;
    while ( pos > -1 && isspace( line[static_cast<size_t>(pos)] ) )
        {
            --pos;
        }
    if ( pos != -1 )
        {
            line.erase( static_cast<size_t>(pos+1) );
        }
    return line;
}


bool startwith_nocase( const std::string& s1, const std::string& s2 )
{
    std::string::const_iterator p1 = s1.begin();
    std::string::const_iterator p2 = s2.begin();

    while ( p1 != s1.end() && p2 != s2.end() )
        {
            if ( toupper( *p1 ) != toupper( *p2 ) )
                {
                    return false;
                }
            ++p1;
            ++p2;
        }

    if ( p1 == s1.end() && p2 != s2.end() )
        {
            return false;
        }

    return true;
}


string toLowerCase( const std::string& s )
{
    std::string result = "";
    for ( std::string::size_type i = 0; i < s.length(); ++i )
        {
            result += static_cast<char>(tolower( s[i] ));
        }

    return result;
}


string toUpperCase( const std::string& s )
{
    std::string result = "";
    for ( std::string::size_type i = 0; i < s.length(); ++i )
        {
            result += static_cast<char>(toupper( s[i] ));
        }

    return result;
}


string replaceAll( std::string& in,
                   const std::string& oldString,
                   const std::string& newString )
{
    size_t pos;
    while ( (pos = in.find( oldString )) != std::string::npos )
        {
            in =
                in.replace( pos, oldString.length(), newString );
        }

    return in;
}


} // Namespace
