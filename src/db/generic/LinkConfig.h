/*
 * LinkConfig.h
 *
 *  Created on: Nov 21, 2012
 *      Author: simonm
 */

#ifndef LINKCONFIG_H_
#define LINKCONFIG_H_

#include <string>


class LinkConfig {
public:
	LinkConfig(){};
	virtual ~LinkConfig(){};

	std::string source;
	std::string destination;
	std::string state;
	std::string symbolic_name;

    int NOSTREAMS;
    int TCP_BUFFER_SIZE;
    int URLCOPY_TX_TO;
    int NO_TX_ACTIVITY_TO;
};

#endif /* LINKCONFIG_H_ */
