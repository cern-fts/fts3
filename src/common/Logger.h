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

/**
 * @file Logger.h
 * @brief simple log4cpp logger wrapper
 * @author Michail Salichos
 * @date 09/02/2012
 * 
 **/


#pragma once

#include <stdio.h>
#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/SimpleLayout.hh>
#include "MutexLocker.h"
#include <boost/scoped_ptr.hpp>
 
#define LOGFILE "/tmp/database.log"


class Logger{

public:
	~Logger();
	
    static Logger & instance() {
        if (i.get() == 0) {
            MutexLocker obtain_lock(m);
            if (i.get() == 0) {
                i.reset(new Logger);
            }
        }
        return *i;
    }
    
    log4cpp::Category& category(){
    	return log4cpp::Category::getInstance("Category");
    }
    
    void info(std::string message);
    void warn(std::string message);    
    void error(std::string message);        
    

private:
	Logger();  // Private so that it can  not be called
	Logger(Logger const&){};             // copy constructor is private
	Logger& operator=(Logger const&){};  // assignment operator is private
	
	log4cpp::Appender *appender;
	log4cpp::Layout *layout;
	std::string getTimeString(std::string message);
	
	
    static boost::scoped_ptr<Logger> i;
    static Mutex m;	
};
