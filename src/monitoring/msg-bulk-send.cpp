/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * The following code initiates various threads which are used to read messages from named pipes
 * as also to sent those messages to the broker
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <vector>
#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
#include <decaf/lang/Long.h>
#include <decaf/util/Date.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>
#include <activemq/library/ActiveMQCPP.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <string.h>
#include <errno.h>
#include <iostream>
#include "MsgPipe.h"
#include "MsgProducer.h"
#include "concurrent_queue.h"
#include "half_duplex.h"
#include "utility_routines.h"
#include "name_to_uid.h"

using namespace std;


/*
	There is a parent process which checks/monitors the status of the child.
	If the child hangs or for any reason die, transparently it's restarting it and restoring all mesgs
*/

void DoServer() throw()
{
    try
        {
            activemq::library::ActiveMQCPP::initializeLibrary();

            //initialize here to avoid race conditions
            concurrent_queue::getInstance();

            MsgPipe pipeMsg1;
            MsgProducer producer;

            // Start the pipe thread.
            Thread pipeThread(&pipeMsg1);
            pipeThread.start();

            // Start the producer thread.
            Thread producerThread(&producer);
            producerThread.start();


            // Wait for the threads to complete.
            pipeThread.join();
            producerThread.join();

            pipeMsg1.cleanup();
            producer.cleanup();

            activemq::library::ActiveMQCPP::shutdownLibrary();
        }
    catch (const std::exception& e)
        {
            std::cerr << "Exception caught: " << e.what() << std::endl;
        }
    catch (...)
        {
            std::cerr << "Unexpected exception! Aborting" << std::endl;
        }
}

int main(int argc,  char** argv)
{
    //switch to non-priviledged user to avoid reading the hostcert
    uid_t pw_uid = name_to_uid();
    setuid(pw_uid);
    seteuid(pw_uid);
   
    if(argc > 1) //if any param is provided stay attached to terminal
    {
	DoServer();
    }

    int d =  daemon(0,0);
    if(d < 0)
        std::cerr << "Can't set daemon, will continue attached to tty" << std::endl;

    int result = fork();

    if (result == 0)
        {
            DoServer();
        }

    if (result < 0)
        {
            exit(1);
        }

    for (;;)
        {
            int status = 0;
            waitpid(-1, &status, 0);

            if (!WIFSTOPPED(status))
                {
                    sleep(60);
                    result = fork();
                    if (result == 0)
                        {
                            DoServer();
                        }
                    if (result < 0)
                        {
                            exit(1);
                        }
                }
            sleep(60);
        }

    return 0;
}
