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

#ifndef _STRINGHELPER_H_
#define _STRINGHELPER_H_

#include <list>
#include <string>


namespace StringHelper
{

template <class T>
void split( const std::string& s, char del,
            T& target,
            std::string::size_type startPos=0, bool useEmpty=true  );

std::string stripWhiteSpace( const std::string& s );
bool startwith_nocase( const std::string& s1, const std::string& s2 );

std::string getValue( const std::string& s, char del );
std::string getValueBefore( const std::string& s, char del );

std::string toLowerCase( const std::string& s );
std::string toUpperCase( const std::string& s );

std::string replaceAll( std::string& in,
                   const std::string& oldString,
                   const std::string& newString );



template <class T>
void split( const std::string& s, char del,
            T& target,
            std::string::size_type startPos, bool useEmpty )
{
    std::string line = s;

    std::string::size_type pos;
    std::string::size_type offset = startPos;
    while ( ( pos = line.find( del, offset ) ) != std::string::npos )
        {
            offset = 0;

            std::string val = line.substr( 0, pos );
            if ( useEmpty || !stripWhiteSpace( val ).empty() )
                {
                    target.push_back( val );
                }
            line.erase( 0, pos+1 );
        }

    if ( line.length() > 0 )
        {
            target.push_back( line );
        }
}


}
#endif
