#pragma once
#ifndef _LOGGER
#define _LOGGER

#include <boost/thread/mutex.hpp>
#include <iostream>

class logger {
public:

    logger( std::ostream& os_);
     ~logger() {os << std::endl;}

    template<class T>
    friend logger& operator<<( logger& log, const T& output );

private:
    std::ostream& os;
};

logger::logger( std::ostream& os_) : os(os_) {}

template<class T>
logger& operator<<( logger& log, const T& output ) {
  if(log.os.good()){
    	log.os << output;
    	log.os.flush(); 
  }
 return log;
}

#endif
