/*
 * cli_exception.h
 *
 *  Created on: 8 May 2014
 *      Author: simonm
 */

#ifndef CLI_EXCEPTION_H_
#define CLI_EXCEPTION_H_

#include <exception>
#include <string>

#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

namespace fts3
{

namespace cli
{

/**
 * Generic CLI exception
 */
class cli_exception
{

public:
    /**
     * Constructor.
     *
     * @param msg - exception message
     */
    cli_exception(std::string const & msg) : msg(msg) {}

    /**
     * Destructor
     */
    virtual ~cli_exception() {};

    /**
     * returns the error message
     */
    virtual char const * what() const
#if __cplusplus >= 199711L
    noexcept (true)
#endif
    {
        return msg.c_str();
    }

    /**
     * returns the error message that should be included in the JSON output
     */
    virtual pt::ptree const json_obj() const
    {
        pt::ptree obj;
        obj.put("message", msg);

        return obj;
    }

    /**
     * returns the node name of the JSON output where the respective error message should be included
     */
    virtual std::string const json_node() const
    {
        return "error";
    }

   /**
    * should a cli try falling back to another protocol after getting this exception
    */
    virtual bool tryFallback() const
    {
        return false;
    }

protected:
    /// the exception message
    std::string msg;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* CLI_EXCEPTION_H_ */
