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
#  Authors: Gabor Gombas <Gabor.Gombas@cern.ch>
#           Paolo Tedesco <paolo.tedesco@cern.ch>
#           Akos Frohner <Akos.Frohner@cern.ch>
#

AC_PREREQ(2.57)
AC_INIT([Glite Data Transfer CLI],[0.0.0])
AC_CONFIG_AUX_DIR([./project])
AM_INIT_AUTOMAKE([1.6.3 subdir-objects foreign])
AC_CONFIG_SRCDIR([src/Makefile.am])

# Notices.
AC_COPYRIGHT([Copyright (c) 2004-2009 The EU EGEE Project
See LICENSE file for details
])
AC_REVISION([$Revision: 1.13 $])

# Arguments.
AC_ARG_ENABLE(
	[debug],
	[AS_HELP_STRING([--enable-debug],[Produce binaries with debugging symbols.])],
	[CFLAGS="-g -D_DEBUG $CFLAGS"])

# Check for the basic compile environment.
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

# Make libtool less verbose
LIBTOOL="$LIBTOOL --silent"

# Turn on warnings
if test "$ac_cv_c_compiler_gnu" = yes; then
	CFLAGS="$CFLAGS -Wall -W -Wmissing-prototypes"
fi

# Check for the basic compile environment.
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_GSOAP_FIX_VERSION
m4_pattern_allow([AC_GSOAP_FIX_VERSION])
m4_pattern_allow([AC_GLOBUS_EMI])

# Set GSOAP Properties
WITH_EMI=no
AC_ARG_WITH(emi,
  [  --with-emi        check for --with-emi ],
  [  WITH_EMI=yes ])
  
  if test "$WITH_EMI" = yes; then
	export GSOAP_LOCATION="/usr"
	export GLOBUS_LOCATION="/usr"
	export GSOAP_VERSION=$GSOAP_FIX_VERSION
	GSOAP_LIBS_EMI="-L/usr/lib64 -lgsoap"
	AC_SUBST(GSOAP_LIBS_EMI)
  fi
  

# Set GSOAP Properties
AC_GSOAP

# Set GSOAP Scripts names
AC_GSOAP_FIX


AM_CONDITIONAL(USE_GSOAP_2_7_13, [test "x$GSOAP_VERSION" = "x2.7.13"])
AC_SUBST(USE_GSOAP_2_7_13)

AM_CONDITIONAL(USE_GSOAP_2_7_16, [test "x$GSOAP_VERSION" = "x2.7.16"])
AC_SUBST(USE_GSOAP_2_7_16)

# Set CGSI GSOAP Plugin Properties
AC_CGSI_GSOAP

AM_CONDITIONAL(WITH_EMI, [test "x$WITH_EMI" = "xyes"])
AC_SUBST(WITH_EMI)

# Set Globus Properties
if test "$WITH_EMI" = yes; then
have_globus=no
AC_GLOBUS_EMI
else
have_globus=no
AC_GLOBUS([], have_globus=yes, have_globus=no)
AC_MSG_RESULT(["GLOBUS found $have_globus"])
fi

# Set GLite Properties
AC_GLITE
AC_GLITE_VERSION
GLITE_SYMBOL_VERSION


AC_GRIDSITE

# Look for xsltproc & manpage stylesheets
GLITE_DOCBOOK_MAN

# Magic needed to ensure glib comes from the repository
AC_ARG_WITH([glib2_devel_prefix], AC_HELP_STRING([--with-glib2-devel-prefix], [Path where glib2 is installed]), [PKG_CONFIG_PATH="$withval/lib/pkgconfig"; export PKG_CONFIG_PATH])

# Look for glib
AM_PATH_GLIB_2_0([2.2.3])
if test "$no_glib" = yes; then
	AC_MSG_ERROR([Required dependency glib2 was not found])
fi

################################################################################

# Python location for optional Python 2.5 support
AC_ARG_WITH([python-location],
    [AS_HELP_STRING([--with-python-location=VAL],[Python location])],
    [PYTHON_LOCATION=${withval}],
    [PYTHON_LOCATION="/usr"])
AC_MSG_RESULT([PYTHON_LOCATION is ${PYTHON_LOCATION}])
AC_SUBST(PYTHON_LOCATION)

# The optional Python location may not be in the PATH 
# and LD_LIBRARY_PATH, so it has to be added that the
# checking macro succeeds.
ac_saved_path="$PATH"
ac_saved_ld_library_path="$LD_LIBRARY_PATH"
PATH="$PYTHON_LOCATION/bin:$PATH"
LD_LIBRARY_PATH="$PYTHON_LOCATION/lib"
export PATH LD_LIBRARY_PATH
AM_PATH_PYTHON([2.3])

# PYTHON_CFLAGS and PYTHON_LIBS in the original
# python.m4 macro are based on the ${prefix} and
# ${execprefix} automake variables. However the
# optional $PYTHON_LOCATION may point to a 
# different place, so these variables are re-evaluated.

