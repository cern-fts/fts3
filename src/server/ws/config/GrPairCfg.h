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

#ifndef GRPAIRCFG_H_
#define GRPAIRCFG_H_

#include "PairCfg.h"

namespace fts3
{
namespace ws
{

/**
 * SE group pair configuration,
 * 	it's derived from PairCfg
 *
 * 	@see PairCfg
 */
class GrPairCfg : public PairCfg
{

public:
    /**
     * Constructor.
     *
     * Retrieves the configuration data from DB for the given source - destination pair.
     *
     * @param dn - client's DN
     * @param source - the source SE group
     * @param destination - the destination SE group
     */
    GrPairCfg(std::string dn, std::string source, std::string destination) : PairCfg(dn, source, destination) {}; // check if SE groups exist

    /**
     * Constructor.
     *
     * Retrieves the configuration from the given CfgParser
     *
     * @param dn - client's DN
     * @param parser - parser that has been used for for parsing JSON configuration
     */
    GrPairCfg(std::string dn, CfgParser& parser);

    /**
     * Destructor.
     */
    virtual ~GrPairCfg();

    /**
     * Creates a std::string containing current configuration in JSON format.
     *
     * @return std::string containing the configuration
     */
    virtual std::string json();

    /**
     * Saves the configuration into the DB.
     */
    virtual void save();

    /**
     * Checks if the configuration concerns a single SE or a group
     */
    virtual bool isgroup()
    {
        return true;
    }
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* GRPAIRCFG_H_ */
