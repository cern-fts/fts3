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

#pragma once

/** \file task.h Interface of Task class. */

#include "common/error.h"

#include <iostream>

namespace fts3 {
namespace server {

/* -------------------------------------------------------------------------- */

/** Common base of all Task<> classes providing Task-wise object identification for
 * tracing and forces the function operator property */
class ITask
{
public:
    ITask() {};

    virtual ~ITask() {};

    virtual void execute() = 0;
};

/* -------------------------------------------------------------------------- */

/** Task wraps the code that must be executed in a thread. It provides traceability,
 *  catches and logs any unhandled exceptions that are not handled in the operation code.
 *
 *  The user must implement the thread code as a function object. The function object type
 *  is the template parameter of the class. The object must be:
 *
 *  - copyable
 *  - must have operator () that executes the task code.
 */
template<class OP_TYPE> class Task : public ITask
{
public:
    typedef Task<OP_TYPE> Type;

    Task(OP_TYPE& op) : ITask(), _op(op) {};

    virtual ~Task() {};

    /** Executes the code, catches and logs all the unhandled exceptions. */
    virtual void execute ()
    {
        try
            {
                _op();
            }
        catch (const fts3::common::Err& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FTS3 Server Exception in " << id() << fts3::common::commit;
                throw;
            }
        catch (const std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "C++ Exception in " << id() << e.what() << fts3::common::commit;
                throw;
            }
        catch (...)
            {
                std::cerr << "Unknown exception " << std::endl;
                throw;
            }
    }

    std::string id() const {
        return typeid(Type).name();
    }

private:
    OP_TYPE _op;
};

} // end namespace server
} // end namespace fts3

