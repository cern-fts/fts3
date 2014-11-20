/*
 * CancelCli.h
 *
 *  Created on: Mar 5, 2014
 *      Author: simonm
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
     * 			vector containing job IDs otherwise an empty vector
     */
    std::vector<std::string> getJobIds()
    {
        return jobIds;
    }

private:

    void prepareJobIds();

    /**
     * the name of the file containing bulk-job description
     */
    std::string bulk_file;

    /**
     * Job IDs
     */
    std::vector<std::string> jobIds;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* CANCELCLI_H_ */
