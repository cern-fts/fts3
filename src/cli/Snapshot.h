/*
 * Snapshot.h
 *
 *  Created on: 25 Aug 2014
 *      Author: simonm
 */

#ifndef SNAPSHOT_H_
#define SNAPSHOT_H_

#include <string>
#include <utility>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

namespace fts3 {
namespace cli {

namespace pt = boost::property_tree;

class Snapshot
{
    friend class ResponseParser;

public:
    Snapshot() :
        vo(), src_se(), dst_se(), active(0), max_active(0), failed(0), finished(0), submitted(0),
        avg_queued(0), _15(0), _30(0), _5(0), _60(0), success_ratio(0), frequent_error()
    {

    }

    virtual ~Snapshot() {}

    void print(pt::ptree & out) const
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

private:

    std::string vo;

    std::string src_se;
    std::string dst_se;

    int active;
    int max_active;
    int failed;
    int finished;
    int submitted;
    int avg_queued;

    double _15;
    double _30;
    double _5;
    double _60;
    double success_ratio;


    std::pair<int,std::string> frequent_error;

};

} /* namespace cli */
} /* namespace fts3 */

#endif /* SNAPSHOT_H_ */
