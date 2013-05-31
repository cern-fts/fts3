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

/** \file ctl.h Common template library for FTS3 developent. */

#include "common/common_dev.h"

#include <boost/type_traits/remove_pointer.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/foreach.hpp>

#include <iosfwd>
#include <iterator>
#include <vector>

FTS3_COMMON_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/** Print the elements of a vector into a stream. Each elements go to a separate line. */
template<class T> inline std::ostream& operator <<
(
    std::ostream& os, /**< Stream to be written into. */
    const std::vector<T>& x /**< Vector to be streamed. */)
{
    std::copy(x.begin(), x.end(), std::ostream_iterator<const T>(os, "\n"));
    return os;
}

/* -------------------------------------------------------------------------- */

/** Read the elements of a vector from a stream. Insert the elements to the end
 * of the vector. Vector elements must be "readable" as well. */
template<class T> inline std::istream& operator >>
(
    std::istream& is, /**< Stream to be reade from. */
    std::vector<T>& x /**< Elements are added here. */)
{
    std::copy(std::istream_iterator<T>(is), std::istream_iterator<T>(), std::back_inserter(x));
    return is;
}

namespace ctl
{

/* -------------------------------------------------------------------------- */

template <class Container>
struct _PtrContainerComparator : std::binary_function<typename Container::value_type, typename Container::value_type, bool>
{
    bool operator() (const typename Container::value_type lhs, const typename Container::value_type rhs) const
    {
        if (lhs == rhs) return true;
        return lhs && rhs ? *rhs == *lhs : false;
    }
};

/* -------------------------------------------------------------------------- */

/** Name explains... No members, no methods. */
struct EmptyType {};

} // namespace ctl

/* -------------------------------------------------------------------------- */

#define CORALMESSAGING_CTL_FOR_PRED(r, state) \
	BOOST_PP_NOT_EQUAL( \
		BOOST_PP_TUPLE_ELEM(2, 0, state), \
		BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 1, state)) \
	)

/* -------------------------------------------------------------------------- */

#define CORALMESSAGING_CTL_FOR_INC(r, state) \
( \
	BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, state)), \
	BOOST_PP_TUPLE_ELEM(2, 1, state) \
)

/* -------------------------------------------------------------------------- */

#define CORALMESSAGING_CTL_FOR(start, stop, MACRO) \
	BOOST_PP_FOR((start, stop), CORALMESSAGING_CTL_FOR_PRED, CORALMESSAGING_CTL_FOR_INC, MACRO);

/* -------------------------------------------------------------------------- */

FTS3_COMMON_NAMESPACE_END

