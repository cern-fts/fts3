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

#ifndef CANCELCLI_H_
#define CANCELCLI_H_

#include "JobIdCli.h"

#include <vector>
#include <string>

namespace fts3
{
namespace cli
{

class CancelCli : public JobIdCli
{

public:
    CancelCli();
    virtual ~CancelCli();

    /**
     * parses the bulk submission file if necessary
     */
    void validate();

    /**
     * Gets a vector with job IDs.
     *
     * @return if job IDs were given as command line parameters a
     *          vector containing job IDs otherwise an empty vector
     */
    std::vector<std::string> getJobIds()
    {
        return jobIds;
    }

    bool cancelAll();

    std::string getVoName();

private:

    void prepareJobIds();

    /**
     * the name of the file containing bulk-job description
     */
    std::string bulk_file;

    /**
     * The vo filter for cancel-all
     */
    std::string vo_name;

    /**
     * Job IDs
     */
    std::vector<std::string> jobIds;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* CANCELCLI_H_ */
