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
 */

#include "OracleTypeConversions.h"
#include "error.h"
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <algorithm>
#include "StreamPtr.h"
#include <iostream>

using namespace FTS3_COMMON_NAMESPACE;

const char * const LONGLONG_FMT           = "99999999999999999999";
const std::string BOOLEAN_TRUE_STR  = "Y";
const std::string BOOLEAN_FALSE_STR = "N";

time_t OracleTypeConversions::toTimeT(const ::oracle::occi::Timestamp& timestamp)
{
    time_t t = (time_t)-1;

    if(!timestamp.isNull())
        {
            int year;
            unsigned int month,day,hour,minute,second,fs;
            int tz_hour = 0;
            int tz_minute = 0;
            // Get Date
            timestamp.getDate(year,month,day);
            // Get Time
            timestamp.getTime(hour,minute,second,fs);
            // Get TimeZone Offset
            timestamp.getTimeZoneOffset(tz_hour,tz_minute);

            struct tm tmp_tm;
            tmp_tm.tm_sec   = static_cast<int>(second);
            tmp_tm.tm_min   = static_cast<int>(minute) - tz_minute;
            tmp_tm.tm_hour  = static_cast<int>(hour) - tz_hour;
            tmp_tm.tm_mday  = static_cast<int>(day);
            tmp_tm.tm_mon   = (month >= 1)   ?static_cast<int>(month) - 1   :0;
            tmp_tm.tm_year  = (year  >= 1900)?static_cast<int>(year)  - 1900:0;
            tmp_tm.tm_wday  = 0;
            tmp_tm.tm_yday  = 0;
            tmp_tm.tm_isdst = 0;

            // Get Time Value
            t = mktime(&tmp_tm);
            if((time_t)-1 == t)
                {
                    // m_log_error("Cannot Convert Timestamp " << timestamp.toText("dd/mm/yyyy hh:mi:ss [tzh:tzm]",0));
                    FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Cannot Convert Timestamp"));
                }
            else
                {
                    t -= timezone;
                }
        }
    return t;
}

oracle::occi::Timestamp OracleTypeConversions::toTimestamp(time_t t, oracle::occi::Environment* m_env)
{
    oracle::occi::Timestamp timestamp;
    try
        {
            struct tm * tmp_tm = gmtime(&t);
            if(0 != tmp_tm)
                {
                    timestamp = oracle::occi::Timestamp(m_env,                  // Environment
                                                        tmp_tm->tm_year + 1900, // Year
                                                        static_cast<unsigned>(tmp_tm->tm_mon + 1),     // Month
                                                        static_cast<unsigned>(tmp_tm->tm_mday),        // Day
                                                        static_cast<unsigned>(tmp_tm->tm_hour),        // Hour
                                                        static_cast<unsigned>(tmp_tm->tm_min),         // Minute
                                                        static_cast<unsigned>(tmp_tm->tm_sec),         // Second
                                                        0,                      // Fraction of Seconds
                                                        0,                      // tz hour
                                                        0);                     // tz minute
                }
        }
    catch(const oracle::occi::SQLException& e)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch(...)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    return timestamp;
}


longlong OracleTypeConversions::toLongLong(const ::oracle::occi::Number& number, oracle::occi::Environment* m_env)
{
    longlong n = -1;
    try
        {
            if(!number.isNull())
                {
                    std::string n_str = number.toText(m_env,LONGLONG_FMT);
                    n = atoll(n_str.c_str());
                }
        }
    catch(const oracle::occi::SQLException& e)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch(...)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }

    return n;
}


oracle::occi::Number OracleTypeConversions::toNumber(longlong n, oracle::occi::Environment* m_env)
{
    std::string n_str;
    std::stringstream str;
    str << n;
    n_str = str.str();

    oracle::occi::Number number(0);
    try
        {
            number.fromText(m_env,n_str,LONGLONG_FMT);
        }
    catch(const oracle::occi::SQLException& e)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
        }
    catch(...)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
    return number;
}


bool OracleTypeConversions::toBoolean(const std::string& str, bool)
{
    if(str.empty())
        {
            return false;
        }
    return std::equal(str.begin(), str.end(),BOOLEAN_TRUE_STR.begin(),boost::is_iequal());
}


const std::string& OracleTypeConversions::toBoolean(bool b)
{
    return (b == true)?BOOLEAN_TRUE_STR:BOOLEAN_FALSE_STR;
}


void OracleTypeConversions::toString(::oracle::occi::Clob clob, std::string& str)
{

    try
        {
            // Check if the Clob is Null
            if(clob.isNull())
                {
                    str.clear();
                    return;
                }

            // Open the Clob
            clob.open(oracle::occi::OCCI_LOB_READONLY);
            unsigned len = clob.length();
            if(len == 0)
                {
                    clob.close();
                    return;
                }

            if(clob.isOpen())
                {
                    // reserve some space on the string
                    str.resize(len, 0);
                    char * buffer = &(*(str.begin()));

                    // Get the stream
                    StreamPtr<oracle::occi::Clob> instream(clob, clob.getStream(1,0));

                    instream->readBuffer(buffer, len);


                    // Close the Clob and the related stream
                    clob.close();
                }
        }
    catch(const ::oracle::occi::SQLException&  exc)
        {
            //Close the Clob
            try
                {
                    clob.close();
                }
            catch(...) {}
            std::string reason = (std::string)"Failed to read clob: " + exc.getMessage();
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom(reason));
        }
    catch(...)
        {
            FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Unknown exception"));
        }
}


