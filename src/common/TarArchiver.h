/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
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
 *
 * TarArchiver.h
 *
 *  Created on: Oct 23, 2012
 *      Author: Michał Simon
 */

#ifndef TARARCHIVER_H_
#define TARARCHIVER_H_

#include <iostream>
#include <string>

namespace fts3
{
namespace common
{

using namespace std;

class TarArchiver
{
public:
    struct PosixTarHeader
    {
        char name[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char checksum[8];
        char typeflag[1];
        char linkname[100];
        char magic[6];
        char version[2];
        char uname[32];
        char gname[32];
        char devmajor[8];
        char devminor[8];
        char prefix[155];
        char pad[12];
    };


private:
    bool finished;
    ostream& out;
    void init(PosixTarHeader& header);
    void setChecksum(PosixTarHeader& header);
    void setSize(PosixTarHeader& header, unsigned long fileSize);
    void setFilename(PosixTarHeader& header, string filename);
    void endRecord(size_t len);

public:
    TarArchiver(ostream& out);
    virtual ~TarArchiver();
    /** writes 2 empty blocks. Should be always called before closing the Tar file */
    void finish();
    void putFile(istream& in, string nameInArchive);
};

} /* namespace common */
} /* namespace fts3 */
#endif /* TARARCHIVER_H_ */
