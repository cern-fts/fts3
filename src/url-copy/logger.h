#pragma once
#ifndef _LOGGER
#define _LOGGER

#include <boost/thread/mutex.hpp>
#include <iostream>

class logger {
public:

    logger( std::ostream& os_, boost::mutex& mutex_ );
     ~logger() {os << std::endl;}

    template<class T>
    friend logger& operator<<( logger& log, const T& output );

private:
    std::ostream& os;
    boost::mutex& mutex;
};

logger::logger( std::ostream& os_, boost::mutex& mutex_) : os(os_), mutex(mutex_) {}

template<class T>
logger& operator<<( logger& log, const T& output ) {
  if(log.os.good()){
        boost::mutex::scoped_lock lock(log.mutex);
    	log.os << output;
    	log.os.flush(); 
  }
 return log;
}

#endif
