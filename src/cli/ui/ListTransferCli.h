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

#ifndef LISTTRANSFERCLI_H_
#define LISTTRANSFERCLI_H_

#include "DnCli.h"
#include "ws-ifce/gsoap/gsoap_stubs.h"
#include "VoNameCli.h"
#include "TransferCliBase.h"

namespace fts3
{
namespace cli
{

/**
 * Command line utility for specifying the list of states of interest
 *
 *      - state, positional parameter, allows for specifying several
 *          states of interest
 */
class ListTransferCli : public DnCli, public VoNameCli, public TransferCliBase
{
public:

    /**
     * Default constructor.
     *
     * Creates the transfer-list specific command line options. State is
     * market as both: hidden and positional
     */
    ListTransferCli();

    /**
     * Destructor.
     */
    virtual ~ListTransferCli();

    /**
     * Validates command line options.
     *  Call the validate methods of the base clases
     */
    virtual void validate();

    /**
     * Gives the instruction how to use the command line tool.
     *
     * @return a string with instruction on how to use the tool
     */
    std::string getUsageString(std::string tool) const;

    /**
     * Gets a pointer to impl__ArrayOf_USCOREsoapenc_USCOREstring object.
     * The object contains set of statuses of interest. This are the statuses
     * that the user passed using command line options. If no statuses were
     * passed, the set is determined depending on the version of the FTS3 service.
     * The object is created using gSOAP memory-allocation utility, and will
     * be garbage collected. If there is a need to delete it manually gSOAP dedicated
     * functions should be used (in particular 'soap_unlink'!)
     *
     * @param soap - soap object corresponding to FTS3 service
     *
     * @return  impl__ArrayOf_USCOREsoapenc_USCOREstring
     */
    std::vector<std::string> getStatusArray();

    /**
     *
     */
    std::string source();

    /**
     *
     */
    std::string destination();

    /**
     * @return true if user requested a deletion query, false otherwise
     */
    bool deletion();
};

}
}

#endif /* LISTTRANSFERCLI_H_ */
