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
 * TarArchiver.cpp
 *
 *  Created on: Oct 23, 2012
 *      Author: Michał Simon
 */

#include "TarArchiver.h"

#include <cstring>
#include <fstream>

namespace fts3
{
namespace common
{

void TarArchiver::init(PosixTarHeader& header)
{
    memset(&header,0,sizeof(PosixTarHeader));
    sprintf(header.magic,"ustar ");
    sprintf(header.mtime,"%011lo",time(NULL));
    sprintf(header.mode,"%07o",0644);
    sprintf(header.gname,"%s","users");
}

void TarArchiver::setChecksum(PosixTarHeader& header)
{
    unsigned int sum = 0;
    char *p = (char *) &header;
    char *q = p + sizeof(PosixTarHeader);
    while (p < header.checksum) sum += *p++ & 0xff;
    for (int i = 0; i < 8; ++i)
        {
            sum += ' ';
            ++p;
        }
    while (p < q) sum += *p++ & 0xff;

    sprintf(header.checksum,"%06o",sum);
}

void TarArchiver::setSize(PosixTarHeader& header, unsigned long fileSize)
{
    sprintf(header.size,"%011llo", (long long unsigned int)fileSize);
}

void TarArchiver::setFilename(PosixTarHeader& header, string filename)
{

    if(filename.empty() || filename.size() >= 100)
        {
            //	TODO THROW("invalid archive name \"" << filename << "\"");
        }

    snprintf(header.name, 100, "%s", filename.c_str());
}

void TarArchiver::endRecord(size_t len)
{
    char c='\0';
    while((len%sizeof(PosixTarHeader))!=0)
        {
            out.write(&c,sizeof(char));
            ++len;
        }
}

TarArchiver::TarArchiver(ostream& out):finished(false),out(out)
{

    if(sizeof(PosixTarHeader)!=512)
        {
            // TODO	THROW(sizeof(PosixTarHeader));
        }
}

TarArchiver::~TarArchiver()
{

    if(!finished)
        {
            // TODO cerr << "[warning]tar file was not finished."<< endl;
        }
}

/** writes 2 empty blocks. Should be always called before closing the Tar file */
void TarArchiver::finish()
{

    finished = true;
    //The end of the archive is indicated by two blocks filled with binary zeros
    PosixTarHeader header;
    memset((void*)& header, 0, sizeof(PosixTarHeader));
    out.write((const char*) &header, sizeof(PosixTarHeader));
    out.write((const char*) &header, sizeof(PosixTarHeader));
    out.flush();
}

void TarArchiver::putFile(istream& in, string nameInArchive)
{

    // buffer
    char buff[BUFSIZ];

    // check file size
    in.seekg (0, ios::end);
    long len = in.tellg();
    in.seekg (0, ios::beg);

    // create tar header
    PosixTarHeader header;
    init(header);
    // set the file name
    setFilename(header, nameInArchive);
    header.typeflag[0] = 0;
    // set file size
    setSize(header, len);
    // do the checksum
    setChecksum(header);
    // write to the archive
    out.write((const char*) &header, sizeof(PosixTarHeader));

    size_t nRead = 0;
    while(!in.eof())
        {
            in.read(buff, BUFSIZ);
            nRead = in.gcount();
            out.write(buff, nRead);
        }

    // mark the end of file
    endRecord(len);
}

} /* namespace common */
} /* namespace fts3 */
