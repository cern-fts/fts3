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
#include <limits>
#include <sys/time.h>

#include <boost/thread/xtime.hpp>

/* ---------------------------------------------------------------------- */

FTS3_COMMON_NAMESPACE_START

/** A type representing timeout.
 *
 * The timeout may be specified in nanosecond granularity. The time is given relative to
 * the actual time. so, if we specify Timeout(5, 0, 0), then timeout orrurs 5 seond after
 * the construction. The timeout may be actualized: it means that the countdown restarts from now.
 */
class Timeout
{
public:
    /** Constructor.
     *
     * The microsec and nanosec parts are additive: 1200 usec will represent 1 sec 200 usec, and 1 sec
     * will be added to the second part. The same attribute has the nanosecond parameter.
     *
     * The constructor also actualizes the timeout, and the countdown starts immediately.
     * */
    explicit Timeout(int secs = 0 /**< second part */,
                     int usecs = 0 /**< microsecond part */,
                     int nsecs = 0 /**< nanosecond part */) : _s(secs), _us(usecs), _ns(nsecs)
    {
        actualize();
    };

    /** Destructor */
    virtual ~Timeout() {}

    /** Assignment operator.
     *
     * Copies the second, nanosecond, microsecond values and actualizes the timeout
     * (the countdown starts immediately).
     */
    Timeout& operator = (const Timeout& x);

    /** boost::xtime representation of the timeout.
     *
     * It is the absolute time when the timeout should occur. The Boost library requires this
     * representation.
     */
    boost::xtime getXtime() const
    {
        return _xt;
    }

    /** Return the second part of the timeout, as given in the constructor */
    int sec() const
    {
        return _s;
    }

    /** Return the microsecond part of the timeout, as given in the constructor */
    int usec() const
    {
        return _us;
    }

    /** Return the nanosecond part of the timeout, as given in the constructor */
    int nsec() const
    {
        return _ns;
    }

    /** Actualize the timeout (countdown restarts from now). Return itself. */
    Timeout& actualize();

    /** Return true if timeout occured else false. */
    virtual bool occured() const;

    virtual Timeout* clone() const;

protected:
    /** Copy constructor
     *
     * Copies the second, nanosecond, microsecond values and actualizes the timeout
     * (the countdown starts immediately).
     */
    Timeout(const Timeout& x) : _s(x._s), _us(x._us), _ns(x._ns), _xt(x._xt) {};

private:
    int _s; /**< second part of the timeout */
    int _us; /**< microsecond part of the timeout */
    int _ns; /**< nanosecond part of the timeout */
    boost::xtime _xt; /**< absolut time of timeout (the @see actualize() method sets it) */
};

/** Class representing infinite timeout (timeout never occurs...) */
class InfiniteTimeout : public Timeout
{
public:
    InfiniteTimeout() : Timeout() {}
    virtual ~InfiniteTimeout() {}

    virtual bool occured() const;

    virtual Timeout* clone() const;

private:
    InfiniteTimeout(const InfiniteTimeout& x) : Timeout(x) {}
};

FTS3_COMMON_NAMESPACE_END

