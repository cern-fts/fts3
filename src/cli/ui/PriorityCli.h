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

#ifndef PRIORITYCLI_H_
#define PRIORITYCLI_H_

#include "CliBase.h"

#include <boost/optional.hpp>
#include <string>

namespace fts3
{
namespace cli
{

class PriorityCli : public CliBase
{

public:
    /**
     *
     */
    PriorityCli();

    /**
     *
     */
    virtual ~PriorityCli();

    /**
     * Validates command line options.
     *  Checks that the priority was set correctly.
     *
     * @return GSoapContexAdapter instance, or null if all activities
     *              requested using program options have been done.
     */
    void validate();


    /**
     * Gives the instruction how to use the command line tool.
     *
     * @return a string with instruction on how to use the tool
     */
    std::string getUsageString(std::string tool) const;

    /**
     *
     */
    std::string getJobId()
    {
        return job_id;
    }

    /**
     *
     */
    int getPriority()
    {
        return priority;
    }

private:
    ///
    std::string job_id;
    ///
    int priority;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* PRIORITYCLI_H_ */
