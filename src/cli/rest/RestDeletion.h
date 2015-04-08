/*
 * RestDeletion.h
 *
 */

#ifndef RESTDELETION_H_
#define RESTDELETION_H_

#include <ostream>
#include <string>
#include <vector>


namespace fts3
{
namespace cli
{

class RestDeletion {
public:
    RestDeletion(const std::vector<std::string>& files);
    virtual ~RestDeletion();

    friend std::ostream& operator<<(std::ostream& os, RestDeletion const & me);

//protected:
    std::vector<std::string> files;
};

}
}

#endif // RESTDELETION_H_
