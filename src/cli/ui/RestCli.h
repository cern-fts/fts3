/*
 * RestCli.h
 *
 *  Created on: Feb 6, 2014
 *      Author: simonm
 */

#ifndef RESTCLI_H_
#define RESTCLI_H_

#include "CliBase.h"

namespace fts3
{
namespace cli
{

/**
 * The command line utility for getting the 'rest' option
 *
 * The class provides:
 * 		- rest, allows to use the REST interface instead of SOAP
 */
class RestCli  : public virtual CliBase
{

public:

    /**
     * Default constructor.
     *
     * Creates the REST command line interface.
     */
    RestCli();

    /**
     * Destructor.
     */
    virtual ~RestCli();

    /**
     * @return true if the rest interface has been selected, false otherwise
     */
    bool rest();

    /**
     * @return the path to the CA certificates
     */
    std::string capath();

    /**
     * @return the path to the proxy certificate
     */
    std::string proxy();
};

} /* namespace cli   */
} /* namespace fts3  */
#endif /* RESTCLI_H_ */
