#include "MsgFile.h"
#include "half_duplex.h" /* For name of the named-pipe */
#include "utility_routines.h"

#include <iostream>
#include <string>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "concurrent_queue.h"

MsgFile::MsgFile()
{
}


MsgFile::~MsgFile()
{
  cleanup();
}

void MsgFile::run()
{

}


void MsgFile::cleanup()
{
  close(fd);
}
