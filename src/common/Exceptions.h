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

/** \file Exceptions.h Interface of FTS3 error handling component. */

#pragma once
#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <string>
#include <exception>


namespace fts3 {
namespace common {

/** General FTS3 error class. */
class BaseException: public std::exception
{
public:
    virtual const char* what() const throw();
};

/* ========================================================================== */

/** System error. */
class SystemError : public BaseException
{
public:
    SystemError(const std::string& userDesc = "System Error.")
        : _userDesc(userDesc)
    {
        // EMPTY
    };

    virtual const char* what() const throw()
    {
        return _userDesc.c_str();
    };

private:
    std::string _userDesc;
};

/* ========================================================================== */

/** Custom (user) error. You must give a textual explanation in the constructor. */
class UserError : public BaseException
{
public:
    UserError(const std::string& aDesc)
        : _desc(aDesc)
    {
        // EMPTY
    };

    virtual const char* what() const throw()
    {
        return _desc.c_str();
    }

private:
    std::string _desc;
};


} // end namespace common
} // end namespace fts3

#endif // EXCEPTIONS_H_
