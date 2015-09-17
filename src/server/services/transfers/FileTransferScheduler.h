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

#ifndef FILETRANSFERSCHEDULER_H_
#define FILETRANSFERSCHEDULER_H_

#include <vector>
#include <string>
#include <set>
#include <map>

#include "boost/tuple/tuple.hpp"
#include <boost/optional.hpp>
#include <boost/regex.hpp>

#include "db/generic/SingleDbInstance.h"


/**
 * The FileTransferScheduler class is responsible for scheduling transfer files
 * A file may either scheduled for transferring, in this case its status is changed
 * from Submitted to Ready, or for waiting until free credits are available for
 * this type of transfer.
 */
class FileTransferScheduler
{

    enum
    {
        SOURCE,
        DESTINATION,
        VO
    };

public:

    /**
     * Constructor
     *
     * @param file - the file for which the scheduling decision has to be taken
     */
    FileTransferScheduler(
        TransferFile const & file,
        std::vector< std::shared_ptr<ShareConfig> > cfgs,
        std::set<std::string> inses = std::set<std::string>(),
        std::set<std::string> outses = std::set<std::string>(),
        std::set<std::string> invos = std::set<std::string>(),
        std::set<std::string> outvos = std::set<std::string>()
    );
    FileTransferScheduler(const FileTransferScheduler&);

    /**
     * Destructor
     */
    ~FileTransferScheduler();

    /**
     * Makes the scheduling decision for the file that has been used to create the
     * object, changes the file status to ready is there are free credits
     *
     * @return returns true if file status has been changed to Ready, false otherwise
     */
    bool schedule(int &currentActive);

private:

    /// pointer to the file that has to be scheduled
    TransferFile const & file;
    /// name of the source SE
    std::string srcSeName;
    /// name of the destination SE
    std::string destSeName;

    std::vector< std::shared_ptr<ShareConfig> > cfgs;

    /// DB singleton instance
    GenericDbIfce* db;

    /**
     * Creates a could-not-allocate-credits error message
     *
     * @param cfg - the configuration that is not allowing to schedule a file transfer
     *
     * @return error message
     */
    std::string getNoCreditsErrMsg(ShareConfig* cfg);
};

#endif /* FILETRANSFERSCHEDULER_H_ */
