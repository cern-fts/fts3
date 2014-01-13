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

#pragma once

#include "common_dev.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_array.hpp>

FTS3_COMMON_NAMESPACE_START

/** Type collection encapsulating different pointer policies */
template <typename T>
struct Pointer
{
    typedef boost::shared_ptr<T> Shared; /**< Shared pointer behaviour: reference counted, deleted if all the users released the pointer */
    typedef boost::scoped_ptr<T> Scoped; /**< Scoped pointer behaviour. The object is deleted when the pointer gets out of scope. */
    typedef boost::scoped_array<T> ScopedArray; /**< Shared pointer for arrays */
    typedef boost::shared_array<T> SharedArray; /**< Scoped pointer for arrays */
    typedef T* Raw; /**< Raw (not smart, C-like) pointer */
};

FTS3_COMMON_NAMESPACE_END
