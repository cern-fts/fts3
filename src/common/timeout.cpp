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

#include "error.h"
#include "timeout.h"

FTS3_COMMON_NAMESPACE_START

/* ---------------------------------------------------------------------- */

Timeout& Timeout::actualize()
{
    static const int MILLISECONDS_PER_SECOND = 1000;
    static const int NANOSECONDS_PER_SECOND = 1000000000;
    static const int NANOSECONDS_PER_MILLISECOND = 1000000;
    int res = boost::xtime_get (&_xt, boost::TIME_UTC);

    if (boost::TIME_UTC != res) FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Time error"));

    int nsecs = _ns + (int) _xt.nsec;
    int usecs = _us + nsecs / NANOSECONDS_PER_MILLISECOND;
    int secs = _s + usecs / MILLISECONDS_PER_SECOND;
    nsecs += (usecs % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND;
    _xt.nsec = nsecs % NANOSECONDS_PER_SECOND;
    _xt.sec += secs + (nsecs / NANOSECONDS_PER_SECOND);
    return *this;
}

/* ---------------------------------------------------------------------- */

Timeout& Timeout::operator = (const Timeout& x)
{
    if (&x != this)
        {
            _s = x._s;
            _us = x._us;
            _ns = x._ns;
        }
    return *this;
}

/* ---------------------------------------------------------------------- */

bool Timeout::occured() const
{
    Timeout actual;
    return actual._xt.sec > _xt.sec || (actual._xt.sec == _xt.sec && actual._xt.nsec > _xt.nsec);
}

/* ---------------------------------------------------------------------- */

timeval* Timeout::tval() const
{
    _tval.tv_sec = sec();
    _tval.tv_usec = usec();
    return &_tval;
}

/* ---------------------------------------------------------------------- */

Timeout* Timeout::clone() const
{
    return new Timeout(*this);
}

/* ---------------------------------------------------------------------- */

bool InfiniteTimeout::occured() const
{
    return false;
}

/* ---------------------------------------------------------------------- */

timeval* InfiniteTimeout::tval() const
{
    return NULL;
}

/* ---------------------------------------------------------------------- */

Timeout* InfiniteTimeout::clone() const
{
    return new InfiniteTimeout(*this);
}

FTS3_COMMON_NAMESPACE_END

