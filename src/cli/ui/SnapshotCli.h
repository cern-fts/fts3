/*
 * SnapshotCli.h
 *
 *  Created on: 10 Mar 2014
 *      Author: simonm
 */

#ifndef SNAPSHOTCLI_H_
#define SNAPSHOTCLI_H_

#include "SrcDestCli.h"
#include "TransferCliBase.h"

namespace fts3
{

namespace cli
{

using namespace std;

class SnapshotCli : public SrcDestCli, public TransferCliBase
{

public:

    SnapshotCli();
    virtual ~SnapshotCli();

    string getVo();
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* SNAPSHOTCLI_H_ */
