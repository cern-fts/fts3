/*
 * wrong_protocol.h
 *
 *  Created on: 22 Oct 2015
 *      Author: dhsmith
 */

#ifndef WRONG_PROTOCOL_H_
#define WRONG_PROTOCOL_H_


#include <exception>
#include <string>

#include "cli_exception.h"

namespace fts3
{
namespace cli
{

/**
 * A Exception class used when the client detects its peer is using an unexpected protocol
 */
class wrong_protocol : public cli_exception
{

public:
    /**
     * Constructors
     */
    wrong_protocol() : cli_exception("Not the expected protocol") {}

    wrong_protocol(std::string const & msg) : cli_exception("Not the expected protocol : " + msg) {}

    virtual bool tryFallback() const
    {
       	return true;
    }
};

}
}

#endif /* WRONG_PROTOCOL_H_ */
