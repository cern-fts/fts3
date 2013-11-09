/*
 * TransferHandler.h
 *
 *  Created on: Aug 9, 2013
 *      Author: simonm
 */

#ifndef TRANSFERHANDLER_H_
#define TRANSFERHANDLER_H_

#include <boost/thread.hpp>

#include "common/ThreadSafeQueue.h"

#include "db/generic/SingleDbInstance.h"

#include "TransferFileHandler.h"
#include "site_name.h"

#include <set>
#include <string>

namespace fts3
{

namespace server
{

using namespace db;
using namespace std;
using namespace boost;
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
    FileTransferExecutor(TransferFileHandler& tfh, bool optimize, bool monitoringMsg, string infosys, string ftsHostName);

    /**
     * Destructor.
     */
    virtual ~FileTransferExecutor();

    /**
     * Worker loop that is executed in a separate thread
     */
    void execute();

    /**
     * Add new FileTransfer to the queue
     *
     * @param tf - the FileTransfer
     */
    void put(TransferFiles* tf)
    {
        queue.put(tf);
    }

    /**
     * Calling this method indicates that no more data will be pushed into the queue
     */
    void noMoreData()
    {
        queue.noMoreData();
    }

    /**
     * @return true if the worker loop is active, fale otherwise
     */
    bool isActive()
    {
        return active;
    }

    /**
     * Waits until the worker thread finishes
     */
    void join()
    {
        t.join();
    }

    /**
     * Stops the worker thread
     */
    void stop()
    {
        active = false;
        // t.interrupt();
    }

    /**
     * Gets the number of TransferFiles that were scheduled
     *
     * @return number of TransferFiles that were scheduled
     */
    int getNumberOfScheduled()
    {
        return scheduled;
    }

private:

    /**
     * @return the metadata string
     */
    static string prepareMetadataString(std::string text);

    /// queue with all the tasks
    ThreadSafeQueue<TransferFiles> queue;

    /// pairs that were already checked and were not scheduled
    set< pair<string, string> > notScheduled;

    /// variables from process_service_handler
    TransferFileHandler& tfh;
    bool optimize;
    bool monitoringMsg;
    string infosys;
    string ftsHostName;
    SiteName siteResolver;

    // DB interface
    GenericDbIfce* db;

    /// worker thread
    thread t;

    /// state of the worker
    bool active;

    /// number of files that went to ready state
    int scheduled;

    /// url_copy command
    static const std::string cmd;

    std::string ftsHostname;
};

} /* namespace server */
} /* namespace fts3 */
#endif /* TRANSFERHANDLER_H_ */
