/*
 * ServiceAdapter.cpp
 *
 *  Created on: 18 Aug 2014
 *      Author: simonm
 */

#include "ServiceAdapter.h"

#include "MsgPrinter.h"

namespace fts3 {
namespace cli {

void ServiceAdapter::printServiceDetails()
{
    // if verbose print general info
    getInterfaceDeatailes();
    MsgPrinter::instance().print_info("# Using endpoint", "endpoint", endpoint);
    MsgPrinter::instance().print_info("# Service version", "service_version", version);
    MsgPrinter::instance().print_info("# Interface version", "service_interface", interface);
    MsgPrinter::instance().print_info("# Schema version", "service_schema", schema);
    MsgPrinter::instance().print_info("# Service features", "service_metadata", metadata);
}

} /* namespace cli */
} /* namespace fts3 */
