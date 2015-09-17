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

#ifndef TRANSFERHANDLER_H_
#define TRANSFERHANDLER_H_

#include <boost/thread.hpp>
#include <boost/any.hpp>

#include "db/generic/SingleDbInstance.h"

#include "TransferFileHandler.h"

#include <set>
#include <string>

namespace fts3
{

namespace server
{

using namespace db;
using namespace fts3::common;

/**
 * FileTransferExecutor is the worker class that executes
 * FileTransfers one by one in a separate thread.
 *
 * The FileTransfers are added to a thread-safe queue, which is processed by the worker.
 * In order to let the worker thread know that all the date were pushed into the queue the
 * 'noMoreData()' method should be used, in order to wait for the thread to finish processing
 * FileTransfers the 'join()' method should be used (before calling join, noMoreData should be called)
 */
class FileTransferExecutor
{

public:

    /**
     * Constructor.
     *
     * @param tfh - the TransferFileHandler reference
     * @param optimize - flag stating if optimization is ON
     * @param monitoringMsg - is true if monitoring messages are in use
     * @param infosys - information system host
     * @param ftsHostName - hostname of the machine hosting FTS3
     */
    FileTransferExecutor(TransferFile& tf, TransferFileHandler& tfh, bool monitoringMsg, std::string infosys, std::string ftsHostName, std::string proxy, std::string logDir);

    /**
     * Destructor.
     */
    virtual ~FileTransferExecutor();

    /**
     * spawns a url_copy
     *
     * @return 0 if the file was not scheduled, 1 otherwise
     */
    virtual void run(boost::any &);

private:

    /// pairs that were already checked and were not scheduled
    std::set< std::pair<std::string, std::string> > notScheduled;

    /// variables from process_service_handler
    TransferFile tf;
    TransferFileHandler const & tfh;
    bool monitoringMsg;
    std::string infosys;
    std::string ftsHostName;
    std::string proxy;
    std::string logsDir;

    // DB interface
    GenericDbIfce* db;
};

} /* namespace server */
} /* namespace fts3 */
#endif /* TRANSFERHANDLER_H_ */
