/*
 *	Copyright notice:
 *	Copyright � Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ConfigurationAssigner.h
 *
 *  Created on: Dec 10, 2012
 *      Author: simonm
 */

#ifndef CONFIGURATIONASSIGNER_H_
#define CONFIGURATIONASSIGNER_H_

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <list>

#include <boost/tuple/tuple.hpp>

namespace fts3
{
namespace server
{

using namespace db;
using namespace std;
using namespace boost;

/**
 * Assigns share configurations to transfer-job
 */
class ConfigurationAssigner
{

    enum
    {
        SHARE = 0, //< the share tuple
        CONTENT //< the content tuple
    };

    enum
    {
        SOURCE = 0, //< source index in the tuple
        DESTINATION, //< destination index in the tuple
        VO //< VO index in the tuple
    };

    /// share tuple (source, destination, VO) -> PK in DB
    typedef boost::tuple<string, string, string> share;
    /// content tuple - defines if a configuration regards the source, the destination or both
    typedef pair<bool, bool> content;
    /// configuration type
    typedef boost::tuple< share, content > cfg_type;

public:

    /**
     * Constructor.
     *
     * @param file - a file that is being scheduled
     */
    ConfigurationAssigner(TransferFiles const & file);
    ConfigurationAssigner(const fts3::server::ConfigurationAssigner&);

    /**
     * Destructor.
     */
    virtual ~ConfigurationAssigner();

    /**
     * Gets the respective configurations without assigning them persistently in DB to transfer-job
     *
     * @return list of configurations
     */
    void assign(vector< std::shared_ptr<ShareConfig> >& out);

private:

    /// file that is being scheduled
    TransferFiles const & file;
    /// DB interface
    GenericDbIfce* db;

    /// number of share configuration that have been assigned to the job
    int assign_count;

    void assignShareCfg(list<cfg_type> arg, vector< std::shared_ptr<ShareConfig> >& out);

    static const int auto_share = -1;
};

} /* namespace server */
} /* namespace fts3 */
#endif /* CONFIGURATIONASSIGNER_H_ */
