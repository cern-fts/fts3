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

#ifndef JOBPARAMETERHANDLER_H_
#define JOBPARAMETERHANDLER_H_

#include <map>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

namespace fts3
{
namespace cli
{

/**
 * The JobParameterHandler class contains list of string values corresponding
 * to transfer job parameter names. Moreover, it allows for mapping the
 * parameter names into the respective values.
 */
class JobParameterHandler
{
public:

    /**
     * Default constructor.
     *
     * Sets the default values for some job parameters,
     * e.g. copy_pin_lifetime = 1
     */
    JobParameterHandler();

    /**
     * Destructor.
     */
    virtual ~JobParameterHandler();

    /**
     * The functional operator.
     *
     * Allows for assigning values to some chosen job parameters
     *
     * @param keys - vector with keys (e.g. keys[0] corresponds to values[0], and so on)
     * @param values - vector with values (e.g. keys[0] corresponds to values[0], and so on)
     */
    void set(const std::vector<std::string>& keys, const std::vector<std::string>& values);

    ///@{
    /**
     * names of transfer job parameters
     */
    static const std::string GRIDFTP;
    static const std::string DELEGATIONID;
    static const std::string SPACETOKEN;
    static const std::string SPACETOKEN_SOURCE;
    static const std::string COPY_PIN_LIFETIME;
    static const std::string BRING_ONLINE;
    static const std::string LAN_CONNECTION;
    static const std::string FAIL_NEARLINE;
    static const std::string OVERWRITEFLAG;
    static const std::string CHECKSUM_METHOD;
    static const std::string CHECKSUM_MODE;
    static const std::string REUSE;
    static const std::string JOB_METADATA;
    static const std::string RETRY;
    static const std::string RETRY_DELAY;
    static const std::string MULTIHOP;
    static const std::string BUFFER_SIZE;
    static const std::string NOSTREAMS;
    static const std::string TIMEOUT;
    static const std::string STRICT_COPY;
    static const std::string CREDENTIALS;
    static const std::string S3ALTERNATE;
    ///@}

    /**
     * Gets the value corresponding to given parameter name
     *
     * @param name - parameter name
     *
     * @return parameter value
     */
    inline std::string get(std::string name) const
    {
        if (parameters.count(name))
            return parameters.at(name);
        else
            return std::string();
    }

    /**
     * Gets the value corresponding to given parameter name
     *
     * @param name - parameter name
     * @param T - typpe of the returned value
     *
     * @return parameter value
     */
    template<typename T>
    inline T get(std::string name) const
    {
        return boost::lexical_cast<T>(this->get(name));
    }

    /**
     * Checks if the given parameter has been set
     *
     * @param name - parameter name
     *
     * @return true if the parameter value has been set
     */
    inline bool isParamSet(std::string name)
    {
        return parameters.find(name) != parameters.end();
    }

    inline void set(std::string key, std::string value)
    {
        parameters[key] = value;
    }

private:

    /// maps parameter names into values
    std::map<std::string, std::string> parameters;

};

}
}

#endif /* JOBPARAMETERHANDLER_H_ */
