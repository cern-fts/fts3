/*
 * Copyright (c) CERN 2015
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
#ifndef BASESERVICE_H_
#define BASESERVICE_H_

#include <boost/noncopyable.hpp>


namespace fts3 {
namespace server {

/// Base class for all services
/// Intented to be able to treat all of them with the same API
class BaseService: public boost::noncopyable {
public:
    virtual ~BaseService() {};
    virtual void operator() () = 0;
};

} // namespace server
} // namespace fts3

#endif // BASESERVICE_H_
