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
 *
 */
class FileTransferExecutor
{

public:

    /**
     *
     */
    FileTransferExecutor(TransferFileHandler& tfh, bool optimize, bool monitoringMsg, string infosys, string ftsHostName);

    /**
     *
     */
    virtual ~FileTransferExecutor();

    /**
     *
     */
    void execute();

    /**
     *
     */
    void put(TransferFiles* tf)
    {
        queue.put(tf);
    }

    /**
     *
     */
    void noMoreData()
    {
        queue.noMoreData();
    }

    /**
     *
     */
    bool isActive()
    {
        return active;
    }

    /**
     *
     */
    void join()
    {
        t.join();
    }

	/**
	 *
	 */
	void stop()
	{
		active = false;
		// t.interrupt();
	}

private:

    static string prepareMetadataString(std::string text);

    /// queue with all the tasks
    ThreadSafeQueue<TransferFiles> queue;

    /// pairs that were already checked and were not scheduled
    set< pair<string, string> > notScheduled;

    /// state of the worker
    bool active;

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

    static const std::string cmd;
};

} /* namespace server */
} /* namespace fts3 */
#endif /* TRANSFERHANDLER_H_ */
