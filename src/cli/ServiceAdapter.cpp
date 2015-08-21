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

#include "ServiceAdapter.h"

#include "MsgPrinter.h"

namespace fts3
{
namespace cli
{

void ServiceAdapter::printServiceDetails()
{
    // if verbose print general info
    getInterfaceDetails();
    MsgPrinter::instance().print_info("# Using endpoint", "endpoint", endpoint);
    MsgPrinter::instance().print_info("# Service version", "service_version", version);
    MsgPrinter::instance().print_info("# Interface version", "service_interface", interface);
    MsgPrinter::instance().print_info("# Schema version", "service_schema", schema);
    MsgPrinter::instance().print_info("# Service features", "service_metadata", metadata);
}

} /* namespace cli */
} /* namespace fts3 */
