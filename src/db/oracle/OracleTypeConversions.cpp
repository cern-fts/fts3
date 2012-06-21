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

time_t OracleTypeConversions::toTimeT(const ::oracle::occi::Timestamp& timestamp){
    time_t t = (time_t)-1;

        if(!timestamp.isNull()){
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
            tmp_tm.tm_sec   = second;
            tmp_tm.tm_min   = minute - tz_minute;
            tmp_tm.tm_hour  = hour - tz_hour;
            tmp_tm.tm_mday  = day;
            tmp_tm.tm_mon   = (month >= 1)   ?month - 1   :0;
            tmp_tm.tm_year  = (year  >= 1900)?year  - 1900:0;
            tmp_tm.tm_wday  = 0;
            tmp_tm.tm_yday  = 0;
            tmp_tm.tm_isdst = 0;

            // Get Time Value
            t = mktime(&tmp_tm);
            if((time_t)-1 == t){
               // m_log_error("Cannot Convert Timestamp " << timestamp.toText("dd/mm/yyyy hh:mi:ss [tzh:tzm]",0));
            } else {
                t -= timezone;
            }
        }
	return t;
}

oracle::occi::Timestamp OracleTypeConversions::toTimestamp(time_t t, oracle::occi::Environment* m_env){
    oracle::occi::Timestamp timestamp;
    try{
        struct tm * tmp_tm = gmtime(&t);
        if(0 != tmp_tm){
            timestamp = oracle::occi::Timestamp(m_env,                  // Environment
                                   tmp_tm->tm_year + 1900, // Year
                                   tmp_tm->tm_mon + 1,     // Month
                                   tmp_tm->tm_mday,        // Day
                                   tmp_tm->tm_hour,        // Hour
                                   tmp_tm->tm_min,         // Minute
                                   tmp_tm->tm_sec,         // Second
                                   0,                      // Fraction of Seconds
                                   0,                      // tz hour
                                   0);                     // tz minute
        }
    } catch(const oracle::occi::SQLException& e){
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }

    return timestamp;
}


longlong OracleTypeConversions::toLongLong(const ::oracle::occi::Number& number, oracle::occi::Environment* m_env){
    longlong n = -1;
    try{
        if(!number.isNull()){
            std::string n_str = number.toText(m_env,LONGLONG_FMT);
            n = atoll(n_str.c_str());
        }
    } catch(const oracle::occi::SQLException& e){
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    return n;
}


oracle::occi::Number OracleTypeConversions::toNumber(longlong n, oracle::occi::Environment* m_env){
    std::string n_str;
    std::stringstream str;
    str << n;
    n_str = str.str();

    oracle::occi::Number number(0);
    try{
        number.fromText(m_env,n_str,LONGLONG_FMT);
    } catch(const oracle::occi::SQLException& e){
        FTS3_COMMON_EXCEPTION_THROW(Err_Custom(e.what()));
    }
    return number;
}


bool OracleTypeConversions::toBoolean(const std::string& str, bool){
    if(str.empty()){
        return false;
    }
    return std::equal(str.begin(), str.end(),BOOLEAN_TRUE_STR.begin(),boost::is_iequal());
}


const std::string& OracleTypeConversions::toBoolean(bool b){
	return (b == true)?BOOLEAN_TRUE_STR:BOOLEAN_FALSE_STR;
}


void OracleTypeConversions::toString(::oracle::occi::Clob clob, std::string& str){
    // Check if the Clob is Null
    if(clob.isNull()){
        str.clear();
        return;
    }

    try{
        // Open the Clob
        clob.open(oracle::occi::OCCI_LOB_READONLY);
        int len = clob.length();
	if(len == 0)
		return;

        // reserve some space on the string
        str.resize(len, 0);
        char * buffer = &(*(str.begin()));

        // Get the stream
        StreamPtr<oracle::occi::Clob> instream(clob, clob.getStream(1,0));

        instream->readBuffer(buffer, len);

 
        // Close the Clob and the related stream
        clob.close();
    } catch(const ::oracle::occi::SQLException&  exc){
        //Close the Clob
        try{
            clob.close();
        }catch(...){}
        std::string reason = (std::string)"Failed to read clob: " + exc.getMessage();
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(reason));
    }
}


