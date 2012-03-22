

#include "stringhelper.h"
#include <cctype>

using namespace std;

namespace StringHelper
{





string getValue( const string& s, char del )
{
    string::size_type pos = s.find( del );
    if ( pos != string::npos && pos+1 < s.length() ) {
        return s.substr( pos + 1 );
    }
    return "";
}


string getValueBefore( const string& s, char del )
{
    string::size_type pos = s.find( del );
    if ( pos != string::npos ) {
        return s.substr( 0, pos );
    }
    return s;
}


string stripWhiteSpace( const string& s )
{
    if ( s.empty() ) {
        return s;
    }

    int pos = 0;
    string line = s;
    int len = (int) line.length();
    while ( pos < len && isspace( line[pos] ) ) {
        ++pos;
    }
    line.erase( 0, pos );
    pos = line.length()-1;
    while ( pos > -1 && isspace( line[pos] ) ) {
        --pos;
    }
    if ( pos != -1 ) {
        line.erase( pos+1 );
    }
    return line;
}


bool startwith_nocase( const string& s1, const string& s2 )
{
    string::const_iterator p1 = s1.begin();
    string::const_iterator p2 = s2.begin();

    while ( p1 != s1.end() && p2 != s2.end() ) {
        if ( toupper( *p1 ) != toupper( *p2 ) ) {
            return false;
        }
        ++p1;
        ++p2;
    }

    if ( p1 == s1.end() && p2 != s2.end() ) {
        return false;
    }

    return true;
}


string toLowerCase( const string& s )
{
    string result = "";
    for ( string::size_type i = 0; i < s.length(); ++i ) {
        result += tolower( s[i] );
    }

    return result;
}


string toUpperCase( const string& s )
{
    string result = "";
    for ( string::size_type i = 0; i < s.length(); ++i ) {
        result += toupper( s[i] );
    }

    return result;
}


string replaceAll( string& in,
                   const string& oldString,
                   const string& newString )
{
    size_t pos;
    while ( (pos = in.find( oldString )) != string::npos ) {
        in =
            in.replace( pos, oldString.length(), newString );
    }

    return in;
}


} // Namespace
