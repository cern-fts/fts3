/*
 * Snapshot.cpp
 *
 *  Created on: 27 Aug 2014
 *      Author: simonm
 */

#include "Snapshot.h"



namespace fts3
{
namespace cli
{

void Snapshot::print(pt::ptree & out) const
{
    out.put("active", active);
    out.put("avg_queued", avg_queued);

    pt::ptree avg_throughput;

    pt::ptree at15;
    at15.put("15", _15);
    avg_throughput.push_back(std::make_pair("", at15));

    pt::ptree at5;
    at5.put("5", _5);
    avg_throughput.push_back(std::make_pair("", at5));

    pt::ptree at30;
    at30.put("30", _30);
    avg_throughput.push_back(std::make_pair("", at30));

    pt::ptree at60;
    at60.put("60", _60);
    avg_throughput.push_back(std::make_pair("", at60));

    out.put_child("avg_throughput", avg_throughput);

    out.put("dest_se", dst_se);
    out.put("failed", failed);
    out.put("finished", finished);

    pt::ptree error;

    pt::ptree count;
    count.put("count", frequent_error.first);
    error.push_back(std::make_pair("", count));

    pt::ptree reason;
    reason.put("reason", frequent_error.second);
    error.push_back(std::make_pair("", reason));

    out.put_child("frequent_error", error);

    out.put("max_active", max_active);
    out.put("source_se", src_se);
    out.put("submitted", submitted);
    out.put("success_ratio", success_ratio);
    out.put("vo_name", vo);
}

} /* namespace cli */
} /* namespace fts3 */
