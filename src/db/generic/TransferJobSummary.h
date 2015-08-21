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

#pragma once

#include <iostream>
#include <vector>
#include "JobStatus.h"

class TransferJobSummary
{

public:
    TransferJobSummary():numReady(0), numFinishing(0),numAwaitingPrestage(0),numPrestaging(0),numWaitingCatalogRegistration(0),numWaitingCatalogResolution(0),numWaitingPrestage(0),jobStatus(NULL), numDone(0), numActive(0),numPending(0),numCanceled(0),numCanceling(0), numFailed(0),numFinished(0), numSubmitted(0), numHold(0), numWaiting(0), numCatalogFailed(0), numRestarted(0) {}
    TransferJobSummary(const TransferJobSummary&);
    ~TransferJobSummary() {}

    int numReady;
    int numFinishing;
    int numAwaitingPrestage;
    int numPrestaging;
    int numWaitingCatalogRegistration;
    int numWaitingCatalogResolution;
    int numWaitingPrestage;
    /**
     * String containing the current state of the job.
     */
    JobStatus* jobStatus;

    /**
     * Number of files belonging to the job that are currently in the 'Done' state.
     */
    int numDone;

    /**
     * Number of files belonging to the job that are currently in the 'Active' state.
     */
    int numActive;

    /**
     * Number of files belonging to the job that are currently in the 'Pending' state.
     */
    int numPending;

    /**
     * Number of files belonging to the job that are currently in the 'Canceled' state.
     */
    int numCanceled;

    /**
     * Number of files belonging to the job that are currently in the 'Canceling' state.
     */
    int numCanceling;

    /**
     * Number of files belonging to the job that are currently in the 'Failed' state.
     */
    int numFailed;

    /**
     * Number of files belonging to the job that are currently in the 'Finished' state.
     */
    int numFinished;

    /**
     * Number of files belonging to the job that are currently in the 'Submitted' state.
     */
    int numSubmitted;

    /**
     * Number of files belonging to the job that are currently in the 'Hold' state.
     */
    int numHold;

    /**
     * Number of files belonging to the job that are currently in the 'Waiting' state.
     */
    int numWaiting;

    /**
     * Number of files belonging to the job that are currently in the 'Done' state.
     */
    int numCatalogFailed;

    /**
     * Indicates how many files belonging to the job have had to be retried (numRetries > 0).
     */
    int numRestarted;
};
