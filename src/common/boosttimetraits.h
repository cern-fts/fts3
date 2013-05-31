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

/** \file boostfiletraits.h
 * Include this file only from timetraits.h. If you include it from elsewhere,
 * you will get compilation error.
 */

#ifndef __FTS3_COMMON_TIMETRAITS_H_GUARD__
#error File is included from elsewhere than timetraits.h
#endif

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/thread/xtime.hpp"

#include "common_dev.h"

FTS3_COMMON_NAMESPACE_START

struct TimeTraits
{
    typedef boost::xtime TIME;
};

FTS3_COMMON_NAMESPACE_END

