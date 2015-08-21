/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
    std::string getDelegationId() const;

    /**
     * Gets user's set expiration time in seconds (long).
     *
     * @return expiration time in seconds if it was given as a CLI option, or 0 string if not
     */
    long getExpirationTime() const;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* DELEGATIONCLI_H_ */
