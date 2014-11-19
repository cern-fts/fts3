/*
 * DelegationCli.h
 *
 *  Created on: Mar 5, 2014
 *      Author: simonm
 */

#ifndef DELEGATIONCLI_H_
#define DELEGATIONCLI_H_

#include "TransferCliBase.h"

namespace fts3
{
namespace cli
{

class DelegationCli : public TransferCliBase
{

public:

    DelegationCli();

    virtual ~DelegationCli();

    /**
     * Gets the delegation ID (string).
     *
     * @return delegation ID string if it was given as a CLI option, or an empty string if not
     */
    std::string getDelegationId();

    /**
     * Gets user's set expiration time in seconds (long).
     *
     * @return expiration time in seconds if it was given as a CLI option, or 0 string if not
     */
    long getExpirationTime();
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* DELEGATIONCLI_H_ */
