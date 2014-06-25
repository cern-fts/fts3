/*
 * gsoap_error.h
 *
 *  Created on: 17 Apr 2014
 *      Author: simonm
 */

#ifndef GSOAP_ERROR_H_
#define GSOAP_ERROR_H_



#include <exception>
#include <string>
#include <sstream>

#include "cli_exception.h"
#include "ws-ifce/gsoap/gsoap_stubs.h"

namespace fts3
{
namespace cli
{

/**
 * An Exception class used when a gsoap error occurred
 */
class gsoap_error : public cli_exception
{

public:

    /**
     * Constructor
     */
    gsoap_error(soap* ctx) : cli_exception("")
    {
        // retriving the error message from gsoap context
        std::stringstream ss;
        soap_stream_fault(ctx, ss);

        // replace the standard gSOAP error message before printing
        msg = ss.str();
        std::string::size_type pos = msg.find("reports Error reading token data header: Connection closed", 0);

        // add an additional hint in case of 'reading token data header' error
        if (pos != std::string::npos)
            {
                msg += " Please consult the FTS3 log files for more details.";
            }

        // remove backspaces from the string
        while((pos = msg.find(8)) != std::string::npos)
            {
                msg.erase(pos, 1);
            }
    }

    /**
     * returns the error message
     */
    virtual char const * what() const
#if __cplusplus >= 199711L
    noexcept (true)
#endif
    {
        return msg.c_str();
    }

    /**
     * returns the error message that should be included in the JSON output
     */
    virtual pt::ptree const json_obj() const
    {
        std::string::size_type start = msg.find("SOAP 1.1 fault: SOAP-ENV:");
        std::string::size_type stop  = msg.find("Detail: ");

        pt::ptree obj;

        if (start != std::string::npos && stop != std::string::npos)
            {
                // shift the starting point
                start += std::string("SOAP 1.1 fault: SOAP-ENV:").size();
                std::string err_msg = msg.substr(start, stop);

                // shift the starting point again
                start = stop + std::string("Detail: ").size();
                std::string detail = msg.substr(start);

                obj.put("message", err_msg);
                obj.put("detail", detail);
            }
        else
            {
                obj.put("message", msg);
            }

        return obj;
    }
};

}
}

#endif /* GSOAP_ERROR_H_ */