dnl Get the values of Python CFLAGS
AC_CACHE_CHECK([for $am_cv_pathless_PYTHON include], [am_cv_python_include],
[am_cv_python_include=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_inc(0,prefix='$PYTHON_LOCATION')"`])
AC_SUBST([PYTHON_CFLAGS], ["-I${am_cv_python_include}"])

dnl Get the values of Python LIBS
AC_CACHE_CHECK([for $am_cv_pathless_PYTHON lib], [am_cv_python_lib],
[am_cv_python_lib=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_lib(0,1,prefix='$PYTHON_LOCATION')" 2>/dev/null || echo "${PYTHON_LOCATION}/lib/python${PYTHON_VERSION}/config"`])
AC_SUBST([PYTHON_LIBS], ["-L${am_cv_python_lib}/config -lpython${PYTHON_VERSION}"])

PATH="$ac_saved_path"
LD_LIBRARY_PATH="$ac_saved_ld_library_path"

AC_MSG_RESULT([PYTHON_LIBS is ${PYTHON_LIBS}])
AC_MSG_RESULT([PYTHON_CFLAGS is ${PYTHON_CFLAGS}])

# For IPv6
AH_TEMPLATE([WITH_IPV6], [For Ipv6 support])
AC_DEFINE(WITH_IPV6)

# Set template config.h (required by stdsoap)
# This part should probably go in a macron in gsoap.m4 (something like AC_GSOAP_SRC)
# BEGIN:
AH_TEMPLATE(HAVE_FTIME)
AH_TEMPLATE(HAVE_GMTIME)
AH_TEMPLATE(HAVE_GMTIME_R)
AH_TEMPLATE(HAVE_GETTIMEOFDAY)
AH_TEMPLATE(HAVE_LOCALTIME_R)
AH_TEMPLATE(HAVE_MBTOWC)
AH_TEMPLATE(HAVE_STRTOF)
AH_TEMPLATE(HAVE_STRTOD)
AH_TEMPLATE(HAVE_STRTOL)
AH_TEMPLATE(HAVE_STRTOLL)
AH_TEMPLATE(HAVE_STRTOUL)
AH_TEMPLATE(HAVE_STRTOULL)
AH_TEMPLATE(HAVE_SSCANF)
AH_TEMPLATE(HAVE_WCTOMB)
AH_TEMPLATE(HAVE_TIMEGM)

AC_CHECK_FUNC(ftime,           [AC_DEFINE(HAVE_FTIME)])
AC_CHECK_FUNC(gmtime,          [AC_DEFINE(HAVE_GMTIME)])
AC_CHECK_FUNC(gmtime_r,        [AC_DEFINE(HAVE_GMTIME_R)])
AC_CHECK_FUNC(gettimeofday,    [AC_DEFINE(HAVE_GETTIMEOFDAY)])
AC_CHECK_FUNC(localtime_r,     [AC_DEFINE(HAVE_LOCALTIME_R)])
AC_CHECK_FUNC(mbtowc,          [AC_DEFINE(HAVE_MBTOWC)])
AC_CHECK_FUNC(strtof,          [AC_DEFINE(HAVE_STRTOF)])
AC_CHECK_FUNC(strtod,          [AC_DEFINE(HAVE_STRTOD)])
AC_CHECK_FUNC(strtol,          [AC_DEFINE(HAVE_STRTOL)])
AC_CHECK_FUNC(strtoll,         [AC_DEFINE(HAVE_STRTOLL)])
AC_CHECK_FUNC(strtoul,         [AC_DEFINE(HAVE_STRTOUL)])
AC_CHECK_FUNC(strtoull,        [AC_DEFINE(HAVE_STRTOULL)])
AC_CHECK_FUNC(sscanf,          [AC_DEFINE(HAVE_SSCANF)])
AC_CHECK_FUNC(wctomb,          [AC_DEFINE(HAVE_WCTOMB)])
AC_CHECK_FUNC(timegm,          [AC_DEFINE(HAVE_TIMEGM)])
#:END

# Set Proxy And Object File Names
GLITE_GSOAP_HEADERNAMES([channel], [ChannelManagement])
GLITE_GSOAP_HEADERNAMES([fts], [FileTransfer])

# Check the location of the WSDL files
GLITE_WSDL_FILE([data], [channel])
GLITE_WSDL_FILE([data], [fts])

#Conditions for Gsoap on-the-fly paches
AM_CONDITIONAL(USE_GSOAP_2_7_6, [test "x$GSOAP_VERSION_NUM" = "x020706"])
AC_MSG_RESULT(["USE_GSOAP_2_7_6 is [$USE_GSOAP_2_7_6]"])
AM_CONDITIONAL(USE_GSOAP_2_7_9, [test "x$GSOAP_VERSION_NUM" = "x020709"])
AC_MSG_RESULT(["USE_GSOAP_2_7_9 is [$USE_GSOAP_2_7_9]"])               

# Check that the util-c package is available
save_CPPFLAGS="$CPPFLAGS"
save_LDFLAGS="$LDFLAGS"
save_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $GLITE_CFLAGS $GSOAP_CFLAGS"
LDFLAGS="$LDFLAGS $GLITE_LDFLAGS"

#AC_CHECK_HEADER([glite/data/glite-util.h],,
#	[AC_MSG_ERROR([Headers from the org.glite.data.util-c package are not available])])
#dnl AC_CHECK_LIB([glite_data_util],,
#dnl 	[AC_MSG_ERROR([Libraries from the org.glite.data.util-c package are not available])])


LIBS="$save_LIBS"
LDFLAGS="$save_LDFLAGS"
CPPFLAGS="$save_CPPFLAGS"

# Location of the HTML documentation
AC_ARG_WITH([htmldir],
	AC_HELP_STRING([--with-htmldir],
		[Where to put the HTML documentation @<:@${datadir}/doc/${PACKAGE}/html@:>@]),
	[htmldir=$withval],
	[htmldir='${datadir}/doc/${PACKAGE}/html'])
AC_SUBST([htmldir])

# Configuration items
AC_PREFIX_DEFAULT([/opt/glite])
AM_CONFIG_HEADER([src/autogen/config.h])
AC_CONFIG_FILES([\
	Makefile \
	doc/Makefile \
	doc/man/Makefile \
    doc/apidoc/Makefile \
	interface/Makefile \
	src/Makefile \
	src/c/Makefile \
	src/tools/Makefile \
    src/python/Makefile
])

AC_OUTPUT
