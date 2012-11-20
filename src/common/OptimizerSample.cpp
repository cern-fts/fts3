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

OptimizerSample::OptimizerSample(){
}

OptimizerSample::~OptimizerSample(){
}

OptimizerSample::OptimizerSample(int spf, int nf, int bs, float gp): streamsperfile(spf), numoffiles(nf), bufsize(bs), goodput(gp){
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

bool OptimizerSample::transferStart(std::string sourceSe, std::string destSe, int currentActive, int sourceActive, int destActive, double trSuccessRateForPair, double lastOneThroughput, int numOfPairInDB){
/*
	currectActive: number of active between src/dest
	sourceActive: number of active for a given source
	destActive:   number of active for a given dest
	trSuccessRateForPair: success rate of last 10 transfers between src/dest, must always be > 90%
	lastOneThroughput: througput of the last transfer between src/dest, must be > 1Mbit/sec
	numOfPairInDB: number of records(no matter the state) in the db for a given source/dest pair
*/

	bool allowed = false;
	std::vector<struct transfersStore>::iterator iter;
	struct transfersStore initial = {currentActive,trSuccessRateForPair,sourceSe, destSe};
	int activeInStore = 0;
	int successInStore = 0;
	bool found = false;	
	
	for ( iter=transfersStoreVector.begin() ; iter < transfersStoreVector.end(); iter++ ){		
		if((*iter).source.compare(sourceSe)==0 && (*iter).dest.compare(destSe)==0){
			if(trSuccessRateForPair>=90){
				(*iter).numOfActivePerPair = currentActive + 2; 
			(*iter).successRate = trSuccessRateForPair; 													
			}else{	
				if(trSuccessRateForPair > (*iter).successRate)			
					(*iter).numOfActivePerPair = currentActive + 1; 
				else
					(*iter).numOfActivePerPair = currentActive - 1; 				
				
				(*iter).successRate = trSuccessRateForPair;								
			}

			activeInStore = (*iter).numOfActivePerPair;
			if(activeInStore <= 0)
				 activeInStore = 1;
			successInStore = (*iter).successRate;			 			
			found  = true;
			break;
		}			
	}
			
	if(found==false){							
		transfersStoreVector.push_back(initial);
	}								
	
	if(sourceActive==0 && destActive==0){ //no active for src and dest, simply let it start
		allowed = true;
	}else if(numOfPairInDB<10 && destActive<10){ //allow not more than 10 per dest if we do not have enough samples per se pair
		allowed = true;
	}else if(numOfPairInDB<10 && destActive>10){ //wait until we collect 10 samples for each src/dest pair
		allowed = false;		
	}else{ 	
		if(currentActive<=activeInStore){
			allowed = true;	
		}else{
			allowed = false;			
		}		
	}
return allowed;
}
