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

#ifndef DNCLI_H_
#define DNCLI_H_

#include "CliBase.h"

namespace fts3
{
namespace cli
{

/**
 * The command line utility for getting user DN
 *
 * The class provides:
 * 		- userdn (u), allows specifying user DN
 */
class DnCli : virtual public CliBase
{
public:

    /**
     * Default constructor.
     *
     * Creates the DN command line interface.
     */
    DnCli();

    /**
     * Destructor.
     */
    virtual ~DnCli();

    /**
     * Gets the user DN, specified by the user.
     *
     * @return user DN
     */
    std::string getUserDn();
};

}
}

#endif /* DNCLI_H_ */
