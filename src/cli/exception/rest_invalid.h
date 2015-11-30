/*
 * rest_invalid.h
 *
 *  Created on: 17 Nov 2015
 *      Author: dhsmith
 */

#ifndef REST_INVALID_H_
#define REST_INVALID_H_

#include <exception>
#include <string>

#include "cli_exception.h"

namespace fts3
{

namespace cli
{

/**
 * REST (json) data could not be interpreted
 */
class rest_invalid : public cli_exception
{

public:
    /**
     * Constructor.
     *
     * @param msg - exception message
     */
    rest_invalid(std::string const & msg) : cli_exception(msg) {}

    /**
     * Destructor
     */
    virtual ~rest_invalid() throw () {};

    /**
     * returns the error message
     */
    virtual char const * what() const throw()
    {
        return msg.c_str();
    }
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* REST_INVALID_H_ */
