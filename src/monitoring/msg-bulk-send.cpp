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
#include "MsgRecoverPipe.h"
#include "MsgProducer.h"
#include "concurrent_queue.h"
#include "half_duplex.h"
#include "utility_routines.h"

using namespace std;


/*
The design of the daeomon changed.
Now there is a parent process which checks/monitors the status of the child.
If the child hangs or for any reason crashes, transparently is restarting it and restore all mesgs
*/

void DoServer() {

    activemq::library::ActiveMQCPP::initializeLibrary();
    //initialize here to avoid race conditions  
    concurrent_queue::getInstance();

    MsgPipe pipeMsg1(HALF_DUPLEX);
    MsgPipe pipeMsg2(HALF_DUPLEX2);
    MsgPipe pipeMsg3(HALF_DUPLEX3);
    MsgProducer producer;


    // Start the producer thread.
    Thread pipeThread(&pipeMsg1);
    pipeThread.start();

    // Start the producer thread.
    Thread pipeThread2(&pipeMsg2);
    pipeThread2.start();

    Thread pipeThread3(&pipeMsg3);
    pipeThread3.start();


    // Start the producer thread.
    Thread producerThread(&producer);
    producerThread.start();


    // Wait for the threads to complete.
    pipeThread.join();
    pipeThread2.join();
    pipeThread3.join();
    producerThread.join();

    pipeMsg1.cleanup();
    pipeMsg2.cleanup();
    pipeMsg3.cleanup();
    producer.cleanup();

    activemq::library::ActiveMQCPP::shutdownLibrary();

}

int main() {

    daemon(0,0);

    //create a recover msgs thread using the last named pipe (barely used)
    MsgRecoverPipe pipeRecover(HALF_DUPLEX3);
    Thread pipeRecoverThread(&pipeRecover);

    int result = fork();

    if (result == 0) {
        DoServer();
    }

    if (result < 0) {
        exit(1);
    }

    for (;;) {
        int status = 0;
        waitpid(-1, &status, 0);

        if (!WIFSTOPPED(status)) {	   
            //Child died, start thread here, reading messages in the parent process
            if (pipeRecoverThread.isAlive() == false)
                pipeRecoverThread.start();
            result = fork();
            if (result == 0) {
                DoServer();
            }
	    pipeRecoverThread.yield();
            //re-sent lost messages back to the pipe
            pipeRecover.sendRecoveredMessages();
            //stop parent reading here, child is active
            if (pipeRecoverThread.isAlive() == false)
                pipeRecover.cleanup();

            if (result < 0) {
                exit(1);
            }
        }
        sleep(10);
    }
    return 0;
}
