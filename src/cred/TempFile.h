/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TEMPFILE_H_
#define TEMPFILE_H_

#include <string>
#include <boost/utility.hpp>


/**
 * TempFile class. The purpose of this class is to unlink a temporary file
 * when this object goes out of scope
 */
class TempFile: boost::noncopyable
{
public:

    /**
     * Constructor
     */
    explicit TempFile(const std::string& prefix, const std::string& dir);

    /**
     * Destructor
     */
    ~TempFile() throw ();

    /**
     * Return The Filename
     */
    const std::string& name() const throw();

    /**
     * Rename the file and release the ownership (this is, name will NOT be unlinked)
     */
    void rename(const std::string& name);

private:
    /**
     * The Name of the temporary file
     */
    std::string m_filename;
};


#endif //TEMPFILE_H_
