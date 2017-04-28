/*
 * Copyright (c) CERN 2016
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

#ifndef EVENTS_H
#define EVENTS_H

#include "events/Message.pb.h"
#include "events/MessageBringonline.pb.h"
#include "events/MessageLog.pb.h"
#include "events/MessageUpdater.pb.h"
#include "events/TransferStart.pb.h"
#include "events/TransferCompleted.pb.h"

#include <boost/date_time/posix_time/posix_time.hpp>

/**
 * To be used with the timestamp fields
 * @return The timestamp in milliseconds
 */
inline uint64_t millisecondsSinceEpoch()
{
    using boost::gregorian::date;
    using boost::posix_time::ptime;
    using boost::posix_time::microsec_clock;

    static ptime const epoch(date(1970, 1, 1));
    return (microsec_clock::universal_time() - epoch).total_milliseconds();
}

#endif // EVENTS_H
