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
#ifndef GLITE_DATA_AGENTS_EXTRA_TEMPFILE_H_
#define GLITE_DATA_AGENTS_EXTRA_TEMPFILE_H_

#include <string>
#include <boost/utility.hpp>
#include <errno.h>
#include <stdio.h> 
#include <string.h> 
#include "common/logger.h"
#include "common/error.h"

using namespace FTS3_COMMON_NAMESPACE; 


/**
 * TempFile class. The purpose of this class is to unlink a temporary file 
 * when this object goes out of scope
 */	
class TempFile: boost::noncopyable{
public:
	
    /**
     * Constructor
     */
    explicit TempFile(const std::string& fname) throw ():m_filename(fname){
    }
	 
    /**
     * Destructor
     */
    ~TempFile() throw () {
        unlink();
    }
 
    /**
     * Return The Filename
     */
    const std::string& name() const throw() {return m_filename;}
    
    /**
     * Rename the file and release the owership
     */
    void rename(const std::string& name)  {
        // Check Preconditions
        if(m_filename.empty()){
   	    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "empty TempFile name" << commit;
        }
        if(name.empty()){
   	    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "empty destination name" << commit;	
        }
        // Rename File
        int r = ::rename(m_filename.c_str(),name.c_str());
        if(0 != r){
            std::string reason = (std::string)"Cannot rename temporary file. Error is: " +
                strerror(errno);
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << reason << commit;
        }
        // Release Ownership
        this->release();
    }
    
    /**
     * Release the ownership without unlinking the file 
     */
    void release() throw() {
        m_filename.clear();
    }
    
    /**
     * Unlink the file
     */
    void unlink() throw() {
        try{
            if(false == m_filename.empty()){
                ::unlink(m_filename.c_str());
            }
        }catch(...){}
        m_filename.clear();
    }

    /**
     * Generate a new temporary file. 
     */
    static std::string generate(const std::string& prefix, const std::string& dir, int * fd) {
        // Check Procondition
        if(0 == fd){
	    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "null File Descriptor pointer" << commit;
        }
        if(true == prefix.empty()){
	    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "empty Prefix" << commit;
        }
        char tmp_proxy[FILENAME_MAX];
        if(false == dir.empty()){
            snprintf(tmp_proxy,FILENAME_MAX,"%s/%s.XXXXXX",dir.c_str(),prefix.c_str());
        } else {
            snprintf(tmp_proxy,FILENAME_MAX,"%s.XXXXXX",prefix.c_str());
        }
        *fd = mkstemp(tmp_proxy);
        if (*fd == -1) {
            std::string reason = (std::string)"Cannot create temporary file <" + 
                tmp_proxy + ">.	Error is: " + strerror(errno);
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << reason << commit;
        }
        return tmp_proxy;
    }

private:
    /**
     * Default Constructor. Not implemented
     */
    TempFile();
	
private:
    /**
     * The Name of the temporary file
     */
    std::string m_filename;
};


#endif //GLITE_DATA_AGENTS_EXTRA_TEMPFILE_H_
