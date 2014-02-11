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

#ifdef __STDC_NO_ATOMICS__
#include <atomic>
#else
#if BOOST_VERSION >= 105300
#include <boost/atomic.hpp>
#else
#include <stdatomic.h>
#endif
#endif




namespace fts3
{

namespace server
{

using namespace std;

/**
 * A pool of FileTransferExecutors. The pool size is set at the construction time.
 * New TransferFiles are added in round-robin fashion to the queues of the FileTransferExecutors.
 */
class FileTransferExecutorPool
{


public:

    /**
     * Constructor.
     *
     * @params size - pool size (number of worker threads)
     * @param tfh - reference to the TranferFileHandler
     * @param monitoringMsg - should be true if monitoring messages are in use
     * @param infosys - information system host
     * @param ftsHostName - hostname of the machine hosting FTS3
     */
    FileTransferExecutorPool(int size, TransferFileHandler& tfh, bool monitoringMsg, string infosys, string ftsHostName);

    /**
     * Destructor.
     */
    virtual ~FileTransferExecutorPool();

    /**
     * Adds new TransferFile to the queue of one of the FileTransferExecutors
     *
     * @param tf - the TransferFile to be carried out
     * @param optimize - should be true if autotuning should be used for this FileTransfer
     */
    void add(TransferFiles* tf, bool optimize);

    /**
     * Waits until all the FileTransferExecutors in the pool finish their job
     */
    void join();

    /**
     * Stops all the threads in the pool
     */
    void stopAll();

    /**
     * @return gets the number of files that were scheduled by the executors in the pool
     */
    int getNumberOfScheduled()
    {
        return scheduled;
    }

private:

    /// vector with executors
    vector<FileTransferExecutor*> executors;

    /// the next queue (executor) to be used
    unsigned int index;
    /// size of the pool
    unsigned int size;

    /// variables from process_service_handler
    TransferFileHandler& tfh;
    bool monitoringMsg;
    string infosys;
    string ftsHostName;

    /// number of files scheduled by the thread pool
#ifdef __STDC_NO_ATOMICS__
    std::atomic<int> scheduled;
#else
#if BOOST_VERSION >= 105300
    boost::atomic<int> scheduled;
#else
    std::atomic_int scheduled;
#endif
#endif        
   
};

} /* namespace server */
} /* namespace fts3 */
#endif /* TRANSFERHANDLERPOOL_H_ */
