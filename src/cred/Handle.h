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

#ifndef HANDLE_H_
#define HANDLE_H_

#include <unistd.h>
#include <boost/utility.hpp>


/**
 * Handle class. The purpose of this class is to close an handle
 * when this object goes out of scope
 */
class Handle
{
public:

    /**
     * Constructor
     */
    Handle() throw ():m_handle(-1)
    {
    }

    /**
     * Constructor
     */
    Handle(int h) throw ():m_handle(h)
    {
    }

    /**
     * Constructor
     */
    Handle(Handle& h) throw ():m_handle(h.m_handle)
    {
        h.m_handle = -1;
    }

    /**
     * Destructor
     */
    ~Handle() throw ()
    {
        this->close();
    }

    /**
     * Assignment Operator
     */
    Handle& operator=(Handle& h) throw ()
    {
        m_handle = h.m_handle;
        h.m_handle = -1;
        return *this;
    }

    /**
     * Assignment Operator
     */
    Handle& operator=(int h) throw ()
    {
        m_handle = h;
        return *this;
    }

    /**
     * Is Null (return true if handle is -1)
     */
    bool isNull() const throw ()
    {
        return (-1 == m_handle)?true:false;
    }

    /**
     * Close
     */
    void close() throw ()
    {
        try
            {
                if(-1 != m_handle)
                    {
                        ::close(m_handle);
                    }
            }
        catch(...) {}
        m_handle = -1;
    }

    /**
     * Return a pointer to teh handle
     */
    int * get() throw ()
    {
        return &(m_handle);
    }

    /**
     * Return the handle
     */
    int operator()() const throw ()
    {
        return m_handle;
    }

    /**
     * Return and release the handle
     */
    int release() throw()
    {
        int h = m_handle;
        m_handle = -1;
        return h;
    }

private:
    /**
     * The Handle
     */
    int m_handle;
};



#endif //TEMPFILE_H_
