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

#ifndef GETCFGCLI_H_
#define GETCFGCLI_H_

#include "VoNameCli.h"
#include "SrcDestCli.h"

namespace fts3
{
namespace cli
{

/**
 *
 */
class GetCfgCli: public SrcDestCli
{

public:

    /**
     *
     */
    GetCfgCli();

    /**
     *
     */
    virtual ~GetCfgCli();

    /**
     * Gives the instruction how to use the command line tool.
     *
     * @return a string with instruction on how to use the tool
     */
    std::string getUsageString(std::string tool) const;

    /**
     * Gets the SE name specified by user.
     *
     * return SE or SE group name if it was specified, otherwise an empty string
     */
    std::string getName();

    /**
     * Checks is all configurations for the given SE were requested
     *
     * return true if yes, fals eotherwise
     */
    bool all();

    /**
     * true if the user is quering the activity configuration for given VO,
     * false otherwise
     */
    bool vo();

    /**
     * true if the user is querying for the limited bandwidth of a given SE
     */
    bool getBandwidth();

    /**
     * Validates command line options
     * 1. Checks the endpoint
     * 2. If -h or -V option were used respective informations are printed
     * 3. GSoapContexAdapter is created, and info about server requested
     * 4. Additional check regarding server are performed
     * 5. If verbal additional info is printed
     *
     * @return GSoapContexAdapter instance, or null if all activities
     * 				requested using program options have been done.
     */
//	virtual optional<GSoapContextAdapter&> validate(bool init = true);
};

}
}

#endif /* GETCFGCLI_H_ */
