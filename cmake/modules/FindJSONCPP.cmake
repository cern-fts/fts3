# This module detects if jsoncpp is installed and defines an import target.
# The code sets the following variables and import target:
#
# JSONCPP_FOUND         - True if system has jsoncpp
# JSONCPP::JSONCPP      - jsoncpp import target

find_package(PkgConfig)
pkg_check_modules(JSONCPP_PKG jsoncpp)
set(JSONCPP_VERSION ${JSONCPP_PKG_VERSION})

find_path(JSONCPP_INCLUDE_DIR
    NAMES json/json.h
    HINTS ${JSONCPP_LOCATION}
    PATH_SUFFIXES jsoncpp
)

find_library(JSONCPP_LIBRARY
    NAMES jsoncpp
    HINTS ${JSONCPP_LOCATION}
    PATH_SUFFIXES ${CMAKE_INSTALL_LIBDIR}
)

# -------------------------------------------------------
# Handle the QUIETLY and REQUIRED arguments and set
# JSONCPP_FOUND to TRUE if all listed variables are TRUE
# -------------------------------------------------------

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JSONCPP
    DEFAULT_MSG JSONCPP_INCLUDE_DIR JSONCPP_LIBRARY
)
mark_as_advanced(JSONCPP_INCLUDE_DIR JSONCPP_LIBRARY)

if (JSONCPP_FOUND AND NOT TARGET JSONCPP::JSONCPP)
    add_library(JSONCPP::JSONCPP UNKNOWN IMPORTED)
    set_target_properties(JSONCPP::JSONCPP PROPERTIES
        IMPORTED_LOCATION "${JSONCPP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${JSONCPP_INCLUDE_DIR}")
endif()
