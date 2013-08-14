/*
 * TransferHandlerPool.h
 *
 *  Created on: Aug 9, 2013
 *      Author: simonm
 */

#ifndef TRANSFERHANDLERPOOL_H_
#define TRANSFERHANDLERPOOL_H_

#include <vector>

#include "FileTransferExecutor.h"

#include "db/generic/TransferFiles.h"
#include "TransferFileHandler.h"


namespace fts3
{

namespace server
{

using namespace std;

class FileTransferExecutorPool
{


public:

    /**
     *
     */
    FileTransferExecutorPool(int size, TransferFileHandler& tfh, bool monitoringMsg, string infosys, string ftsHostName);

    /**
     *
     */
    virtual ~FileTransferExecutorPool();

    /**
     *
     */
    void add(TransferFiles* tf, bool optimize);

    /**
     *
     */
    void join();

private:
    vector<FileTransferExecutor*> executors;

    unsigned int index;
    unsigned int size;

    /// variables from process_service_handler
    TransferFileHandler& tfh;
    bool monitoringMsg;
    string infosys;
    string ftsHostName;
};

} /* namespace server */
} /* namespace fts3 */
#endif /* TRANSFERHANDLERPOOL_H_ */
