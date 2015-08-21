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
    bool rest() const;

    /**
     * @return the path to the CA certificates
     */
    std::string capath() const;
};

} /* namespace cli   */
} /* namespace fts3  */
#endif /* RESTCLI_H_ */
