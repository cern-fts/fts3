/*
 * ShareConfig.h
 *
 *  Created on: Nov 21, 2012
 *      Author: simonm
 */

#ifndef SHARECONFIG_H_
#define SHARECONFIG_H_


class ShareConfig {
public:
	ShareConfig(){};
	~ShareConfig(){};

	std::string source;
	std::string destination;
	std::string vo;
	int active_transfers;
};

#endif /* SHARECONFIG_H_ */
