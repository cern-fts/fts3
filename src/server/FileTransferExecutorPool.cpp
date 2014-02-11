/*
 * FileTransferExecutorPool.cpp
 *
 *  Created on: Aug 9, 2013
 *      Author: simonm
 */

#include "FileTransferExecutorPool.h"

namespace fts3
{

namespace server
{

FileTransferExecutorPool::FileTransferExecutorPool(int size, TransferFileHandler& tfh, bool monitoringMsg, string infosys, string ftsHostName) :
    size (size),
    tfh(tfh),
    monitoringMsg(monitoringMsg),
    infosys(infosys),
    ftsHostName(ftsHostName),
    index(0),
    scheduled(0)
{
}

FileTransferExecutorPool::~FileTransferExecutorPool()
{
    vector<FileTransferExecutor*>::iterator it;

    for (it = executors.begin(); it != executors.end(); it++)
        {
            if (*it) delete *it;
        }
}

void FileTransferExecutorPool::add(TransferFiles* tf, bool optimize)
{
    // if needed add new worker
    if (executors.size() < size)
        {
            executors.push_back(new FileTransferExecutor(tfh, optimize, monitoringMsg, infosys, ftsHostName));
        }
    // add the task to worker's queue
    executors[index]->put(tf);
    index++;
    if(index == size) index = 0;
}

void FileTransferExecutorPool::join()
{
    vector<FileTransferExecutor*>::iterator it;

    // first let each of the worker threads know that there are no more data
    for (it = executors.begin(); it != executors.end(); it++)
        {
            if (*it)
                (*it)->noMoreData();
        }

    // then if a thread is active join
    for (it = executors.begin(); it != executors.end(); it++)
        {
            FileTransferExecutor* exec = *it;
            if(exec)
                {
                    if (exec->isActive())
                        {
                            exec->join();
                        }

                    scheduled += exec->getNumberOfScheduled();
                }
        }
}

void FileTransferExecutorPool::stopAll()
{
    vector<FileTransferExecutor*>::iterator it;

    for (it = executors.begin(); it != executors.end(); it++)
        {
            if (*it)
                (*it)->stop();
        }
}

} /* namespace server */
} /* namespace fts3 */
