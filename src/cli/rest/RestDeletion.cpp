#include "RestDeletion.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace fts3::cli;
namespace pt = boost::property_tree;

namespace fts3 {
namespace cli {


RestDeletion::RestDeletion(const std::vector<std::string>& files): files(files)
{

}


RestDeletion::~RestDeletion()
{

}


std::ostream& operator<<(std::ostream& os, RestDeletion const & me)
{
    pt::ptree root, files;

    // prepare the files array
    std::vector<std::string>::const_iterator it;
    for (it = me.files.begin(); it != me.files.end(); ++it)
        {
            files.push_back(std::make_pair(std::string(), pt::ptree(*it)));
        }
    // add files to the root
    root.push_back(std::make_pair("delete", files));

    // Write
    pt::write_json(os, root);
    return os;
}

}
}
