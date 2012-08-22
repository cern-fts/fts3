#pragma once


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
	int streamsperfile;
	int numoffiles;
	int bufsize;
	float goodput;
	int timeout;
	int file_id;

};

