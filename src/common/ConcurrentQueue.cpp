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

#include "ConcurrentQueue.h"

bool ConcurrentQueue::instanceFlag = false;
ConcurrentQueue* ConcurrentQueue::single = NULL;

ConcurrentQueue* ConcurrentQueue::getInstance()
{
    if (!instanceFlag)
        {
            single = new ConcurrentQueue();
            instanceFlag = true;
            return single;
        }
    else
        {
            return single;
        }
}


/**
 * Pop an element off the queue
 * If wait is 0, return null if the queue is empty, otherwise wait until an item is placed in the queue
 * Potentially in the future:
 *  wait = -1 => block until an item is placed in the queue, and return it
 *  wait = 0  => don't block, return null if the queue is empty
 *  wait > 0  => block for <block> seconds
 */
std::string ConcurrentQueue::pop(int wait)
{
    std::string ret;
    pthread_mutex_lock(&lock);

    /**
     * If the queue is empty
     *   if we're in non-blocking mode, just return
     *   otherwise wait on the condition to be triggered. The condition gets triggered
     *     either when something is added, in which case the while condition fails and
     *     the value gets returned, or the queue is changed into non-blocking mode and returns
     **/

    while(the_queue.empty())
        {
            if(wait == 0 || blck == 0)
                {
                    pthread_mutex_unlock(&lock);
                    return NULL;
                }

            pthread_cond_wait(&cv, &lock);
        }

    //we know there is at least one item, so dequeue one and return
    ret = the_queue.front();
    the_queue.pop();

    pthread_mutex_unlock(&lock);

    return ret;
}

//push on the queue
void ConcurrentQueue::push( std::string  val )
{
    pthread_mutex_lock(&lock);
    if( the_queue.size() < MAX_NUM_MSGS_MON)
        the_queue.push(val);
    pthread_mutex_unlock(&lock);
    pthread_cond_signal(&cv);
}

unsigned int ConcurrentQueue::size()
{
    //pthread_mutex_lock(&lock);
    unsigned int ret = (unsigned int)the_queue.size();
    //pthread_mutex_unlock(&lock);
    return ret;
}

bool ConcurrentQueue::empty()
{
    pthread_mutex_lock(&lock);
    bool ret = the_queue.empty();
    pthread_mutex_unlock(&lock);
    return ret;
}

//set in blocking mode, so a call to pop without args waits until something is added to the queue
void ConcurrentQueue::block()
{
    pthread_mutex_lock(&lock);
    blck = 1;
    pthread_mutex_unlock(&lock);
    pthread_cond_broadcast(&cv);
}

//set in non-blocking mode, so a call to pop returns immediately, returning NULL if the queue is empty
//also tells all currently blocking pop calls to return immediately
void ConcurrentQueue::nonblock()
{
    pthread_mutex_lock(&lock);
    blck = 0;
    pthread_mutex_unlock(&lock);
    pthread_cond_broadcast(&cv);
}
