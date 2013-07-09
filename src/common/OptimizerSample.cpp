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


#define LOWTR 2
#define MAXTR 4


OptimizerSample::OptimizerSample()
{
}

OptimizerSample::~OptimizerSample()
{
}

OptimizerSample::OptimizerSample(int spf, int nf, int bs, float gp) : streamsperfile(spf), numoffiles(nf), bufsize(bs), goodput(gp), timeout(3600), file_id(0), throughput(0.0), avgThr(0.0)
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
                                    destActive, double trSuccessRateForPair, double numberOfFinishedAll, double numberOfFailedAll, double throughput,
                                    double avgThr)
{
    /*
            currectActive: number of active for a given src/dest pair
            sourceActive: number of active for a given source
            destActive:   number of active for a given dest
            trSuccessRateForPair: success rate of the last 10 transfers between src/dest, must always be > 90%
     */

    bool allowed = false;
    std::vector<struct transfersStore>::iterator iter;
    int activeInStore = 0;


    //check if this src/dest pair already exists
    if (transfersStoreVector.empty())
        {
            struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair, sourceSe,destSe,0, avgThr, 0};
            transfersStoreVector.push_back(initial);
        }
    else
        {
	    bool found = false;
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
                    struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair, sourceSe, destSe, 0, avgThr, 0};
                    transfersStoreVector.push_back(initial);
                }
        }

    for (iter = transfersStoreVector.begin(); iter < transfersStoreVector.end(); ++iter)
        {
            if ((*iter).source.compare(sourceSe) == 0 && (*iter).dest.compare(destSe) == 0)
                {
                    if((*iter).storedMaxActive <= currentActive) //keep count of the max active per pair
                        (*iter).storedMaxActive = currentActive;

                    if((*iter).numOfActivePerPair <= currentActive)
                        (*iter).numOfActivePerPair = currentActive;
			
                    if((*iter).numberOfFinishedAll != numberOfFinishedAll)  //one more tr finished
                        {
                            if(trSuccessRateForPair >= 99 && throughput > (*iter).throughput)
                                {
                                    if( (*iter).storedMaxActive == (*iter).numOfActivePerPair)
                                        {
                                            (*iter).numOfActivePerPair += 1;
                                            (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                        }
                                }
                            else if( trSuccessRateForPair >= 99 && throughput == (*iter).throughput)
                                {
                                    if(throughput > avgThr )
                                        {
                                            if( (*iter).storedMaxActive == (*iter).numOfActivePerPair)
                                                {
                                                    (*iter).numOfActivePerPair += 0;
                                                    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                                }
                                        }
                                    else
                                        {
                                            (*iter).numOfActivePerPair -= 1;
					    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                        }
                                }
                            else if( trSuccessRateForPair >= 99 && throughput < (*iter).throughput)
                                {
                                    if(throughput > avgThr)
                                        {
                                            if( (*iter).storedMaxActive == (*iter).numOfActivePerPair)
                                                {
                                                    (*iter).numOfActivePerPair += 0;
                                                    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                                }
                                        }
                                    else
                                        {
                                            (*iter).numOfActivePerPair -= 2;
					    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                        }
                                }
                            else if( trSuccessRateForPair < 99)
                                {
                                    (*iter).numOfActivePerPair -= 2;
				    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                }

                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                            (*iter).avgThr = avgThr;
                        }
                    else if((*iter).numberOfFailedAll != numberOfFailedAll)
                        {
                            (*iter).numOfActivePerPair -= 2;
                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                            (*iter).avgThr = avgThr;
			    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                        }
		    			
                    if((*iter).numOfActivePerPair <=0 )
                        activeInStore = 0;
                    else
                        activeInStore = (*iter).numOfActivePerPair;
                    break;
                }
        }


    if (sourceActive == 0 && destActive == 0)
        {
            return true;
        }
    else if (currentActive <= (trSuccessRateForPair >= 99? MAXTR: LOWTR ) )
        {
            return true;
        }
    else if(currentActive < activeInStore)
        {
            return true;
        }
    else
        {
            return false;
        }

    return allowed;
}

int OptimizerSample::getFreeCredits(int numFinished, int numFailed, std::string sourceSe, std::string destSe, int currentActive, int, int,
                                    double trSuccessRateForPair, double numberOfFinishedAll, double numberOfFailedAll, double throughput, double avgThr)
{

    std::vector<struct transfersStore>::iterator iter;
    int activeInStore = 0;


    //check if this src/dest pair already exists
    if (transfersStoreVector.empty())
        {
            struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair,sourceSe,destSe, throughput, avgThr, 0};
            transfersStoreVector.push_back(initial);
        }
    else
        {
	    bool found = false;
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
                    struct transfersStore initial = {numberOfFinishedAll,numberOfFinishedAll, numFinished, numFailed, currentActive, trSuccessRateForPair,sourceSe, destSe, throughput, avgThr, 0};
                    transfersStoreVector.push_back(initial);
                }
        }

    for (iter = transfersStoreVector.begin(); iter < transfersStoreVector.end(); ++iter)
        {

            if ((*iter).source.compare(sourceSe) == 0 && (*iter).dest.compare(destSe) == 0)
                {

                   if((*iter).storedMaxActive <= currentActive) //keep count of the max active per pair
                        (*iter).storedMaxActive = currentActive;

                    if((*iter).numOfActivePerPair <= currentActive)
                        (*iter).numOfActivePerPair = currentActive;
			
                    if((*iter).numberOfFinishedAll != numberOfFinishedAll)  //one more tr finished
                        {
                            if(trSuccessRateForPair >= 99 && throughput > (*iter).throughput)
                                {
                                    if( (*iter).storedMaxActive == (*iter).numOfActivePerPair)
                                        {
                                            (*iter).numOfActivePerPair += 1;
                                            (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                        }
                                }
                            else if( trSuccessRateForPair >= 99 && throughput == (*iter).throughput)
                                {
                                    if(throughput > avgThr )
                                        {
                                            if( (*iter).storedMaxActive == (*iter).numOfActivePerPair)
                                                {
                                                    (*iter).numOfActivePerPair += 0;
                                                    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                                }
                                        }
                                    else
                                        {
                                            (*iter).numOfActivePerPair -= 1;
					    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                        }
                                }
                            else if( trSuccessRateForPair >= 99 && throughput < (*iter).throughput)
                                {
                                    if(throughput > avgThr)
                                        {
                                            if( (*iter).storedMaxActive == (*iter).numOfActivePerPair)
                                                {
                                                    (*iter).numOfActivePerPair += 0;
                                                    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                                }
                                        }
                                    else
                                        {
                                            (*iter).numOfActivePerPair -= 2;
					    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                        }
                                }
                            else if( trSuccessRateForPair < 99)
                                {
                                    (*iter).numOfActivePerPair -= 2;
				    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
                                }

                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                            (*iter).avgThr = avgThr;
                        }
                    else if((*iter).numberOfFailedAll != numberOfFailedAll)
                        {
                            (*iter).numOfActivePerPair -= 2;
                            (*iter).numFinished = numFinished;
                            (*iter).numFailed = numFailed;
                            (*iter).successRate = trSuccessRateForPair;
                            (*iter).numberOfFinishedAll = numberOfFinishedAll;
                            (*iter).numberOfFailedAll = numberOfFailedAll;
                            (*iter).throughput = throughput;
                            (*iter).avgThr = avgThr;
			    (*iter).storedMaxActive = (*iter).numOfActivePerPair;
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
