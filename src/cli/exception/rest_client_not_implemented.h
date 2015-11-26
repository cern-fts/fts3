/*
 * rest_client_not_implemented.h
 *
 *  Created on: 22 Oct 2015
 *      Author: dhsmith
 */

#ifndef REST_CLIENT_NOT_IMPLEMENTED_H_
#define REST_CLIENT_NOT_IMPLEMENTED_H_


#include <exception>
#include <string>

#include "cli_exception.h"

namespace fts3
{
namespace cli
{

/**
 * A Exception class used when the required functionality has not been implemented
 * in this (c++) rest client.
 */
class rest_client_not_implemented : public cli_exception
{

public:
    /**
     * Constructors
     */
    rest_client_not_implemented() : cli_exception("Not implemented in this REST client") {}

    rest_client_not_implemented(std::string const & msg) : cli_exception("Not implemented in this REST client: " + msg) {}

    virtual bool tryFallback() const
    {
       	return true;
    }
};

}
}

#endif /* REST_CLIENT_NOT_IMPLEMENTED_H_ */
