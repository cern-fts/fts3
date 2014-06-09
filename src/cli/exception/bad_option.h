/*
 * bad_option.h
 *
 *  Created on: 17 Apr 2014
 *      Author: simonm
 */

#ifndef BAD_OPTION_H_
#define BAD_OPTION_H_


#include <exception>
#include <string>

#include "cli_exception.h"

namespace fts3
{
namespace cli
{

/**
 * A Exception class used man a program option was wrongly used
 */
class bad_option : public cli_exception
{

public:
    /**
     * Constructor
     */
    bad_option(std::string const & opt, std::string const & msg) : cli_exception(msg), opt(opt), full_msg(opt + ": " + msg) {}

    /**
     * returns the error message
     */
    virtual char const * what() const
#if __cplusplus >= 199711L
    noexcept (true)
#endif
    {
        return full_msg.c_str();
    }

    /**
     * returns the error message that should be included in the JSON output
     */
    virtual pt::ptree const json_obj() const
    {
        pt::ptree obj;
        obj.put(opt, msg);

        return obj;
    }

private:
    /// program option name
    std::string opt;
    /// combined error message
    std::string full_msg;
};

}
}

#endif /* BAD_OPTION_H_ */
