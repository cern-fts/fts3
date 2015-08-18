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

#pragma once

#include <vector>
#include <iostream>
#include <ctime>
#include <boost/thread.hpp>

#include "common_dev.h"


struct transfersStore
{
    double numberOfFinishedAll;
    double numberOfFailedAll;
    int numFinished;
    int numFailed;
    int numOfActivePerPair;
    double successRate;
    std::string source;
    std::string dest;
    double throughput;
    double avgThr;
    int storedMaxActive;
};


class OptimizerSample
{

public:
    OptimizerSample();
    ~OptimizerSample();
    OptimizerSample(int spf, int nf, int bs, float gp);
    int getStreamsperFile();
    int getNumOfFiles();
    int getBufSize();
    float getGoodput();
    int getTimeout();
    bool transferStart(int numFinished, int numFailed, std::string sourceSe, std::string destSe, int currentActive, int sourceActive, int destActive, double
                       lastTenSuccessRate, double numberOfFinishedAll, double numberOfFailedAll, double throughput, double avgThr, int lowDefault, int highDefault);

    int getFreeCredits(int numFinished, int numFailed, std::string sourceSe, std::string destSe, int currentActive, int sourceActive, int destActive, double
                       lastTenSuccessRate, double numberOfFinishedAll, double numberOfFailedAll, double throughput, double avgThr);

    int streamsperfile;
    int numoffiles;
    int bufsize;
    float goodput;
    int timeout;
    int file_id;
    double throughput;
    double avgThr;

private:
    std::vector<struct transfersStore> transfersStoreVector;
    mutable boost::mutex _mutex;

};

