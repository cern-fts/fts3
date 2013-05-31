/********************************************//**
 * Copyright @ Members of the EMI Collaboration, 2010.
 * See www.eu-emi.eu for details on the copyright holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***********************************************/

#include "OptimizerSample.h"

OptimizerSample::OptimizerSample()
{
}

OptimizerSample::~OptimizerSample()
{
}

OptimizerSample::OptimizerSample(int spf, int nf, int bs, float gp) : streamsperfile(spf), numoffiles(nf), bufsize(bs), goodput(gp), timeout(3600), file_id(0), throughput(0.0)
{
}

int OptimizerSample::getBufSize()
{
    return bufsize;
}

float OptimizerSample::getGoodput()
{
    return goodput;
}

int OptimizerSample::getNumOfFiles()
{
    return numoffiles;
}

int OptimizerSample::getStreamsperFile()
{
    return streamsperfile;
}

int OptimizerSample::getTimeout()
{
    return timeout;
}

bool OptimizerSample::transferStart(int numFinished, int numFailed, std::string sourceSe, std::string destSe, int currentActive, int sourceActive, int
                                    destActive, double trSuccessRateForPair, double numberOfFinishedAll, double numberOfFailedAll, double throughput)
{
    /*
            currectActive: number of active for a given src/dest pair
            sourceActive: number of active for a given source
            destActive:   number of active for a given dest
            trSuccessRateForPair: success rate of the last 10 transfers between src/dest, must always be > 90%
     */

    bool allowed = false;
    std::vector<struct transfersStore>::iterator iter;
    bool found = false;
    int activeInStore = 0;


    //check if this src/dest pair already exists
    if (transfersStoreVector.empty())
        {
            struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair, sourceSe, destSe, throughput};
            transfersStoreVector.push_back(initial);
        }
    else
        {
            for (iter = transfersStoreVector.begin(); iter < transfersStoreVector.end(); ++iter)
                {
                    if ((*iter).source.compare(sourceSe) == 0 && (*iter).dest.compare(destSe) == 0)
                        {
                            found = true;
                            break;
                        }
                }
            if (!found)
                {
                    struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair, sourceSe, destSe, throughput};
                    transfersStoreVector.push_back(initial);
                }
        }

    for (iter = transfersStoreVector.begin(); iter < transfersStoreVector.end(); ++iter)
        {

            if ((*iter).source.compare(sourceSe) == 0 && (*iter).dest.compare(destSe) == 0)
                {

                    if((*iter).numberOfFinishedAll != numberOfFinishedAll)  //one more tr finished
                        {
                            if(trSuccessRateForPair >= 98 && throughput > (*iter).throughput)
                                {
                                    (*iter).numOfActivePerPair += 2;
                                }
                            else if( trSuccessRateForPair >= 98 && throughput == (*iter).throughput)
                                {
                                    (*iter).numOfActivePerPair += 0;
                                }
                            else if( trSuccessRateForPair >= 98 && throughput < (*iter).throughput)
                                {
                                    (*iter).numOfActivePerPair -= 1;
                                }
                            else if( trSuccessRateForPair < 98)
                                {
                                    (*iter).numOfActivePerPair -= 2;
                                }
                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                        }
                    else if((*iter).numberOfFailedAll != numberOfFailedAll)
                        {
                            (*iter).numOfActivePerPair -= 4;
                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                        }

                    if((*iter).numOfActivePerPair <=0 )
                        activeInStore = 1;
                    else
                        activeInStore = (*iter).numOfActivePerPair;
                    break;
                }
        }


    if (sourceActive == 0 && destActive == 0)   //no active for src and dest, simply let it start
        {
            return true;
        }
    else if (currentActive <= 5)     //allow no more than 4 per pair if we do not have enough samples
        {
            return true;
        }
    else
        {
            if (currentActive < activeInStore)
                {
                    return true;
                }
            else
                {
                    return false;
                }
        }

    return allowed;
}

int OptimizerSample::getFreeCredits(int numFinished, int numFailed, std::string sourceSe, std::string destSe, int currentActive, int, int, double trSuccessRateForPair, double numberOfFinishedAll, double numberOfFailedAll, double throughput)
{

    std::vector<struct transfersStore>::iterator iter;
    bool found = false;
    int activeInStore = 0;


    //check if this src/dest pair already exists
    if (transfersStoreVector.empty())
        {
            struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair, sourceSe, destSe, throughput};
            transfersStoreVector.push_back(initial);
        }
    else
        {
            for (iter = transfersStoreVector.begin(); iter < transfersStoreVector.end(); ++iter)
                {
                    if ((*iter).source.compare(sourceSe) == 0 && (*iter).dest.compare(destSe) == 0)
                        {
                            found = true;
                            break;
                        }
                }
            if (!found)
                {
                    struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair, sourceSe, destSe, throughput};
                    transfersStoreVector.push_back(initial);
                }
        }

    for (iter = transfersStoreVector.begin(); iter < transfersStoreVector.end(); ++iter)
        {

            if ((*iter).source.compare(sourceSe) == 0 && (*iter).dest.compare(destSe) == 0)
                {

                    if((*iter).numberOfFinishedAll != numberOfFinishedAll)   //one more tr finished
                        {
                            if(trSuccessRateForPair >= 98 && throughput > (*iter).throughput)
                                {
                                    (*iter).numOfActivePerPair += 2;
                                }
                            else if( trSuccessRateForPair >= 98 && throughput == (*iter).throughput)
                                {
                                    (*iter).numOfActivePerPair += 0;
                                }
                            else if( trSuccessRateForPair >= 98 && throughput < (*iter).throughput)
                                {
                                    (*iter).numOfActivePerPair -= 1;
                                }
                            else if( trSuccessRateForPair < 98)
                                {
                                    (*iter).numOfActivePerPair -= 2;
                                }
                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                        }
                    else if((*iter).numberOfFailedAll != numberOfFailedAll)
                        {
                            (*iter).numOfActivePerPair -= 4;
                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                        }

                    activeInStore = (*iter).numOfActivePerPair - currentActive;


                    if(activeInStore <= 0 && currentActive == 0)
                        activeInStore = 1;
                    else if (activeInStore < 0)
                        {
                            activeInStore = 0;
                        }

                    break;
                }
        }

    return activeInStore;
}
