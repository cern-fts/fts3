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
 * @file StreamPrt
 * @brief get a stream from a Clob (Oracle Stream smart Pointer)
 * @author Michail Salichos
 * @date 09/02/2012
 *
 **/

#pragma once

#include "occi.h"
#include <boost/utility.hpp>


/**
 * Oracle Stream smart Pointer. It will acquire the ownership of the
 * Stream object and then call closeStream when destroyed (RAII)
 */
template<class L>
class StreamPtr : boost::noncopyable
{
public:

    /**
     * Constructor
     */
    StreamPtr(L& lob,::oracle::occi::Stream * s) throw ():m_lob(lob),m_stream(s) {}

    /**
     * Destructor
     */
    ~StreamPtr() throw ()
    {
        try
            {
                if(0 != m_stream)
                    {
                        m_lob.closeStream(m_stream);
                    }
            }
        catch(...) {}
    }

    /**
     * Operator*
     */
    ::oracle::occi::Stream& operator*() const
    {
        return *m_stream;
    }

    /**
     * Operator->
     */
    ::oracle::occi::Stream* operator->() const throw()
    {
        return m_stream;
    }

    /**
     * Get the Statement Pointer
     */
    ::oracle::occi::Stream* get() const throw()
    {
        return m_stream;
    }

    /**
     * Get the Statement Pointer
     */
    ::oracle::occi::Stream* release() throw()
    {
        ::oracle::occi::Stream* s = m_stream;
        m_stream = 0;
        return s;
    }

private:
    /**
     * Default Constructor. Not Implemented
     */
    StreamPtr();

private:
    /**
     * The Statement
     */
    L&  m_lob;

    /**
     * The ResultSet
     */
    ::oracle::occi::Stream * m_stream;
};

