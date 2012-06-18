

if USE_GSOAP_2_7_13
LDFLAGS += -L$(GLITE_LOCATION)/lib64
LDFLAGS += -L/usr/lib64
endif

AM_CPPFLAGS= \
	$(GSOAP_CFLAGS) \
	$(CGSI_GSOAP_CFLAGS) \
	$(GRIDSITE_GLOBUS_CFLAGS) \
	$(GLOBUS_NOTHR_CFLAGS) \
	$(GLIB_CFLAGS) \
	-I/usr/include/globus \
	-I/usr/lib64/globus/include

lib_LTLIBRARIES = libdelegation_api_simple_cpp.la

libdelegation_api_simple_cpp_la_SOURCES  = delegation_simple_api.cpp

libdelegation_api_simple_c_la_LIBADD = \
	-ldelegation_api_cpp \
	$(GRIDSITE_GLOBUS_LIBS) \
	-lglite_data_util \
	$(XML2_LIBS) \
	$(GLIB_LIBS) \
	$(GLOBUS_GSS_NOTHR_LIBS) \
	$(GSOAP_LIBS_EMI) \
	$(EMI_LTDL_LIBS)
