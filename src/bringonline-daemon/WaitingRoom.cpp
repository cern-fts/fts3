/*
 * WaitingRoom.cpp
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#include "WaitingRoom.h"
extern bool stopThreads;


void WaitingRoom::run()
{
    WaitingRoom& me = WaitingRoom::instance();

    while (true)
        {
            if(stopThreads) return;//either  gracefully or not

            {
                // lock the mutex
                boost::mutex::scoped_lock lock(me.m);
                // get current time
                time_t now = time(NULL);
                // iterate over all all tasks
                boost::ptr_list<StagingTask>::iterator it, next = me.tasks.begin();
                while ((it = next) != me.tasks.end())
                    {
                        if(stopThreads)
                            return;
                        // next item to check
                        ++next;
                        // if the time has not yet come for the task simply continue
                        if (it->waiting(now)) continue;
                        // otherwise start the task
                        me.pool->start(me.tasks.release(it).release());
                    }
            }
            // wait a while before checking again
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }
}
