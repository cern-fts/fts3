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

