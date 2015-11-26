/*
 * rest_failure.h
 *
 *  Created on: 13 Nov 2015
 *      Author: dhsmith
 */

#ifndef REST_FAILURE_H_
#define REST_FAILURE_H_

#include <exception>
#include <string>
#include <sstream>

#include "cli_exception.h"

namespace fts3
{

namespace cli
{

/**
 * REST reply was received indicating overall failure of the command
 * (distinct from the case of individual files within the request having errors)
 */
class rest_failure : public cli_exception
{

public:
    /**
     * Constructor.
     *
     * @param msg - exception message
     */
    rest_failure(long code, std::string const &data, std::string const &httpmsg) : cli_exception(std::string()), code(code), data(data)
    {
        std::stringstream ss;
        ss << "Status " << code;
        if (!httpmsg.empty()) {
            ss << ": " << httpmsg;
        }
        msg = ss.str();
    }

    /**
     * Destructor
     */
    virtual ~rest_failure() {};

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

    virtual long getCode() const
    {
        return code;
    }

    virtual char const *getData() const
    {
        return data.c_str();
    }

protected:
    long code;
    std::string data;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* REST_FAILURE_H_ */
