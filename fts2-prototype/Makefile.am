#
# Copyright (c) Members of the EGEE Collaboration. 2004.
# See http://www.eu-egee.org/partners/ for details on the copyright holders.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
#  Main Makefile.am file for the gLite Transfer Command-Line Interface module
#
#  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
#  Version info: $Id: Makefile.am,v 1.2 2005/03/04 18:12:58 gombasg Exp $
#  Release: $Name:  $
#
#  Revision history:
#  $Log: Makefile.am,v $
#  Revision 1.2  2005/03/04 18:12:58  gombasg
#  Transfer CLI updates
#
#  Revision 1.1  2005/03/03 17:25:59  gombasg
#  New transfer CLI import
#
#

# Location of the macro files
ACLOCAL_AMFLAGS = -I project

SUBDIRS	= src interface doc #test

docdir = $(datadir)/doc/$(PACKAGE)
dist_doc_DATA = RELEASE-NOTES LICENSE

stage: install

distsrc: dist
	mv $(distdir).tar.gz $(DISTTAR)/$(distdir)_src.tar.gz

distbin: all
	mkdir -p $(top_builddir)/dist_bin/$(prefix)
	$(MAKE) $(AM_MAKEFLAGS) install DESTDIR=`cd $(top_builddir); pwd`/dist_bin
	(cd $(top_builddir)/dist_bin/$(prefix); $(AMTAR) -zcf $(DISTTAR)/$(distdir)_bin.tar.gz .)
	rm  -rf $(top_builddir)/dist_bin

.PHONY: stage distsrc distbindir distbin
