/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

/** \file error.h Interface of FTS3 error handling component. */

#pragma once

#include "common_dev.h"

#include <string>
#include <exception>

#include "logger.h"

/* -------------------------------------------------------------------------- */

/// Write an error to the log, but do not throw! Parameter is an FTS3 Error
/// class (Inherited from Err).
#define FTS3_COMMON_EXCEPTION_LOGERROR(aError) \
{ \
    aError.log(__FILE__, __FUNCTION__, __LINE__);\
}

/* -------------------------------------------------------------------------- */

/// Throw an FTS 3 exception, and log the error. Parameter is an FTS3 Error
/// class (Inherited from Err).
#define FTS3_COMMON_EXCEPTION_THROW(aError) \
{ \
    FTS3_COMMON_EXCEPTION_LOGERROR(aError) \
}

/* ========================================================================== */

FTS3_COMMON_NAMESPACE_START

/** General FTS3 error class. */
class Err: public std::exception
{
public:
    enum ErrorType
    {
        e_defaultReport,
        e_detailedReport
    };

    /* ---------------------------------------------------------------------- */

    void log(const char* aFile, const char* aFunc, const int aLineNo);

    /* ---------------------------------------------------------------------- */

    virtual const char* what() const throw();

protected:

    /* ---------------------------------------------------------------------- */

    virtual void _logSystemError()
    {
        // EMPTY
    };

    /* ---------------------------------------------------------------------- */

    virtual std::string _description() const
    {
        return "";
    };
};

/* ========================================================================== */

/** Generic FTS3 error class. */
template<bool IS_SYSTEM_ERROR, Err::ErrorType = Err::e_defaultReport>
class Error : virtual public Err
{
private:
    virtual const char* what() const throw()
    {
        return Err::what();
    };

protected:

    /* ---------------------------------------------------------------------- */

    virtual void _logSystemError()
    {
        // EMPTY
    };
};

/* ========================================================================== */

/** System error. */
class Err_System : public Error<true>
{
public:
    Err_System(const std::string& userDesc = "System Error.")
        : _userDesc(userDesc)
    {
        // EMPTY
    };

    /* ---------------------------------------------------------------------- */

    virtual ~Err_System() throw()
    {
        // EMPTY
    };

    virtual const char* what() const throw()
    {
        return _userDesc.c_str();
    };

private:

    /* ---------------------------------------------------------------------- */

    virtual std::string _description() const;

    /* ---------------------------------------------------------------------- */

    std::string _userDesc;
};

/* ========================================================================== */

/** Custom (user) error. You must give a textual explanation in the constructor. */
class Err_Custom : public Error<false, Err::e_detailedReport>
{
public:
    Err_Custom(const std::string& aDesc)
        : _desc(aDesc)
    {
        // EMPTY
    };

    /* ---------------------------------------------------------------------- */

    virtual ~Err_Custom() throw()
    {
        // EMPTY
    };

    /* ---------------------------------------------------------------------- */
    virtual std::string _description() const;

    virtual const char* what() const throw()
    {
        return _desc.c_str();
    }
private:

    /* ---------------------------------------------------------------------- */

    std::string _desc;
};

/** An error that is transient and should not be propagated to the user.*/
class Err_Transient : public Err_Custom
{
public:
    Err_Transient(const std::string& aDesc) : Err_Custom(aDesc) {}
};

FTS3_COMMON_NAMESPACE_END

