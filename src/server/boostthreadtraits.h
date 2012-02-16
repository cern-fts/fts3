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

#ifndef FTS3_SERVER_BOOSTTHREADTRAITS_H_
#define FTS3_SERVER_BOOSTTHREADTRAITS_H_

/** \file boostthreadtraits.h
 * Threading strategy implemented on top of Boost threads.
 * Include this file only from threadtraits.h. If you include it from elsewhere,
 * you will get compilation error.
 */

#ifndef __THREADTRAITS_H_GUARD__
  #error File is included from elsewhere that ThreadTraits.h
#endif

// Disable warnings triggered by the CppUnit headers
// See http://wiki.services.openoffice.org/wiki/Writing_warning-free_code
// See also http://www.artima.com/cppsource/codestandards.html
#if defined __GNUC__
#  pragma GCC system_header
#elif defined __SUNPRO_CC
#  pragma disable_warn
#elif defined _MSC_VER
#  pragma warning(push, 1)
#endif

#include "server_dev.h"

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

#ifndef BOOST_HAS_THREADS
  #error Threaded version of Boost is required.
#endif

#include <boost/version.hpp>

FTS3_SERVER_NAMESPACE_START

/** Strategy implementing common threading constructs, on top of Boost threads. */
struct ThreadTraits {
	typedef boost::mutex              MUTEX;
	typedef boost::condition          CONDITION;
	typedef boost::mutex::scoped_lock LOCK;
	typedef boost::thread_group       THREAD_GROUP;
	typedef boost::thread             THREAD;

#if (BOOST_VERSION >= 103500)
	static boost::thread::id get_id()
	{
		return boost::this_thread::get_id();
	}
#else
    static int get_id()
    {
        return 0;
    }
#endif

};

FTS3_SERVER_NAMESPACE_END

#if defined __SUNPRO_CC
#  pragma enable_warn
#elif defined _MSC_VER
#  pragma warning(pop)
#endif

#endif /*FTS3_SERVER_BOOSTTHREADTRAITS_H_*/

